#pragma once

#include "orbit/graphics/canvas.hpp"
#include "orbit/graphics/font_renderer.hpp"
#include "orbit/graphics/texture.hpp"

#include <array>
#include <cstdint>

namespace orbit::ui {

struct Rect { int x, y, width, height; };
struct Insets { int left{0}, top{0}, right{0}, bottom{0}; };
enum class Direction : std::uint8_t { Up, Down, Left, Right };
enum class Orientation : std::uint8_t { Horizontal, Vertical };
enum class Action : std::uint8_t { Up, Down, Left, Right, Confirm, Back, TabPrevious, TabNext, Context };
using Callback = void (*)(void* context) noexcept;

class Widget {
public:
    explicit Widget(Rect bounds = {}) noexcept : bounds_(bounds) {}
    virtual ~Widget() = default;
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
    void set_bounds(Rect bounds) noexcept { bounds_ = bounds; }
    Rect bounds() const noexcept { return bounds_; }
    void set_visible(bool visible) noexcept { visible_ = visible; }
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    bool visible() const noexcept { return visible_; }
    bool enabled() const noexcept { return enabled_; }
    void set_opacity(std::uint8_t opacity) noexcept { opacity_ = opacity; }
    void set_margin(Insets margin) noexcept { margin_ = margin; }
    Insets margin() const noexcept { return margin_; }
    virtual void render(graphics::Canvas& canvas, graphics::FontRenderer& font,
                        int origin_x = 0, int origin_y = 0) noexcept = 0;
protected:
    Rect bounds_{};
    Insets margin_{};
    bool visible_{true};
    bool enabled_{true};
    std::uint8_t opacity_{255};
};

class Label final : public Widget {
public:
    Label(const char* text, int pixel_size, graphics::Color color, Rect bounds = {}) noexcept;
    void set_text(const char* text) noexcept { text_ = text; }
    void set_color(graphics::Color color) noexcept { color_ = color; }
    void set_pixel_size(int size) noexcept { pixel_size_ = size; }
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    const char* text_;
    int pixel_size_;
    graphics::Color color_;
};

class Panel : public Widget {
public:
    static constexpr std::size_t kMaxChildren = 16;
    explicit Panel(Rect bounds = {}) noexcept : Widget(bounds) {}
    bool add(Widget& child) noexcept;
    void set_padding(Insets padding) noexcept { padding_ = padding; }
    void set_background(graphics::Color color, std::uint8_t opacity = 255) noexcept;
    void set_border(graphics::Color color, int thickness = 0) noexcept;
    void set_radius(int radius) noexcept { radius_ = radius; }
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
protected:
    std::array<Widget*, kMaxChildren> children_{};
    std::size_t child_count_{0};
    Insets padding_{};
    graphics::Color background_{0, 0, 0};
    graphics::Color border_{0, 0, 0};
    std::uint8_t background_opacity_{0};
    int border_thickness_{0};
    int radius_{0};
};

class Card final : public Panel {
public:
    using Panel::Panel;
    void set_accent(graphics::Color accent) noexcept { accent_ = accent; }
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    graphics::Color accent_{48, 207, 255};
};

class StackPanel final : public Panel {
public:
    StackPanel(Orientation orientation, Rect bounds = {}) noexcept
        : Panel(bounds), orientation_(orientation) {}
    bool add(Widget& child) noexcept;
    void set_spacing(int spacing) noexcept;
    void layout() noexcept;
private:
    Orientation orientation_;
    int spacing_{0};
};

class Button final : public Widget {
public:
    Button(const char* text, int pixel_size, Rect bounds, Callback callback = nullptr,
           void* context = nullptr) noexcept;
    void set_focused(bool focused) noexcept { focused_ = focused; }
    void set_selected(bool selected) noexcept { selected_ = selected; }
    bool focused() const noexcept { return focused_; }
    bool activate() noexcept;
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    const char* text_;
    int pixel_size_;
    Callback callback_;
    void* context_;
    bool focused_{false};
    bool selected_{false};
};

class Image final : public Widget {
public:
    Image(const graphics::Texture* texture, Rect bounds) noexcept : Widget(bounds), texture_(texture) {}
    void set_texture(const graphics::Texture* texture) noexcept { texture_ = texture; }
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    const graphics::Texture* texture_;
};

class Separator final : public Widget {
public:
    Separator(graphics::Color color, bool vertical, Rect bounds) noexcept
        : Widget(bounds), color_(color), vertical_(vertical) {}
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    graphics::Color color_;
    bool vertical_;
};

class ProgressBar final : public Widget {
public:
    explicit ProgressBar(Rect bounds) noexcept : Widget(bounds) {}
    void set_value(std::uint16_t value) noexcept { value_ = value > 1000 ? 1000 : value; }
    void set_colors(graphics::Color track, graphics::Color fill) noexcept { track_ = track; fill_ = fill; }
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    std::uint16_t value_{0};
    graphics::Color track_{18, 36, 60};
    graphics::Color fill_{48, 207, 255};
};

class FocusGroup final {
public:
    static constexpr std::size_t kCapacity = 24;
    bool add(Button& button) noexcept;
    bool move(Direction direction) noexcept;
    bool confirm() noexcept;
    void focus(std::size_t index) noexcept;
    std::size_t focused_index() const noexcept { return focused_; }
    std::size_t count() const noexcept { return count_; }
private:
    std::array<Button*, kCapacity> buttons_{};
    std::size_t count_{0};
    std::size_t focused_{0};
};

class ScrollArea final : public Widget {
public:
    static constexpr std::size_t kMaxChildren = 24;
    explicit ScrollArea(Rect bounds) noexcept : Widget(bounds) {}
    bool add(Widget& child) noexcept;
    void set_content_height(int height) noexcept;
    void scroll_by(int pixels) noexcept;
    int scroll_offset() const noexcept { return scroll_y_; }
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    std::array<Widget*, kMaxChildren> children_{};
    std::size_t child_count_{0};
    int content_height_{0};
    int scroll_y_{0};
};

class List final : public Widget {
public:
    static constexpr std::size_t kCapacity = 24;
    explicit List(Rect bounds, int row_height = 64) noexcept
        : Widget(bounds), row_height_(row_height < 1 ? 1 : (row_height > 4096 ? 4096 : row_height)) {}
    bool add(Button& item) noexcept;
    bool move(Direction direction) noexcept;
    bool confirm() noexcept;
    std::size_t selected_index() const noexcept { return focus_.focused_index(); }
    int scroll_offset() const noexcept { return scroll_y_; }
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    std::array<Button*, kCapacity> items_{};
    std::size_t count_{0};
    int row_height_;
    int scroll_y_{0};
    FocusGroup focus_{};
};

class Dialog final : public Panel {
public:
    using Panel::Panel;
    void set_title(const char* title) noexcept { title_ = title; }
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    const char* title_{nullptr};
};

class Notification final : public Widget {
public:
    Notification(const char* message, Rect bounds) noexcept : Widget(bounds), message_(message) {}
    void set_message(const char* message) noexcept { message_ = message; }
    void render(graphics::Canvas&, graphics::FontRenderer&, int = 0, int = 0) noexcept override;
private:
    const char* message_;
};

class Screen {
public:
    explicit Screen(const char* id) noexcept : id_(id) {}
    virtual ~Screen() = default;
    const char* id() const noexcept { return id_; }
    virtual void render(graphics::Canvas&, graphics::FontRenderer&) noexcept = 0;
    void focus_first() noexcept { focus_.focus(0); }
    virtual bool handle(Action action) noexcept;
    FocusGroup& focus_group() noexcept { return focus_; }
private:
    const char* id_;
    FocusGroup focus_{};
};

class ScreenManager final {
public:
    static constexpr std::size_t kMaxScreens = 8;
    bool register_screen(Screen& screen) noexcept;
    bool show(const char* id, std::uint16_t transition_ms = 180) noexcept;
    bool handle(Action action) noexcept;
    bool update(std::uint16_t elapsed_ms) noexcept;
    void render(graphics::Canvas&, graphics::FontRenderer&) noexcept;
    const Screen* active() const noexcept { return active_; }
    bool transitioning() const noexcept { return transition_duration_ != 0; }
private:
    std::array<Screen*, kMaxScreens> screens_{};
    std::size_t screen_count_{0};
    Screen* active_{nullptr};
    Screen* pending_{nullptr};
    std::uint16_t transition_duration_{0};
    std::uint16_t transition_elapsed_{0};
};

} // namespace orbit::ui
