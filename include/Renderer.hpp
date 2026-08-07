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

};
