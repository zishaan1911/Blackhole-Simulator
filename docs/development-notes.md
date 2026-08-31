# Development notes

Things that went wrong, and how they were found. Recorded because most of them
were invisible to the numerical tests and only showed up when the output was
looked at directly.

## The shader was cross-compiled to a CPU renderer

The integrator is a physics calculation, so it was checked against analytic
results rather than judged by eye. But numerical agreement on a handful of
scalars does not prove the rendered image is right. The GLSL was mechanically
translated to C++ and run on the CPU, which made it possible to inspect frames,
render debug channels (ray fate, step count, closest approach to the polar
axis) and diff two versions of the shader pixel for pixel.

Every bug below was found that way.

## Bug 1: dark seam along the projected spin axis

Boyer-Lindquist coordinates are singular on the polar axis: `g^{phi phi}` goes
as `1/sin^2(theta)` and its theta derivative as `1/sin^3(theta)`.

The first attempt clamped `sin(theta)` away from zero. That reduced the seam but
did not remove it. A debug channel showed the affected rays were neither being
captured nor running out of steps, which ruled out the obvious explanations and
pointed at accuracy instead: near the axis `dphi/dl ~ L/(Sigma sin^2 theta)`
grows large and the photon whips around the pole faster than the step could
resolve, so the outgoing direction came out slightly wrong.

Fixed by two step limiters, one bounding the approach in theta and one bounding
the azimuthal rotation per step to 0.25 rad. The seam disappeared and the step
budget was unchanged.

## Bug 2: stars visible inside the shadow

A ray-fate debug image showed escaping rays speckled through the capture region.
RK4 stages were stepping to `r < r+`, where `Delta` changes sign, and photons
were being ejected back out of the horizon.

The first fix (approach the horizon geometrically, stop at `1.02 r+`) looked
correct at `a/M = 0.85` but was hiding a spin-dependent failure. Rendering a
near-extremal edge-on view brought it straight back. Measured leakage was 0.07%
of shadow pixels at `a/M = 0.85`, rising to 2.9% at `a/M = 0.998`.

Fixed by stopping rays relative to the prograde photon orbit rather than the
horizon: `r+ + 0.75 (r_photon - r+)`. A photon below the photon shell moving
inward is captured regardless, and this keeps `Delta` away from zero. Leakage
went to zero up to `a/M = 0.95`.

Crucially, the shadow-size verification was re-run afterwards, because moving
the capture surface outward could plausibly have shrunk the shadow. It did not:
`b_crit` still matches the analytic Kerr value to 2e-7.

## Non-bug: the 3.9% shadow error that would not converge

An early harness converted emission angle to impact parameter with the
flat-space relation `b = r0 sin(alpha)` at `r0 = 26 M`, and reported a stubborn
3.9% error against the analytic value.

Refining the step size did not move it at all, which is the tell: integration
error shrinks under refinement and this did not. The relation is only asymptotic;
at finite radius in curved spacetime `b != r0 sin(alpha)`. Measuring the
conserved `L/E` directly gave agreement to 1e-7.

An error that refuses to converge under refinement is usually in the
measurement, not the integrator.

## Port from compute shader to fragment shader

The renderer originally used an OpenGL 4.3 compute shader. That excludes a lot
of older hardware, including Intel HD 4000, which caps at OpenGL 4.0 and has no
compute support at all.

The geodesic integration does not care what stage it runs in. Moving it to a
fragment shader on a fullscreen triangle drops the requirement to OpenGL 3.3.
The physics core was carried over unchanged; only the input and output wrapper
differ.

Verified by rendering the same frame through both paths at 800x450 and diffing:
0 of 360000 pixels differed.
