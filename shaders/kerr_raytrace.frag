#version 330 core
// =============================================================================
//  Backwards ray tracing of null geodesics in the Kerr spacetime.
//
//  Fragment-shader version. The physics below is byte-identical to the
//  compute-shader path; only the input/output wrapper differs, so this runs on
//  OpenGL 3.3 hardware that has no compute shader support.
//
//  Nothing here is a screen-space trick: for every pixel we build a photon
//  four-momentum in the local frame of the camera, lower its indices with the
//  Kerr metric, and integrate Hamilton's equations
//
//       dx^i/dl =  dH/dp_i ,      dp_i/dl = -dH/dx^i
//       H       =  1/2 g^{mn} p_m p_n  ( = 0 for a photon )
//
//  with RK4 in Boyer-Lindquist coordinates. t and phi are cyclic, so
//  E = -p_t and L = p_phi are constants of motion and only (r, th, phi, p_r,
//  p_th) have to be carried around.
//
//  Output is LINEAR radiance. Tone mapping happens in the present pass, so
//  frames can be accumulated linearly first.
//
//  Units: G = c = 1, lengths in GM/c^2.
// =============================================================================

out vec4 FragColour;

// ---- camera -----------------------------------------------------------------
uniform vec2  uResolution;
uniform vec3  uCamPos;        // pseudo-Cartesian, +z = spin axis
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform vec3  uCamForward;
uniform float uTanHalfFov;

// ---- black hole -------------------------------------------------------------
uniform float uM;             // mass
uniform float uA;             // spin parameter a = (a/M) * M

// ---- disk -------------------------------------------------------------------
uniform float uDiskInner;
uniform float uDiskOuter;
uniform float uDiskTemp;
uniform float uDiskBrightness;
uniform float uDiskOpacity;
uniform int   uEnableDisk;

// ---- integration / misc -----------------------------------------------------
uniform float uTime;          // simulation time, drives disk rotation
uniform int   uMaxSteps;
uniform float uStepScale;
uniform float uEscapeRadius;
uniform float uHorizonMargin; // where to stop, between r+ (0) and r_photon (1)
uniform int   uEnableShift;   // gravitational redshift + Doppler + beaming

// ---- temporal accumulation --------------------------------------------------
uniform vec2  uJitter;        // sub-pixel offset in pixels, for antialiasing

const float PI = 3.141592653589793;


// =============================================================================
//  Inverse Kerr metric (Boyer-Lindquist) and its first derivatives.
//
//  Sigma = r^2 + a^2 cos^2 th
//  Delta = r^2 - 2 M r + a^2
//  A     = (r^2 + a^2)^2 - a^2 Delta sin^2 th
//
//  g^tt = -A/(Sigma Delta)          g^tp = -2 M a r/(Sigma Delta)
//  g^pp = (Delta - a^2 sin^2 th)/(Sigma Delta sin^2 th)
//  g^rr = Delta/Sigma               g^thth = 1/Sigma
// =============================================================================
struct IM {
    float tt;   // g^tt
    float tp;   // g^t phi
    float pp;   // g^phi phi
    float rr;   // g^rr
    float hh;   // g^th th
};

