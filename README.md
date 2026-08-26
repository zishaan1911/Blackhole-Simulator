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

