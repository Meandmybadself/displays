// Tiny dependency-free PNG writer for the host preview.
//
// Emits a truecolor (8-bit RGB) PNG using uncompressed DEFLATE ("stored") blocks,
// so there's no zlib/libpng to link. Plenty fast for a 960x540 preview.
#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

namespace png {

inline uint32_t crc32(const uint8_t* p, size_t n, uint32_t crc = 0) {
    crc = ~crc;
    for (size_t i = 0; i < n; ++i) {
        crc ^= p[i];
        for (int k = 0; k < 8; ++k)
            crc = (crc >> 1) ^ (0xEDB88320u & (-(int32_t)(crc & 1)));
    }
    return ~crc;
}

inline uint32_t adler32(const uint8_t* p, size_t n) {
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < n; ++i) { a = (a + p[i]) % 65521; b = (b + a) % 65521; }
    return (b << 16) | a;
}

inline void put_be32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(x >> 24); v.push_back(x >> 16); v.push_back(x >> 8); v.push_back(x);
}

inline void chunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data) {
    put_be32(out, (uint32_t)data.size());
    size_t crc_start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    uint32_t c = crc32(out.data() + crc_start, out.size() - crc_start);
    put_be32(out, c);
}

// rgb: w*h*3 bytes, row-major. Returns true on success.
inline bool write_rgb(const char* path, const uint8_t* rgb, int w, int h) {
    std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

    std::vector<uint8_t> ihdr;
    put_be32(ihdr, (uint32_t)w);
    put_be32(ihdr, (uint32_t)h);
    ihdr.push_back(8);    // bit depth
    ihdr.push_back(2);    // color type: truecolor RGB
    ihdr.push_back(0);    // compression
    ihdr.push_back(0);    // filter
    ihdr.push_back(0);    // interlace
    chunk(out, "IHDR", ihdr);

    // Raw image: each row prefixed with filter byte 0.
    std::vector<uint8_t> raw;
    raw.reserve((size_t)h * (1 + (size_t)w * 3));
    for (int y = 0; y < h; ++y) {
        raw.push_back(0);
        const uint8_t* row = rgb + (size_t)y * w * 3;
        raw.insert(raw.end(), row, row + (size_t)w * 3);
    }

    // zlib stream wrapping DEFLATE stored blocks.
    std::vector<uint8_t> zlib;
    zlib.push_back(0x78); zlib.push_back(0x01);  // CMF/FLG
    size_t off = 0;
    while (off < raw.size()) {
        size_t n = raw.size() - off;
        if (n > 65535) n = 65535;
        bool final = (off + n >= raw.size());
        zlib.push_back(final ? 1 : 0);
        zlib.push_back(n & 0xFF); zlib.push_back((n >> 8) & 0xFF);
        uint16_t nlen = ~(uint16_t)n;
        zlib.push_back(nlen & 0xFF); zlib.push_back((nlen >> 8) & 0xFF);
        zlib.insert(zlib.end(), raw.begin() + off, raw.begin() + off + n);
        off += n;
    }
    put_be32(zlib, adler32(raw.data(), raw.size()));
    chunk(out, "IDAT", zlib);

    chunk(out, "IEND", {});

    FILE* f = fopen(path, "wb");
    if (!f) return false;
    fwrite(out.data(), 1, out.size(), f);
    fclose(f);
    return true;
}

}  // namespace png
