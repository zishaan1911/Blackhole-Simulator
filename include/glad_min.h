// glad_min.h -----------------------------------------------------------------
//
// A tiny, self-contained, GLAD-compatible loader for the subset of OpenGL 3.3
// core that this project uses. It exposes the same entry point as GLAD
// (`gladLoadGLLoader`) so you can delete this file and drop in the official
// GLAD generator output without touching any other source file.
//
// Do NOT include <GL/gl.h> anywhere; always build GLFW with GLFW_INCLUDE_NONE.
// -----------------------------------------------------------------------------
#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#define GLAPIENTRY __stdcall
#else
#define GLAPIENTRY
#endif

// --- Core GL types -----------------------------------------------------------
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef int            GLsizei;
typedef float          GLfloat;
typedef double         GLdouble;
typedef char           GLchar;
typedef ptrdiff_t      GLintptr;
typedef ptrdiff_t      GLsizeiptr;

// --- Enums we need -----------------------------------------------------------
#define GL_FALSE                            0
#define GL_TRUE                             1
#define GL_NO_ERROR                         0
#define GL_TRIANGLES                        0x0004
#define GL_FLOAT                            0x1406
#define GL_RENDERER                         0x1F01
#define GL_VERSION                          0x1F02
#define GL_RGBA                             0x1908
#define GL_RGBA32F                          0x8814
#define GL_TEXTURE_2D                       0x0DE1
#define GL_TEXTURE_MAG_FILTER               0x2800
#define GL_TEXTURE_MIN_FILTER               0x2801
#define GL_TEXTURE_WRAP_S                   0x2802
#define GL_TEXTURE_WRAP_T                   0x2803
#define GL_LINEAR                           0x2601
#define GL_NEAREST                          0x2600
#define GL_CLAMP_TO_EDGE                    0x812F
#define GL_TEXTURE0                         0x84C0
#define GL_COLOR_BUFFER_BIT                 0x00004000
#define GL_VERTEX_SHADER                    0x8B31
#define GL_FRAGMENT_SHADER                  0x8B30
#define GL_COMPILE_STATUS                   0x8B81
#define GL_LINK_STATUS                      0x8B82
#define GL_INFO_LOG_LENGTH                  0x8B84
#define GL_READ_ONLY                        0x88B8
#define GL_WRITE_ONLY                       0x88B9
#define GL_READ_WRITE                       0x88BA
#define GL_FRAMEBUFFER                      0x8D40
#define GL_READ_FRAMEBUFFER                 0x8CA8
#define GL_DRAW_FRAMEBUFFER                 0x8CA9
#define GL_COLOR_ATTACHMENT0                0x8CE0
#define GL_FRAMEBUFFER_COMPLETE             0x8CD5
#define GL_RGB                              0x1907
#define GL_UNSIGNED_BYTE                    0x1401
#define GL_PACK_ALIGNMENT                   0x0D05
#define GL_UNPACK_ALIGNMENT                 0x0CF5
#define GL_FRAMEBUFFER_SRGB                 0x8DB9

// --- Function list -----------------------------------------------------------
// X(returnType, name, (argTypes...))
#define GLAD_MIN_FUNCTION_LIST(X)                                                        \
    X(void,               Clear,               (GLbitfield))                             \
    X(void,               ClearColor,          (GLfloat, GLfloat, GLfloat, GLfloat))     \
    X(void,               Viewport,            (GLint, GLint, GLsizei, GLsizei))         \
    X(const GLubyte*,     GetString,           (GLenum))                                 \
    X(void,               GetIntegerv,         (GLenum, GLint*))                         \
    X(GLenum,             GetError,            (void))                                   \
    X(void,               GenTextures,         (GLsizei, GLuint*))                       \
    X(void,               DeleteTextures,      (GLsizei, const GLuint*))                 \
    X(void,               BindTexture,         (GLenum, GLuint))                         \
    X(void,               TexParameteri,       (GLenum, GLenum, GLint))                  \
    X(void,               TexImage2D,          (GLenum, GLint, GLint, GLsizei, GLsizei,  \
                                                GLint, GLenum, GLenum, const void*))     \
    X(void,               ActiveTexture,       (GLenum))                                 \
    X(GLuint,             CreateShader,        (GLenum))                                 \
    X(void,               ShaderSource,        (GLuint, GLsizei,                         \
                                                const GLchar* const*, const GLint*))     \
    X(void,               CompileShader,       (GLuint))                                 \
    X(void,               GetShaderiv,         (GLuint, GLenum, GLint*))                 \
    X(void,               GetShaderInfoLog,    (GLuint, GLsizei, GLsizei*, GLchar*))     \
    X(void,               DeleteShader,        (GLuint))                                 \
    X(GLuint,             CreateProgram,       (void))                                   \
    X(void,               AttachShader,        (GLuint, GLuint))                         \
    X(void,               DetachShader,        (GLuint, GLuint))                         \
    X(void,               LinkProgram,         (GLuint))                                 \
    X(void,               GetProgramiv,        (GLuint, GLenum, GLint*))                 \
    X(void,               GetProgramInfoLog,   (GLuint, GLsizei, GLsizei*, GLchar*))     \
    X(void,               UseProgram,          (GLuint))                                 \
    X(void,               DeleteProgram,       (GLuint))                                 \
    X(GLint,              GetUniformLocation,  (GLuint, const GLchar*))                  \
    X(void,               Uniform1i,           (GLint, GLint))                           \
    X(void,               Uniform1f,           (GLint, GLfloat))                         \
    X(void,               Uniform2f,           (GLint, GLfloat, GLfloat))                \
    X(void,               Uniform3f,           (GLint, GLfloat, GLfloat, GLfloat))       \
    X(void,               Uniform4f,           (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) \
    X(void,               GenVertexArrays,     (GLsizei, GLuint*))                       \
    X(void,               BindVertexArray,     (GLuint))                                 \
    X(void,               DeleteVertexArrays,  (GLsizei, const GLuint*))                 \
    X(void,               DrawArrays,          (GLenum, GLint, GLsizei))                 \
    X(void,               GenFramebuffers,     (GLsizei, GLuint*))                       \
    X(void,               DeleteFramebuffers,  (GLsizei, const GLuint*))                 \
    X(void,               BindFramebuffer,     (GLenum, GLuint))                         \
    X(void,               FramebufferTexture2D,(GLenum, GLenum, GLenum, GLuint, GLint))  \
    X(GLenum,             CheckFramebufferStatus, (GLenum))                              \
    X(void,               DrawBuffers,         (GLsizei, const GLenum*))                 \
    X(void,               ReadPixels,          (GLint, GLint, GLsizei, GLsizei,          \
                                                GLenum, GLenum, void*))                  \
    X(void,               PixelStorei,         (GLenum, GLint))

#define GLAD_MIN_DECLARE(ret, name, args)            \
    typedef ret(GLAPIENTRY* PFN_gl##name) args;      \
    extern PFN_gl##name gl##name;
GLAD_MIN_FUNCTION_LIST(GLAD_MIN_DECLARE)
#undef GLAD_MIN_DECLARE

typedef void* (*GLADloadproc)(const char* name);

// Returns true when every required entry point was resolved.
bool gladLoadGLLoader(GLADloadproc load);
