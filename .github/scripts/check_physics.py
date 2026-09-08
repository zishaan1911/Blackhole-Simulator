#!/usr/bin/env python3
"""Gate CI on the numbers in docs/verification.md.

The two harnesses in docs/ print their results and exit 0 whatever happens,
which is right for a human reading them and useless for CI. This script runs
both and asserts the tolerances the documentation claims, so a change that
quietly moves the shadow fails the build instead of merging.

The thresholds are deliberately a little looser than the measured values. They
are there to catch a broken integrator, not to fail on the last digit of a
different compiler's libm.
"""

from __future__ import annotations

import re
import subprocess
import sys
import tempfile
from pathlib import Path

DOCS = Path(__file__).resolve().parents[2] / "docs"

failures: list[str] = []
checks = 0


def check(ok: bool, label: str, detail: str) -> None:
    global checks
    checks += 1
    if ok:
        print(f"  pass  {label}: {detail}")
    else:
        print(f"  FAIL  {label}: {detail}")
        failures.append(label)


def build_and_run(source: str, workdir: Path) -> str:
    exe = workdir / Path(source).stem
    compiler = ["c++", "-std=c++17", "-O2", str(DOCS / source), "-o", str(exe), "-lm"]
    print(f"$ {' '.join(compiler)}")
    subprocess.run(compiler, check=True)
    done = subprocess.run([str(exe)], check=True, capture_output=True, text=True)
    print(done.stdout)
    return done.stdout


def check_geodesics(out: str) -> None:
    print("verify_geodesics.cpp")

    # Test 1: the null condition H = 0 must hold along the whole ray.
    m = re.search(r"max\|H\| along ray\s*=\s*([0-9.eE+-]+)", out)
    if not m:
        check(False, "null condition", "could not find max|H| in the output")
    else:
        max_h = float(m.group(1))
        check(max_h < 1e-5, "null condition",
              f"max|H| = {max_h:.3e}, tolerance 1e-5")

    # Test 2: weak-field deflection. The residual is the neglected 2PN term, so
    # it must fall as 1/b^2 rather than sit at a floor set by integration error.
    rows = re.findall(
        r"^\s*(\d+)\s+([0-9.eE+-]+)\s+([0-9.eE+-]+)\s+([0-9.eE+-]+)\s*$",
        out, re.MULTILINE)
    deflection = [(int(b), float(err)) for b, _, _, err in rows if int(b) >= 40]
    if len(deflection) < 2:
        check(False, "deflection", "could not parse the deflection table")
    else:
        errs = [e for _, e in deflection]
        check(errs[-1] < 5e-4, "deflection at largest b",
              f"rel.err = {errs[-1]:.2e}, tolerance 5e-4")
        check(all(x > y for x, y in zip(errs, errs[1:])), "deflection convergence",
              "residual falls monotonically with b: " +
              ", ".join(f"{e:.1e}" for e in errs))

    # Test 4: frame dragging. Exactly zero without spin is the sharp half of
    # this - a nonzero value there means a sign or a term has gone astray.
    drag = dict(
        (float(a), float(v)) for a, v in
        re.findall(r"a/M=([0-9.]+)\s+phi swept by a radially-aimed photon\s*=\s*([+-][0-9.]+)",
                   out))
    if 0.0 not in drag or 0.9 not in drag:
        check(False, "frame dragging", "could not parse the frame-dragging results")
    else:
        check(abs(drag[0.0]) < 1e-6, "frame dragging at a = 0",
              f"phi swept = {drag[0.0]:+.6f}, must be 0")
        check(abs(drag[0.9] - 2.15986) < 1e-3, "frame dragging at a/M = 0.9",
              f"phi swept = {drag[0.9]:+.5f}, expected +2.15986")


def check_shadow(out: str) -> None:
    print("verify_shadow_size.cpp")

    rows = re.findall(
        r"^\s*([0-9.]+)\s+([0-9.]+)\s+([0-9.]+)\s+([0-9.]+)\s+([0-9.eE+-]+)\s+(\d+)\s*$",
        out, re.MULTILINE)
    if not rows:
        check(False, "b_crit", "could not parse the shadow-size table")
        return

    # b_crit is the angular radius of the shadow. It only comes out right if
    # the metric, the ZAMO tetrad, the constants of motion and the integrator
    # are all correct at once, which makes it the single most informative
    # number in the project.
    for _, spin, numeric, analytic, err, p99 in rows:
        rel = float(err)
        check(rel < 1e-6, f"b_crit at a/M = {float(spin):.2f}",
              f"numeric {float(numeric):.6f} vs analytic {float(analytic):.6f}, "
              f"rel.err {rel:.2e}, tolerance 1e-6")
        check(int(p99) < 320, f"step budget at a/M = {float(spin):.2f}",
              f"p99 = {p99} steps, cap is 320")


def main() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        workdir = Path(tmp)
        check_geodesics(build_and_run("verify_geodesics.cpp", workdir))
        check_shadow(build_and_run("verify_shadow_size.cpp", workdir))

    print()
    if failures:
        print(f"{len(failures)} of {checks} checks failed: {', '.join(failures)}")
        print("If this change is deliberate, update docs/verification.md in the "
              "same commit and say why the number moved.")
        return 1

    print(f"all {checks} checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
