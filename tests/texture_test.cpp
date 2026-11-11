#include "orbit/graphics/texture.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

int main() {
    using namespace orbit::graphics;
    TextureCache cache;
    const Texture* mark = cache.get_png("assets/images/orbit-mark.png");
    assert(mark && mark->valid());
    assert(mark->width() == 512 && mark->height() == 512);
    assert(cache.count() == 1 && cache.bytes() == 512u * 512u * 4u);
    assert(cache.get_png("assets/images/orbit-mark.png") == mark);
    assert(cache.count() == 1);

    std::array<std::uint32_t, 64 * 64> pixels{};
    Canvas canvas(pixels.data(), 64, 64);
    canvas.clear({4, 8, 16});
    mark->draw(canvas, -8, -8, 48, 48, true);
    bool changed = false;
    for (const auto pixel : pixels)
        if (pixel != (0x80000000u | 0x00040810u)) { changed = true; break; }
    assert(changed);
    assert(cache.get_png("assets/icons/not-present.png") == nullptr);
    assert(cache.count() == 1);
    cache.clear();
    assert(cache.count() == 0 && cache.bytes() == 0);

    const char* fixture = "build/tests/texture-lru";
    assert(mkdir(fixture, 0700) == 0);
    for (int i = 0; i < 17; ++i) {
        char path[96]{}; std::snprintf(path, sizeof(path), "%s/image-%02d.png", fixture, i);
        FILE* in = std::fopen("assets/images/orbit-mark.png", "rb");
        FILE* out = std::fopen(path, "wb"); assert(in && out);
        unsigned char block[4096]; std::size_t count = 0;
        while ((count = std::fread(block, 1, sizeof(block), in)) != 0) assert(std::fwrite(block, 1, count, out) == count);
        std::fclose(in); std::fclose(out);
        assert(cache.get_png(path) && cache.count() <= TextureCache::kCapacity);
    }
    assert(cache.count() == TextureCache::kCapacity && cache.bytes() <= TextureCache::kMaxCacheBytes);
    assert(cache.get_png("build/tests/texture-lru/image-00.png"));
    assert(cache.count() == TextureCache::kCapacity && cache.bytes() <= TextureCache::kMaxCacheBytes);
    cache.clear();
    for (int i = 0; i < 17; ++i) { char path[96]{}; std::snprintf(path, sizeof(path), "%s/image-%02d.png", fixture, i); unlink(path); }
    rmdir(fixture);
}
