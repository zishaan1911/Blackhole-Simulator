# Changelog

All notable changes to Kerrscope are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project
follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

What "breaking" means here: a change to the controls, to the command-line
interface, or to the meaning of a tunable in `src/main.cpp`. Changes to the
rendered image are not breaking on their own, but any change that moves
`b_crit` is called out explicitly, with the before and after — that number is
the angular radius of the shadow, and it is the one thing in this project that
is not allowed to drift silently.

## [Unreleased]

## [1.1.0] — 2026-09-08

The renderer is unchanged. `b_crit` is identical at every spin. Everything in
this release is about the project around the renderer: it now has a licence, a
name, continuous integration, published builds, and a way to tell you which
build you are running.

### Added

- **`--version` and `--help`.** Every binary reports its version, the commit it
  was built from and the build date. The version line is also printed at
  startup, above the `GL renderer` and `GL version` lines, so a screenshot of
  the console identifies the build.
- **Published binaries.** Tagging `v*` builds Linux and Windows, packages each
  with its shaders, `README.md`, `CHANGELOG.md` and `LICENSE`, publishes
  `SHA256SUMS`, and attaches the lot to a GitHub release. Nothing to install.
- **`KERRSCOPE_STATIC_RUNTIME`.** A CMake option, off by default, that links the
  C++ runtime statically so a release binary runs on a machine that has never
  had a compiler on it.
- **Continuous integration.** Three jobs on every push and pull request: a build
  matrix across GCC, Clang and MSVC that also verifies the shaders were copied
  next to the binary; a `glslang` pass that parses every shader as GLSL 330 core
  without needing a GPU; and a physics job that asserts the tolerances in
  `docs/verification.md` — thirteen checks, `b_crit` among them.
- **A landing page** at
  [zishaan1911.github.io/Kerrscope](https://zishaan1911.github.io/Kerrscope/),
  deployed from `site/` on push.
- **A brand**: the project is called Kerrscope, with a mark, two lockups and a
  social card in `brand/`. The palette is `blackbody(T)` at the temperatures the
  renderer already works in, which `brand/README.md` sets out in full.
- **MIT licence.** The repository had been public with no licence file, which
  meant nobody could legally reuse any of it.
- **`CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md`**, issue forms and a
  pull request template. The bug form asks for the two things that actually
  diagnose a report: the GL renderer and version lines, and the window title.
- **`.gitattributes`** normalising line endings, tagging the shaders as GLSL and
  marking the vendored GL loader as vendored.

### Changed

- The window title, the startup banner and the HUD line now say **Kerrscope**.
  The CMake project is renamed to match.
- **The repository is now `zishaan1911/Kerrscope`.** GitHub redirects the old
  `Blackhole-Simulator` URLs and existing clones keep working, but the Pages
  site does not redirect: it moved from
  `zishaan1911.github.io/Blackhole-Simulator/` to
  `zishaan1911.github.io/Kerrscope/`. Every link in the README, the CHANGELOG,
  the landing page, the issue templates and the `--version` banner points at
  the new name.
- The README opens with the lockup, a one-line description and status badges,
  and links to the landing page and the verification notes.

### Fixed

- **The README's header image.** It pointed at `docs/kerr_a085.png`, which is not
  in the repository and never has been, so the header rendered as a broken image
  on every clone and on the GitHub page. It now points at
  `docs/orbiting_bodies_lensed.png`, and `docs/schwarzschild_a0.png` is placed
  next to the frame-dragging paragraph it illustrates.

### Unchanged, and deliberately so

- The executable is still **`kerr`**. Kerrscope is the project; `kerr` is the
  command, and renaming a binary is a breaking change bought for nothing.
- No new runtime dependency. GLFW and GLM remain the only two, the GL loader is
  still 44 vendored entry points, and the PNG writer still emits DEFLATE stored
  blocks rather than linking zlib.
- OpenGL 3.3 core, no compute shaders. The floor stays where it is.

## [1.0.0] — 2026-09-03

First tagged version: the renderer itself.

- Null geodesics integrated through the Kerr metric with RK4 in a fragment
  shader, using analytic derivatives of the inverse metric.
- Accretion disk with the full redshift factor `g = 1 / [u^t (E − Ω L)]`,
  specific intensity scaled by `g³` and local blackbody temperature by `g`.
- Up to eight self-luminous orbiting bodies, intersected along the photon's
  geodesic so they lens like everything else.
- Temporal accumulation as an exact running mean over a Halton sequence, to 256
  samples.
- Off-screen screenshots at settings independent of the interactive ones, via a
  dependency-free PNG writer.
- Automatic camera orbit, shader hot-reload, and a window-title HUD.
- Verification harnesses in `docs/`, agreeing with the analytic Kerr `b_crit` to
  2×10⁻⁷ or better at every spin tested.

[Unreleased]: https://github.com/zishaan1911/Kerrscope/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/zishaan1911/Kerrscope/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/zishaan1911/Kerrscope/releases/tag/v1.0.0
