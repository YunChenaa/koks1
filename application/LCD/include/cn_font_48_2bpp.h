#pragma once

#include <cstdint>

struct cn_font_48_2bppGlyph {
    uint32_t codepoint;
    const uint8_t* bitmap;
};

extern const int cn_font_48_2bpp_font_size;
extern const int cn_font_48_2bpp_font_width;
extern const int cn_font_48_2bpp_font_height;
extern const int cn_font_48_2bpp_font_bpp;
extern const int cn_font_48_2bpp_font_rotation_degrees;
extern const int cn_font_48_2bpp_bytes_per_glyph;
extern const int cn_font_48_2bpp_glyph_count;
extern const cn_font_48_2bppGlyph cn_font_48_2bpp_glyphs[];
