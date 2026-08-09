#include "Renderer.hpp"

#include "PngWriter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

// Low-discrepancy sequence for sub-pixel jitter. Halton beats plain random
// here: successive samples spread evenly over the pixel instead of clumping,
// so the accumulated image converges noticeably faster.
float halton(int index, int base)
{
    float f = 1.0f, r = 0.0f;
    int i = index;
    while (i > 0) {
        f /= static_cast<float>(base);
        r += f * static_cast<float>(i % base);
        i /= base;
    }
    return r;
}

}  // namespace

bool Renderer::init(int framebufferWidth, int framebufferHeight)
{
    if (!reloadShaders()) return false;

    glGenVertexArrays(1, &m_vao);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    m_fbW = std::max(1, framebufferWidth);
    m_fbH = std::max(1, framebufferHeight);
    resize(m_fbW, m_fbH);
    return m_traceTarget.fbo != 0;
}

bool Renderer::reloadShaders()
{
    // Shader::load* only swaps in the new program once it has linked, so a
    // failed hot-reload leaves the running programs untouched.
    if (!m_trace.loadGraphics("fullscreen.vert", "kerr_raytrace.frag")) return false;
    if (!m_accum.loadGraphics("fullscreen.vert", "accumulate.frag"))    return false;
    if (!m_present.loadGraphics("fullscreen.vert", "present.frag"))     return false;
    invalidate();
    return true;
}