void metricInv(float r, float th, out IM g, out IM dr, out IM dth)
{
    float M  = uM;
    float a  = uA;
    float a2 = a * a;

    float s = sin(th);
    float c = cos(th);
    if (abs(s) < 1e-3) s = (s < 0.0) ? -1e-3 : 1e-3;   // axis regularisation
    float s2 = s * s;

    float r2 = r * r;

    float Sig    = r2 + a2 * c * c;
    float dSig_r = 2.0 * r;
    float dSig_h = -2.0 * a2 * s * c;

    float Del    = r2 - 2.0 * M * r + a2;
    float dDel_r = 2.0 * r - 2.0 * M;

    float rr_a2 = r2 + a2;
    float A     = rr_a2 * rr_a2 - a2 * Del * s2;
    float dA_r  = 4.0 * r * rr_a2 - a2 * dDel_r * s2;
    float dA_h  = -2.0 * a2 * Del * s * c;

    // f = 1/(Sigma Delta)
    float f    = 1.0 / (Sig * Del);
    float f2   = f * f;
    float df_r = -(dSig_r * Del + Sig * dDel_r) * f2;
    float df_h = -(dSig_h * Del) * f2;

    // g^tt = -A f
    g.tt   = -A * f;
    dr.tt  = -(dA_r * f + A * df_r);
    dth.tt = -(dA_h * f + A * df_h);

    // g^tp = k r f,  k = -2 M a
    float k = -2.0 * M * a;
    g.tp   = k * r * f;
    dr.tp  = k * (f + r * df_r);
    dth.tp = k * r * df_h;

    // g^pp = B f  with  B = Delta/sin^2 th - a^2
    float B    = Del / s2 - a2;
    float dB_r = dDel_r / s2;
    float dB_h = -2.0 * Del * c / (s2 * s);
    g.pp   = B * f;
    dr.pp  = dB_r * f + B * df_r;
    dth.pp = dB_h * f + B * df_h;

    // g^rr = Delta/Sigma
    float invSig  = 1.0 / Sig;
    float invSig2 = invSig * invSig;
    g.rr   = Del * invSig;
    dr.rr  = (dDel_r * Sig - Del * dSig_r) * invSig2;
    dth.rr = (-Del * dSig_h) * invSig2;

    // g^thth = 1/Sigma
    g.hh   = invSig;
    dr.hh  = -dSig_r * invSig2;
    dth.hh = -dSig_h * invSig2;
}

// Hamilton's equations. x = (r, th, phi), p = (p_r, p_th).
void geodesicRHS(vec3 x, vec2 p, float E, float L, out vec3 dx, out vec2 dp)
{
    IM g, gr, gh;
    metricInv(x.x, x.y, g, gr, gh);

    float pt = -E;
    float pf =  L;

    dx.x = g.rr * p.x;                 // dr/dl     = g^rr p_r
    dx.y = g.hh * p.y;                 // dth/dl    = g^thth p_th
    dx.z = g.tp * pt + g.pp * pf;      // dphi/dl   = g^tp p_t + g^pp p_phi

    dp.x = -0.5 * (gr.tt * pt * pt + 2.0 * gr.tp * pt * pf + gr.pp * pf * pf
                 + gr.rr * p.x * p.x + gr.hh * p.y * p.y);
    dp.y = -0.5 * (gh.tt * pt * pt + 2.0 * gh.tp * pt * pf + gh.pp * pf * pf
                 + gh.rr * p.x * p.x + gh.hh * p.y * p.y);
}

void rk4Step(inout vec3 x, inout vec2 p, float E, float L, float h)
{
    vec3 a1, a2, a3, a4;
    vec2 b1, b2, b3, b4;
    geodesicRHS(x,                    p,                    E, L, a1, b1);
    geodesicRHS(x + (0.5 * h) * a1,   p + (0.5 * h) * b1,   E, L, a2, b2);
    geodesicRHS(x + (0.5 * h) * a2,   p + (0.5 * h) * b2,   E, L, a3, b3);
    geodesicRHS(x + h * a3,           p + h * b3,           E, L, a4, b4);

    x += (h / 6.0) * (a1 + 2.0 * a2 + 2.0 * a3 + a4);
    p += (h / 6.0) * (b1 + 2.0 * b2 + 2.0 * b3 + b4);
}

// =============================================================================
//  Coordinate helpers.
//
//  Boyer-Lindquist maps onto oblate spheroidal Cartesian coordinates:
//      x = sqrt(r^2+a^2) sin th cos ph
//      y = sqrt(r^2+a^2) sin th sin ph
//      z = r cos th
// =============================================================================
void toBoyerLindquist(vec3 pos, out float r, out float th, out float ph)
{
    float a2   = uA * uA;
    float rho2 = dot(pos, pos);
    float t    = rho2 - a2;
    float r2   = 0.5 * (t + sqrt(max(t * t + 4.0 * a2 * pos.z * pos.z, 0.0)));
    r  = sqrt(max(r2, 1e-8));
    th = acos(clamp(pos.z / r, -1.0, 1.0));
    ph = atan(pos.y, pos.x);
}

