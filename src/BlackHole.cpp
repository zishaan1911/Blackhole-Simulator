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

