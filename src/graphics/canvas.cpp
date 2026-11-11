#include "orbit/graphics/canvas.hpp"

#include <cstdlib>

namespace orbit::graphics {
Canvas::Canvas(std::uint32_t* pixels, int width, int height) noexcept
    : pixels_(pixels), width_(width), height_(height) {}

void Canvas::pixel(int x, int y, Color color) noexcept {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    pixels_[static_cast<std::size_t>(y) * width_ + x] =
        0x80000000u | (static_cast<std::uint32_t>(color.r) << 16) |
        (static_cast<std::uint32_t>(color.g) << 8) | color.b;
}

void Canvas::clear(Color color) noexcept { fill_rect(0, 0, width_, height_, color); }

void Canvas::fill_rect(int x, int y, int width, int height, Color color) noexcept {
    fill_rect_alpha(x, y, width, height, color, 255);
}

void Canvas::fill_rect_alpha(int x, int y, int width, int height, Color color,
                             std::uint8_t opacity) noexcept {
    if (!pixels_ || width <= 0 || height <= 0 || opacity == 0) return;
    const int x0 = x < 0 ? 0 : x;
    const int y0 = y < 0 ? 0 : y;
    const int x1 = x + width > width_ ? width_ : x + width;
    const int y1 = y + height > height_ ? height_ : y + height;
    if (x0 >= x1 || y0 >= y1) return;
    if (opacity == 255) {
        const std::uint32_t value = 0x80000000u | (static_cast<std::uint32_t>(color.r) << 16) |
                                    (static_cast<std::uint32_t>(color.g) << 8) | color.b;
        for (int row = y0; row < y1; ++row) {
            auto* target = pixels_ + static_cast<std::size_t>(row) * width_ + x0;
            for (int column = x0; column < x1; ++column) *target++ = value;
        }
        return;
    }
    for (int row = y0; row < y1; ++row)
        for (int col = x0; col < x1; ++col)
            blend_pixel(col, row, color, opacity);
}

void Canvas::blend_pixel(int x, int y, Color color, std::uint8_t opacity) noexcept {
    if (!pixels_ || x < 0 || y < 0 || x >= width_ || y >= height_ || opacity == 0) return;
    auto& destination = pixels_[static_cast<std::size_t>(y) * width_ + x];
    if (opacity == 255) {
        destination = 0x80000000u | (static_cast<std::uint32_t>(color.r) << 16) |
                      (static_cast<std::uint32_t>(color.g) << 8) | color.b;
        return;
    }

    const unsigned inverse = 255u - opacity;
    const unsigned red = ((destination >> 16) & 0xffu) * inverse + color.r * opacity;
    const unsigned green = ((destination >> 8) & 0xffu) * inverse + color.g * opacity;
    const unsigned blue = (destination & 0xffu) * inverse + color.b * opacity;
    destination = 0x80000000u | (((red + 127u) / 255u) << 16) |
                  (((green + 127u) / 255u) << 8) | ((blue + 127u) / 255u);
}

void Canvas::line(int x0, int y0, int x1, int y1, Color color) noexcept {
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int twice = 2 * error;
        if (twice >= dy) { error += dy; x0 += sx; }
        if (twice <= dx) { error += dx; y0 += sy; }
    }
}

void Canvas::circle_outline(int cx, int cy, int radius, Color color) noexcept {
    int x = radius, y = 0, error = 1 - radius;
    while (x >= y) {
        pixel(cx + x, cy + y, color); pixel(cx + y, cy + x, color);
        pixel(cx - y, cy + x, color); pixel(cx - x, cy + y, color);
        pixel(cx - x, cy - y, color); pixel(cx - y, cy - x, color);
        pixel(cx + y, cy - x, color); pixel(cx + x, cy - y, color);
        ++y;
        if (error < 0) error += 2 * y + 1;
        else { --x; error += 2 * (y - x + 1); }
    }
}
} // namespace orbit::graphics