// Columns of the Jacobian d(x,y,z)/d(r,th,ph). They are mutually orthogonal.
void blJacobian(float r, float th, float ph, out vec3 Jr, out vec3 Jh, out vec3 Jp)
{
    float R  = sqrt(r * r + uA * uA);
    float s  = sin(th), c = cos(th);
    float cp = cos(ph), sp = sin(ph);
    Jr = vec3((r / R) * s * cp, (r / R) * s * sp, c);
    Jh = vec3(R * c * cp,       R * c * sp,      -r * s);
    Jp = vec3(-R * s * sp,      R * s * cp,       0.0);
}

// =============================================================================
//  Noise / colour utilities
// =============================================================================
float hash13(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

vec3 hash33(vec3 p)
{
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.xxy + p.yxx) * p.zyx);
}

float vnoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = hash13(i + vec3(0, 0, 0));
    float n100 = hash13(i + vec3(1, 0, 0));
    float n010 = hash13(i + vec3(0, 1, 0));
    float n110 = hash13(i + vec3(1, 1, 0));
    float n001 = hash13(i + vec3(0, 0, 1));
    float n101 = hash13(i + vec3(1, 0, 1));
    float n011 = hash13(i + vec3(0, 1, 1));
    float n111 = hash13(i + vec3(1, 1, 1));
    float nx00 = mix(n000, n100, f.x);
    float nx10 = mix(n010, n110, f.x);
    float nx01 = mix(n001, n101, f.x);
    float nx11 = mix(n011, n111, f.x);
    return mix(mix(nx00, nx10, f.y), mix(nx01, nx11, f.y), f.z);
}

float fbm3(vec3 p)
{
    float v = 0.0, amp = 0.5;
    for (int i = 0; i < 3; ++i) {
        v += amp * vnoise(p);
        p *= 2.03;
        amp *= 0.5;
    }
    return v;
}

// Planckian locus -> sRGB (Tanner Helland's fit), normalised to [0,1].
vec3 blackbodyRGB(float T)
{
    T = clamp(T, 1000.0, 40000.0);
    float t = T / 100.0;
    float r, g, b;

    if (t <= 66.0) r = 255.0;
    else           r = 329.698727446 * pow(t - 60.0, -0.1332047592);

    if (t <= 66.0) g = 99.4708025861 * log(t) - 161.1195681661;
    else           g = 288.1221695283 * pow(t - 60.0, -0.0755148492);

    if (t >= 66.0)      b = 255.0;
    else if (t <= 19.0) b = 0.0;
    else                b = 138.5177312231 * log(t - 10.0) - 305.0447927307;

    return clamp(vec3(r, g, b) / 255.0, 0.0, 1.0);
}

