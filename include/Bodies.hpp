#pragma once

#include "BlackHole.hpp"

#include <vector>

// Self-luminous bodies on circular orbits around the black hole.
//
// These are intersected along the photon's geodesic, not composited on top, so
// they lens exactly like everything else: a body passing behind the hole is
// smeared into an arc, can appear on both sides at once, and picks up the same
// gravitational redshift and Doppler shift as the disk.
struct OrbitingBody {
    float orbitRadius = 30.0f;   // coordinate radius, in units of M
    float radius      = 0.6f;    // body radius, in units of M
    float phase       = 0.0f;    // orbital phase at t = 0, radians
    float inclination = 0.0f;    // tilt of the orbit from the equatorial plane

    float colourR = 1.0f;
    float colourG = 0.85f;
    float colourB = 0.6f;
    float brightness = 1.0f;
};

// The shader carries a fixed-size array; keep these in step.
constexpr int kMaxBodies = 8;

class BodySystem {
public:
    // A small default set: two inside the outer disk radius and two beyond it,
    // on tilted orbits so they cross the disk plane and get lensed.
    void addDefaults(const BlackHole& bh);

    void clear() { m_bodies.clear(); }
    bool add(const OrbitingBody& b);

    const std::vector<OrbitingBody>& bodies() const { return m_bodies; }
    int count() const { return static_cast<int>(m_bodies.size()); }

    // Packs into the vec4 layout the shader expects.
    //   orbit[i]  = (orbitRadius, radius, phase, inclination)
    //   colour[i] = (r, g, b, brightness)
    void pack(float* orbit, float* colour) const;

private:
    std::vector<OrbitingBody> m_bodies;
};
