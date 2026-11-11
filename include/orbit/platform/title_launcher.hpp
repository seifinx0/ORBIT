#pragma once

namespace orbit::platform {
class TitleLauncher final {
public:
    // Returns the SceLncUtil result. A nonnegative result means the system
    // accepted the launch request; the title transition itself is external.
    int launch(const char* title_id) noexcept;
};
}
