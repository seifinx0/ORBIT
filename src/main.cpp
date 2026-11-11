#include "orbit/graphics/canvas.hpp"
#include "orbit/graphics/font_renderer.hpp"
#include "orbit/platform/video_output.hpp"

#include <cstdio>
#include <cstdint>

namespace {
using orbit::graphics::Canvas;
using orbit::graphics::Color;
constexpr Color kNavy{7, 13, 28};
constexpr Color kPanel{12, 23, 43};
constexpr Color kCyan{48, 207, 255};
constexpr Color kBlue{42, 103, 240};
constexpr Color kWhite{231, 241, 252};
constexpr Color kMuted{113, 143, 176};
constexpr Color kGreen{72, 224, 174};
constexpr Color kAmber{255, 175, 92};

void draw_centered_text(Canvas& canvas, orbit::graphics::FontRenderer& font,
                        int y, const char* value, int size, Color color) noexcept {
    const int width = font.measure_width(value, size);
    font.draw_text(canvas, (orbit::platform::VideoOutput::kWidth - width) / 2,
                   y, value, size, color);
}

void draw_foundation(Canvas& canvas, orbit::graphics::FontRenderer& font,
                     bool typography_ready) noexcept {
    canvas.clear(kNavy);
    canvas.fill_rect(0, 0, 1920, 5, kCyan);
    canvas.fill_rect_alpha(150, 137, 1620, 1, {35, 73, 105}, 130);
    canvas.fill_rect_alpha(150, 943, 1620, 1, {35, 73, 105}, 130);

    canvas.circle_outline(960, 441, 88, kBlue);
    canvas.circle_outline(960, 441, 71, {21, 71, 111});
    canvas.circle_outline(960, 441, 102, {14, 44, 72});
    canvas.fill_rect(954, 435, 13, 13, kCyan);
    canvas.line(897, 504, 1023, 378, kCyan);

    canvas.fill_rect_alpha(590, 740, 740, 126, kPanel, 238);
    canvas.fill_rect(590, 740, 4, 126, kCyan);
    canvas.fill_rect(626, 778, 12, 12, typography_ready ? kGreen : kAmber);

    if (!typography_ready) return;
    draw_centered_text(canvas, font, 548, "ORBIT", 92, kWhite);
    draw_centered_text(canvas, font, 666, "SYSTEM COMMAND CENTER", 30, kCyan);
    font.draw_text(canvas, 660, 768, "GRAPHICS FOUNDATION", 23, kWhite);
    font.draw_text(canvas, 660, 805, "FREETYPE GLYPH CACHE  /  ALPHA BLENDING", 16, kMuted);
    draw_centered_text(canvas, font, 891, "SCEVIDEOOUT  /  1920 X 1080  /  VSYNC", 16, kMuted);
}
}

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

    for (int index = 0; index < orbit::platform::VideoOutput::kBufferCount; ++index) {
        Canvas canvas(video.buffer(index), orbit::platform::VideoOutput::kWidth,
                      orbit::platform::VideoOutput::kHeight);
        draw_foundation(canvas, font, typography_ready);
    }

    std::puts("ORBIT: FreeType text and composited canvas ready; entering v-sync presentation loop");
    std::int64_t frame = 0;
    int index = 0;
    for (;;) {
        if (!video.present(index, frame++)) {
            std::puts("ORBIT: frame submission/wait failed");
            for (;;) {}
        }
        index = (index + 1) % orbit::platform::VideoOutput::kBufferCount;
    }
}
