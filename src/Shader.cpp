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

