#include "orbit/modules/dashboard.hpp"

#include "orbit/graphics/canvas.hpp"
#include "orbit/graphics/font_renderer.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <new>
#include <sys/stat.h>

namespace orbit::modules {
namespace {
using graphics::Canvas;
using graphics::Color;
using graphics::FontRenderer;
using graphics::Texture;
constexpr Color kInk{4, 8, 18};
constexpr Color kNavy{6, 15, 31};
constexpr Color kPanel{10, 23, 43};
constexpr Color kWhite{242, 247, 255};
constexpr Color kMuted{157, 178, 202};
constexpr Color kCyan{57, 211, 255};
constexpr Color kBlue{63, 116, 255};
constexpr Color kAccent[4]{{57, 211, 255}, {115, 132, 255}, {91, 229, 178}, {255, 168, 93}};
constexpr const char* kTabs[] = {"LIBRARY", "FAVORITES", "COLLECTIONS", "SEARCH", "SETTINGS"};
constexpr const char* kContext[] = {"TOGGLE FAVORITE", "ADD / REMOVE COLLECTION", "SWITCH LAYOUT", "CHANGE SORT", "NEW COLLECTION", "REFRESH LIBRARY", "SETTINGS"};
constexpr const char* kKeyboard = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_<>";

bool contains_casefold(const char* text, const char* query) noexcept {
    if (!text || !query || !*query) return true;
    for (const char* start = text; *start; ++start) {
        const char* a = start; const char* b = query;
        while (*a && *b && std::tolower(static_cast<unsigned char>(*a)) == std::tolower(static_cast<unsigned char>(*b))) { ++a; ++b; }
        if (!*b) return true;
    }
    return false;
}
void text(Canvas& c, FontRenderer& f, int x, int y, const char* value, int size, Color color) noexcept {
    if (!value) return;
    c.push_clip(x, y, 1800, 140);
    f.draw_text(c, x, y, value, size, color);
    c.pop_clip();
}
void label(Canvas& c, FontRenderer& f, int x, int y, const char* value, Color color = kMuted) noexcept {
    text(c, f, x, y, value, 16, color);
}
void panel(Canvas& c, int x, int y, int w, int h, Color fill, std::uint8_t alpha = 235,
           Color border = {35, 59, 86}, int radius = 18) noexcept {
    c.fill_rounded_rect(x, y, w, h, radius, fill, alpha);
    c.stroke_rect(x, y, w, h, border, 1);
}
const Texture* load_texture(graphics::TextureCache* cache, const char* path) noexcept {
    return cache && path && *path ? cache->get_png(path) : nullptr;
}
}

Dashboard::~Dashboard() { delete catalog_; }

bool Dashboard::initialize(const graphics::Texture* mark, graphics::TextureCache* textures) noexcept {
    if (catalog_) return false;
    catalog_ = new (std::nothrow) library::Catalog();
    if (!catalog_) return false;
    mark_ = mark; textures_ = textures;
    (void)mkdir("/data/orbit", 0777);
    (void)preferences_.load("/data/orbit/preferences.cfg");
    const char* roots[] = {"/user/app", "/mnt/ext0/user/app"};
    const auto scan = catalog_->scan_roots(roots, sizeof(roots) / sizeof(roots[0]));
    rebuild_results();
    if (scan.status == library::ScanResult::Status::Unavailable) {
        show_notice("Installed game storage is not accessible to this application.");
    } else if (scan.truncated) {
        show_notice("Library is at its 512 title safety limit. Review installed titles.");
    } else if (result_count_ == 0) {
        show_notice("No installed full-game applications were found.");
    }
    return true;
}

void Dashboard::show_notice(const char* notice) noexcept {
    std::snprintf(notice_.data(), notice_.size(), "%s", notice ? notice : "");
    notice_ms_ = 3200;
}

bool Dashboard::save_preferences() noexcept {
    const bool saved = preferences_.save("/data/orbit/preferences.cfg");
    if (!saved) show_notice("Could not save library preferences.");
    return saved;
}

const library::Game* Dashboard::selected_game() const noexcept {
    if (selected_ >= result_count_) return nullptr;
    return catalog_ ? catalog_->at(results_[selected_]) : nullptr;
}

void Dashboard::rebuild_results() noexcept {
    const char* query = (search_active_ || view_ == View::Search) ? query_.data() : nullptr;
    result_count_ = 0;
    if (!catalog_) return;
    for (std::size_t i = 0; i < catalog_->size(); ++i) {
        const auto* game = catalog_->at(i);
        if (!contains_casefold(game->title.data(), query) && !contains_casefold(game->title_id.data(), query)) continue;
        if (category_ == Category::Favorites && !preferences_.favorite(game->title_id.data())) continue;
        if (category_ == Category::Collections && !preferences_.in_collection(collection_index_, game->title_id.data())) continue;
        results_[result_count_++] = static_cast<std::uint16_t>(i);
    }
    if (preferences_.sort == library::SortOrder::TitleId) {
        std::sort(results_.begin(), results_.begin() + result_count_, [this](std::uint16_t a, std::uint16_t b) {
            const auto* ga = catalog_->at(a); const auto* gb = catalog_->at(b);
            if (!ga || !gb) return a < b;
            return std::strcmp(ga->title_id.data(), gb->title_id.data()) < 0;
        });
    }
    selected_ = result_count_ ? std::min(selected_, result_count_ - 1) : 0;
}

void Dashboard::change_selection(int delta) noexcept {
    if (!result_count_ || delta == 0) return;
    const auto previous = selected_;
    const auto n = static_cast<long long>(result_count_);
    selected_ = static_cast<std::size_t>((static_cast<long long>(selected_) + delta % n + n) % n);
    if (selected_ != previous && preferences_.animation_level) {
        animation_duration_ = preferences_.animation_level == 1 ? 150 : 240;
        animation_ms_ = animation_duration_;
        carousel_animated_ = true;
    }
}

void Dashboard::confirm() noexcept {
    if (view_ == View::Search) {
        if (keyboard_index_ >= std::strlen(kKeyboard)) {
            view_ = View::Library; rebuild_results(); return;
        }
        const char ch = kKeyboard[keyboard_index_];
        if (ch == '<') {
            const auto n = std::strlen(query_.data()); if (n) query_[n - 1] = '\0';
        } else if (ch == '>') {
            view_ = View::Library; search_active_ = true;
        } else {
            const auto n = std::strlen(query_.data());
            if (n + 1 < query_.size()) { query_[n] = ch; query_[n + 1] = '\0'; }
        }
        rebuild_results(); return;
    }
    if (view_ == View::Collections) {
        const auto* collection = preferences_.collection(collection_index_);
        if (collection) { category_ = Category::Collections; view_ = View::Library; selected_ = 0; rebuild_results(); }
        else show_notice("Create a collection from a game's action menu.");
        return;
    }
    if (view_ == View::Settings) {
        if (focused_tab_ == 0) preferences_.layout = preferences_.layout == library::Layout::Carousel ? library::Layout::Grid : library::Layout::Carousel;
        if (focused_tab_ == 1) preferences_.animation_level = (preferences_.animation_level + 1) % 3;
        if (focused_tab_ == 2) preferences_.accent = (preferences_.accent + 1) % 4;
        if (focused_tab_ == 3) preferences_.sort = preferences_.sort == library::SortOrder::Title ? library::SortOrder::TitleId : library::SortOrder::Title;
        rebuild_results();
        (void)save_preferences(); return;
    }
    if (view_ == View::Details) {
        const auto* game = selected_game();
        if (!game) { show_notice("Select an installed title first."); return; }
        const int result = launcher_.launch(game->title_id.data());
        if (result >= 0) show_notice("Launch request accepted by the PS4 title service.");
        else {
            char message[96]{}; std::snprintf(message, sizeof(message), "PS4 rejected launch request (%d).", result); show_notice(message);
        }
        return;
    }
    if (view_ == View::Library) {
        if (tabs_focused_) {
            tabs_focused_ = false;
            if (focused_tab_ == 0) { category_ = Category::All; selected_ = 0; rebuild_results(); return; }
            if (focused_tab_ == 1) { category_ = Category::Favorites; selected_ = 0; rebuild_results(); return; }
            if (focused_tab_ == 2) { view_ = View::Collections; collection_index_ = 0; return; }
            if (focused_tab_ == 3) { view_ = View::Search; search_active_ = false; keyboard_index_ = 0; rebuild_results(); return; }
            if (focused_tab_ == 4) { view_ = View::Settings; focused_tab_ = 0; return; }
        } else if (selected_game()) { view_ = View::Details; return; }
    }
}

void Dashboard::open_context() noexcept {
    if (view_ == View::Details || view_ == View::Library) { context_open_ = !context_open_; context_index_ = 0; }
    else if (view_ == View::Collections) show_notice("Collections are managed from a game's action menu.");
}

void Dashboard::context_confirm() noexcept {
    const auto* game = selected_game();
    switch (context_index_) {
        case 0:
            if (!game || !preferences_.toggle_favorite(game->title_id.data())) show_notice("Could not update favorites.");
            else { rebuild_results(); if (save_preferences()) show_notice(preferences_.favorite(game->title_id.data()) ? "Added to favorites." : "Removed from favorites."); }
            break;
        case 1:
            if (!game || preferences_.collection_count() == 0) { show_notice("Create a collection first from this action menu."); break; }
            collection_index_ %= preferences_.collection_count();
            if (preferences_.in_collection(collection_index_, game->title_id.data())) {
                (void)preferences_.remove_from_collection(collection_index_, game->title_id.data()); show_notice("Removed from collection.");
            } else if (preferences_.add_to_collection(collection_index_, game->title_id.data())) show_notice("Added to collection.");
            else show_notice("Collection is full.");
            rebuild_results(); (void)save_preferences(); break;
        case 2:
            preferences_.layout = preferences_.layout == library::Layout::Carousel ? library::Layout::Grid : library::Layout::Carousel;
            if (save_preferences()) show_notice(preferences_.layout == library::Layout::Grid ? "Grid layout enabled." : "Carousel layout enabled.");
            break;
        case 3:
            preferences_.sort = preferences_.sort == library::SortOrder::Title ? library::SortOrder::TitleId : library::SortOrder::Title;
            rebuild_results(); if (save_preferences()) show_notice("Library sort updated."); break;
        case 4:
            collection_name_.fill(0); collection_keyboard_ = true; view_ = View::Search; keyboard_index_ = 0; break;
        case 5: refresh_library(); break;
        case 6: view_ = View::Settings; focused_tab_ = 0; break;
        default: break;
    }
    context_open_ = false;
}

void Dashboard::refresh_library() noexcept {
    char previous_id[13]{};
    if (const auto* game = selected_game()) std::snprintf(previous_id, sizeof(previous_id), "%s", game->title_id.data());
    const char* roots[] = {"/user/app", "/mnt/ext0/user/app"};
    const auto scan = catalog_->scan_roots(roots, sizeof(roots) / sizeof(roots[0]));
    selected_ = 0;
    rebuild_results();
    for (std::size_t i = 0; previous_id[0] && i < result_count_; ++i) {
        const auto* game = catalog_->at(results_[i]);
        if (game && std::strcmp(game->title_id.data(), previous_id) == 0) { selected_ = i; break; }
    }
    if (scan.status == library::ScanResult::Status::Unavailable) show_notice("Installed game storage is unavailable.");
    else {
        char message[96]{}; std::snprintf(message, sizeof(message), "Library refreshed: %zu games found.", scan.discovered); show_notice(message);
    }
}

bool Dashboard::handle(std::uint16_t events) noexcept {
    using namespace input;
    if (context_open_) {
        if (events & (Back | Context)) { context_open_ = false; return true; }
        if ((events & (TabPrevious | TabNext)) && preferences_.collection_count()) {
            const auto count = preferences_.collection_count();
            collection_index_ = events & TabNext ? (collection_index_ + 1) % count : (collection_index_ + count - 1) % count;
            return true;
        }
        if (events & Up) context_index_ = (context_index_ + 6) % 7;
        else if (events & Down) context_index_ = (context_index_ + 1) % 7;
        else if (events & Confirm) context_confirm();
        else return false;
        return true;
    }
    if (view_ == View::Search) {
        if (events & Back) {
            if (collection_keyboard_) { collection_keyboard_ = false; view_ = View::Details; }
            else { view_ = View::Library; search_active_ = true; }
            rebuild_results(); return true;
        }
        if (events & Context) { query_.fill(0); search_active_ = false; rebuild_results(); return true; }
        if (events & Left) keyboard_index_ = keyboard_index_ ? keyboard_index_ - 1 : std::strlen(kKeyboard) - 1;
        else if (events & Right) keyboard_index_ = (keyboard_index_ + 1) % std::strlen(kKeyboard);
        else if (events & Up) keyboard_index_ = (keyboard_index_ + std::strlen(kKeyboard) - 8) % std::strlen(kKeyboard);
        else if (events & Down) keyboard_index_ = (keyboard_index_ + 8) % std::strlen(kKeyboard);
        else if (events & Confirm) {
            if (collection_keyboard_) {
                const char ch = kKeyboard[keyboard_index_];
                if (ch == '>') {
                    if (preferences_.add_collection(collection_name_.data())) { collection_keyboard_ = false; view_ = View::Details; collection_index_ = preferences_.collection_count() - 1; const auto* g = selected_game(); if (g) (void)preferences_.add_to_collection(collection_index_, g->title_id.data()); if (save_preferences()) show_notice("Collection created and game added."); }
                    else show_notice("Collection name is invalid or already used.");
                } else if (ch == '<') { const auto n = std::strlen(collection_name_.data()); if (n) collection_name_[n - 1] = '\0'; }
                else { const auto n = std::strlen(collection_name_.data()); if (n + 1 < collection_name_.size()) { collection_name_[n] = ch; collection_name_[n + 1] = '\0'; } }
            } else confirm();
        } else return false;
        return true;
    }
    if (events & Context) { open_context(); return true; }
    if (view_ == View::Details) {
        if (events & Back) { view_ = View::Library; return true; }
        if (events & Confirm) { confirm(); return true; }
        if (events & (Left | Right)) { change_selection((events & Right) ? 1 : -1); return true; }
        return false;
    }
    if (view_ == View::Collections) {
        if (events & Back) { view_ = View::Library; category_ = Category::All; focused_tab_ = 0; rebuild_results(); return true; }
        if (events & Up && collection_index_) --collection_index_;
        else if (events & Down && collection_index_ + 1 < preferences_.collection_count()) ++collection_index_;
        else if (events & Confirm) { confirm(); return true; }
        else return false;
        return true;
    }
    if (view_ == View::Settings) {
        if (events & Back) { view_ = View::Library; return true; }
        if (events & Up) focused_tab_ = (focused_tab_ + 3) % 4;
        else if (events & Down) focused_tab_ = (focused_tab_ + 1) % 4;
        else if (events & (Left | Right | Confirm)) confirm();
        else return false;
        return true;
    }
    if (events & Back) {
        if (category_ != Category::All) { category_ = Category::All; focused_tab_ = 0; selected_ = 0; rebuild_results(); return true; }
        return false;
    }
    if (events & TabPrevious) { focused_tab_ = focused_tab_ ? focused_tab_ - 1 : 4; tabs_focused_ = true; }
    else if (events & TabNext) { focused_tab_ = (focused_tab_ + 1) % 5; tabs_focused_ = true; }
    else if (events & Up) {
        if (!tabs_focused_ && preferences_.layout == library::Layout::Grid && selected_ >= 8) change_selection(-8);
        else tabs_focused_ = true;
    }
    else if (events & Down) {
        if (tabs_focused_) tabs_focused_ = false;
        else if (preferences_.layout == library::Layout::Grid && selected_ + 8 < result_count_) change_selection(8);
        else return false;
    }
    else if (events & Left) {
        if (tabs_focused_) focused_tab_ = (focused_tab_ + 4) % 5;
        else if (preferences_.layout == library::Layout::Grid) { if (selected_ % 8) change_selection(-1); }
        else if (result_count_) change_selection(-1);
        else focused_tab_ = (focused_tab_ + 4) % 5;
    } else if (events & Right) {
        if (tabs_focused_) focused_tab_ = (focused_tab_ + 1) % 5;
        else if (preferences_.layout == library::Layout::Grid) { if (selected_ + 1 < result_count_ && selected_ % 8 != 7) change_selection(1); }
        else if (result_count_) change_selection(1);
        else focused_tab_ = (focused_tab_ + 1) % 5;
    } else if (events & Confirm) { confirm(); }
    else return false;

    if (tabs_focused_ && focused_tab_ == 0 && category_ != Category::All) { category_ = Category::All; selected_ = 0; rebuild_results(); }
    else if (tabs_focused_ && focused_tab_ == 1 && category_ != Category::Favorites) { category_ = Category::Favorites; selected_ = 0; rebuild_results(); }
    return true;
}

bool Dashboard::tick(std::uint16_t elapsed_ms) noexcept {
    bool changed = false;
    if (animation_ms_) { animation_ms_ = elapsed_ms >= animation_ms_ ? 0 : static_cast<std::uint16_t>(animation_ms_ - elapsed_ms); changed = true; }
    if (transition_ms_) { transition_ms_ = elapsed_ms >= transition_ms_ ? 0 : static_cast<std::uint16_t>(transition_ms_ - elapsed_ms); changed = true; }
    if (notice_ms_) {
        notice_ms_ = elapsed_ms >= notice_ms_ ? 0 : static_cast<std::uint16_t>(notice_ms_ - elapsed_ms);
        if (!notice_ms_) changed = true;
    }
    return changed;
}

void Dashboard::draw_background(Canvas& canvas, FontRenderer& font) noexcept {
    const auto* game = selected_game();
    const Texture* background = nullptr;
    if (game && game->has_background) background = load_texture(textures_, game->background_path.data());
    if (!background && game && game->has_icon) background = load_texture(textures_, game->icon_path.data());
    if (background && background->valid()) background->draw(canvas, 0, 0, 1920, 1080, true);
    else canvas.fill_vertical_gradient(0, 0, 1920, 1080, {5, 12, 27}, {7, 23, 47});
    canvas.fill_rect_alpha(0, 0, 1920, 1080, {2, 7, 16}, background ? 155 : 255);
    if (background) canvas.fill_rect_alpha(0, 0, 1920, 650, {3, 8, 18}, 70);
    canvas.fill_rect_alpha(0, 850, 1920, 230, kInk, 155);
    // Restrained cyan light ribbon gives the interface a visual signature.
    canvas.fill_rect_alpha(1240, 0, 680, 2, kAccent[preferences_.accent], 180);
    (void)font;
}

void Dashboard::draw_library(Canvas& canvas, FontRenderer& font) noexcept {
    draw_background(canvas, font);
    const Color accent = kAccent[preferences_.accent];
    const auto* mark = textures_ ? load_texture(textures_, "/app0/assets/images/orbit-mark.png") : mark_;
    if (mark && mark->valid()) mark->draw(canvas, 70, 48, 48, 48, true);
    text(canvas, font, 132, 45, "ORBIT", 30, kWhite);
    label(canvas, font, 136, 82, "GAME LIBRARY", accent);
    for (std::size_t i = 0; i < 5; ++i) {
        const int x = 758 + static_cast<int>(i) * 210;
        const bool active = i == focused_tab_;
        const bool focused = active && tabs_focused_;
        text(canvas, font, x, 66, kTabs[i], 16, active ? accent : kMuted);
        if (active) canvas.fill_rounded_rect(x, 94, focused ? 112 : 82, focused ? 4 : 3, 2, accent, 235);
    }
    if (search_active_ && query_[0]) {
        text(canvas, font, 136, 116, "FILTER", 16, accent);
        text(canvas, font, 205, 116, query_.data(), 16, kWhite);
    }
    canvas.fill_rect_alpha(70, 125, 1780, 1, {77, 112, 151}, 95);

    const auto* game = selected_game();
    if (!game) {
        text(canvas, font, 138, 342, !catalog_ || catalog_->size() == 0 ? "Your library is ready." : "No games match this view.", 50, kWhite);
        const char* hint = !catalog_ || catalog_->size() == 0 ? "ORBIT scans installed full-game applications and reads their own title metadata." : "Adjust search or choose another library category.";
        text(canvas, font, 142, 420, hint, 23, kMuted);
        text(canvas, font, 142, 492, !catalog_ || catalog_->size() == 0 ? "Press OPTIONS for library actions." : "OPTIONS  /  CLEAR SEARCH     L1 R1  /  CATEGORIES", 16, accent);
    } else {
        const Texture* cover = game->has_icon ? load_texture(textures_, game->icon_path.data()) : nullptr;
        // Large cover artwork is the hero. A rim and soft backing make arbitrary source dimensions feel intentional.
        canvas.fill_rounded_rect(1405, 170, 360, 470, 24, {18, 42, 69}, 185);
        canvas.stroke_rect(1405, 170, 360, 470, accent, 2);
        if (cover && cover->valid()) cover->draw(canvas, 1420, 185, 330, 440, true);
        else {
            canvas.fill_rounded_rect(1420, 185, 330, 440, 17, {12, 28, 48}, 255);
            if (mark && mark->valid()) mark->draw(canvas, 1522, 310, 126, 126, true);
            label(canvas, font, 1470, 506, "ARTWORK UNAVAILABLE", kMuted);
        }
        label(canvas, font, 145, 205, "NOW BROWSING", accent);
        text(canvas, font, 140, 250, game->title.data(), 50, kWhite);
        text(canvas, font, 145, 326, game->title_id.data(), 23, kMuted);
        if (game->version[0]) {
            char line[64]{}; std::snprintf(line, sizeof(line), "INSTALLED VERSION  %s", game->version.data());
            label(canvas, font, 145, 370, line, kMuted);
        }
        if (preferences_.favorite(game->title_id.data())) text(canvas, font, 145, 420, "FAVORITE", 23, accent);
        else text(canvas, font, 145, 420, "INSTALLED PS4 GAME", 23, accent);
        char count[64]{}; std::snprintf(count, sizeof(count), "%zu  /  %zu TITLES", selected_ + 1, result_count_);
        label(canvas, font, 145, 475, count, kMuted);
        if (preferences_.layout == library::Layout::Carousel) {
            constexpr int shown = 7, stride = 190;
            const int start = static_cast<int>(selected_) - shown / 2;
            for (int i = 0; i < shown; ++i) {
                const long index = static_cast<long>(start + i);
                if (index < 0 || index >= static_cast<long>(result_count_)) continue;
                const auto* item = catalog_->at(results_[static_cast<std::size_t>(index)]);
                const bool selected = static_cast<std::size_t>(index) == selected_;
                const int center = 241 + i * stride;
                const float raw = animation_ms_ && selected ? static_cast<float>(animation_duration_ - animation_ms_) / animation_duration_ : 1.0f;
                const float eased = raw * raw * (3.0f - 2.0f * raw);
                const int w = selected ? static_cast<int>(132.0f + 26.0f * eased) : 132;
                const int h = selected ? static_cast<int>(158.0f + 32.0f * eased) : 158;
                const int x = center - w / 2;
                const int y = selected ? 700 - (h - 158) / 2 : 700;
                const auto* tex = item->has_icon ? load_texture(textures_, item->icon_path.data()) : nullptr;
                canvas.fill_rounded_rect(x - 5, y - 5, w + 10, h + 10, 13, selected ? accent : Color{37, 56, 78}, selected ? 210 : 150);
                if (tex && tex->valid()) tex->draw(canvas, x, y, w, h, true);
                else { canvas.fill_rounded_rect(x, y, w, h, 9, {18, 33, 51}, 255); text(canvas, font, x + 10, y + 70, "NO ART", 16, kMuted); }
            }
        } else {
            constexpr int cols = 8, cell_w = 185, cell_h = 120, gap = 22;
            const int page_start = (static_cast<int>(selected_) / (cols * 2)) * cols * 2;
            for (int i = 0; i < cols * 2 && page_start + i < static_cast<int>(result_count_); ++i) {
                const int idx = page_start + i, x = 145 + (i % cols) * (cell_w + gap), y = 680 + (i / cols) * 145;
                const auto* item = catalog_->at(results_[static_cast<std::size_t>(idx)]);
                const bool selected = static_cast<std::size_t>(idx) == selected_;
                const auto* tex = item->has_icon ? load_texture(textures_, item->icon_path.data()) : nullptr;
                canvas.fill_rounded_rect(x - 4, y - 4, cell_w + 8, cell_h + 8, 13, selected ? accent : Color{30, 47, 67}, selected ? 230 : 155);
                if (tex && tex->valid()) tex->draw(canvas, x, y, cell_w, cell_h, true);
                else canvas.fill_rounded_rect(x, y, cell_w, cell_h, 9, {18, 33, 51}, 255);
                if (selected) selected_ = static_cast<std::size_t>(idx);
            }
        }
    }

    panel(canvas, 70, 986, 1780, 48, {4, 12, 25}, 224, {28, 51, 76}, 14);
    label(canvas, font, 96, 1000, "CROSS  DETAILS     CIRCLE  BACK     L1 / R1  CATEGORIES     OPTIONS  ACTIONS", kMuted);
    label(canvas, font, 1490, 1000, preferences_.layout == library::Layout::Grid ? "GRID" : "CAROUSEL", accent);
    if (view_ == View::Library && focused_tab_ == 4) draw_modal(canvas, font);
}

void Dashboard::draw_details(Canvas& canvas, FontRenderer& font) noexcept {
    draw_background(canvas, font);
    const auto* game = selected_game();
    if (!game) return;
    const Color accent = kAccent[preferences_.accent];
    panel(canvas, 92, 120, 1736, 820, {5, 15, 31}, 218, {39, 64, 94}, 28);
    const auto* cover = game->has_icon ? load_texture(textures_, game->icon_path.data()) : nullptr;
    canvas.fill_rounded_rect(148, 183, 420, 560, 25, {19, 41, 67}, 255);
    if (cover && cover->valid()) cover->draw(canvas, 160, 195, 396, 536, true);
    else {
        const auto* mark = textures_ ? load_texture(textures_, "/app0/assets/images/orbit-mark.png") : mark_;
        if (mark && mark->valid()) mark->draw(canvas, 270, 350, 170, 170, true);
    }
    label(canvas, font, 640, 198, "GAME DETAILS", accent);
    text(canvas, font, 636, 250, game->title.data(), 50, kWhite);
    text(canvas, font, 642, 326, game->title_id.data(), 23, kMuted);
    if (game->version[0]) {
        char version[64]{}; std::snprintf(version, sizeof(version), "INSTALLED VERSION  %s", game->version.data());
        text(canvas, font, 642, 372, version, 23, kMuted);
    }
    if (game->content_id[0]) {
        text(canvas, font, 642, 418, "CONTENT ID", 16, accent);
        text(canvas, font, 642, 446, game->content_id.data(), 16, kMuted);
    }
    text(canvas, font, 642, 522, preferences_.favorite(game->title_id.data()) ? "FAVORITE  /  YES" : "FAVORITE  /  NO", 23, accent);
    text(canvas, font, 642, 584, "CROSS  LAUNCH GAME", 23, kWhite);
    text(canvas, font, 642, 632, "OPTIONS  ORGANIZE THIS GAME", 16, kMuted);
    panel(canvas, 142, 788, 1610, 96, {8, 27, 47}, 236, {36, 74, 108}, 18);
    text(canvas, font, 176, 815, "CROSS   LAUNCH", 23, accent);
    text(canvas, font, 500, 819, "CIRCLE   RETURN TO LIBRARY", 16, kMuted);
    text(canvas, font, 1240, 819, "OPTIONS   FAVORITES / COLLECTIONS", 16, kMuted);
    if (context_open_) draw_modal(canvas, font);
}

void Dashboard::draw_modal(Canvas& canvas, FontRenderer& font) noexcept {
    canvas.fill_rect_alpha(0, 0, 1920, 1080, {1, 4, 11}, 150);
    panel(canvas, 1260, 200, 560, 540, {8, 20, 37}, 250, kAccent[preferences_.accent], 22);
    text(canvas, font, 1360, 254, "LIBRARY ACTIONS", 23, kWhite);
    for (std::size_t i = 0; i < 5; ++i) {
        const int y = 294 + static_cast<int>(i) * 56;
        if (i == context_index_) canvas.fill_rounded_rect(1282, y - 8, 516, 46, 10, {22, 57, 82}, 220);
        char collection_action[96]{};
        const char* row = kContext[i];
        if (i == 1 && preferences_.collection_count()) {
            const auto* collection = preferences_.collection(collection_index_ % preferences_.collection_count());
            std::snprintf(collection_action, sizeof(collection_action), "%s: %s", kContext[i], collection->name.data());
            row = collection_action;
        }
        text(canvas, font, 1300, y, row, 16, i == context_index_ ? kAccent[preferences_.accent] : kMuted);
    }
    label(canvas, font, 1300, 688, preferences_.collection_count() ? "UP / DOWN SELECT    L1 / R1 COLLECTION    CROSS APPLY" : "UP / DOWN SELECT    CROSS APPLY    CIRCLE CLOSE");
}

void Dashboard::render(graphics::Canvas& canvas, graphics::FontRenderer& font) noexcept {
    if (rendered_view_ != view_) {
        rendered_view_ = view_;
        transition_duration_ = preferences_.animation_level == 0 ? 0 : (preferences_.animation_level == 1 ? 100 : 160);
        transition_ms_ = transition_duration_;
    }
    if (view_ == View::Details) draw_details(canvas, font);
    else draw_library(canvas, font);
    if (view_ == View::Search) {
        canvas.fill_rect_alpha(0, 0, 1920, 1080, {1, 4, 11}, 185);
        panel(canvas, 195, 160, 1530, 760, {7, 18, 34}, 252, kAccent[preferences_.accent], 26);
        text(canvas, font, 250, 212, collection_keyboard_ ? "NEW COLLECTION" : "SEARCH YOUR LIBRARY", 30, kWhite);
        const char* value = collection_keyboard_ ? collection_name_.data() : query_.data();
        text(canvas, font, 258, 284, *value ? value : "TYPE A TITLE", 30, kAccent[preferences_.accent]);
        for (std::size_t i = 0; i < std::strlen(kKeyboard); ++i) {
            const int x = 270 + static_cast<int>(i % 8) * 172, y = 380 + static_cast<int>(i / 8) * 82;
            const bool focused = i == keyboard_index_;
            if (focused) canvas.fill_rounded_rect(x - 12, y - 10, 132, 62, 12, {23, 63, 92}, 245);
            const char ch[2]{kKeyboard[i], '\0'};
            const char* key_label = kKeyboard[i] == '<' ? "DELETE" : (kKeyboard[i] == '>' ? "DONE" : (kKeyboard[i] == ' ' ? "SPACE" : ch));
            text(canvas, font, x, y, key_label, 23, focused ? kAccent[preferences_.accent] : kWhite);
        }
        label(canvas, font, 270, 845, "D-PAD  CHOOSE     CROSS  ENTER CHARACTER     CIRCLE  FINISH     OPTIONS  CLEAR", kMuted);
    } else if (view_ == View::Collections) {
        canvas.fill_rect_alpha(0, 0, 1920, 1080, {1, 4, 11}, 176);
        panel(canvas, 280, 178, 1360, 710, {7, 18, 34}, 250, kAccent[preferences_.accent], 26);
        text(canvas, font, 340, 230, "YOUR COLLECTIONS", 30, kWhite);
        if (!preferences_.collection_count()) text(canvas, font, 352, 330, "No collections yet. Add a game from its action menu.", 23, kMuted);
        for (std::size_t i = 0; i < preferences_.collection_count(); ++i) {
            const int y = 316 + static_cast<int>(i) * 66;
            const auto* collection = preferences_.collection(i);
            if (i == collection_index_) canvas.fill_rounded_rect(330, y - 10, 1240, 52, 12, {22, 57, 82}, 220);
            text(canvas, font, 360, y, collection->name.data(), 23, i == collection_index_ ? kAccent[preferences_.accent] : kWhite);
            char count[48]{}; std::snprintf(count, sizeof(count), "%zu GAMES", collection->count); label(canvas, font, 1360, y + 5, count);
        }
        label(canvas, font, 340, 825, "UP / DOWN CHOOSE    CROSS OPEN    CIRCLE RETURN", kMuted);
    } else if (view_ == View::Settings) {
        canvas.fill_rect_alpha(0, 0, 1920, 1080, {1, 4, 11}, 176);
        panel(canvas, 280, 178, 1360, 710, {7, 18, 34}, 250, kAccent[preferences_.accent], 26);
        text(canvas, font, 340, 230, "LIBRARY CUSTOMIZATION", 30, kWhite);
        const char* layout = preferences_.layout == library::Layout::Carousel ? "CINEMATIC CAROUSEL" : "ARTWORK GRID";
        const char* animation = preferences_.animation_level == 0 ? "REDUCED" : (preferences_.animation_level == 1 ? "SUBTLE" : "FULL");
        const char* accent_names[] = {"ELECTRIC CYAN", "INDIGO", "MINT", "AMBER"};
        const char* lines[] = {layout, animation, accent_names[preferences_.accent], preferences_.sort == library::SortOrder::Title ? "TITLE" : "TITLE ID"};
        const char* names[] = {"BROWSING LAYOUT", "ANIMATION INTENSITY", "ACCENT COLOR", "SORT ORDER"};
        for (std::size_t i = 0; i < 4; ++i) {
            const int y = 330 + static_cast<int>(i) * 100;
            if (i == focused_tab_) canvas.fill_rounded_rect(330, y - 12, 1240, 72, 14, {21, 54, 80}, 215);
            text(canvas, font, 365, y, names[i], 16, kMuted);
            text(canvas, font, 900, y - 2, lines[i], 23, kAccent[preferences_.accent]);
        }
        label(canvas, font, 340, 825, "D-PAD OR CROSS  CHANGE SETTING    CIRCLE  RETURN", kMuted);
    }
    if (notice_ms_ && notice_[0]) {
        panel(canvas, 430, 914, 1060, 58, {8, 24, 42}, 238, {45, 88, 123}, 16);
        text(canvas, font, 464, 930, notice_.data(), 16, kWhite);
    }
    if (transition_ms_) {
        const auto opacity = static_cast<std::uint8_t>(transition_ms_ * 150u / transition_duration_);
        canvas.fill_rect_alpha(0, 0, 1920, 1080, {1, 4, 11}, opacity);
    }
}

} // namespace orbit::modules
