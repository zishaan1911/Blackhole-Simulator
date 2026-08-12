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

bool Renderer::createTarget(Target& t, int w, int h)
{
    destroyTarget(t);

    glGenTextures(1, &t.tex);
    glBindTexture(GL_TEXTURE_2D, t.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &t.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);

    const GLenum bufs[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, bufs);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr,
                     "[gl] float framebuffer incomplete (0x%04X) at %dx%d.\n"
                     "     RGBA32F render targets are required.\n", status, w, h);
        destroyTarget(t);
        return false;
    }
    return true;
}

void Renderer::setRenderScale(float s)
{
    m_renderScale = std::clamp(s, 0.05f, 1.0f);
    resize(m_fbW, m_fbH);
}

void Renderer::resize(int framebufferWidth, int framebufferHeight)
{
    m_fbW = std::max(1, framebufferWidth);
    m_fbH = std::max(1, framebufferHeight);

    const int w = std::max(16, static_cast<int>(m_fbW * m_renderScale));
    const int h = std::max(16, static_cast<int>(m_fbH * m_renderScale));
    if (w == m_traceW && h == m_traceH && m_traceTarget.fbo) return;

    m_traceW = w;
    m_traceH = h;
    createTarget(m_traceTarget, w, h);
    createTarget(m_accumTarget[0], w, h);
    createTarget(m_accumTarget[1], w, h);
    invalidate();
}

void Renderer::drawFullscreen()
{
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Renderer::setTraceUniforms(const Camera& cam, const BlackHole& bh,
                                const Simulation& sim, int w, int h,
                                int steps, float jx, float jy)
{
    const glm::vec3 pos = cam.position();
    const glm::vec3 fwd = cam.forward();
    const glm::vec3 rgt = cam.right();
    const glm::vec3 upv = cam.up();

    m_trace.set("uResolution", static_cast<float>(w), static_cast<float>(h));
    m_trace.set("uCamPos",     pos.x, pos.y, pos.z);
    m_trace.set("uCamForward", fwd.x, fwd.y, fwd.z);
    m_trace.set("uCamRight",   rgt.x, rgt.y, rgt.z);
    m_trace.set("uCamUp",      upv.x, upv.y, upv.z);
    m_trace.set("uTanHalfFov", cam.tanHalfFovY());

    m_trace.set("uM", bh.mass);
    m_trace.set("uA", bh.a());

    m_trace.set("uDiskInner",      bh.diskInner());
    m_trace.set("uDiskOuter",      bh.diskOuter());
    m_trace.set("uDiskTemp",       bh.diskTemperature);
    m_trace.set("uDiskBrightness", bh.diskBrightness);
    m_trace.set("uDiskOpacity",    bh.diskOpacity);
    m_trace.set("uEnableDisk",     showDisk ? 1 : 0);

    m_trace.set("uTime",          static_cast<float>(sim.time()));
    m_trace.set("uMaxSteps",      steps);
    m_trace.set("uStepScale",     stepScale);
    m_trace.set("uEscapeRadius",  escapeRadius);
    m_trace.set("uHorizonMargin", horizonMargin);
    m_trace.set("uEnableShift",   enableShift ? 1 : 0);
    m_trace.set("uJitter",        jx, jy);
}

