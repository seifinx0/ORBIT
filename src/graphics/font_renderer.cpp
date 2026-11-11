#include "orbit/graphics/font_renderer.hpp"

#include <orbis/Sysmodule.h>
#include <orbis/_types/sysmodule.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>

// OpenOrbis ships the FreeType public types/constants in its headers but keeps
// the callable declarations in proto-include.h. Declare only the API used here
// so this module remains compatible with upstream FreeType headers as well.
extern "C" {
FT_Error FT_Init_FreeType(FT_Library* library);
FT_Error FT_Done_FreeType(FT_Library library);
FT_Error FT_New_Face(FT_Library library, const char* path, FT_Long face_index, FT_Face* face);
FT_Error FT_Done_Face(FT_Face face);
FT_Error FT_Load_Char(FT_Face face, FT_ULong character, FT_Int32 load_flags);
FT_Error FT_Render_Glyph(FT_GlyphSlot slot, FT_Render_Mode render_mode);
FT_UInt FT_Get_Char_Index(FT_Face face, FT_ULong character);
FT_Error FT_Get_Kerning(FT_Face face, FT_UInt left, FT_UInt right,
                        FT_UInt mode, FT_Vector* delta);
}

namespace orbit::graphics {
namespace {
constexpr int kFirstCharacter = 32;
constexpr int kLastCharacter = 126;
constexpr int kCharacterSlots = 128;
constexpr int kSizeSlots = 5;
}

struct FontRenderer::State {
    struct Glyph {
        std::vector<std::uint8_t> coverage;
        int width{0};
        int rows{0};
        int left{0};
        int top{0};
        int advance{0};
        FT_UInt index{0};
        bool loaded{false};
    };

    struct SizeCache {
        std::array<Glyph, kCharacterSlots> glyphs{};
        int pixel_size{0};
        int ascender{0};
        int line_height{0};
    };

    FT_Library library{nullptr};
    FT_Face face{nullptr};
    std::array<SizeCache, kSizeSlots> sizes{};
};

FontRenderer::FontRenderer() noexcept = default;
FontRenderer::~FontRenderer() { shutdown(); }

bool FontRenderer::initialize(const char* font_path) noexcept {
    shutdown();
    if (!font_path || sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL) < 0) return false;

    state_.reset(new (std::nothrow) State());
    if (!state_) return false;
    if (FT_Init_FreeType(&state_->library) != 0 ||
        FT_New_Face(state_->library, font_path, 0, &state_->face) != 0) {
        shutdown();
        return false;
    }
    return true;
}

bool FontRenderer::prepare_size(int pixel_size) noexcept {
    if (!state_ || !state_->face || pixel_size < 8 || pixel_size > 128) return false;
    for (auto& size : state_->sizes)
        if (size.pixel_size == pixel_size) return true;

    State::SizeCache* target = nullptr;
    for (auto& size : state_->sizes) {
        if (size.pixel_size == 0) { target = &size; break; }
    }
    if (!target || FT_Set_Pixel_Sizes(state_->face, 0, static_cast<FT_UInt>(pixel_size)) != 0)
        return false;

    target->ascender = static_cast<int>((state_->face->size->metrics.ascender + 32) >> 6);
    target->line_height = static_cast<int>((state_->face->size->metrics.height + 32) >> 6);
    if (target->line_height < pixel_size) target->line_height = pixel_size;

    for (int code = kFirstCharacter; code <= kLastCharacter; ++code) {
        auto& glyph = target->glyphs[code];
        if (FT_Load_Char(state_->face, static_cast<FT_ULong>(code), FT_LOAD_DEFAULT) != 0 ||
            FT_Render_Glyph(state_->face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
            target->pixel_size = 0;
            return false;
        }

        const auto& slot = *state_->face->glyph;
        const auto& bitmap = slot.bitmap;
        glyph.width = bitmap.width;
        glyph.rows = bitmap.rows;
        glyph.left = slot.bitmap_left;
        glyph.top = slot.bitmap_top;
        glyph.advance = static_cast<int>(slot.advance.x >> 6);
        glyph.index = FT_Get_Char_Index(state_->face, static_cast<FT_ULong>(code));
        if (glyph.width > 0 && glyph.rows > 0) {
            if (!bitmap.buffer || (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY &&
                                   bitmap.pixel_mode != FT_PIXEL_MODE_MONO)) {
                target->pixel_size = 0;
                return false;
            }
            glyph.coverage.resize(static_cast<std::size_t>(glyph.width) * glyph.rows);
            const int pitch = bitmap.pitch < 0 ? -bitmap.pitch : bitmap.pitch;
            for (int row = 0; row < glyph.rows; ++row) {
                const int source_row = bitmap.pitch < 0 ? glyph.rows - row - 1 : row;
                const auto* source = bitmap.buffer + static_cast<std::size_t>(source_row) * pitch;
                auto* destination = glyph.coverage.data() + static_cast<std::size_t>(row) * glyph.width;
                for (int column = 0; column < glyph.width; ++column) {
                    if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                        destination[column] = (source[column / 8] & (0x80u >> (column % 8))) ? 255 : 0;
                    } else {
                        const unsigned levels = bitmap.num_grays > 1 ? bitmap.num_grays - 1 : 255;
                        destination[column] = static_cast<std::uint8_t>(
                            (static_cast<unsigned>(source[column]) * 255u + levels / 2u) / levels);
                    }
                }
            }
        }
        glyph.loaded = true;
    }
    target->pixel_size = pixel_size;
    return true;
}

