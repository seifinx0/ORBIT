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
}
