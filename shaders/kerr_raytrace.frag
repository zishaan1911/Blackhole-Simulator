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

