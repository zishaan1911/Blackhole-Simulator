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

    void set(const char* name, int v);
    void set(const char* name, float v);
    void set(const char* name, float x, float y);
    void set(const char* name, float x, float y, float z);

    // Searches a few sensible locations for `relativePath`.
    static std::string resolvePath(const std::string& relativePath);
    static bool readFile(const std::string& path, std::string& out);

private:
    GLint  location(const char* name);
    static GLuint compile(GLenum stage, const std::string& src, const std::string& label);

    GLuint m_program = 0;
    std::unordered_map<std::string, GLint> m_uniforms;
};
