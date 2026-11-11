#include "orbit/ui/ui.hpp"

#include <algorithm>
#include <cstring>

namespace orbit::ui {
namespace {
using graphics::Canvas;
using graphics::Color;
using graphics::FontRenderer;

Rect placed(Rect bounds, Insets margin, int origin_x, int origin_y) noexcept {
    return {origin_x + bounds.x + margin.left, origin_y + bounds.y + margin.top,
            std::max(0, bounds.width - margin.left - margin.right),
            std::max(0, bounds.height - margin.top - margin.bottom)};
}

bool same_id(const char* lhs, const char* rhs) noexcept {
    return lhs && rhs && std::strcmp(lhs, rhs) == 0;
}
}

Label::Label(const char* text, int pixel_size, Color color, Rect bounds) noexcept
    : Widget(bounds), text_(text), pixel_size_(pixel_size), color_(color) {}

void Label::render(Canvas& canvas, FontRenderer& font, int origin_x, int origin_y) noexcept {
    if (!visible_ || !text_ || pixel_size_ <= 0) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    if (r.width <= 0 || r.height <= 0) return;
    if (!canvas.push_clip(r.x, r.y, r.width, r.height)) return;
    font.draw_text(canvas, r.x, r.y, text_, pixel_size_, color_);
    canvas.pop_clip();
}

bool Panel::add(Widget& child) noexcept {
    if (child_count_ == children_.size()) return false;
    children_[child_count_++] = &child;
    return true;
}

void Panel::set_background(Color color, std::uint8_t opacity) noexcept {
    background_ = color;
    background_opacity_ = opacity;
}

void Panel::set_border(Color color, int thickness) noexcept {
    border_ = color;
    border_thickness_ = std::max(0, thickness);
}

void Panel::render(Canvas& canvas, FontRenderer& font, int origin_x, int origin_y) noexcept {
    if (!visible_) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    if (r.width <= 0 || r.height <= 0) return;
    if (background_opacity_ && enabled_)
        canvas.fill_rounded_rect(r.x, r.y, r.width, r.height, radius_, background_, background_opacity_);
    if (border_thickness_ > 0 && enabled_)
        canvas.stroke_rect(r.x, r.y, r.width, r.height, border_, border_thickness_);
    if (!canvas.push_clip(r.x, r.y, r.width, r.height)) return;
    const int content_x = r.x + padding_.left;
    const int content_y = r.y + padding_.top;
    for (std::size_t i = 0; i < child_count_; ++i)
        if (children_[i]) children_[i]->render(canvas, font, content_x, content_y);
    canvas.pop_clip();
}

void Card::render(Canvas& canvas, FontRenderer& font, int origin_x, int origin_y) noexcept {
    Panel::render(canvas, font, origin_x, origin_y);
    if (!visible_ || !enabled_) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    canvas.fill_rounded_rect(r.x, r.y, 4, r.height, std::min(radius_, 2), accent_, opacity_);
}

bool StackPanel::add(Widget& child) noexcept {
    if (!Panel::add(child)) return false;
    layout();
    return true;
}

void StackPanel::set_spacing(int spacing) noexcept {
    spacing_ = std::max(0, spacing);
    layout();
}

void StackPanel::layout() noexcept {
    int cursor = 0;
    for (std::size_t i = 0; i < child_count_; ++i) {
        Rect r = children_[i]->bounds();
        if (orientation_ == Orientation::Vertical) { r.x = 0; r.y = cursor; cursor += r.height; }
        else { r.x = cursor; r.y = 0; cursor += r.width; }
        children_[i]->set_bounds(r);
        if (i + 1 < child_count_) cursor += spacing_;
    }
}

Button::Button(const char* text, int pixel_size, Rect bounds, Callback callback,
               void* context) noexcept
    : Widget(bounds), text_(text), pixel_size_(pixel_size), callback_(callback), context_(context) {}

bool Button::activate() noexcept {
    if (!visible_ || !enabled_) return false;
    selected_ = true;
    if (callback_) callback_(context_);
    return true;
}

void Button::render(Canvas& canvas, FontRenderer& font, int origin_x, int origin_y) noexcept {
    if (!visible_) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    if (r.width <= 0 || r.height <= 0) return;
    const Color base = enabled_ ? Color{13, 28, 49} : Color{10, 17, 28};
    canvas.fill_rounded_rect(r.x, r.y, r.width, r.height, 10, base, opacity_);
    if (focused_) {
        canvas.fill_rounded_rect(r.x, r.y, r.width, r.height, 10, {17, 61, 88}, 150);
        canvas.stroke_rect(r.x, r.y, r.width, r.height, {48, 207, 255}, 2);
    } else if (selected_) {
        canvas.stroke_rect(r.x, r.y, r.width, r.height, {42, 103, 240}, 2);
    } else {
        canvas.stroke_rect(r.x, r.y, r.width, r.height, {29, 51, 77}, 1);
    }
    if (text_ && enabled_) {
        const int text_width = font.measure_width(text_, pixel_size_);
        const int line_height = font.line_height(pixel_size_);
        const int x = r.x + std::max(12, (r.width - text_width) / 2);
        const int y = r.y + std::max(0, (r.height - line_height) / 2);
        if (!canvas.push_clip(r.x, r.y, r.width, r.height)) return;
        font.draw_text(canvas, x, y, text_, pixel_size_, {225, 239, 252});
        canvas.pop_clip();
    }
}

void Image::render(Canvas& canvas, FontRenderer&, int origin_x, int origin_y) noexcept {
    if (!visible_ || !texture_ || !texture_->valid()) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    if (r.width <= 0 || r.height <= 0 || !canvas.push_clip(r.x, r.y, r.width, r.height)) return;
    texture_->draw(canvas, r.x, r.y, r.width, r.height);
    canvas.pop_clip();
}

void Separator::render(Canvas& canvas, FontRenderer&, int origin_x, int origin_y) noexcept {
    if (!visible_ || !enabled_) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    canvas.fill_rect_alpha(r.x, r.y, r.width, r.height, color_, opacity_);
}

void ProgressBar::render(Canvas& canvas, FontRenderer&, int origin_x, int origin_y) noexcept {
    if (!visible_) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    if (r.width <= 0 || r.height <= 0) return;
    const int radius = std::min(8, r.height / 2);
    canvas.fill_rounded_rect(r.x, r.y, r.width, r.height, radius, track_, opacity_);
    const int fill_width = static_cast<int>(static_cast<std::int64_t>(r.width) * value_ / 1000);
    if (fill_width > 0) canvas.fill_rounded_rect(r.x, r.y, fill_width, r.height, radius, fill_, opacity_);
}

bool ScrollArea::add(Widget& child) noexcept {
    if (child_count_ == children_.size()) return false;
    children_[child_count_++] = &child;
    const Rect r = child.bounds();
    content_height_ = std::max(content_height_, r.y + r.height);
    return true;
}

void ScrollArea::set_content_height(int height) noexcept {
    content_height_ = std::max(0, height);
    scroll_y_ = std::clamp(scroll_y_, 0, std::max(0, content_height_ - bounds_.height));
}

void ScrollArea::scroll_by(int pixels) noexcept {
    scroll_y_ = std::clamp(scroll_y_ + pixels, 0, std::max(0, content_height_ - bounds_.height));
}

void ScrollArea::render(Canvas& canvas, FontRenderer& font, int origin_x, int origin_y) noexcept {
    if (!visible_) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    if (r.width <= 0 || r.height <= 0 || !canvas.push_clip(r.x, r.y, r.width, r.height)) return;
    for (std::size_t i = 0; i < child_count_; ++i)
        if (children_[i]) children_[i]->render(canvas, font, r.x, r.y - scroll_y_);
    canvas.pop_clip();
}

bool List::add(Button& item) noexcept {
    if (count_ == items_.size()) return false;
    items_[count_++] = &item;
    return focus_.add(item);
}

bool List::move(Direction direction) noexcept {
    if (direction != Direction::Up && direction != Direction::Down) return false;
    if (!focus_.move(direction)) return false;
    const int item_top = static_cast<int>(focus_.focused_index()) * row_height_;
    const int item_bottom = item_top + row_height_;
    if (item_top < scroll_y_) scroll_y_ = item_top;
    else if (item_bottom > scroll_y_ + bounds_.height) scroll_y_ = item_bottom - bounds_.height;
    const int content = static_cast<int>(count_) * row_height_;
    scroll_y_ = std::clamp(scroll_y_, 0, std::max(0, content - bounds_.height));
    return true;
}

bool List::confirm() noexcept { return focus_.confirm(); }

void List::render(Canvas& canvas, FontRenderer& font, int origin_x, int origin_y) noexcept {
    if (!visible_) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    if (r.width <= 0 || r.height <= 0 || !canvas.push_clip(r.x, r.y, r.width, r.height)) return;
    for (std::size_t i = 0; i < count_; ++i) {
        const int row_y = static_cast<int>(i) * row_height_ - scroll_y_;
        if (row_y + row_height_ <= 0 || row_y >= r.height) continue;
        items_[i]->set_bounds({0, row_y, r.width, row_height_});
        items_[i]->render(canvas, font, r.x, r.y);
    }
    canvas.pop_clip();
}

void Dialog::render(Canvas& canvas, FontRenderer& font, int origin_x, int origin_y) noexcept {
    if (!visible_) return;
    canvas.fill_rect_alpha(0, 0, 1920, 1080, {2, 5, 13}, 176);
    Panel::render(canvas, font, origin_x, origin_y);
    if (title_) {
        const Rect r = placed(bounds_, margin_, origin_x, origin_y);
        font.draw_text(canvas, r.x + padding_.left + 4, r.y + padding_.top + 4,
                       title_, 30, {48, 207, 255});
    }
}

void Notification::render(Canvas& canvas, FontRenderer& font, int origin_x, int origin_y) noexcept {
    if (!visible_ || !message_) return;
    const Rect r = placed(bounds_, margin_, origin_x, origin_y);
    canvas.fill_rounded_rect(r.x, r.y, r.width, r.height, 10, {12, 28, 49}, opacity_);
    canvas.fill_rounded_rect(r.x, r.y, 4, r.height, 2, {48, 207, 255}, opacity_);
    canvas.stroke_rect(r.x, r.y, r.width, r.height, {34, 67, 97}, 1);
    font.draw_text(canvas, r.x + 20, r.y + 12, message_, 18, {225, 239, 252});
}

bool FocusGroup::add(Button& button) noexcept {
    if (count_ == buttons_.size()) return false;
    buttons_[count_] = &button;
    button.set_focused(count_ == focused_);
    ++count_;
    if (count_ == 1) button.set_focused(true);
    return true;
}

void FocusGroup::focus(std::size_t index) noexcept {
    if (count_ == 0) { focused_ = 0; return; }
    focused_ = index % count_;
    for (std::size_t i = 0; i < count_; ++i)
        buttons_[i]->set_focused(i == focused_ && buttons_[i]->enabled() && buttons_[i]->visible());
}

bool FocusGroup::move(Direction direction) noexcept {
    if (count_ < 2) return false;
    const int sign = (direction == Direction::Up || direction == Direction::Left) ? -1 : 1;
    std::size_t candidate = focused_;
    for (std::size_t tries = 0; tries < count_; ++tries) {
        candidate = static_cast<std::size_t>((static_cast<int>(candidate) + sign + static_cast<int>(count_)) % static_cast<int>(count_));
        if (buttons_[candidate]->enabled() && buttons_[candidate]->visible()) {
            focus(candidate);
            return true;
        }
    }
    return false;
}

bool FocusGroup::confirm() noexcept {
    if (count_ == 0 || !buttons_[focused_]) return false;
    return buttons_[focused_]->activate();
}

bool Screen::handle(Action action) noexcept {
    switch (action) {
        case Action::Up: return focus_.move(Direction::Up);
        case Action::Down: return focus_.move(Direction::Down);
        case Action::Left: return focus_.move(Direction::Left);
        case Action::Right: return focus_.move(Direction::Right);
        case Action::Confirm: return focus_.confirm();
        default: return false;
    }
}

bool ScreenManager::register_screen(Screen& screen) noexcept {
    if (screen_count_ == screens_.size()) return false;
    for (std::size_t i = 0; i < screen_count_; ++i)
        if (same_id(screens_[i]->id(), screen.id())) return false;
    screens_[screen_count_++] = &screen;
    if (!active_) active_ = &screen;
    screen.focus_first();
    return true;
}

bool ScreenManager::show(const char* id, std::uint16_t transition_ms) noexcept {
    Screen* target = nullptr;
    for (std::size_t i = 0; i < screen_count_; ++i)
        if (same_id(screens_[i]->id(), id)) { target = screens_[i]; break; }
    if (!target || target == active_ || transition_duration_ != 0) return false;
    if (!active_ || transition_ms == 0) {
        active_ = target; pending_ = nullptr; target->focus_first(); return true;
    }
    pending_ = target;
    transition_duration_ = transition_ms;
    transition_elapsed_ = 0;
    return true;
}

bool ScreenManager::handle(Action action) noexcept {
    if (!active_ || transition_duration_ != 0) return false;
    if (action == Action::TabNext || action == Action::TabPrevious) {
        std::size_t index = 0;
        while (index < screen_count_ && screens_[index] != active_) ++index;
        if (index == screen_count_) return false;
        const auto delta = action == Action::TabNext ? 1 : screen_count_ - 1;
        return show(screens_[(index + delta) % screen_count_]->id());
    }
    return active_->handle(action);
}

bool ScreenManager::update(std::uint16_t elapsed_ms) noexcept {
    if (transition_duration_ == 0) return false;
    const unsigned next = static_cast<unsigned>(transition_elapsed_) + elapsed_ms;
    transition_elapsed_ = static_cast<std::uint16_t>(std::min<unsigned>(next, transition_duration_));
    if (transition_elapsed_ * 2 >= transition_duration_ && pending_) {
        active_ = pending_;
        pending_ = nullptr;
        active_->focus_first();
    }
    if (transition_elapsed_ >= transition_duration_) {
        transition_duration_ = 0;
        transition_elapsed_ = 0;
    }
    return true;
}

void ScreenManager::render(Canvas& canvas, FontRenderer& font) noexcept {
    if (active_) active_->render(canvas, font);
    if (transition_duration_ != 0) {
        const unsigned half = std::max<unsigned>(1, transition_duration_ / 2);
        const unsigned phase = transition_elapsed_ <= half ? transition_elapsed_ : transition_duration_ - transition_elapsed_;
        const auto opacity = static_cast<std::uint8_t>(std::min<unsigned>(190, phase * 190 / half));
        canvas.fill_rect_alpha(0, 0, 1920, 1080, {0, 0, 0}, opacity);
    }
}

} // namespace orbit::ui
