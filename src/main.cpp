// =============================================================================
//  Real-time Kerr black hole ray tracer.
//
//  Controls
//    Mouse drag ....... orbit the camera
//    Scroll ........... zoom
//    W / S ............ zoom in / out
//    A / D ............ rotate around the black hole
//    Space ............ pause / resume
//    R ................ reset camera + simulation
//    T / G ............ increase / decrease simulation speed
//    Esc .............. quit
//
//  Extras (not required, but handy)
//    Q / E ............ decrease / increase spin a/M
//    K ................ toggle the accretion disk
//    L ................ toggle redshift / Doppler / beaming
//    [ / ] ............ lower / raise render resolution scale
//    - / = ............ fewer / more integration steps
//    P ................ save a high-quality screenshot (PNG)
//    F2 ............... toggle temporal accumulation
//    F5 ............... hot-reload the shaders
// =============================================================================

#include "BlackHole.hpp"
#include "Camera.hpp"
#include "Renderer.hpp"
#include "PngWriter.hpp"
#include "Simulation.hpp"
#include "glad_min.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

struct App {
    Camera     camera;
    BlackHole  hole;
    Simulation sim;
    Renderer   renderer;

    bool   dragging = false;
    double lastX = 0.0, lastY = 0.0;

    bool  screenshotRequested = false;

    // Screenshot settings, deliberately independent of the interactive ones so
    // a slow GPU can still produce clean stills.
    int shotWidth   = 1600;
    int shotHeight  = 900;
    int shotSteps   = 600;
    int shotSamples = 32;

    float mouseSensitivity = 0.0045f;
    float keyOrbitSpeed    = 0.9f;   // rad/s
    float keyZoomSpeed     = 1.1f;   // log units/s
};

App* appFromWindow(GLFWwindow* w)
{
    return static_cast<App*>(glfwGetWindowUserPointer(w));
}

