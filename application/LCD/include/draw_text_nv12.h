#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

static inline int round_to_int(float v) {
  return (v >= 0.0f) ? static_cast<int>(std::floor(v + 0.5f))
                     : static_cast<int>(std::ceil(v - 0.5f));
}

struct FontGlyphView {
    uint32_t codepoint;
    const uint8_t* bitmap;
};

struct FontView {
    const FontGlyphView* glyphs;
    int glyphCount;
    int glyphWidth;
    int glyphHeight;
    int bpp;
};

struct RgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct TextDrawOptions {
    enum class Layout {
        Horizontal,
        Vertical
    };

    enum class Anchor {
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    int x = 0;
    int y = 0;
    int offsetX = 0;
    int offsetY = 0;
    float scale = 1.0f;
    int letterSpacing = 0;
    Layout layout = Layout::Horizontal;
    Anchor anchor = Anchor::TopLeft;
    bool reverse = false;
    RgbColor color{255, 255, 255};
};

struct TextBoxSize {
    int width = 0;
    int height = 0;
};

class Nv12TextRenderer {
public:
    Nv12TextRenderer(
        uint8_t* nv12,
        int imageWidth,
        int imageHeight,
        int yStride,
        int uvStride
    );

    static TextBoxSize MeasureTextBox(
        const std::u32string& text,
        const FontView& font,
        const TextDrawOptions& options
    );

    void DrawText(const std::u32string& text, const FontView& font, const TextDrawOptions& options) const;

private:
    uint8_t* nv12_;
    int imageWidth_;
    int imageHeight_;
    int yStride_;
    int uvStride_;
};

TextBoxSize MeasureTextBoxOnNV12(
    const std::u32string& text,
    const FontView& font,
    const TextDrawOptions& options
);

void DrawTextOnNV12(
    uint8_t* nv12,
    int imageWidth,
    int imageHeight,
    const std::u32string& text,
    const FontView& font,
    int x,
    int y,
    float scale,
    uint8_t r,
    uint8_t g,
    uint8_t b
);

void DrawTextOnNV12(
    uint8_t* nv12,
    int imageWidth,
    int imageHeight,
    int yStride,
    int uvStride,
    const std::u32string& text,
    const FontView& font,
    const TextDrawOptions& options
);

template <typename TGlyph>
FontView MakeFontView(
    const TGlyph* glyphs,
    int glyphCount,
    int glyphWidth,
    int glyphHeight,
    int bpp
) {
    static std::vector<FontGlyphView> converted;
    converted.clear();
    converted.reserve(static_cast<size_t>(glyphCount));
    for (int i = 0; i < glyphCount; ++i) {
        converted.push_back(FontGlyphView{glyphs[i].codepoint, glyphs[i].bitmap});
    }

    FontView view{};
    view.glyphs = converted.data();
    view.glyphCount = static_cast<int>(converted.size());
    view.glyphWidth = glyphWidth;
    view.glyphHeight = glyphHeight;
    view.bpp = bpp;
    return view;
}
