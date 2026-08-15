#include "PngWriter.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// PNG needs zlib-wrapped DEFLATE data. Rather than pull in zlib for what is a
// once-per-keypress screenshot, this emits DEFLATE "stored" (uncompressed)
// blocks, which are perfectly legal and every decoder accepts. Files come out
// slightly larger than a compressed encoder would produce, which is a fine
// trade for having no dependency at all.
// ---------------------------------------------------------------------------

uint32_t crc32Of(const uint8_t* data, size_t len, uint32_t crc = 0)
{
    static uint32_t table[256];
    static bool built = false;
    if (!built) {
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[n] = c;
        }
        built = true;
    }
    crc = crc ^ 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

uint32_t adler32Of(const uint8_t* data, size_t len)
{
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

void push32be(std::vector<uint8_t>& v, uint32_t x)
{
    v.push_back(uint8_t(x >> 24));
    v.push_back(uint8_t(x >> 16));
    v.push_back(uint8_t(x >> 8));
    v.push_back(uint8_t(x));
}

void writeChunk(std::vector<uint8_t>& out, const char tag[4],
                const std::vector<uint8_t>& payload)
{
    push32be(out, static_cast<uint32_t>(payload.size()));
    std::vector<uint8_t> withTag;
    withTag.reserve(4 + payload.size());
    withTag.insert(withTag.end(), tag, tag + 4);
    withTag.insert(withTag.end(), payload.begin(), payload.end());
    out.insert(out.end(), withTag.begin(), withTag.end());
    push32be(out, crc32Of(withTag.data(), withTag.size()));
}

}  // namespace

}
