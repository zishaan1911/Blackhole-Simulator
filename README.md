# Real-time Kerr black hole ray tracer

A live OpenGL 3.3 renderer that integrates null geodesics through the Kerr
metric on the GPU. Gravitational lensing here is not a screen-space effect:
every pixel's photon path is obtained by integrating Hamilton's equations in
curved spacetime.

![Kerr black hole, a/M = 0.85](docs/kerr_a085.png)

---

## Build

Requires a C++17 compiler, CMake 3.16+, and a GPU/driver with **OpenGL 3.3**.
The geodesic integration runs in a fragment shader rather than a compute
shader, so no compute support is needed and hardware as old as Intel HD 4000
(2012) can run it. GLFW and GLM are used from the system if present, and
fetched automatically otherwise.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/kerr
```

On Debian/Ubuntu the optional system packages are:

```sh
sudo apt install libglfw3-dev libglm-dev
```

### Windows

Use a Developer PowerShell (MSVC) or a shell with MinGW on PATH:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\kerr.exe
```

With MinGW, use `-G "MinGW Makefiles"` and the binary lands in `build\kerr.exe`.
If you get `CMAKE_CXX_COMPILER not set`, you are in a shell that cannot see the
compiler — that is an environment problem, not a project one.

macOS caps at OpenGL 4.1, which is above 3.3, so it should work there too,
though it is untested.

---

## Controls

| Input        | Action                        |
|--------------|-------------------------------|
| Mouse drag   | Orbit the camera              |
| Scroll       | Zoom                          |
| `W` / `S`    | Zoom in / out                 |
| `A` / `D`    | Rotate around the black hole  |
| `Space`      | Pause / resume                |
| `R`          | Reset camera and simulation   |
| `T` / `G`    | Simulation speed up / down    |
| `Esc`        | Quit                          |

Extras:

| Input        | Action                                    |
|--------------|-------------------------------------------|
| `Q` / `E`    | Spin `a/M` down / up                      |
| `K`          | Toggle the accretion disk                 |
| `L`          | Toggle redshift / Doppler / beaming       |
| `[` / `]`    | Render resolution scale down / up         |
| `-` / `=`    | Fewer / more integration steps            |
| `F5`         | Hot-reload the shaders                    |

There is no GUI. The window title is the HUD: spin, mass, camera radius,
simulation time, speed, trace resolution, step count, accumulated samples per
pixel and FPS.

---

## What is being computed

Units are geometrised, `G = c = 1`, so all lengths are in units of `GM/c²`.

For each pixel the shader:

1. Converts the camera position from pseudo-Cartesian (oblate spheroidal)
   coordinates to Boyer-Lindquist `(r, θ, φ)`.
2. Builds the photon four-momentum in the local orthonormal frame of a ZAMO
   (zero angular momentum observer), normalised to unit local energy.
3. Lowers indices with the covariant Kerr metric to obtain the conserved
   `E = -p_t` and `L = p_φ`.
4. Integrates Hamilton's equations for `(r, θ, φ, p_r, p_θ)` with RK4:

   ```
   dxⁱ/dλ =  ∂H/∂pᵢ        H = ½ g^{μν} p_μ p_ν  ( = 0 for a photon )
   dpᵢ/dλ = -∂H/∂xⁱ
   ```

   using analytic derivatives of the inverse metric. `t` and `φ` are cyclic, so
   only five quantities are carried.
5. Accumulates emission at each equatorial-plane crossing that lands between the
   ISCO and the outer disk edge, and samples a procedural sky if the photon
   escapes to `r = 1000 M`.

The trace pass writes **linear** radiance to a float target. A second pass keeps
a running mean across frames, and a third tone maps to the screen.

Rays that reach the capture surface contribute nothing — that is the shadow, and
the photon ring falls out of the integration on its own.

### Relativistic effects on the disk

The emitting gas is on a prograde circular orbit with
`Ω = √M / (r^{3/2} + a√M)`. Because the photon was normalised to unit energy in
the camera frame, the full redshift factor collapses to

```
g = 1 / [ u^t (E - Ω L) ]
```

which contains gravitational redshift, transverse and longitudinal Doppler, and
frame dragging in one number. Specific intensity is scaled by `g³` and the local
blackbody temperature by `g`, so the approaching side is both brighter and bluer.

Frame dragging also enters the photon paths directly through the `g^{tφ}` terms,
which is why the shadow is visibly off-centre and flattened on one side at high
spin.

---

## Verification

Because the renderer is a physics calculation, the integrator was checked
against known results rather than judged by eye. The numbers below come from a
double-precision harness (`docs/verification.md`) that runs the same equations.

