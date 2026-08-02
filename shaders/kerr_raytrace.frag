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

