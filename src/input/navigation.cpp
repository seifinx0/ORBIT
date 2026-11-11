#include "orbit/input/controller.hpp"

namespace orbit::input {
namespace {
constexpr std::uint16_t kDirections = Up | Down | Left | Right;
}

std::uint16_t NavigationInput::update(const PadSample& sample, std::uint16_t elapsed_ms) noexcept {
    const std::uint16_t digital = sample.buttons & kDirections;
    std::uint16_t direction = 0;
    if (digital & Up) direction = Up;
    else if (digital & Down) direction = Down;
    else if (digital & Left) direction = Left;
    else if (digital & Right) direction = Right;
    if (direction == 0) {
        if (sample.stick_y < kStickLow) direction = Up;
        else if (sample.stick_y > kStickHigh) direction = Down;
        else if (sample.stick_x < kStickLow) direction = Left;
        else if (sample.stick_x > kStickHigh) direction = Right;
    }

    const std::uint16_t edge_buttons = sample.buttons & ~previous_buttons_;
    previous_buttons_ = sample.buttons;
    std::uint16_t events = edge_buttons & ~kDirections;
    if (direction == 0) {
        held_direction_ = held_ms_ = repeat_ms_ = 0;
        return events;
    }
    if (direction != held_direction_) {
        held_direction_ = direction;
        held_ms_ = repeat_ms_ = 0;
        events |= direction;
        return events;
    }

    const unsigned capped = elapsed_ms > 100 ? 100 : elapsed_ms;
    if (held_ms_ < kInitialRepeatMs) {
        held_ms_ = static_cast<std::uint16_t>(held_ms_ + capped);
        if (held_ms_ >= kInitialRepeatMs) {
            held_ms_ = kInitialRepeatMs;
            repeat_ms_ = 0;
            events |= direction;
        }
    } else {
        repeat_ms_ = static_cast<std::uint16_t>(repeat_ms_ + capped);
        if (repeat_ms_ >= kRepeatIntervalMs) {
            repeat_ms_ = static_cast<std::uint16_t>(repeat_ms_ - kRepeatIntervalMs);
            events |= direction;
        }
    }
    return events;
}

void NavigationInput::reset() noexcept {
    previous_buttons_ = held_direction_ = held_ms_ = repeat_ms_ = 0;
}

} // namespace orbit::input
