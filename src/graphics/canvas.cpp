#include "orbit/graphics/canvas.hpp"

#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <cstdint>

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
    const auto right = static_cast<std::int64_t>(x) + width;
    const auto bottom = static_cast<std::int64_t>(y) + height;
    const int x0 = std::max(0, x), y0 = std::max(0, y);
    const int x1 = static_cast<int>(std::min<std::int64_t>(width_, right));
    const int y1 = static_cast<int>(std::min<std::int64_t>(height_, bottom));
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

void Canvas::fill_vertical_gradient(int x, int y, int width, int height,
                                    Color top, Color bottom) noexcept {
    if (!pixels_ || width <= 0 || height <= 0) return;
    const auto end_x = static_cast<std::int64_t>(x) + width;
    const auto end_y = static_cast<std::int64_t>(y) + height;
    const int x0 = std::max(0, x), x1 = static_cast<int>(std::min<std::int64_t>(width_, end_x));
    const int y0 = std::max(0, y), y1 = static_cast<int>(std::min<std::int64_t>(height_, end_y));
    if (x0 >= x1 || y0 >= y1) return;
    for (int row = y0; row < y1; ++row) {
        const auto relative_y = static_cast<std::uint64_t>(static_cast<std::int64_t>(row) - y);
        const unsigned t = height == 1 ? 0 : static_cast<unsigned>(relative_y * 255u / (height - 1));
        const auto channel = [t](unsigned a, unsigned b) {
            return static_cast<std::uint8_t>((a * (255u - t) + b * t + 127u) / 255u);
        };
        fill_rect(x0, row, x1 - x0,
                  1, {channel(top.r, bottom.r), channel(top.g, bottom.g), channel(top.b, bottom.b)});
    }
}

void Canvas::stroke_rect(int x, int y, int width, int height, Color color, int thickness) noexcept {
    if (width <= 0 || height <= 0 || thickness <= 0) return;
    const int t = std::min(thickness, std::min(width, height));
    fill_rect(x, y, width, t, color);
    if (height > t) fill_rect(x, y + height - t, width, t, color);
    if (height > 2 * t) {
        fill_rect(x, y + t, t, height - 2 * t, color);
        if (width > t) fill_rect(x + width - t, y + t, t, height - 2 * t, color);
    }
}

void Canvas::fill_rounded_rect(int x, int y, int width, int height, int radius,
                               Color color, std::uint8_t opacity) noexcept {
    if (width <= 0 || height <= 0 || radius <= 0) {
        fill_rect_alpha(x, y, width, height, color, opacity);
        return;
    }
    radius = std::min(radius, std::min(width, height) / 2);
    if (radius == 0) { fill_rect_alpha(x, y, width, height, color, opacity); return; }
    for (int row = 0; row < height; ++row) {
        int inset = 0;
        if (row < radius) {
            const double dy = radius - row - 0.5;
            inset = radius - static_cast<int>(std::sqrt(std::max(0.0, radius * radius - dy * dy)));
        } else if (row >= height - radius) {
            const double dy = row - (height - radius) + 0.5;
            inset = radius - static_cast<int>(std::sqrt(std::max(0.0, radius * radius - dy * dy)));
        }
        fill_rect_alpha(x + inset, y + row, width - 2 * inset, 1, color, opacity);
    }
}

void Canvas::draw_rgba(const std::uint8_t* rgba, int source_width, int source_height,
                       int source_stride, int x, int y, int width, int height,
                       bool bilinear) noexcept {
    if (!pixels_ || !rgba || source_width <= 0 || source_height <= 0 ||
        static_cast<std::int64_t>(source_stride) < static_cast<std::int64_t>(source_width) * 4 ||
        width <= 0 || height <= 0) return;
    const auto end_x = static_cast<std::int64_t>(x) + width;
    const auto end_y = static_cast<std::int64_t>(y) + height;
    const int x0 = std::max(0, x), y0 = std::max(0, y);
    const int x1 = static_cast<int>(std::min<std::int64_t>(width_, end_x));
    const int y1 = static_cast<int>(std::min<std::int64_t>(height_, end_y));
    if (x0 >= x1 || y0 >= y1) return;
    for (int dy = y0; dy < y1; ++dy) {
        const double sy = (static_cast<double>(dy - y) + 0.5) * source_height / height - 0.5;
        const int iy = static_cast<int>(std::floor(sy));
        const double fy = bilinear ? sy - iy : 0.0;
        const int ylo = std::clamp(iy, 0, source_height - 1);
        const int yhi = std::clamp(iy + 1, 0, source_height - 1);
        for (int dx = x0; dx < x1; ++dx) {
            const double sx = (static_cast<double>(dx - x) + 0.5) * source_width / width - 0.5;
            const int ix = static_cast<int>(std::floor(sx));
            const double fx = bilinear ? sx - ix : 0.0;
            const int xlo = std::clamp(ix, 0, source_width - 1);
            const int xhi = std::clamp(ix + 1, 0, source_width - 1);
            const int xs[2] = {xlo, xhi}, ys[2] = {ylo, yhi};
            const double weights[2][2] = {{(1.0-fx)*(1.0-fy), fx*(1.0-fy)},
                                           {(1.0-fx)*fy, fx*fy}};
            double alpha = 0, red = 0, green = 0, blue = 0;
            for (int row = 0; row < 2; ++row) for (int col = 0; col < 2; ++col) {
                const auto* p = rgba + static_cast<std::size_t>(ys[row]) * source_stride + xs[col] * 4;
                const double a = p[3] / 255.0, w = weights[row][col];
                alpha += a * w; red += p[0] * a * w; green += p[1] * a * w; blue += p[2] * a * w;
            }
            if (alpha <= 0.0) continue;
            const auto to_byte = [](double value) {
                return static_cast<std::uint8_t>(std::clamp(static_cast<int>(value + 0.5), 0, 255));
            };
            blend_pixel(dx, dy, {to_byte(red / alpha), to_byte(green / alpha), to_byte(blue / alpha)},
                        to_byte(alpha * 255.0));
        }
    }
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
