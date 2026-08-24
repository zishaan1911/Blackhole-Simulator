# Verification

Two standalone harnesses reproduce the shader's equations in double precision
and check them against analytic results. Neither is part of the build.

```sh
g++ -O2 -o verify_geodesics  verify_geodesics.cpp  -lm && ./verify_geodesics
g++ -O2 -o verify_shadow     verify_shadow_size.cpp -lm && ./verify_shadow
```

## verify_geodesics.cpp

1. **Null condition.** `H = ½ g^{μν} p_μ p_ν` must stay 0 along a photon path.
   Measured max |H| = 2×10⁻⁶ over a full ray at `a/M = 0.9`.
2. **Weak-field deflection.** Compares the angle between the incoming and
   outgoing asymptotic directions against `4M/b + 15πM²/4b²`:

   | b/M | numeric | analytic | rel. err |
   |-----|---------|----------|----------|
   | 40  | 1.081040e-01 | 1.073631e-01 | 6.9e-03 |
   | 80  | 5.192847e-02 | 5.184078e-02 | 1.7e-03 |
   | 160 | 2.547086e-02 | 2.546019e-02 | 4.2e-04 |
   | 320 | 1.261636e-02 | 1.261505e-02 | 1.0e-04 |

   The residual falls as 1/b², i.e. it is the neglected 2PN term, not
   integration error.
3. **Frame dragging.** A photon aimed exactly radially inward sweeps +2.15986 rad
   of φ at `a/M = 0.9`, and exactly 0 at `a = 0`.

