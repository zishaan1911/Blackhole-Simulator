# Contributing

This is a physics calculation that happens to draw pictures. That shapes what a
good contribution looks like: a change is judged by whether the numbers still
come out right, not by whether the frame looks nicer.

## Before you start

Open an issue for anything larger than a bug fix. The rendering path is short
but tightly coupled — the metric, the ZAMO tetrad and the integrator only give
the correct shadow size when all three agree — so a change in one place usually
needs a matching change in another.

## Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/kerr
```

CI builds on Ubuntu and Windows. It compiles and links; it cannot run the
renderer, because GitHub runners have no GPU with an OpenGL 3.3 context. Verify
visual changes locally and say what you saw.

## Verifying a change to the integrator

`docs/verification.md` describes a double-precision harness that runs the same
equations as the shader. Two standalone programs live beside it:

```sh
c++ -std=c++17 -O2 docs/verify_geodesics.cpp   -o /tmp/verify_geodesics   && /tmp/verify_geodesics
c++ -std=c++17 -O2 docs/verify_shadow_size.cpp -o /tmp/verify_shadow_size && /tmp/verify_shadow_size
```

If you touch the metric, the tetrad, the Hamiltonian derivatives or the step
limiters, run both and paste the output into the pull request. The critical
impact parameter `b_crit` is the strongest single check — it is the angular size
of the shadow, and it only comes out right if everything upstream of it is
correct. A change that moves it by more than ~1e-6 needs an explanation.

## Style

- C++17. Four spaces, no tabs; `.editorconfig` has the rest.
- The build is warning-clean under `-Wall -Wextra` and `/W3`. Keep it that way.
- Shaders are GLSL 330 core. No compute shaders, no extensions — the OpenGL 3.3
  floor is a deliberate constraint so the project runs on 2012 hardware.
- Comments explain *why*, especially where a constant was chosen empirically.
  `0.35 * sinθ / |dθ/dλ|` means nothing without the sentence next to it.

## Commits

One idea per commit, present-tense subject under ~72 characters, prefixed with
the area it touches (`shader:`, `camera:`, `docs:`, `ci:`). The existing history
is the reference.

## What tends to get rejected

- Screen-space fakery. Lensing here is integrated, not post-processed, and a
  cheaper approximation defeats the point of the project.
- New runtime dependencies. GLFW and GLM are it; the GL loader is vendored
  precisely so there is not a third.
- Tuning the default image without saying which physical quantity changed.
  `diskTemperature` is an artistic choice and is documented as one; the metric
  is not.
