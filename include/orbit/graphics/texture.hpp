#pragma once

#include "orbit/graphics/canvas.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace orbit::graphics {

class Texture final {
public:
    Texture() noexcept = default;
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool load_png(const char* path) noexcept;
    void reset() noexcept;
    void draw(Canvas& canvas, int x, int y, int width, int height,
              bool bilinear = true) const noexcept;
    bool valid() const noexcept { return pixels_ != nullptr; }
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    std::size_t byte_size() const noexcept { return byte_size_; }

private:
    std::uint8_t* pixels_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    std::size_t byte_size_ = 0;
};

class TextureCache final {
public:
    static constexpr std::size_t kCapacity = 16;
    static constexpr std::size_t kMaxTextureBytes = 16u * 1024u * 1024u;
    static constexpr std::size_t kMaxCacheBytes = 32u * 1024u * 1024u;
    static constexpr std::size_t kPathCapacity = 256;

    const Texture* get_png(const char* path) noexcept;
    void clear() noexcept;
    std::size_t count() const noexcept { return count_; }
    std::size_t bytes() const noexcept { return bytes_; }

private:
    struct Entry {
        std::array<char, kPathCapacity> path{};
        Texture texture{};
    };
    std::array<Entry, kCapacity> entries_{};
    std::size_t count_ = 0;
    std::size_t bytes_ = 0;
};

} // namespace orbit::graphics
