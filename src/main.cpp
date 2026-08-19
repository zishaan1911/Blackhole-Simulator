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

int main()
{
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialise GLFW\n");
        return EXIT_FAILURE;
    }

    // 3.3 core is all this needs: the geodesic integration runs in a fragment
    // shader, not a compute shader, so it works on GPUs without compute support
    // (Intel HD 4000 and similar).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Kerr Black Hole", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr,
                     "Failed to create an OpenGL 3.3 core context.\n"
                     "Update your graphics driver; if the GPU predates ~2010 it\n"
                     "may not support OpenGL 3.3 at all.\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "Failed to load the required OpenGL entry points\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    std::printf("GL renderer : %s\n", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    std::printf("GL version  : %s\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    App app;

    // -------------------------------------------------------------------------
    //  Scene setup. Everything below is safe to edit.
    // -------------------------------------------------------------------------
    app.hole.mass           = 1.0f;    // M, geometrised units
    app.hole.spin           = 0.85f;   // a/M
    app.hole.diskOuterRadii = 20.0f;   // outer disk edge, in units of M
    app.hole.diskTemperature = 6500.0f;
    app.hole.diskBrightness  = 0.50f;
    app.hole.diskOpacity     = 0.80f;

    app.camera.setDefaults(/*distance*/ 26.0f * app.hole.mass,
                           /*yaw     */ 0.0f,
                           /*pitch   */ 0.16f);   // ~9 degrees above the disk
    app.camera.setFovY(1.0f);
    app.camera.minDistance = app.hole.horizonRadius() * 2.5f;
    app.camera.maxDistance = 400.0f * app.hole.mass;

    app.sim.baseRate = 6.0;            // units of M per real second at speed 1

    // Conservative default: this runs on GPUs without compute shaders, which
    // are generally slow. 0.25 of a 1280x720 window is 320x180 traced pixels.
    // Raise it with ']' if your GPU can take it; temporal accumulation cleans
    // up the softness within a second of holding the camera still.
    app.renderer.setRenderScale(0.25f);
    app.renderer.maxSteps     = 320;
    app.renderer.stepScale    = 0.25f;
    app.renderer.escapeRadius = 1000.0f * app.hole.mass;
    app.renderer.exposure     = 1.0f;
    // -------------------------------------------------------------------------

    glfwSetWindowUserPointer(window, &app);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    if (!app.renderer.init(fbW, fbH)) {
        std::fprintf(stderr, "Renderer initialisation failed (see shader errors above)\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    printControls();
    std::printf("  horizon r+ = %.4f M   photon sphere = %.4f M   ISCO = %.4f M\n\n",
                app.hole.horizonRadius() / app.hole.mass,
                app.hole.photonSphereRadius() / app.hole.mass,
                app.hole.iscoRadius() / app.hole.mass);

    double lastTime  = glfwGetTime();
    double titleTime = lastTime;
    double fpsAccum  = 0.0;
    int    fpsFrames = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const double now = glfwGetTime();
        const double dt  = std::min(now - lastTime, 0.25);
        lastTime = now;

        // Held keys.
        bool moved = false;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            app.camera.zoom(-app.keyZoomSpeed * static_cast<float>(dt)); moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            app.camera.zoom(app.keyZoomSpeed * static_cast<float>(dt)); moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            app.camera.addYaw(app.keyOrbitSpeed * static_cast<float>(dt)); moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            app.camera.addYaw(-app.keyOrbitSpeed * static_cast<float>(dt)); moved = true;
        }

        const double tBefore = app.sim.time();
        app.sim.update(dt);
        // Any change to the image invalidates the accumulated samples.
        if (moved || app.sim.time() != tBefore) app.renderer.invalidate();

        if (app.screenshotRequested) {
            app.screenshotRequested = false;
            char name[128];
            std::snprintf(name, sizeof(name), "kerr_%.0f.png", glfwGetTime() * 1000.0);
            app.renderer.screenshot(app.camera, app.hole, app.sim, name,
                                    app.shotWidth, app.shotHeight,
                                    app.shotSteps, app.shotSamples);
        }

        app.renderer.render(app.camera, app.hole, app.sim);

        glfwSwapBuffers(window);

        // Window title acts as the HUD, since there is no GUI.
        fpsAccum += dt;
        ++fpsFrames;
        if (now - titleTime > 0.4) {
            const double fps = (fpsAccum > 0.0) ? (fpsFrames / fpsAccum) : 0.0;
            char title[320];
            std::snprintf(title, sizeof(title),
                          "Kerr Black Hole  |  a/M %.2f  M %.2f  |  r %.1f M  |  t %.1f M  "
                          "|  speed %.2fx%s  |  %dx%d  steps %d  spp %d  |  %.0f FPS",
                          app.hole.spin, app.hole.mass,
                          app.camera.distance() / app.hole.mass,
                          app.sim.time(), app.sim.speed(),
                          app.sim.paused() ? "  [PAUSED]" : "",
                          app.renderer.traceWidth(), app.renderer.traceHeight(),
                          app.renderer.maxSteps, app.renderer.sampleCount(), fps);
            glfwSetWindowTitle(window, title);
            titleTime = now;
            fpsAccum  = 0.0;
            fpsFrames = 0;
        }
    }

    app.renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
