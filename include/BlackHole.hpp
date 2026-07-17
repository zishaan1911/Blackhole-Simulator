#pragma once

// Kerr black hole parameters.
//
// Everything is in geometrised units (G = c = 1), so lengths and times are
// measured in units of the gravitational radius GM/c^2. With mass == 1 a
// distance of "10" means 10 GM/c^2.
//
// This is the one place to edit if you want a different hole.
class BlackHole {
public:
    // ---- Tweakables ---------------------------------------------------------
    float mass = 1.0f;    // M
    float spin = 0.85f;   // dimensionless a/M, valid range [0, kMaxSpin]

    // See setSpin() in BlackHole.cpp for why this is not 1.0.
    static constexpr float kMaxSpin = 0.95f;

    // Accretion disk geometry. The inner edge defaults to the ISCO; set
    // diskInnerOverride > 0 to pin it somewhere else.
    float diskInnerOverride = -1.0f;
    float diskOuterRadii    = 20.0f;  // outer edge, in units of M

    // Disk appearance.
    float diskTemperature = 6500.0f;  // peak local blackbody temperature (K)
    float diskBrightness  = 0.50f;
    float diskOpacity     = 0.80f;    // per-crossing opacity of the (thin) disk

    // ---- Derived quantities -------------------------------------------------
    float a() const { return spin * mass; }              // angular momentum per unit mass
    float horizonRadius() const;                         // r+ (outer event horizon)
    float innerHorizonRadius() const;                    // r-
    float ergosphereRadius(float cosTheta) const;        // static limit surface
    float photonSphereRadius() const;                    // prograde equatorial photon orbit
    float iscoRadius() const;                            // prograde ISCO
    float diskInner() const;
    float diskOuter() const { return diskOuterRadii * mass; }

    void setSpin(float s);
};
