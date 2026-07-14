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

