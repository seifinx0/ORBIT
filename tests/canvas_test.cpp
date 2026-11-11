#include "orbit/graphics/canvas.hpp"

#include <array>
#include <cassert>
#include <cstdint>

using orbit::graphics::Canvas;
int main() {
    constexpr std::uint32_t opaque = 0x80000000u;
    std::array<std::uint32_t, 4 * 3> pixels{};
    Canvas canvas(pixels.data(), 4, 3);

    canvas.clear({255, 0, 0});
    assert(pixels[0] == (opaque | 0x00ff0000u));

    canvas.blend_pixel(1, 1, {0, 0, 255}, 128);
    assert(pixels[5] == (opaque | 0x00000080u | 0x007f0000u));

    const auto unchanged = pixels[5];
    canvas.blend_pixel(1, 1, {0, 255, 0}, 0);
    assert(pixels[5] == unchanged);

    canvas.fill_rect_alpha(-1, 0, 2, 1, {0, 255, 0}, 255);
    assert(pixels[0] == (opaque | 0x0000ff00u));
    assert(pixels[1] == (opaque | 0x00ff0000u));

    canvas.line(0, 2, 3, 2, {255, 255, 255});
    for (int x = 0; x < 4; ++x)
        assert(pixels[8 + x] == (opaque | 0x00ffffffu));

    canvas.fill_rect_alpha(0, 0, 4, 3, {0, 0, 0}, 0);
    assert(pixels[0] == (opaque | 0x0000ff00u));

    std::array<std::uint32_t, 4 * 2> image_target{};
    Canvas image_canvas(image_target.data(), 4, 2);
    image_canvas.clear({0, 0, 0});
    const std::uint8_t rgba[] = {255, 0, 0, 255, 0, 255, 0, 0};
    image_canvas.draw_rgba(rgba, 2, 1, 8, 0, 0, 4, 1);
    assert(((image_target[0] >> 16) & 0xffu) > 0 && (image_target[0] & 0xff00u) == 0);
    assert(((image_target[1] >> 16) & 0xffu) > 0 && (image_target[1] & 0xff00u) == 0);
    assert(((image_target[2] >> 16) & 0xffu) > 0 && (image_target[2] & 0xff00u) == 0);
    assert(image_target[3] == opaque);
    image_canvas.draw_rgba(rgba, 2, 1, 8, -2, 1, 4, 1, false);
    assert(image_target[4] == (opaque | 0x00ff0000u));
    assert(image_target[5] == opaque);
    assert(image_target[6] == (opaque | 0x00000000u));

    image_canvas.clear({0, 0, 0});
    image_canvas.fill_vertical_gradient(0, 0, 1, 2, {0, 0, 0}, {0, 0, 255});
    assert(image_target[0] == opaque);
    assert(image_target[4] == (opaque | 0x000000ffu));
    image_canvas.fill_rounded_rect(0, 0, 4, 2, 1, {0, 255, 255});
    assert(image_target[1] == (opaque | 0x0000ffffu));
    image_canvas.stroke_rect(0, 0, 4, 2, {255, 255, 255}, 1);
    assert(image_target[0] == (opaque | 0x00ffffffu));
}
