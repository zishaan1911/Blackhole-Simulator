#include "BlackHole.hpp"

#include <algorithm>
#include <cmath>

void BlackHole::setSpin(float s)
{
    // Capped below extremal on purpose. As a/M -> 1 the horizon, the inner
    // horizon and the prograde photon orbit all crowd into a shell a few
    // hundredths of M thick, and Boyer-Lindquist ray tracing in 32-bit floats
    // stops being trustworthy there (photons leak back out of the horizon and
    // speckle the shadow). Rendering is verified accurate up to a/M = 0.95;
    // going closer to extremal needs horizon-penetrating coordinates.
    spin = std::min(kMaxSpin, std::max(0.0f, s));
}

float BlackHole::horizonRadius() const
{
    const float s = std::min(spin, 0.999999f);
    return mass * (1.0f + std::sqrt(std::max(0.0f, 1.0f - s * s)));
}

float BlackHole::innerHorizonRadius() const
{
    const float s = std::min(spin, 0.999999f);
    return mass * (1.0f - std::sqrt(std::max(0.0f, 1.0f - s * s)));
}

float BlackHole::ergosphereRadius(float cosTheta) const
{
    const float s = spin;
    return mass * (1.0f + std::sqrt(std::max(0.0f, 1.0f - s * s * cosTheta * cosTheta)));
}

float BlackHole::photonSphereRadius() const
{
    // Prograde equatorial circular photon orbit (Bardeen, Press & Teukolsky 1972).
    const float s = std::min(spin, 0.999999f);
    return mass * 2.0f * (1.0f + std::cos((2.0f / 3.0f) * std::acos(-s)));
}

float BlackHole::iscoRadius() const
{
    // Bardeen, Press & Teukolsky 1972, prograde branch.
    const float s  = std::min(std::fabs(spin), 0.999999f);
    const float Z1 = 1.0f + std::cbrt(1.0f - s * s) *
                                (std::cbrt(1.0f + s) + std::cbrt(1.0f - s));
    const float Z2 = std::sqrt(3.0f * s * s + Z1 * Z1);
    const float r  = 3.0f + Z2 - std::sqrt(std::max(0.0f, (3.0f - Z1) * (3.0f + Z1 + 2.0f * Z2)));
    return r * mass;
}

float BlackHole::diskInner() const
{
    if (diskInnerOverride > 0.0f)
        return std::max(diskInnerOverride, horizonRadius() * 1.05f);
    return std::max(iscoRadius(), horizonRadius() * 1.05f);
}
