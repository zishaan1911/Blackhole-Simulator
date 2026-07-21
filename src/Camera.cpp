#include "Camera.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPitchLimit = 1.5533f;  // ~89 degrees, keeps us off the axis
}

Camera::Camera() = default;

void Camera::setDefaults(float distance, float yawRad, float pitchRad)
{
    m_distance0 = distance;
    m_yaw0      = yawRad;
    m_pitch0    = pitchRad;
    reset();
}

void Camera::reset()
{
    m_distance = std::clamp(m_distance0, minDistance, maxDistance);
    m_yaw      = m_yaw0;
    m_pitch    = std::clamp(m_pitch0, -kPitchLimit, kPitchLimit);
}

void Camera::orbit(float dYaw, float dPitch)
{
    m_yaw += dYaw;
    m_pitch = std::clamp(m_pitch + dPitch, -kPitchLimit, kPitchLimit);

    // keep yaw bounded so it never loses float precision during long sessions
    const float twoPi = 6.28318530718f;
    if (m_yaw > twoPi)  m_yaw -= twoPi;
    if (m_yaw < -twoPi) m_yaw += twoPi;
}

void Camera::addYaw(float dYaw)
{
    orbit(dYaw, 0.0f);
}

void Camera::zoom(float logFactor)
{
    m_distance = std::clamp(m_distance * std::exp(logFactor), minDistance, maxDistance);
}

