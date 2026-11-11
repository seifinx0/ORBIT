#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "orbit/graphics/texture.hpp"

#include <cstring>
#include <limits>

namespace orbit::graphics {

Texture::~Texture() { reset(); }

Texture::Texture(Texture&& other) noexcept
    : pixels_(other.pixels_), width_(other.width_), height_(other.height_), byte_size_(other.byte_size_) {
    other.pixels_ = nullptr;
    other.width_ = other.height_ = 0;
    other.byte_size_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this == &other) return *this;
    reset();
    pixels_ = other.pixels_; width_ = other.width_; height_ = other.height_; byte_size_ = other.byte_size_;
    other.pixels_ = nullptr;
    other.width_ = other.height_ = 0;
    other.byte_size_ = 0;
    return *this;
}

bool Texture::load_png(const char* path) noexcept {
    if (!path || !*path) return false;
    int width = 0, height = 0, components = 0;
    if (!stbi_info(path, &width, &height, &components) || width <= 0 || height <= 0 ||
        width > 4096 || height > 4096) return false;
    const auto pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (pixels > TextureCache::kMaxTextureBytes / 4) return false;
    int decoded_width = 0, decoded_height = 0, decoded_components = 0;
    stbi_uc* decoded = stbi_load(path, &decoded_width, &decoded_height, &decoded_components, STBI_rgb_alpha);
    if (!decoded || decoded_width != width || decoded_height != height) {
        if (decoded) stbi_image_free(decoded);
        return false;
    }
    reset();
    pixels_ = decoded;
    width_ = width;
    height_ = height;
    byte_size_ = pixels * 4;
    return true;
}

bool Texture::png_decoded_size(const char* path, std::size_t& bytes) noexcept {
    bytes = 0;
    if (!path || !*path) return false;
    int width = 0, height = 0, components = 0;
    if (!stbi_info(path, &width, &height, &components) || width <= 0 || height <= 0 ||
        width > 4096 || height > 4096) return false;
    const auto pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (pixels > TextureCache::kMaxTextureBytes / 4) return false;
    bytes = pixels * 4;
    return true;
}

void Texture::reset() noexcept {
    if (pixels_) stbi_image_free(pixels_);
    pixels_ = nullptr;
    width_ = height_ = 0;
    byte_size_ = 0;
}

void Texture::draw(Canvas& canvas, int x, int y, int width, int height, bool bilinear) const noexcept {
    if (pixels_) canvas.draw_rgba(pixels_, width_, height_, width_ * 4, x, y, width, height, bilinear);
}

const Texture* TextureCache::get_png(const char* path) noexcept {
    if (!path || !*path) return nullptr;
    ++clock_;
    for (std::size_t i = 0; i < count_; ++i) {
        if (std::strcmp(entries_[i].path.data(), path) == 0) {
            entries_[i].last_used = clock_;
            return &entries_[i].texture;
        }
    }
    if (std::strlen(path) >= kPathCapacity) return nullptr;
    std::size_t decoded_size = 0;
    if (!Texture::png_decoded_size(path, decoded_size) || decoded_size > kMaxCacheBytes) return nullptr;
    // Make room using the PNG dimensions before decoding so cache plus the
    // incoming RGBA image remains within the aggregate decoded-pixel budget.
    while (count_ == kCapacity || decoded_size > kMaxCacheBytes - bytes_) {
        if (count_ == 0) return nullptr;
        std::size_t oldest = 0;
        for (std::size_t i = 1; i < count_; ++i)
            if (entries_[i].last_used < entries_[oldest].last_used) oldest = i;
        bytes_ -= entries_[oldest].texture.byte_size();
        entries_[oldest] = std::move(entries_[count_ - 1]);
        --count_;
    }
    Texture decoded;
    if (!decoded.load_png(path) || decoded.byte_size() != decoded_size) return nullptr;
    if (decoded.byte_size() > kMaxCacheBytes) return nullptr;
    Entry& entry = entries_[count_];
    entry.texture = std::move(decoded);
    std::strcpy(entry.path.data(), path);
    entry.last_used = clock_;
    bytes_ += entry.texture.byte_size();
    ++count_;
    return &entry.texture;
}

void TextureCache::clear() noexcept {
    for (std::size_t i = 0; i < count_; ++i) {
        entries_[i].texture.reset();
        entries_[i].path[0] = '\0';
    }
    count_ = bytes_ = 0;
    clock_ = 0;
}

} // namespace orbit::graphics
