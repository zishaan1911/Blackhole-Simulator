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

