#include "draw_text_nv12.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>


static TextBoxSize measure_text_box_internal(const std::u32string &text,
                                             const FontView &font,
                                             const TextDrawOptions &options) {
  TextBoxSize size{};
  if (text.empty() || font.glyphWidth <= 0 || font.glyphHeight <= 0 ||
      options.scale <= 0.0f) {
    return size;
  }

  const int scaledW =
      std::max(1, static_cast<int>(round_to_int(
                      static_cast<float>(font.glyphWidth) * options.scale)));
  const int scaledH =
      std::max(1, static_cast<int>(round_to_int(
                      static_cast<float>(font.glyphHeight) * options.scale)));
  const int count = static_cast<int>(text.size());

  if (options.layout == TextDrawOptions::Layout::Vertical) {
    size.width = scaledW;
    size.height =
        count * scaledH + std::max(0, count - 1) * options.letterSpacing;
  } else {
    size.width =
        count * scaledW + std::max(0, count - 1) * options.letterSpacing;
    size.height = scaledH;
  }

  if (size.width < 0) {
    size.width = 0;
  }
  if (size.height < 0) {
    size.height = 0;
  }
  return size;
}

static void anchor_offset(const TextBoxSize &box,
                          TextDrawOptions::Anchor anchor, int &ax, int &ay) {
  switch (anchor) {
  case TextDrawOptions::Anchor::TopLeft:
    ax = 0;
    ay = 0;
    break;
  case TextDrawOptions::Anchor::TopCenter:
    ax = -box.width / 2;
    ay = 0;
    break;
  case TextDrawOptions::Anchor::TopRight:
    ax = -box.width;
    ay = 0;
    break;
  case TextDrawOptions::Anchor::MiddleLeft:
    ax = 0;
    ay = -box.height / 2;
    break;
  case TextDrawOptions::Anchor::MiddleCenter:
    ax = -box.width / 2;
    ay = -box.height / 2;
    break;
  case TextDrawOptions::Anchor::MiddleRight:
    ax = -box.width;
    ay = -box.height / 2;
    break;
  case TextDrawOptions::Anchor::BottomLeft:
    ax = 0;
    ay = -box.height;
    break;
  case TextDrawOptions::Anchor::BottomCenter:
    ax = -box.width / 2;
    ay = -box.height;
    break;
  case TextDrawOptions::Anchor::BottomRight:
    ax = -box.width;
    ay = -box.height;
    break;
  }
}

static inline uint8_t clamp_u8(int v) {
  if (v < 0) {
    return 0;
  }
  if (v > 255) {
    return 255;
  }
  return static_cast<uint8_t>(v);
}

static inline uint8_t blend_u8_alpha(uint8_t dst, uint8_t src, uint8_t alpha) {
  const int a = static_cast<int>(alpha);
  const int out = static_cast<int>(dst) * (255 - a) + static_cast<int>(src) * a;
  return static_cast<uint8_t>((out + 127) / 255);
}

static inline void rgb_to_yuv_bt601(uint8_t r, uint8_t g, uint8_t b, uint8_t &y,
                                    uint8_t &u, uint8_t &v) {
  const float rf = static_cast<float>(r);
  const float gf = static_cast<float>(g);
  const float bf = static_cast<float>(b);

  const int yi =
      static_cast<int>(round_to_int(0.299f * rf + 0.587f * gf + 0.114f * bf));
  const int ui = static_cast<int>(
      round_to_int(-0.169f * rf - 0.331f * gf + 0.5f * bf + 128.0f));
  const int vi = static_cast<int>(
      round_to_int(0.5f * rf - 0.419f * gf - 0.081f * bf + 128.0f));

  y = clamp_u8(yi);
  u = clamp_u8(ui);
  v = clamp_u8(vi);
}

static inline uint8_t read_glyph_value(const uint8_t *data, int pixelIndex,
                                       int bpp) {
  if (bpp == 8) {
    return data[pixelIndex];
  }

  const int bitPos = pixelIndex * bpp;
  const int byteIndex = bitPos / 8;
  const int shift = 8 - bpp - (bitPos % 8);
  const uint8_t mask = static_cast<uint8_t>((1 << bpp) - 1);
  return static_cast<uint8_t>((data[byteIndex] >> shift) & mask);
}

static inline uint8_t glyph_alpha_u8(const uint8_t *data, int pixelIndex,
                                     int bpp) {
  if (bpp == 8) {
    return data[pixelIndex];
  }

  const int value = static_cast<int>(read_glyph_value(data, pixelIndex, bpp));
  const int maxValue = (1 << bpp) - 1;
  return static_cast<uint8_t>((value * 255 + maxValue / 2) / maxValue);
}

static inline uint8_t glyph_alpha_at_u8(const uint8_t *data, int x, int y,
                                        int glyphW, int glyphH, int bpp) {
  if (x < 0) {
    x = 0;
  } else if (x >= glyphW) {
    x = glyphW - 1;
  }

  if (y < 0) {
    y = 0;
  } else if (y >= glyphH) {
    y = glyphH - 1;
  }

  const int pixelIndex = y * glyphW + x;
  return glyph_alpha_u8(data, pixelIndex, bpp);
}

