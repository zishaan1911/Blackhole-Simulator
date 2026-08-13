#pragma once

#include <cstdint>
#include <string>

// Writes an 8-bit RGB PNG with no external dependencies.
//
// `rgb` must hold width*height*3 bytes, top-left origin unless
// `flipVertically` is set (which is what glReadPixels needs, since GL returns
// rows bottom-up).
bool writePng(const std::string& path, int width, int height,
              const uint8_t* rgb, bool flipVertically = false);
