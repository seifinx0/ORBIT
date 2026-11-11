#include "orbit/graphics/font_renderer.hpp"
#include "orbit/graphics/texture.hpp"
#include "orbit/input/controller.hpp"
#include "orbit/modules/dashboard.hpp"

#include <orbis/Sysmodule.h>

#include <array>
#include <cassert>
#include <cstdint>

extern "C" int32_t sceSysmoduleLoadModule(OrbisSysModule) { return 0; }

int main() {
    using namespace orbit;
    constexpr int width = 1920, height = 1080;
    std::array<std::uint32_t, width * height> pixels{};
    graphics::Canvas canvas(pixels.data(), width, height);
    graphics::FontRenderer font;
    assert(font.initialize("assets/fonts/Quicksand-Medium.ttf"));
    assert(font.prepare_size(92) && font.prepare_size(30) &&
           font.prepare_size(23) && font.prepare_size(16));
    graphics::TextureCache textures;
    const auto* mark = textures.get_png("assets/images/orbit-mark.png");
    assert(mark);

    modules::Dashboard dashboard;
    assert(dashboard.initialize(mark));
    dashboard.render(canvas, font);
    assert(pixels[0] != 0);

    assert(dashboard.handle(input::TabNext));
    assert(dashboard.tick(100));
    dashboard.render(canvas, font);
    assert(dashboard.tick(100));
    dashboard.render(canvas, font);

    assert(dashboard.handle(input::Back));
    assert(dashboard.tick(100));
    assert(dashboard.tick(100));
    dashboard.render(canvas, font);

    assert(dashboard.handle(input::Context));
    dashboard.render(canvas, font);
    assert(dashboard.handle(input::Back));
    dashboard.render(canvas, font);
    font.shutdown();
}