// =============================================================================
//  Background sky (only evaluated for photons that escape to infinity)
// =============================================================================
vec3 skyColour(vec3 d)
{
    vec3 col = vec3(0.0);

    // Three star layers of decreasing brightness / increasing density.
    for (int k = 0; k < 3; ++k) {
        float sc = 110.0 * pow(2.3, float(k));
        vec3  p  = d * sc;
        vec3  ip = floor(p);
        vec3  fp = fract(p) - 0.5;

        vec3 h1 = hash33(ip + float(k) * 19.7);
        float threshold = 0.955 - 0.020 * float(k);
        if (h1.x > threshold) {
            vec3  centre = (hash33(ip + 7.31 + float(k) * 3.1) - 0.5) * 0.55;
            float dist   = length(fp - centre);
            float core   = pow(max(0.0, 1.0 - dist / 0.11), 7.0);
            float glow   = pow(max(0.0, 1.0 - dist / 0.34), 3.0) * 0.12;
            float mag    = pow(h1.y, 3.0);
            vec3  tint   = mix(vec3(0.62, 0.74, 1.0), vec3(1.0, 0.82, 0.58), h1.z);
            col += tint * (core + glow) * mag * (2.2 - 0.55 * float(k));
        }
    }

    // A faint tilted galactic band so lensing has some large-scale structure.
    vec3  axis = normalize(vec3(0.34, 0.62, 0.71));
    float t    = dot(d, axis);
    float band = exp(-t * t * 14.0);
    float neb  = fbm3(d * 5.0) * 0.7 + fbm3(d * 13.0) * 0.3;
    // Near-neutral; the faint blue bias here was arbitrary, and it fought
    // the warm disk. Tint these if you want a coloured sky.
    col += band * neb * vec3(0.060, 0.058, 0.062);
    col += vec3(0.005, 0.005, 0.006);

    return col;
}

// =============================================================================
//  Accretion disk
//
//  The emitting gas is on a prograde circular (Keplerian) orbit, so its
//  four-velocity is u = u^t (d_t + Omega d_phi) with
//      Omega = sqrt(M) / (r^{3/2} + a sqrt(M)).
//  The photon's local energy in that frame is -p.u = u^t (E - Omega L), and
//  because we normalised the photon to unit energy in the camera frame the
//  full redshift factor (gravitational + Doppler + frame dragging) is just
//      g = 1 / [ u^t (E - Omega L) ].
// =============================================================================
vec3 sampleDisk(float r, float ph, float E, float L, out float alpha)
{
    alpha = 0.0;

    float M  = uM;
    float a  = uA;
    float sM = sqrt(M);
    float Om = sM / (r * sqrt(r) + a * sM);          // Keplerian angular velocity

    // Equatorial covariant metric components.
    float r2   = r * r;
    float g_tt = -(1.0 - 2.0 * M / r);
    float g_tp = -2.0 * M * a / r;
    float g_pp = r2 + a * a + 2.0 * M * a * a / r;

    float den = -(g_tt + 2.0 * Om * g_tp + Om * Om * g_pp);
    float ut  = (den > 1e-6) ? inversesqrt(den) : 0.0;

    float Elocal = ut * (E - Om * L);
    float g = (Elocal > 1e-5) ? (1.0 / Elocal) : 0.0;
    if (uEnableShift == 0) g = 1.0;
    g = clamp(g, 0.0, 6.0);

    // Novikov-Thorne-like radial profile:  F(r) ~ (1 - sqrt(r_in/r)) / r^3
    float u    = uDiskInner / r;
    float prof = max(1.0 - sqrt(u), 0.0);
    float base = u * u * u * prof;                   // peaks at ~0.0634
    float Tloc = uDiskTemp * pow(max(base, 0.0), 0.25) * 1.995;

    // Turbulent structure, carried around with the gas (co-rotating angle).
    float ang = ph - Om * uTime;
    vec3  q   = vec3(cos(ang), sin(ang), 0.0) * (r * 0.55) + vec3(0.0, 0.0, r * 0.85);
    float n   = fbm3(q);
    float structure = 0.72 + 0.50 * n + 0.20 * sin(ang * 3.0 - r * 0.85 + n * 5.0);
    structure = max(structure, 0.0);

    float s     = (r - uDiskInner) / max(uDiskOuter - uDiskInner, 1e-3);
    float edge  = smoothstep(0.0, 0.09, s) * smoothstep(1.0, 0.72, s);

    float emis = base * 15.8 * structure * edge;     // normalised so peak ~ 1
    alpha = clamp(uDiskOpacity * structure * edge, 0.0, 1.0);

    // Relativistic beaming: I_obs / I_emit = g^3 for the frequency-integrated
    // specific intensity ratio used here. The colour is shifted by g as well.
    vec3 colour = blackbodyRGB(Tloc * g) * emis * (g * g * g) * uDiskBrightness;
    return colour;
}

