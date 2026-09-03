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

glm::vec3 Camera::position() const
{
    const float cp = std::cos(m_pitch), sp = std::sin(m_pitch);
    const float cy = std::cos(m_yaw),   sy = std::sin(m_yaw);
    return glm::vec3(m_distance * cp * cy,
                     m_distance * cp * sy,
                     m_distance * sp);
}

glm::vec3 Camera::forward() const
{
    return glm::normalize(-position());
}

glm::vec3 Camera::right() const
{
    const glm::vec3 worldUp(0.0f, 0.0f, 1.0f);
    glm::vec3 r = glm::cross(forward(), worldUp);
    const float len = glm::length(r);
    if (len < 1e-5f) return glm::vec3(1.0f, 0.0f, 0.0f);
    return r / len;
}

glm::vec3 Camera::up() const
{
    return glm::normalize(glm::cross(right(), forward()));
}

bool Camera::advance(float dt)
{
    if (!autoOrbit || autoOrbitSpeed == 0.0f || dt <= 0.0f) return false;
    addYaw(autoOrbitSpeed * dt);
    return true;
}

void Camera::scaleAutoOrbitSpeed(float factor)
{
    float s = std::fabs(autoOrbitSpeed) * factor;
    s = std::clamp(s, autoOrbitMin, autoOrbitMax);
    autoOrbitSpeed = (autoOrbitSpeed < 0.0f) ? -s : s;
}

float Camera::tanHalfFovY() const
{
    return std::tan(0.5f * m_fovY);
}
