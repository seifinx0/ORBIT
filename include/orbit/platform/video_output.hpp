#pragma once

#include <cstddef>
#include <cstdint>

namespace orbit::platform {

class VideoOutput final {
public:
    static constexpr int kWidth = 1920;
    static constexpr int kHeight = 1080;
    static constexpr int kBufferCount = 2;

    VideoOutput() = default;
    ~VideoOutput();
    VideoOutput(const VideoOutput&) = delete;
    VideoOutput& operator=(const VideoOutput&) = delete;

    bool initialize() noexcept;
    void shutdown() noexcept;
    std::uint32_t* buffer(int index) noexcept;
    bool present(int index, std::int64_t frame_id) noexcept;

private:
    int handle_{-1};
    std::uint64_t queue_{0};
    std::int64_t memory_offset_{0};
    std::size_t memory_size_{0};
    void* memory_{nullptr};
    void* buffers_[kBufferCount]{};
    bool registered_{false};
};

} // namespace orbit::platform
