#pragma once

#include "orbit/graphics/texture.hpp"
#include "orbit/input/controller.hpp"
#include "orbit/library/game.hpp"
#include "orbit/library/preferences.hpp"
#include "orbit/platform/title_launcher.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace orbit::graphics { class Canvas; class FontRenderer; }

namespace orbit::modules {

// The existing application entry point is retained for source compatibility;
// its content is now the ORBIT installed-game library.
class Dashboard final {
public:
    Dashboard() noexcept = default;
    ~Dashboard();
    Dashboard(const Dashboard&) = delete;
    Dashboard& operator=(const Dashboard&) = delete;
    bool initialize(const graphics::Texture* mark, graphics::TextureCache* textures = nullptr) noexcept;
    bool handle(std::uint16_t input_events) noexcept;
    bool tick(std::uint16_t elapsed_ms) noexcept;
    void render(graphics::Canvas& canvas, graphics::FontRenderer& font) noexcept;
private:
    enum class View : std::uint8_t { Library, Details, Search, Collections, Settings };
    enum class Category : std::uint8_t { All, Favorites, Collections };
    static constexpr std::size_t kSearchCapacity = 32;
    void rebuild_results() noexcept;
    bool save_preferences() noexcept;
    void show_notice(const char* text) noexcept;
    const library::Game* selected_game() const noexcept;
    void confirm() noexcept;
    void open_context() noexcept;
    void context_confirm() noexcept;
    void change_selection(int delta) noexcept;
    void refresh_library() noexcept;
    void draw_background(graphics::Canvas& canvas, graphics::FontRenderer& font) noexcept;
    void draw_library(graphics::Canvas& canvas, graphics::FontRenderer& font) noexcept;
    void draw_details(graphics::Canvas& canvas, graphics::FontRenderer& font) noexcept;
    void draw_modal(graphics::Canvas& canvas, graphics::FontRenderer& font) noexcept;

    library::Catalog* catalog_{nullptr};
    library::Preferences preferences_{};
    graphics::TextureCache* textures_{nullptr};
    const graphics::Texture* mark_{nullptr};
    platform::TitleLauncher launcher_{};
    std::array<std::uint16_t, library::Catalog::kCapacity> results_{};
    std::size_t result_count_{0};
    std::size_t selected_{0};
    std::size_t focused_tab_{0};
    std::size_t context_index_{0};
    View view_{View::Library};
    View rendered_view_{View::Library};
    Category category_{Category::All};
    bool context_open_{false};
    bool tabs_focused_{false};
    bool search_active_{false};
    bool search_keyboard_{false};
    bool collection_keyboard_{false};
    bool carousel_animated_{false};
    std::uint16_t animation_ms_{0};
    std::uint16_t animation_duration_{240};
    std::uint16_t transition_ms_{0};
    std::uint16_t transition_duration_{160};
    std::uint16_t notice_ms_{0};
    std::array<char, kSearchCapacity> query_{};
    std::array<char, kSearchCapacity> collection_name_{};
    std::array<char, 96> notice_{};
    std::size_t keyboard_index_{0};
    std::size_t collection_index_{0};
};

} // namespace orbit::modules
