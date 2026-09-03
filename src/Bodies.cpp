#include "Bodies.hpp"

#include <cmath>

bool BodySystem::add(const OrbitingBody& b)
{
    if (static_cast<int>(m_bodies.size()) >= kMaxBodies) return false;
    m_bodies.push_back(b);
    return true;
}

void BodySystem::addDefaults(const BlackHole& bh)
{
    m_bodies.clear();

    const float isco = bh.iscoRadius();
    const float out  = bh.diskOuter();

    // Keep everything comfortably outside the ISCO so the orbits are stable,
    // and give each a different period so the configuration keeps changing.
    OrbitingBody a;
    a.orbitRadius = std::max(out * 0.55f, isco * 2.2f);
    a.radius      = 0.45f;
    a.phase       = 0.0f;
    a.inclination = 0.22f;
    a.colourR = 1.00f; a.colourG = 0.72f; a.colourB = 0.42f;  // warm
    a.brightness = 0.85f;
    add(a);

    OrbitingBody b;
    b.orbitRadius = out * 1.35f;
    b.radius      = 0.85f;
    b.phase       = 2.1f;
    b.inclination = -0.35f;
    b.colourR = 0.62f; b.colourG = 0.78f; b.colourB = 1.00f;  // blue-white
    b.brightness = 1.05f;
    add(b);

    OrbitingBody c;
    c.orbitRadius = out * 2.10f;
    c.radius      = 1.30f;
    c.phase       = 4.0f;
    c.inclination = 0.55f;
    c.colourR = 1.00f; c.colourG = 0.95f; c.colourB = 0.85f;  // white
    c.brightness = 0.80f;
    add(c);

    OrbitingBody d;
    d.orbitRadius = out * 3.05f;
    d.radius      = 1.05f;
    d.phase       = 5.4f;
    d.inclination = -0.15f;
    d.colourR = 1.00f; d.colourG = 0.55f; d.colourB = 0.35f;  // red
    d.brightness = 0.65f;
    add(d);
}

void BodySystem::pack(float* orbit, float* colour) const
{
    for (int i = 0; i < kMaxBodies; ++i) {
        const int o = i * 4;
        if (i < count()) {
            const OrbitingBody& b = m_bodies[static_cast<size_t>(i)];
            orbit[o + 0] = b.orbitRadius;
            orbit[o + 1] = b.radius;
            orbit[o + 2] = b.phase;
            orbit[o + 3] = b.inclination;
            colour[o + 0] = b.colourR;
            colour[o + 1] = b.colourG;
            colour[o + 2] = b.colourB;
            colour[o + 3] = b.brightness;
        } else {
            orbit[o] = orbit[o+1] = orbit[o+2] = orbit[o+3] = 0.0f;
            colour[o] = colour[o+1] = colour[o+2] = colour[o+3] = 0.0f;
        }
    }
}
