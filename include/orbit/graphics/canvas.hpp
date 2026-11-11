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
    void blend_pixel(int x, int y, Color color, std::uint8_t opacity) noexcept;
    void line(int x0, int y0, int x1, int y1, Color color) noexcept;
    void circle_outline(int cx, int cy, int radius, Color color) noexcept;
private:
    void pixel(int x, int y, Color color) noexcept;
    std::uint32_t* pixels_;
    int width_;
    int height_;
};

} // namespace orbit::graphics
