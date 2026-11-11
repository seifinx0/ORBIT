#pragma once

#include <cstdint>

namespace orbit::graphics {

struct Color { std::uint8_t r, g, b; };

class Canvas final {
public:
    Canvas(std::uint32_t* pixels, int width, int height) noexcept;
    void clear(Color color) noexcept;
    void fill_rect(int x, int y, int width, int height, Color color) noexcept;
    void fill_rect_alpha(int x, int y, int width, int height, Color color,
                         std::uint8_t opacity) noexcept;
    void fill_vertical_gradient(int x, int y, int width, int height, Color top,
                                Color bottom) noexcept;
    void stroke_rect(int x, int y, int width, int height, Color color,
                     int thickness = 1) noexcept;
    void fill_rounded_rect(int x, int y, int width, int height, int radius,
                           Color color, std::uint8_t opacity = 255) noexcept;
    void draw_rgba(const std::uint8_t* rgba, int source_width, int source_height,
                   int source_stride, int x, int y, int width, int height,
                   bool bilinear = true) noexcept;
    bool push_clip(int x, int y, int width, int height) noexcept;
    void pop_clip() noexcept;
    void blend_pixel(int x, int y, Color color, std::uint8_t opacity) noexcept;
    void line(int x0, int y0, int x1, int y1, Color color) noexcept;
    void circle_outline(int cx, int cy, int radius, Color color) noexcept;
private:
    struct Clip { int x, y, width, height; };
    static constexpr int kClipDepth = 8;
    void pixel(int x, int y, Color color) noexcept;
    std::uint32_t* pixels_;
    int width_;
    int height_;
    Clip clip_{};
    Clip clip_stack_[kClipDepth]{};
    int clip_depth_{0};
};

} // namespace orbit::graphics
