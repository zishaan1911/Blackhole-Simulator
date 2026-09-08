<!--
One idea per pull request. If it touches the metric, the tetrad, the
Hamiltonian derivatives or the step limiters, the verification section below
is not optional.
-->

## What this changes

## Why

## Area

- [ ] Shader / integrator
- [ ] Disk and emission model
- [ ] Camera and controls
- [ ] Build, CI or packaging
- [ ] Documentation only

## Verification

<!--
Physics changes: run both harnesses and paste the output.

  c++ -std=c++17 -O2 docs/verify_geodesics.cpp   -o /tmp/vg && /tmp/vg
  c++ -std=c++17 -O2 docs/verify_shadow_size.cpp -o /tmp/vs && /tmp/vs

b_crit is the one that matters most. State the before and after.
-->

- [ ] Builds clean under `-Wall -Wextra` (or `/W3` on MSVC)
- [ ] Ran it and looked at the image
- [ ] `b_crit` unchanged, or the change is explained above
- [ ] Docs updated if behaviour or a default moved

## Screenshots

<!-- Press `P`. Before and after, same camera, if the image changed. -->

## Hardware you tested on

<!-- The `GL renderer` and `GL version` lines the program prints at startup. -->