// =============================================================================
//  Adaptive affine step
// =============================================================================
float stepSize(float r, float th, float pth, float L, float rHorizon)
{
    // Base rule: large strides far away, short ones near the hole.
    float h = uStepScale * r / (1.0 + 8.0 * uM / r);

    // Keep the disk crossing well resolved.
    if (uEnableDisk == 1 && r < uDiskOuter * 1.3)
        h = min(h, 2.5 * uStepScale);

    // Boyer-Lindquist coordinates are singular on the polar axis, where the
    // g^{phi phi} ~ 1/sin^2(th) term and its derivative blow up. Limit the
    // step so theta can only ever approach the axis geometrically instead of
    // stepping across it. This costs ~15 extra steps for axis-crossing rays
    // and removes the artefact along the projected spin axis.
    float s     = max(abs(sin(th)), 1e-3);
    float Sigma = r * r + uA * uA * cos(th) * cos(th);
    float dthdl = abs(pth) / Sigma + 1e-9;
    h = min(h, 0.35 * s / dthdl);

    // Near the axis dphi/dl ~ L/(Sigma sin^2 th) also grows quickly: the photon
    // whips around the pole. Bound the azimuthal rotation per step so the
    // outgoing direction stays accurate across the axis.
    float dphidl = abs(L) / (Sigma * s * s) + 1e-9;
    h = min(h, 0.25 / dphidl);

    // Boyer-Lindquist is also singular on the horizon itself: p_r diverges as
    // Delta -> 0. Approach it geometrically so an RK4 stage can never step to
    // r < r+, where Delta changes sign and the photon would be spat back out.
    h = min(h, 0.25 * max(r - rHorizon, 1e-4));

    return max(h, 1e-5);
}