int FontRenderer::measure_width(const char* text, int pixel_size) noexcept {
    if (!text || !prepare_size(pixel_size) ||
        FT_Set_Pixel_Sizes(state_->face, 0, static_cast<FT_UInt>(pixel_size)) != 0)
        return 0;

    const State::SizeCache* cache = nullptr;
    for (const auto& size : state_->sizes)
        if (size.pixel_size == pixel_size) { cache = &size; break; }
    if (!cache) return 0;

    int current_width = 0;
    int maximum_width = 0;
    FT_UInt previous = 0;
    for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text); *cursor; ++cursor) {
        if (*cursor == '\n') {
            if (current_width > maximum_width) maximum_width = current_width;
            current_width = 0;
            previous = 0;
            continue;
        }
        const int code = (*cursor >= kFirstCharacter && *cursor <= kLastCharacter) ? *cursor : '?';
        const auto& glyph = cache->glyphs[code];
        if (!glyph.loaded) continue;
        if (previous && glyph.index) {
            FT_Vector delta{};
            if (FT_Get_Kerning(state_->face, previous, glyph.index, FT_KERNING_DEFAULT, &delta) == 0)
                current_width += static_cast<int>(delta.x >> 6);
        }
        current_width += glyph.advance;
        previous = glyph.index;
    }
    return current_width > maximum_width ? current_width : maximum_width;
}

int FontRenderer::line_height(int pixel_size) const noexcept {
    if (!state_) return 0;
    for (const auto& size : state_->sizes)
        if (size.pixel_size == pixel_size) return size.line_height;
    return 0;
}

void FontRenderer::draw_text(Canvas& canvas, int x, int y, const char* text,
                             int pixel_size, Color color) noexcept {
    if (!text || !prepare_size(pixel_size) ||
        FT_Set_Pixel_Sizes(state_->face, 0, static_cast<FT_UInt>(pixel_size)) != 0)
        return;

    const State::SizeCache* cache = nullptr;
    for (const auto& size : state_->sizes)
        if (size.pixel_size == pixel_size) { cache = &size; break; }
    if (!cache) return;

    const int origin_x = x;
    int baseline = y + cache->ascender;
    FT_UInt previous = 0;
    for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text); *cursor; ++cursor) {
        if (*cursor == '\n') {
            x = origin_x;
            baseline += cache->line_height;
            previous = 0;
            continue;
        }
        const int code = (*cursor >= kFirstCharacter && *cursor <= kLastCharacter) ? *cursor : '?';
        const auto& glyph = cache->glyphs[code];
        if (!glyph.loaded) continue;
        if (previous && glyph.index) {
            FT_Vector delta{};
            if (FT_Get_Kerning(state_->face, previous, glyph.index, FT_KERNING_DEFAULT, &delta) == 0)
                x += static_cast<int>(delta.x >> 6);
        }

        const int glyph_x = x + glyph.left;
        const int glyph_y = baseline - glyph.top;
        for (int row = 0; row < glyph.rows; ++row) {
            for (int column = 0; column < glyph.width; ++column) {
                const auto opacity = glyph.coverage[static_cast<std::size_t>(row) * glyph.width + column];
                if (opacity)
                    canvas.blend_pixel(glyph_x + column, glyph_y + row, color, opacity);
            }
        }
        x += glyph.advance;
        previous = glyph.index;
    }
}

void FontRenderer::shutdown() noexcept {
    if (!state_) return;
    if (state_->face) FT_Done_Face(state_->face);
    if (state_->library) FT_Done_FreeType(state_->library);
    state_.reset();
}

} // namespace orbit::graphics
