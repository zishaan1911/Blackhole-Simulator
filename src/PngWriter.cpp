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

bool writePng(const std::string& path, int width, int height,
              const uint8_t* rgb, bool flipVertically)
{
    if (width <= 0 || height <= 0 || !rgb) return false;

    // --- raw scanlines, each prefixed with filter type 0 (None) -------------
    const size_t stride = static_cast<size_t>(width) * 3;
    std::vector<uint8_t> raw;
    raw.reserve((stride + 1) * static_cast<size_t>(height));
    for (int y = 0; y < height; ++y) {
        const int src = flipVertically ? (height - 1 - y) : y;
        raw.push_back(0);
        const uint8_t* row = rgb + static_cast<size_t>(src) * stride;
        raw.insert(raw.end(), row, row + stride);
    }

    // --- zlib stream: 2-byte header, stored deflate blocks, adler32 ---------
    std::vector<uint8_t> z;
    z.push_back(0x78);  // CMF: deflate, 32K window
    z.push_back(0x01);  // FLG: no dict, fastest; (0x7801 % 31) == 0

    const size_t kMaxBlock = 65535;
    size_t offset = 0;
    while (offset < raw.size()) {
        const size_t n = (raw.size() - offset < kMaxBlock) ? (raw.size() - offset) : kMaxBlock;
        const bool last = (offset + n >= raw.size());
        z.push_back(last ? 1 : 0);
        z.push_back(uint8_t(n & 0xFF));
        z.push_back(uint8_t((n >> 8) & 0xFF));
        z.push_back(uint8_t((~n) & 0xFF));
        z.push_back(uint8_t(((~n) >> 8) & 0xFF));
        z.insert(z.end(), raw.begin() + offset, raw.begin() + offset + n);
        offset += n;
    }
    push32be(z, adler32Of(raw.data(), raw.size()));

    // --- assemble the file --------------------------------------------------
    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

    std::vector<uint8_t> ihdr;
    push32be(ihdr, static_cast<uint32_t>(width));
    push32be(ihdr, static_cast<uint32_t>(height));
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(2);  // colour type 2 = truecolour RGB
    ihdr.push_back(0);  // deflate
    ihdr.push_back(0);  // adaptive filtering
    ihdr.push_back(0);  // no interlace
    writeChunk(png, "IHDR", ihdr);
    writeChunk(png, "IDAT", z);
    writeChunk(png, "IEND", {});

    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const size_t written = std::fwrite(png.data(), 1, png.size(), f);
    std::fclose(f);
    return written == png.size();
}
