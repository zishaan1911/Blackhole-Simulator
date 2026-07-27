#include "Shader.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

#ifndef SHADER_SOURCE_DIR
#define SHADER_SOURCE_DIR "shaders"
#endif

Shader::~Shader() { destroy(); }

void Shader::destroy()
{
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    m_uniforms.clear();
}

bool Shader::readFile(const std::string& path, std::string& out)
{
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

std::string Shader::resolvePath(const std::string& relativePath)
{
    const std::string candidates[] = {
        relativePath,
        "shaders/" + relativePath,
        "../shaders/" + relativePath,
        std::string(SHADER_SOURCE_DIR) + "/" + relativePath,
    };
    for (const auto& c : candidates) {
        std::ifstream f(c);
        if (f.good()) return c;
    }
    return relativePath;
}

GLuint Shader::compile(GLenum stage, const std::string& src, const std::string& label)
{
    GLuint sh = glCreateShader(stage);
    const char* csrc = src.c_str();
    GLint len = static_cast<GLint>(src.size());
    glShaderSource(sh, 1, &csrc, &len);
    glCompileShader(sh);

    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen > 1 ? logLen : 1, '\0');
        glGetShaderInfoLog(sh, logLen, nullptr, log.data());
        std::fprintf(stderr, "[shader] compile failed (%s):\n%s\n", label.c_str(), log.data());
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static bool linkProgram(GLuint prog)
{
    glLinkProgram(prog);
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen > 1 ? logLen : 1, '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        std::fprintf(stderr, "[shader] link failed:\n%s\n", log.data());
        return false;
    }
    return true;
}

bool Shader::loadGraphics(const std::string& vertPath, const std::string& fragPath)
{
    const std::string vp = resolvePath(vertPath);
    const std::string fp = resolvePath(fragPath);
    std::string vsrc, fsrc;
    if (!readFile(vp, vsrc) || !readFile(fp, fsrc)) {
        std::fprintf(stderr, "[shader] cannot open '%s' / '%s'\n", vp.c_str(), fp.c_str());
        return false;
    }

    GLuint vs = compile(GL_VERTEX_SHADER, vsrc, vp);
    if (!vs) return false;
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsrc, fp);
    if (!fs) { glDeleteShader(vs); return false; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    bool ok = linkProgram(prog);
    glDetachShader(prog, vs);
    glDetachShader(prog, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!ok) {
        glDeleteProgram(prog);
        return false;
    }

    destroy();
    m_program = prog;
    return true;
}

