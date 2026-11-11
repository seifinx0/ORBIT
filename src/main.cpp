#include "orbit/graphics/canvas.hpp"
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

void draw_foundation(Canvas& canvas) noexcept {
    canvas.clear(kNavy);
    // Subtle framing and the ORBIT mark make this a platform bring-up screen,
    // not a nonfunctional dashboard mockup.
    canvas.fill_rect(0, 0, 1920, 5, kCyan);
    canvas.fill_rect(150, 137, 1620, 1, {24, 48, 75});
    canvas.fill_rect(150, 943, 1620, 1, {24, 48, 75});

    canvas.circle_outline(960, 441, 88, kBlue);
    canvas.circle_outline(960, 441, 71, {21, 71, 111});
    canvas.fill_rect(954, 435, 13, 13, kCyan);
    canvas.line(897, 504, 1023, 378, kCyan);
    canvas.text(844, 575, "ORBIT", 8, kWhite);
    canvas.text(824, 665, "SYSTEM FOUNDATION", 3, kCyan);

    canvas.fill_rect(640, 748, 640, 98, kPanel);
    canvas.fill_rect(640, 748, 4, 98, kCyan);
    canvas.fill_rect(670, 779, 12, 12, {72, 224, 174});
    canvas.text(700, 772, "VIDEO OUTPUT READY", 2, kWhite);
    canvas.text(700, 806, "1920 X 1080  /  DOUBLE BUFFERED", 1, kMuted);
    canvas.text(805, 890, "OPENORBIS NATIVE APPLICATION", 1, kMuted);
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

    for (int index = 0; index < orbit::platform::VideoOutput::kBufferCount; ++index) {
        Canvas canvas(video.buffer(index), orbit::platform::VideoOutput::kWidth,
                      orbit::platform::VideoOutput::kHeight);
        draw_foundation(canvas);
    }

    std::puts("ORBIT: framebuffer initialized; entering v-sync presentation loop");
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
