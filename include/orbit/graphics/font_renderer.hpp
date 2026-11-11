#pragma once

#include "orbit/graphics/canvas.hpp"

#include <memory>

namespace orbit::graphics {

// FreeType-backed ASCII text renderer. Glyph bitmaps are cached by pixel size;
// prepare_size() should be called before drawing to keep allocation out of the
// frame path.
class FontRenderer final {
public:
    FontRenderer() noexcept;
    ~FontRenderer();
    FontRenderer(const FontRenderer&) = delete;
    FontRenderer& operator=(const FontRenderer&) = delete;

    bool initialize(const char* font_path) noexcept;
    bool prepare_size(int pixel_size) noexcept;
    int measure_width(const char* text, int pixel_size) noexcept;
    int line_height(int pixel_size) const noexcept;
    void draw_text(Canvas& canvas, int x, int y, const char* text,
                   int pixel_size, Color color) noexcept;
    void shutdown() noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace orbit::graphics
