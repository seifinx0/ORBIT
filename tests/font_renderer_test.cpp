#include "orbit/graphics/canvas.hpp"
#include "orbit/graphics/font_renderer.hpp"

#include <orbis/Sysmodule.h>

#include <array>
#include <cassert>
#include <cstdint>

extern "C" int32_t sceSysmoduleLoadModule(OrbisSysModule) { return 0; }

int main() {
    using namespace orbit::graphics;
    constexpr int width = 640;
    constexpr int height = 96;
    constexpr std::uint32_t background = 0x80070d1cu;
    std::array<std::uint32_t, width * height> pixels{};
    Canvas canvas(pixels.data(), width, height);
    canvas.clear({7, 13, 28});

    FontRenderer font;
    assert(font.initialize("assets/fonts/Quicksand-Medium.ttf"));
    assert(font.prepare_size(32));
    assert(font.prepare_size(50));
    assert(font.line_height(32) >= 32);
    const int label_width = font.measure_width("ORBIT 2.0", 32);
    assert(label_width > 100 && label_width < width);

    font.draw_text(canvas, 12, 8, "ORBIT 2.0", 32, {48, 207, 255});
    std::size_t changed_pixels = 0;
    std::size_t antialiased_pixels = 0;
    for (const auto pixel : pixels) {
        if (pixel != background) ++changed_pixels;
        if (pixel != background && pixel != 0x8030cfffu) ++antialiased_pixels;
    }
    assert(changed_pixels > 500);
    assert(antialiased_pixels > 10);

    font.draw_text(canvas, -12, 50, "CLIPPED", 32, {231, 241, 252});
    font.shutdown();
}
