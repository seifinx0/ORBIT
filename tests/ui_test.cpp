#include "orbit/graphics/font_renderer.hpp"
#include "orbit/ui/ui.hpp"

#include <orbis/Sysmodule.h>

#include <array>
#include <cassert>
#include <cstdint>

extern "C" int32_t sceSysmoduleLoadModule(OrbisSysModule) { return 0; }

namespace {
int activated = 0;
void on_activate(void*) noexcept { ++activated; }

class TestScreen final : public orbit::ui::Screen {
public:
    TestScreen(const char* id, std::uint32_t color) noexcept : Screen(id), color_(color) {}
    void render(orbit::graphics::Canvas& canvas, orbit::graphics::FontRenderer&) noexcept override {
        canvas.clear({static_cast<std::uint8_t>(color_ >> 16),
                      static_cast<std::uint8_t>(color_ >> 8),
                      static_cast<std::uint8_t>(color_)});
    }
private:
    std::uint32_t color_;
};

class Fill final : public orbit::ui::Widget {
public:
    Fill(orbit::ui::Rect bounds, orbit::graphics::Color color) noexcept : Widget(bounds), color_(color) {}
    void render(orbit::graphics::Canvas& canvas, orbit::graphics::FontRenderer&,
                int origin_x, int origin_y) noexcept override {
        if (!visible_) return;
        canvas.fill_rect(origin_x + bounds_.x, origin_y + bounds_.y,
                         bounds_.width, bounds_.height, color_);
    }
private:
    orbit::graphics::Color color_;
};
}

int main() {
    using namespace orbit;
    constexpr int width = 320, height = 180;
    std::array<std::uint32_t, width * height> pixels{};
    graphics::Canvas canvas(pixels.data(), width, height);
    graphics::FontRenderer font;
    assert(font.initialize("assets/fonts/Quicksand-Medium.ttf"));
    assert(font.prepare_size(18) && font.prepare_size(24));

    ui::Panel panel({8, 8, 180, 90});
    panel.set_background({12, 23, 43}, 255);
    panel.set_border({48, 207, 255}, 2);
    panel.set_radius(12);
    panel.set_padding({8, 8, 8, 8});
    ui::Label label("ORBIT UI", 18, {231, 241, 252}, {0, 0, 150, 24});
    ui::Button first("OPEN", 18, {0, 30, 72, 38}, on_activate);
    ui::Button second("DETAILS", 18, {82, 30, 82, 38});
    assert(panel.add(label) && panel.add(first) && panel.add(second));
    ui::FocusGroup focus;
    assert(focus.add(first) && focus.add(second));
    assert(first.focused() && !second.focused());
    assert(focus.move(ui::Direction::Right));
    assert(second.focused() && !first.focused());
    assert(focus.move(ui::Direction::Left));
    assert(focus.confirm() && activated == 1);
    panel.render(canvas, font);
    assert(pixels[40 * width + 40] == (0x80000000u | 0x000c172bu));
    assert(pixels[120 * width + 220] == 0);

    canvas.clear({7, 13, 28});
    ui::ScrollArea scroll({20, 20, 40, 30});
    Fill upper({0, 0, 40, 22}, {255, 0, 0});
    Fill lower({0, 22, 40, 22}, {0, 255, 0});
    assert(scroll.add(upper) && scroll.add(lower));
    scroll.scroll_by(12);
    scroll.render(canvas, font);
    assert(pixels[25 * width + 25] == (0x80000000u | 0x00ff0000u));
    assert(pixels[45 * width + 25] == (0x80000000u | 0x0000ff00u));
    assert(pixels[55 * width + 25] == (0x80000000u | 0x00070d1cu));

    ui::StackPanel stack(ui::Orientation::Vertical, {0, 0, 100, 100});
    stack.set_spacing(5);
    Fill stack_first({0, 0, 40, 10}, {10, 20, 30});
    Fill stack_second({0, 0, 40, 12}, {30, 20, 10});
    assert(stack.add(stack_first) && stack.add(stack_second));
    assert(stack_first.bounds().y == 0 && stack_second.bounds().y == 15);

    ui::List list({80, 70, 120, 50}, 30);
    ui::Button row_a("A", 18, {0, 0, 1, 1}), row_b("B", 18, {0, 0, 1, 1});
    ui::Button row_c("C", 18, {0, 0, 1, 1}), row_d("D", 18, {0, 0, 1, 1});
    assert(list.add(row_a) && list.add(row_b) && list.add(row_c) && list.add(row_d));
    assert(list.move(ui::Direction::Down) && list.move(ui::Direction::Down));
    assert(list.selected_index() == 2 && list.scroll_offset() == 40);
    list.render(canvas, font);
    assert(!list.move(ui::Direction::Left));

    ui::ScreenManager manager;
    TestScreen home("home", 0x070d1c), tools("tools", 0x102030);
    assert(manager.register_screen(home) && manager.register_screen(tools));
    assert(!manager.register_screen(home));
    assert(manager.show("tools", 100) && manager.transitioning());
    assert(manager.update(50) && manager.active() == &tools);
    manager.render(canvas, font);
    assert(pixels[0] != (0x80000000u | 0x00102030u));
    assert(manager.update(50) && !manager.transitioning());
    manager.render(canvas, font);
    assert(pixels[0] == (0x80000000u | 0x00102030u));

    font.shutdown();
}