struct AxisMap {
  int i0 = 0;
  int i1 = 0;
  uint16_t w1 = 0;
};

static inline uint8_t glyph_alpha_bilinear_u8(const uint8_t *data,
                                              const AxisMap &xMap,
                                              const AxisMap &yMap, int glyphW,
                                              int glyphH, int bpp) {
  const int wx1 = static_cast<int>(xMap.w1);
  const int wx0 = 256 - wx1;
  const int wy1 = static_cast<int>(yMap.w1);
  const int wy0 = 256 - wy1;

  const int a00 = static_cast<int>(
      glyph_alpha_at_u8(data, xMap.i0, yMap.i0, glyphW, glyphH, bpp));
  const int a10 = static_cast<int>(
      glyph_alpha_at_u8(data, xMap.i1, yMap.i0, glyphW, glyphH, bpp));
  const int a01 = static_cast<int>(
      glyph_alpha_at_u8(data, xMap.i0, yMap.i1, glyphW, glyphH, bpp));
  const int a11 = static_cast<int>(
      glyph_alpha_at_u8(data, xMap.i1, yMap.i1, glyphW, glyphH, bpp));

  const int top = (a00 * wx0 + a10 * wx1 + 128) >> 8;
  const int bottom = (a01 * wx0 + a11 * wx1 + 128) >> 8;
  return static_cast<uint8_t>((top * wy0 + bottom * wy1 + 128) >> 8);
}

static const std::unordered_map<uint32_t, const uint8_t *> &
cached_glyph_map(const FontView &font) {
  struct GlyphMapCache {
    const FontGlyphView *glyphs = nullptr;
    int glyphCount = 0;
    std::unordered_map<uint32_t, const uint8_t *> map;
  };

  static thread_local GlyphMapCache cache;
  if (cache.glyphs != font.glyphs || cache.glyphCount != font.glyphCount) {
    cache.glyphs = font.glyphs;
    cache.glyphCount = font.glyphCount;
    cache.map.clear();
    cache.map.reserve(static_cast<size_t>(font.glyphCount));
    for (int i = 0; i < font.glyphCount; ++i) {
      cache.map[font.glyphs[i].codepoint] = font.glyphs[i].bitmap;
    }
  }
  return cache.map;
}

Nv12TextRenderer::Nv12TextRenderer(uint8_t *nv12, int imageWidth,
                                   int imageHeight, int yStride, int uvStride)
    : nv12_(nv12), imageWidth_(imageWidth), imageHeight_(imageHeight),
      yStride_(yStride), uvStride_(uvStride) {}

TextBoxSize Nv12TextRenderer::MeasureTextBox(const std::u32string &text,
                                             const FontView &font,
                                             const TextDrawOptions &options) {
  return measure_text_box_internal(text, font, options);
}

