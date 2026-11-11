#pragma once

#include <cstdint>

namespace orbit::input {

enum ActionBits : std::uint16_t {
    Up = 1u << 0, Down = 1u << 1, Left = 1u << 2, Right = 1u << 3,
    Confirm = 1u << 4, Back = 1u << 5, TabPrevious = 1u << 6,
    TabNext = 1u << 7, Context = 1u << 8,
};

struct PadSample {
    std::uint16_t buttons{0};
    std::uint8_t stick_x{128};
    std::uint8_t stick_y{128};
};

class NavigationInput final {
public:
    static constexpr std::uint16_t kInitialRepeatMs = 360;
    static constexpr std::uint16_t kRepeatIntervalMs = 95;
    static constexpr std::uint8_t kStickLow = 64;
    static constexpr std::uint8_t kStickHigh = 192;

    std::uint16_t update(const PadSample& sample, std::uint16_t elapsed_ms) noexcept;
    void reset() noexcept;
private:
    std::uint16_t previous_buttons_{0};
    std::uint16_t held_direction_{0};
    std::uint16_t held_ms_{0};
    std::uint16_t repeat_ms_{0};
};

class ControllerDevice final {
public:
    ControllerDevice() noexcept = default;
    ~ControllerDevice();
    ControllerDevice(const ControllerDevice&) = delete;
    ControllerDevice& operator=(const ControllerDevice&) = delete;
    bool initialize() noexcept;
    bool read(PadSample& sample) noexcept;
    void shutdown() noexcept;
    bool ready() const noexcept { return handle_ >= 0; }
private:
    int handle_{-1};
};

} // namespace orbit::input