// =============================================================================
//  Trace one photon
// =============================================================================
vec3 trace(vec3 camPos, vec3 dir)
{
    float M  = uM;
    float a  = uA;
    float a2 = a * a;

    // --- initial position -----------------------------------------------------
    float r0, th0, ph0;
    toBoyerLindquist(camPos, r0, th0, ph0);

    // --- local orthonormal triad at the camera --------------------------------
    vec3 Jr, Jh, Jp;
    blJacobian(r0, th0, ph0, Jr, Jh, Jp);
    vec3 er = normalize(Jr);
    vec3 eh = normalize(Jh);
    vec3 ep = normalize(Jp);

    vec3 n = vec3(dot(dir, er), dot(dir, eh), dot(dir, ep));
    n = normalize(n);

    // --- build the photon four-momentum in the ZAMO frame ---------------------
    float s   = sin(th0);
    float c   = cos(th0);
    float Sig = r0 * r0 + a2 * c * c;
    float Del = r0 * r0 - 2.0 * M * r0 + a2;
    float rr_a2 = r0 * r0 + a2;
    float A   = rr_a2 * rr_a2 - a2 * Del * s * s;

    float alphaLapse = sqrt(max(Sig * Del / A, 1e-12));   // lapse
    float omegaDrag  = 2.0 * M * a * r0 / A;              // frame dragging
    float varpi      = max(sqrt(A / Sig) * abs(s), 1e-6); // cylindrical radius

    // p^mu = e_(t) + n_r e_(r) + n_th e_(th) + n_phi e_(phi)
    float pt_up  = 1.0 / alphaLapse;
    float pph_up = omegaDrag / alphaLapse + n.z / varpi;

    // Lower indices to get the conserved quantities.
    float g_tt = -(1.0 - 2.0 * M * r0 / Sig);
    float g_tp = -2.0 * M * a * r0 * s * s / Sig;
    float g_pp = A * s * s / Sig;

    float E = -(g_tt * pt_up + g_tp * pph_up);
    float L =  (g_tp * pt_up + g_pp * pph_up);

    float pr  = sqrt(max(Sig / Del, 0.0)) * n.x;
    float pth = sqrt(Sig) * n.y;

    // --- integrate ------------------------------------------------------------
    vec3 x = vec3(r0, th0, ph0);
    vec2 p = vec2(pr, pth);

    float rHorizon = M + sqrt(max(M * M - a2, 0.0));
    // Boyer-Lindquist degenerates at the horizon (p_r ~ 1/Delta), so stopping
    // right at r+ is numerically hopeless in 32-bit floats. Stop part-way
    // between r+ and the prograde equatorial photon orbit instead: a photon
    // that gets below the photon shell moving inward is captured regardless,
    // and this keeps Delta comfortably away from zero.
    float rPhoton = 2.0 * M * (1.0 + cos((2.0 / 3.0) * acos(clamp(-a / M, -1.0, 1.0))));
    float rStop   = rHorizon + uHorizonMargin * max(rPhoton - rHorizon, 0.0);

    vec3  colour        = vec3(0.0);
    float transmittance = 1.0;
    bool  captured      = false;
    bool  escaped       = false;

    for (int i = 0; i < uMaxSteps; ++i) {
        float r = x.x;
        if (r <= rStop)          { captured = true; break; }
        if (r >= uEscapeRadius)  { escaped  = true; break; }

        vec3 xPrev = x;
        float h = stepSize(r, x.y, p.y, L, rHorizon);
        rk4Step(x, p, E, L, h);

        // Reflect across the polar axis instead of letting theta run away.
        if (x.y < 0.0)      { x.y = -x.y;            x.z += PI; p.y = -p.y; }
        else if (x.y > PI)  { x.y = 2.0 * PI - x.y;  x.z += PI; p.y = -p.y; }

        // Equatorial-plane crossing -> possible disk hit.
        if (uEnableDisk == 1) {
            float c0 = cos(xPrev.y);
            float c1 = cos(x.y);
            if (c0 * c1 < 0.0) {
                float f    = c0 / (c0 - c1);
                float rHit = mix(xPrev.x, x.x, f);
                if (rHit > uDiskInner && rHit < uDiskOuter) {
                    float phHit = mix(xPrev.z, x.z, f);
                    float alpha;
                    vec3  emission = sampleDisk(rHit, phHit, E, L, alpha);
                    colour += transmittance * emission * alpha;
                    transmittance *= (1.0 - alpha);
                    if (transmittance < 0.015) break;
                }
            }
        }
    }

    if (escaped && transmittance > 0.0) {
        // Convert the final coordinate velocity back into a Cartesian direction.
        IM g, gr, gh;
        metricInv(x.x, x.y, g, gr, gh);
        float rdot  = g.rr * p.x;
        float thdot = g.hh * p.y;
        float phdot = g.tp * (-E) + g.pp * L;

        vec3 Br, Bh, Bp;
        blJacobian(x.x, x.y, x.z, Br, Bh, Bp);
        vec3 outDir = normalize(Br * rdot + Bh * thdot + Bp * phdot);

        colour += transmittance * skyColour(outDir);
    }

    // `captured` (and the "ran out of steps" case) contribute nothing: that is
    // the black hole shadow.
    return colour;
}



void main()
{
    // Sub-pixel jitter drives the temporal accumulation: each frame samples a
    // slightly different point inside the pixel, and the accumulate pass
    // averages them. Holding the camera still converges to a clean image.
    vec2 pix = gl_FragCoord.xy + uJitter;

    vec2 uv  = pix / uResolution;
    vec2 ndc = uv * 2.0 - 1.0;
    float aspect = uResolution.x / uResolution.y;

    vec3 dir = normalize(uCamForward
                       + uCamRight * (ndc.x * aspect * uTanHalfFov)
                       + uCamUp    * (ndc.y * uTanHalfFov));

    // Linear radiance; the present pass tone maps.
    FragColour = vec4(trace(uCamPos, dir), 1.0);
}