void Nv12TextRenderer::DrawText(const std::u32string &text,
                                const FontView &font,
                                const TextDrawOptions &options) const {
  if (nv12_ == nullptr || imageWidth_ <= 0 || imageHeight_ <= 0) {
    return;
  }
  if (font.glyphs == nullptr || font.glyphCount <= 0 || font.glyphWidth <= 0 ||
      font.glyphHeight <= 0) {
    return;
  }
  if (!(font.bpp == 1 || font.bpp == 2 || font.bpp == 4 || font.bpp == 8)) {
    return;
  }
  if (options.scale <= 0.0f) {
    return;
  }

  uint8_t *yPlane = nv12_;
  uint8_t *uvPlane =
      nv12_ + static_cast<size_t>(yStride_) * static_cast<size_t>(imageHeight_);

  uint8_t yText = 0;
  uint8_t uText = 0;
  uint8_t vText = 0;
  rgb_to_yuv_bt601(options.color.r, options.color.g, options.color.b, yText,
                   uText, vText);

  const auto &glyphMap = cached_glyph_map(font);
  const int scaledW =
      std::max(1, static_cast<int>(round_to_int(
                      static_cast<float>(font.glyphWidth) * options.scale)));
  const int scaledH =
      std::max(1, static_cast<int>(round_to_int(
                      static_cast<float>(font.glyphHeight) * options.scale)));
  const bool noScale =
      (scaledW == font.glyphWidth) && (scaledH == font.glyphHeight);
  std::vector<AxisMap> xMap;
  std::vector<AxisMap> yMap;
  if (!noScale) {
    xMap.resize(static_cast<size_t>(scaledW));
    yMap.resize(static_cast<size_t>(scaledH));

    for (int dx = 0; dx < scaledW; ++dx) {
      const float srcX =
          ((static_cast<float>(dx) + 0.5f) *
           static_cast<float>(font.glyphWidth) / static_cast<float>(scaledW)) -
          0.5f;
      const int x0 = static_cast<int>(std::floor(srcX));
      const float tx = srcX - static_cast<float>(x0);
      AxisMap m;
      m.i0 = x0;
      m.i1 = x0 + 1;
      m.w1 = static_cast<uint16_t>(round_to_int(tx * 256.0f));
      xMap[static_cast<size_t>(dx)] = m;
    }

    for (int dy = 0; dy < scaledH; ++dy) {
      const float srcY =
          ((static_cast<float>(dy) + 0.5f) *
           static_cast<float>(font.glyphHeight) / static_cast<float>(scaledH)) -
          0.5f;
      const int y0 = static_cast<int>(std::floor(srcY));
      const float ty = srcY - static_cast<float>(y0);
      AxisMap m;
      m.i0 = y0;
      m.i1 = y0 + 1;
      m.w1 = static_cast<uint16_t>(round_to_int(ty * 256.0f));
      yMap[static_cast<size_t>(dy)] = m;
    }
  }

  const TextBoxSize box = measure_text_box_internal(text, font, options);
  int anchorX = 0;
  int anchorY = 0;
  anchor_offset(box, options.anchor, anchorX, anchorY);

  int penX = options.x + options.offsetX + anchorX;
  int penY = options.y + options.offsetY + anchorY;

  for (int i = 0; i < static_cast<int>(text.size()); ++i) {
    const int textIndex =
        options.reverse ? (static_cast<int>(text.size()) - 1 - i) : i;
    const uint32_t ch = text[static_cast<size_t>(textIndex)];
    auto it = glyphMap.find(ch);
    const uint8_t *glyphData = (it == glyphMap.end()) ? nullptr : it->second;

    if (glyphData != nullptr) {
      const int drawX0 = std::max(0, penX);
      const int drawY0 = std::max(0, penY);
      const int drawX1 = std::min(imageWidth_, penX + scaledW);
      const int drawY1 = std::min(imageHeight_, penY + scaledH);

      if (drawX0 < drawX1 && drawY0 < drawY1) {
        for (int dstY = drawY0; dstY < drawY1; ++dstY) {
          const int dy = dstY - penY;
          uint8_t *yRow = yPlane + dstY * yStride_;
          uint8_t *uvRow = uvPlane + (dstY / 2) * uvStride_;

          for (int dstX = drawX0; dstX < drawX1; ++dstX) {
            const int dx = dstX - penX;
            uint8_t alpha = 0;
            if (noScale) {
              const int pixelIndex = dy * font.glyphWidth + dx;
              alpha = glyph_alpha_u8(glyphData, pixelIndex, font.bpp);
            } else {
              alpha = glyph_alpha_bilinear_u8(
                  glyphData, xMap[static_cast<size_t>(dx)],
                  yMap[static_cast<size_t>(dy)], font.glyphWidth,
                  font.glyphHeight, font.bpp);
            }
            if (alpha == 0) {
              continue;
            }

            uint8_t &yDst = yRow[dstX];
            yDst = blend_u8_alpha(yDst, yText, alpha);

            const int uvX = (dstX / 2) * 2;
            if (uvX + 1 < uvStride_ && (dstY / 2) < (imageHeight_ / 2)) {
              uint8_t &uDst = uvRow[uvX];
              uint8_t &vDst = uvRow[uvX + 1];
              uDst = blend_u8_alpha(uDst, uText, alpha);
              vDst = blend_u8_alpha(vDst, vText, alpha);
            }
          }
        }
      }
    }

    const int spacing = options.letterSpacing;
    if (options.layout == TextDrawOptions::Layout::Vertical) {
      penY += scaledH + spacing;
    } else {
      penX += scaledW + spacing;
    }
  }
}

TextBoxSize MeasureTextBoxOnNV12(const std::u32string &text,
                                 const FontView &font,
                                 const TextDrawOptions &options) {
  return Nv12TextRenderer::MeasureTextBox(text, font, options);
}

void DrawTextOnNV12(uint8_t *nv12, int imageWidth, int imageHeight,
                    const std::u32string &text, const FontView &font, int x,
                    int y, float scale, uint8_t r, uint8_t g, uint8_t b) {
  TextDrawOptions options{};
  options.x = x;
  options.y = y;
  options.scale = scale;
  options.letterSpacing = 0;
  options.layout = TextDrawOptions::Layout::Horizontal;
  options.color = {r, g, b};

  int yStride = imageWidth;  // Assuming tightly packed Y plane
  int uvStride = imageWidth; // Assuming tightly packed UV plane

  Nv12TextRenderer renderer(nv12, imageWidth, imageHeight, yStride, uvStride);

  renderer.DrawText(text, font, options);
}

void DrawTextOnNV12(uint8_t *nv12, int imageWidth, int imageHeight, int yStride,
                    int uvStride, const std::u32string &text,
                    const FontView &font, const TextDrawOptions &options) {

  Nv12TextRenderer renderer(nv12, imageWidth, imageHeight, yStride, uvStride);
  renderer.DrawText(text, font, options);
}