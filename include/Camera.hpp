#pragma once

#include <glm/glm.hpp>

// Camera that orbits the origin (where the black hole sits).
//
// Coordinates are "pseudo-Cartesian": +z is the black hole's spin axis and the
// accretion disk lies in the z = 0 plane. The renderer converts this position
// into Boyer-Lindquist coordinates before launching rays.
class Camera {
public:
    Camera();

    void  setDefaults(float distance, float yawRad, float pitchRad);
    void  reset();

    void  orbit(float dYaw, float dPitch);   // radians, from mouse drag
    void  addYaw(float dYaw);                // A / D keys
    void  zoom(float logFactor);             // W / S keys, scroll wheel

    glm::vec3 position() const;
    glm::vec3 forward()  const;              // unit, points at the origin
    glm::vec3 right()    const;
    glm::vec3 up()       const;

    float distance() const { return m_distance; }
    float fovY()     const { return m_fovY; }
    void  setFovY(float radians) { m_fovY = radians; }
    float tanHalfFovY() const;

    float minDistance = 2.5f;
    float maxDistance = 400.0f;

private:
    float m_distance = 26.0f;
    float m_yaw      = 0.0f;    // rotation about the spin axis
    float m_pitch    = 0.17f;   // elevation above the equatorial plane
    float m_fovY     = 1.0f;    // ~57 degrees

    float m_distance0 = 26.0f, m_yaw0 = 0.0f, m_pitch0 = 0.17f;
};
