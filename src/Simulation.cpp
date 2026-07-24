#include "Simulation.hpp"

#include <algorithm>

void Simulation::update(double realDeltaSeconds)
{
    if (m_paused) return;
    // Clamp so an alt-tab or a stalled frame does not teleport the disk.
    realDeltaSeconds = std::min(realDeltaSeconds, 0.1);
    m_time += realDeltaSeconds * baseRate * m_speed;
}

void Simulation::reset()
{
    m_time   = 0.0;
    m_speed  = 1.0;
    m_paused = false;
}

void Simulation::togglePause()
{
    m_paused = !m_paused;
}

void Simulation::multiplySpeed(double factor)
{
    m_speed = std::clamp(m_speed * factor, minSpeed, maxSpeed);
}
