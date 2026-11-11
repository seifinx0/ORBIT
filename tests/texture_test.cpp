#include "orbit/graphics/texture.hpp"

#include <array>
#include <cassert>
#include <cstdint>

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
}
