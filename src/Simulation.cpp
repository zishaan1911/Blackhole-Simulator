#include "Simulation.hpp"

#include <algorithm>

void Simulation::update(double realDeltaSeconds)
{
    if (m_paused) return;
    // Clamp so an alt-tab or a stalled frame does not teleport the disk.
    realDeltaSeconds = std::min(realDeltaSeconds, 0.1);
    m_time += realDeltaSeconds * baseRate * m_speed;
}

