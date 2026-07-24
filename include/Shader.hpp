#pragma once

#include "glad_min.h"

#include <string>
#include <unordered_map>

// Thin wrapper around a linked GL program (graphics or compute).
class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Builds a vertex + fragment program.
    bool loadGraphics(const std::string& vertPath, const std::string& fragPath);

    void use() const;
    void destroy();
    GLuint id() const { return m_program; }
    bool valid() const { return m_program != 0; }

};
