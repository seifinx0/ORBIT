#pragma once

#include "orbit/graphics/font_renderer.hpp"
#include "orbit/graphics/texture.hpp"
#include "orbit/input/controller.hpp"
#include "orbit/ui/ui.hpp"

namespace orbit::modules {

class Dashboard final {
public:
    Dashboard() noexcept = default;
    ~Dashboard();
    Dashboard(const Dashboard&) = delete;
    Dashboard& operator=(const Dashboard&) = delete;

    bool initialize(const graphics::Texture* mark) noexcept;
    bool handle(std::uint16_t input_events) noexcept;
    bool tick(std::uint16_t elapsed_ms) noexcept;
    void render(graphics::Canvas& canvas, graphics::FontRenderer& font) noexcept;
private:
    struct Pages;
    ui::ScreenManager screens_{};
    Pages* pages_{nullptr};
    ui::Dialog controls_{{560, 300, 800, 480}};
    ui::Label control_move_{"D-pad or left stick moves the focus.", 23, {225, 239, 252}, {0, 0, 700, 42}};
    ui::Label control_confirm_{"Cross confirms. Circle returns.", 23, {225, 239, 252}, {0, 58, 700, 42}};
    ui::Label control_tabs_{"L1/R1 switches pages. Options closes this panel.", 23, {225, 239, 252}, {0, 116, 700, 42}};
    ui::Button control_close_{"CLOSE", 23, {230, 210, 260, 64}};
    ui::FocusGroup control_focus_{};
    bool controls_visible_{false};
};

} // namespace orbit::modules
