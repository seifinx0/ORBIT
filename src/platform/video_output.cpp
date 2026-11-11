#include "orbit/platform/video_output.hpp"

#include <orbis/VideoOut.h>
#include <orbis/libkernel.h>

#include <cstdio>

namespace orbit::platform {
namespace {
constexpr std::size_t kBytesPerPixel = 4;
constexpr std::size_t kBufferBytes = VideoOutput::kWidth * VideoOutput::kHeight * kBytesPerPixel;
constexpr std::size_t kAlignment = 2 * 1024 * 1024;
constexpr std::size_t kAllocationBytes =
    ((kBufferBytes * VideoOutput::kBufferCount + kAlignment - 1) / kAlignment) * kAlignment;
}

VideoOutput::~VideoOutput() { shutdown(); }

bool VideoOutput::initialize() noexcept {
    handle_ = sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN, ORBIS_VIDEO_OUT_BUS_MAIN, 0, nullptr);
    if (handle_ < 0) {
        std::fprintf(stderr, "ORBIT: sceVideoOutOpen failed (%d)\n", handle_);
        handle_ = -1;
        return false;
    }

    int result = sceKernelCreateEqueue(reinterpret_cast<OrbisKernelEqueue*>(&queue_), "ORBIT flip queue");
    if (result < 0) {
        std::fprintf(stderr, "ORBIT: sceKernelCreateEqueue failed (%d)\n", result);
        shutdown();
        return false;
    }
    result = sceVideoOutAddFlipEvent(queue_, handle_, nullptr);
    if (result < 0) {
        std::fprintf(stderr, "ORBIT: sceVideoOutAddFlipEvent failed (%d)\n", result);
        shutdown();
        return false;
    }

    result = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), kAllocationBytes,
                                            kAlignment, 3, &memory_offset_);
    if (result < 0) {
        std::fprintf(stderr, "ORBIT: direct memory allocation failed (%d)\n", result);
        shutdown();
        return false;
    }
    memory_size_ = kAllocationBytes;
    result = sceKernelMapDirectMemory(&memory_, memory_size_, 0x33, 0, memory_offset_, kAlignment);
    if (result < 0) {
        std::fprintf(stderr, "ORBIT: direct memory mapping failed (%d)\n", result);
        shutdown();
        return false;
    }

    auto* base = static_cast<std::uint8_t*>(memory_);
    buffers_[0] = base;
    buffers_[1] = base + kBufferBytes;
    OrbisVideoOutBufferAttribute attribute{};
    sceVideoOutSetBufferAttribute(&attribute, 0x80000000, 1, 0, kWidth, kHeight, kWidth);
    result = sceVideoOutRegisterBuffers(handle_, 0, buffers_, kBufferCount, &attribute);
    if (result < 0) {
        std::fprintf(stderr, "ORBIT: sceVideoOutRegisterBuffers failed (%d)\n", result);
        shutdown();
        return false;
    }
    registered_ = true;
    result = sceVideoOutSetFlipRate(handle_, 0);
    if (result < 0) {
        std::fprintf(stderr, "ORBIT: sceVideoOutSetFlipRate failed (%d)\n", result);
        shutdown();
        return false;
    }
    return true;
}

void VideoOutput::shutdown() noexcept {
    if (registered_ && handle_ >= 0) {
        sceVideoOutUnregisterBuffers(handle_, 0);
        registered_ = false;
    }
    if (queue_ != 0) {
        sceKernelDeleteEqueue(queue_);
        queue_ = 0;
    }
    if (memory_) {
        sceKernelMunmap(memory_, memory_size_);
        memory_ = nullptr;
    }
    if (memory_size_ > 0) {
        sceKernelReleaseDirectMemory(memory_offset_, memory_size_);
        memory_size_ = 0;
        memory_offset_ = 0;
    }
    if (handle_ >= 0) {
        sceVideoOutClose(handle_);
        handle_ = -1;
    }
}

std::uint32_t* VideoOutput::buffer(int index) noexcept {
    if (index < 0 || index >= kBufferCount || !buffers_[index]) return nullptr;
    return static_cast<std::uint32_t*>(buffers_[index]);
}

bool VideoOutput::present(int index, std::int64_t frame_id) noexcept {
    if (handle_ < 0 || queue_ == 0 || index < 0 || index >= kBufferCount) return false;
    if (sceVideoOutSubmitFlip(handle_, index, ORBIS_VIDEO_OUT_FLIP_VSYNC, frame_id) < 0) return false;

    for (;;) {
        OrbisVideoOutFlipStatus status{};
        if (sceVideoOutGetFlipStatus(handle_, &status) < 0) return false;
        if (status.flipArg == frame_id) break;
        OrbisKernelEvent event{};
        int count = 0;
        if (sceKernelWaitEqueue(queue_, &event, 1, &count, nullptr) < 0)
            return false;
    }
    return true;
}

} // namespace orbit::platform
