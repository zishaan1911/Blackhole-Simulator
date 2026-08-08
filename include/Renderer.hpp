#pragma once

#include "BlackHole.hpp"
#include "Camera.hpp"
#include "Shader.hpp"
#include "Simulation.hpp"
#include "glad_min.h"

#include <string>

// Three passes, all OpenGL 3.3:
//
//   1. trace       - integrates geodesics into a float target (linear radiance)
//   2. accumulate  - running mean of that target across frames (antialiasing)
//   3. present     - tone maps the accumulator to the default framebuffer
//
// The trace pass is a fragment shader rather than a compute shader so this runs
// on hardware without compute support (Intel HD 4000 and similar). The physics
// is identical either way.
class Renderer {
public:
    bool init(int framebufferWidth, int framebufferHeight);
    void shutdown();

    void resize(int framebufferWidth, int framebufferHeight);
    void render(const Camera& cam, const BlackHole& bh, const Simulation& sim);

    bool reloadShaders();

    // Discards accumulated samples. Call whenever the image would change:
    // camera moved, simulation advanced, parameters edited.
    void invalidate() { m_sampleIndex = 0; }
    int  sampleCount() const { return m_sampleIndex; }

    // Renders one high-quality frame off-screen and writes it to `path`.
    // Independent of the interactive settings, so a slow GPU can still produce
    // clean stills. Returns false if anything failed (see stderr).
    bool screenshot(const Camera& cam, const BlackHole& bh, const Simulation& sim,
                    const std::string& path, int width, int height,
                    int steps, int samples);

    // ---- Quality knobs ------------------------------------------------------
    void  setRenderScale(float s);
    float renderScale() const { return m_renderScale; }

    int   maxSteps     = 320;      // integration steps per photon
    float stepScale    = 0.25f;    // affine step ~ stepScale * r
    float escapeRadius = 1000.0f;  // treated as infinity
    float exposure     = 1.0f;
    bool  showDisk     = true;
    bool  enableShift  = true;     // redshift / Doppler / beaming
    bool  accumulate   = true;

    // Where a photon is declared captured, as a fraction of the gap between the
    // horizon r+ (0.0) and the prograde photon orbit (1.0). Boyer-Lindquist
    // coordinates degenerate at r+, so stopping strictly there is numerically
    // unusable in 32-bit floats. 0.75 leaves the measured shadow size exact to
    // ~2e-7 while eliminating rays that leak back out of the horizon.
    float horizonMargin = 0.75f;

    int traceWidth()  const { return m_traceW; }
    int traceHeight() const { return m_traceH; }

private:
    struct Target {
        GLuint fbo = 0;
        GLuint tex = 0;
    };

    bool  createTarget(Target& t, int w, int h);
    void  destroyTarget(Target& t);
    void  setTraceUniforms(const Camera& cam, const BlackHole& bh,
                           const Simulation& sim, int w, int h,
                           int steps, float jx, float jy);
    void  drawFullscreen();

    Shader m_trace;
    Shader m_accum;
    Shader m_present;

    Target m_traceTarget;      // this frame's raw trace
    Target m_accumTarget[2];   // ping-pong accumulator
    int    m_accumFront = 0;

    GLuint m_vao = 0;

    int   m_fbW = 0, m_fbH = 0;
    int   m_traceW = 0, m_traceH = 0;
    float m_renderScale = 0.25f;
    int   m_sampleIndex = 0;
};
