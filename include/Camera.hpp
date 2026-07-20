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

};
