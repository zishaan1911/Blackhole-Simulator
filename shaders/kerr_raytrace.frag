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
