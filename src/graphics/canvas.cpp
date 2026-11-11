#include "orbit/graphics/canvas.hpp"

#include <cstdlib>

namespace orbit::graphics {
namespace {
constexpr std::uint8_t kGlyphs[37][7] = {
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8}, {14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}, {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31},
    {31,16,16,30,16,16,16}, {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17},
    {16,16,16,16,16,16,31}, {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13},
    {30,17,17,30,20,18,17}, {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10},
    {17,17,10,4,10,17,17}, {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {0,0,0,0,0,0,0}
};
int glyph_index(char c) noexcept {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return 36;
}
}

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
    const int x0 = x < 0 ? 0 : x;
    const int y0 = y < 0 ? 0 : y;
    const int x1 = x + width > width_ ? width_ : x + width;
    const int y1 = y + height > height_ ? height_ : y + height;
    const std::uint32_t value = 0x80000000u | (static_cast<std::uint32_t>(color.r) << 16) |
                                (static_cast<std::uint32_t>(color.g) << 8) | color.b;
    for (int row = y0; row < y1; ++row) {
        auto* target = pixels_ + static_cast<std::size_t>(row) * width_ + x0;
        for (int col = x0; col < x1; ++col) *target++ = value;
    }
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

void Canvas::text(int x, int y, const char* value, int scale, Color color) noexcept {
    if (!value || scale < 1) return;
    for (const char* cursor = value; *cursor; ++cursor, x += 6 * scale) {
        if (*cursor == ' ') continue;
        const auto& glyph = kGlyphs[glyph_index(*cursor)];
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (glyph[row] & (1u << (4 - col))) fill_rect(x + col * scale, y + row * scale, scale, scale, color);
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
