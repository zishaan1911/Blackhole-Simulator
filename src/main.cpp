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

void keyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    App* app = appFromWindow(window);
    if (!app) return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_SPACE:
        if (action == GLFW_PRESS) app->sim.togglePause();
        break;

    case GLFW_KEY_R:
        if (action == GLFW_PRESS) {
            app->camera.reset();
            app->sim.reset();
            app->renderer.invalidate();
        }
        break;

    case GLFW_KEY_T: app->sim.multiplySpeed(1.25); break;
    case GLFW_KEY_G: app->sim.multiplySpeed(1.0 / 1.25); break;

    case GLFW_KEY_Q:
        app->hole.setSpin(app->hole.spin - 0.02f);
        app->renderer.invalidate();
        break;
    case GLFW_KEY_E:
        app->hole.setSpin(app->hole.spin + 0.02f);
        app->renderer.invalidate();
        break;

    case GLFW_KEY_K:
        if (action == GLFW_PRESS) {
            app->renderer.showDisk = !app->renderer.showDisk;
            app->renderer.invalidate();
        }
        break;

    case GLFW_KEY_L:
        if (action == GLFW_PRESS) {
            app->renderer.enableShift = !app->renderer.enableShift;
            app->renderer.invalidate();
        }
        break;

    case GLFW_KEY_P:
        // Deferred: the render loop owns the GL context flow.
        if (action == GLFW_PRESS) app->screenshotRequested = true;
        break;

    case GLFW_KEY_F2:
        if (action == GLFW_PRESS) {
            app->renderer.accumulate = !app->renderer.accumulate;
            app->renderer.invalidate();
            std::printf("[render] temporal accumulation %s\n",
                        app->renderer.accumulate ? "on" : "off");
        }
        break;

    case GLFW_KEY_LEFT_BRACKET:
        app->renderer.setRenderScale(app->renderer.renderScale() - 0.05f);
        break;
    case GLFW_KEY_RIGHT_BRACKET:
        app->renderer.setRenderScale(app->renderer.renderScale() + 0.05f);
        break;

    case GLFW_KEY_MINUS:
        app->renderer.maxSteps = std::max(40, app->renderer.maxSteps - 20);
        app->renderer.invalidate();
        break;
    case GLFW_KEY_EQUAL:
        app->renderer.maxSteps = std::min(2000, app->renderer.maxSteps + 20);
        app->renderer.invalidate();
        break;

    case GLFW_KEY_F5:
        if (action == GLFW_PRESS) {
            if (app->renderer.reloadShaders()) std::printf("[shader] reloaded\n");
        }
        break;

    default:
        break;
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    App* app = appFromWindow(window);
    if (!app || button != GLFW_MOUSE_BUTTON_LEFT) return;

    if (action == GLFW_PRESS) {
        app->dragging = true;
        glfwGetCursorPos(window, &app->lastX, &app->lastY);
    } else if (action == GLFW_RELEASE) {
        app->dragging = false;
    }
}

void cursorPosCallback(GLFWwindow* window, double x, double y)
{
    App* app = appFromWindow(window);
    if (!app || !app->dragging) return;

    const float dx = static_cast<float>(x - app->lastX);
    const float dy = static_cast<float>(y - app->lastY);
    app->lastX = x;
    app->lastY = y;

    app->camera.orbit(-dx * app->mouseSensitivity, dy * app->mouseSensitivity);
    app->renderer.invalidate();
}

void scrollCallback(GLFWwindow* window, double, double yoffset)
{
    App* app = appFromWindow(window);
    if (!app) return;
    app->camera.zoom(static_cast<float>(-yoffset) * 0.12f);
    app->renderer.invalidate();
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    App* app = appFromWindow(window);
    if (!app) return;
    app->renderer.resize(width, height);
}

void printControls()
{
    std::printf(
        "\n"
        "  Kerr black hole ray tracer\n"
        "  --------------------------\n"
        "  Mouse drag   orbit camera            Space   pause / resume\n"
        "  Scroll       zoom                    R       reset\n"
        "  W / S        zoom in / out           T / G   sim speed up / down\n"
        "  A / D        rotate around hole      Esc     quit\n"
        "\n"
        "  Q / E   spin a/M down / up           K   toggle disk\n"
        "  [ / ]   render scale down / up       L   toggle redshift + Doppler\n"
        "  - / =   fewer / more RK4 steps       P   save screenshot (PNG)\n"
        "  F2      toggle accumulation          F5  reload shaders\n"
        "\n");
}

}  // namespace

