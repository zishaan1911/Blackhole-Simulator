#include "Camera.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPitchLimit = 1.5533f;  // ~89 degrees, keeps us off the axis
}

Camera::Camera() = default;

