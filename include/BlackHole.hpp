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
    //
    // diskTemperature is the PEAK local blackbody temperature and is an
    // artistic choice, not a physical one. Real thin disks run 1e4-1e7 K, which
    // renders as white or blue-white because the normalised Planckian locus is
    // almost colourless above ~5000 K. 2000 K puts the peak in the saturated
    // orange part of the curve, so the radial temperature gradient and the
    // Doppler shift both read as visible colour. Raise it towards 10000 for
    // physical realism and a much paler disk.
    float diskTemperature = 2000.0f;  // peak local blackbody temperature (K)
    float diskBrightness  = 1.60f;
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
