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

void Renderer::shutdown()
{
    destroyTarget(m_traceTarget);
    destroyTarget(m_accumTarget[0]);
    destroyTarget(m_accumTarget[1]);
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    m_trace.destroy();
    m_accum.destroy();
    m_present.destroy();
}

void Renderer::destroyTarget(Target& t)
{
    if (t.fbo) { glDeleteFramebuffers(1, &t.fbo); t.fbo = 0; }
    if (t.tex) { glDeleteTextures(1, &t.tex);     t.tex = 0; }
}