| Test | Result |
|---|---|
| Null condition `H = 0` conserved along a ray (`a/M = 0.9`) | max &#124;H&#124; = 2×10⁻⁶ |
| Weak-field deflection vs `4M/b + 15πM²/4b²` | 1.0×10⁻⁴ at `b = 320 M`, falling as 1/b² |
| Critical impact parameter `b_crit` vs analytic Kerr value | 8×10⁻⁸ (`a=0`), 1.6×10⁻⁷ (`a=0.85`), 2.0×10⁻⁷ (`a=0.95`) |
| Frame dragging, radially aimed photon | +2.16 rad of φ at `a/M = 0.9`; exactly 0 at `a = 0` |
| Disk redshift, radial ray at ISCO | reduces to `√(1 - 3M/r)` analytically |

`b_crit` is the strongest of these: it is the angular size of the shadow, and it
only comes out right if the metric, the ZAMO tetrad and the integrator are all
correct together.

The fragment-shader port was checked against the original compute-shader output
at 800×450: **0 of 360000 pixels differ**. Only the I/O wrapper changed.

The shader was also cross-compiled to a CPU renderer and inspected visually,
which caught two bugs the numerical tests could not:

- a dark seam along the projected spin axis, from the `sinθ → 0` coordinate
  singularity;
- stars visible *inside* the shadow, caused by RK4 stages stepping to `r < r₊`
  where `Δ` changes sign and photons get ejected back out.

Both are fixed by the step limiters described below.

---

## Temporal accumulation

Each frame jitters the ray by a sub-pixel offset drawn from a Halton sequence
and averages into a float buffer, weighting the new sample by `1/(n+1)` so it is
an exact running mean rather than an exponential fade. Any change to the image —
camera motion, simulation time, a parameter edit — discards the history.

The practical effect: while you orbit, the image is rough; the moment you stop,
it converges to a clean antialiased frame within about a second. This suits slow
hardware far better than blurring would, because the feature most damaged by low
resolution is the photon ring, which is only a couple of pixels wide. Blur
destroys it; accumulation sharpens it.

Accumulation stops at 256 samples, after which a still camera costs almost
nothing. `F2` turns it off.

## Screenshots

`P` renders one frame off-screen at settings independent of the interactive
ones — 1600x900, 600 steps, 32 samples by default — tone maps it, reads it back
and writes a PNG next to the executable. It ignores the render scale entirely,
so a GPU too slow for smooth interaction can still produce clean stills. Expect
it to take a few seconds and the window to stop responding while it works.

The defaults live in the `App` struct in `src/main.cpp`:

```cpp
int shotWidth = 1600, shotHeight = 900;
int shotSteps = 600, shotSamples = 32;
```

PNG encoding is done by `src/PngWriter.cpp`, which has no dependencies: it emits
DEFLATE stored blocks rather than linking zlib. Files are a little larger than a
compressing encoder would produce, which is a fair trade for a screenshot key.

## Tuning

Everything worth changing lives in the setup block in `src/main.cpp` and in
`BlackHole.hpp` / `Renderer.hpp`.

```cpp
hole.mass            = 1.0f;    // M
hole.spin            = 0.85f;   // a/M
hole.diskOuterRadii  = 20.0f;   // outer disk edge, in M
hole.diskTemperature = 6500.0f; // peak local blackbody temperature (K)

renderer.setRenderScale(0.25f); // trace resolution / window resolution
renderer.maxSteps  = 320;       // RK4 steps per photon
renderer.stepScale = 0.25f;     // affine step ~ stepScale * r
```

The disk inner edge defaults to the ISCO, computed from the spin.

### Performance

Cost is roughly `pixels × steps`. The 99th-percentile ray uses about 163 steps at
`stepScale = 0.25`, and 0.4% of pixels hit the 320-step cap (see limitations).

Rough arithmetic for a 1280×720 window:

| render scale | traced pixels | work per frame |
|---|---|---|
| 0.15 | 192×108 | ~0.9 GFLOP |
| 0.25 | 320×180 | ~2.4 GFLOP |
| 0.35 | 448×251 | ~4.6 GFLOP |
| 0.50 | 640×360 | ~9.4 GFLOP |

The default is 0.25. Raise it with `]` on a capable GPU; drop it with `[` if you
are below 30 FPS. `stepScale` up to 0.30 is still accurate to ~5×10⁻⁷ on
`b_crit` if you need more speed.

On something like an Intel HD 4000 (~300 GFLOPS peak, and realistically well
under that on transcendental-heavy code — the metric evaluation calls `sin` and
`cos` about 1300 times per pixel) expect single-digit to low-teens FPS at 0.15–0.25.
That is a genuine hardware limit, not a tuning failure. Use `P` for stills.

