# Verification

Two standalone harnesses reproduce the shader's equations in double precision
and check them against analytic results. Neither is part of the build.

```sh
g++ -O2 -o verify_geodesics  verify_geodesics.cpp  -lm && ./verify_geodesics
g++ -O2 -o verify_shadow     verify_shadow_size.cpp -lm && ./verify_shadow
```

