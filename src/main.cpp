#include "orbit/graphics/canvas.hpp"
#include "orbit/graphics/font_renderer.hpp"
#include "orbit/graphics/texture.hpp"
#include "orbit/input/controller.hpp"
#include "orbit/modules/dashboard.hpp"
#include "orbit/platform/video_output.hpp"

#include <cstdio>
#include <cstdint>

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::puts("ORBIT: initializing SceVideoOut foundation");

    orbit::platform::VideoOutput video;
    if (!video.initialize()) {
        std::puts("ORBIT: video output initialization failed");
        for (;;) {}
    }

    orbit::graphics::FontRenderer font;
    const bool typography_ready = font.initialize("/app0/assets/fonts/Quicksand-Medium.ttf") &&
                                  font.prepare_size(92) && font.prepare_size(30) &&
                                  font.prepare_size(23) && font.prepare_size(16);
    if (!typography_ready) std::puts("ORBIT: FreeType initialization or glyph cache failed");

    orbit::graphics::TextureCache textures;
    const auto* mark = textures.get_png("/app0/assets/images/orbit-mark.png");
    if (!mark) std::puts("ORBIT: ORBIT mark texture unavailable; using vector fallback");

    orbit::modules::Dashboard dashboard;
    if (!dashboard.initialize(mark)) {
        std::puts("ORBIT: dashboard initialization failed");
        for (;;) {}
    }

    for (int index = 0; index < orbit::platform::VideoOutput::kBufferCount; ++index) {
        orbit::graphics::Canvas canvas(video.buffer(index), orbit::platform::VideoOutput::kWidth,
                                      orbit::platform::VideoOutput::kHeight);
        dashboard.render(canvas, font);
    }

    orbit::input::ControllerDevice controller;
    const bool controller_ready = controller.initialize();
    if (!controller_ready) std::puts("ORBIT: controller unavailable; dashboard remains on screen");
    orbit::input::NavigationInput navigation;

    std::puts("ORBIT: controller-driven dashboard ready; entering v-sync presentation loop");
    std::int64_t frame = 0;
    int index = 0;
    for (;;) {
        orbit::input::PadSample sample{};
        if (!controller.read(sample)) sample = {};
        const auto events = navigation.update(sample, 16);
        bool redraw = dashboard.tick(16);
        if (events) redraw = dashboard.handle(events) || redraw;
        if (redraw) {
            for (int buffer = 0; buffer < orbit::platform::VideoOutput::kBufferCount; ++buffer) {
                orbit::graphics::Canvas canvas(video.buffer(buffer), orbit::platform::VideoOutput::kWidth,
                                               orbit::platform::VideoOutput::kHeight);
                dashboard.render(canvas, font);
            }
        }
        if (!video.present(index, frame++)) {
            std::puts("ORBIT: frame submission/wait failed");
            for (;;) {}
        }
        index = (index + 1) % orbit::platform::VideoOutput::kBufferCount;
    }
}
