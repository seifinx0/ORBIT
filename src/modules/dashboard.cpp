#include "orbit/modules/dashboard.hpp"

#include <new>

namespace orbit::modules {
namespace {
using namespace graphics;
using namespace ui;

constexpr Color kNavyTop{5, 11, 25};
constexpr Color kNavyBottom{7, 18, 35};
constexpr Color kPanel{12, 23, 43};
constexpr Color kCyan{48, 207, 255};
constexpr Color kBlue{42, 103, 240};
constexpr Color kWhite{231, 241, 252};
constexpr Color kMuted{132, 158, 187};

enum class PageKind : std::uint8_t { Home, Graphics, Developer };

class DashboardPage final : public Screen {
public:
    DashboardPage(ScreenManager& manager, const Texture* mark, const char* id,
                  PageKind kind) noexcept
        : Screen(id), manager_(manager), kind_(kind), logo_(mark, {24, 16, 80, 80}),
          brand_("ORBIT", 30, kWhite, {122, 19, 260, 42}),
          tagline_("SYSTEM COMMAND CENTER", 16, kCyan, {124, 59, 350, 28}),
          nav_home_("HOME", 23, {1080, 22, 168, 64}),
          nav_graphics_("GRAPHICS", 23, {1264, 22, 168, 64}),
          nav_developer_("DEVELOPER", 23, {1448, 22, 196, 64}),
          eyebrow_(nullptr, 16, kCyan, {62, 42, 900, 28}),
          title_(nullptr, 92, kWhite, {58, 82, 1000, 118}),
          subtitle_(nullptr, 30, kCyan, {64, 216, 1040, 48}),
          detail_(nullptr, 23, kMuted, {64, 285, 1060, 100}),
          hero_mark_(mark, {1190, 72, 350, 350}),
          card_title_a_(nullptr, 23, kWhite, {26, 28, 470, 40}),
          card_body_a_(nullptr, 16, kMuted, {28, 78, 470, 64}),
          card_title_b_(nullptr, 23, kWhite, {26, 28, 470, 40}),
          card_body_b_(nullptr, 16, kMuted, {28, 78, 470, 64}),
          card_title_c_(nullptr, 23, kWhite, {26, 28, 470, 40}),
          card_body_c_(nullptr, 16, kMuted, {28, 78, 470, 64}),
          footer_("D-PAD / STICK MOVE     CROSS SELECT     CIRCLE BACK     L1/R1 SWITCH     OPTIONS HELP",
                  16, kMuted, {4, 4, 1600, 30}) {
        root_.set_bounds({0, 0, 1920, 1080});
        root_.set_background(kNavyTop, 0);

        header_.set_bounds({120, 56, 1680, 112});
        header_.set_background(kPanel, 238);
        header_.set_border({28, 52, 79}, 1);
        header_.set_radius(18);
        header_.add(logo_); header_.add(brand_); header_.add(tagline_);
        header_.add(nav_home_); header_.add(nav_graphics_); header_.add(nav_developer_);

        nav_home_.set_selected(kind == PageKind::Home);
        nav_graphics_.set_selected(kind == PageKind::Graphics);
        nav_developer_.set_selected(kind == PageKind::Developer);
        focus_group().add(nav_home_); focus_group().add(nav_graphics_); focus_group().add(nav_developer_);

        hero_.set_bounds({120, 208, 1680, 520});
        hero_.set_background({10, 22, 41}, 245);
        hero_.set_border({23, 48, 74}, 1);
        hero_.set_radius(26);
        hero_.add(eyebrow_); hero_.add(title_); hero_.add(subtitle_); hero_.add(detail_);
        hero_.add(hero_mark_);

        cards_[0].set_bounds({120, 752, 528, 190});
        cards_[1].set_bounds({672, 752, 528, 190});
        cards_[2].set_bounds({1224, 752, 576, 190});
        for (std::size_t i = 0; i < cards_.size(); ++i) {
            cards_[i].set_background(kPanel, 238);
            cards_[i].set_border({23, 48, 74}, 1);
            cards_[i].set_radius(20);
            cards_[i].set_padding({24, 18, 20, 14});
            cards_[i].set_accent(i == 1 ? kBlue : kCyan);
        }
        cards_[0].add(card_title_a_); cards_[0].add(card_body_a_);
        cards_[1].add(card_title_b_); cards_[1].add(card_body_b_);
        cards_[2].add(card_title_c_); cards_[2].add(card_body_c_);

        footer_panel_.set_bounds({120, 984, 1680, 42});
        footer_panel_.set_background({10, 20, 36}, 0);
        footer_panel_.add(footer_);
        root_.add(header_); root_.add(hero_);
        for (auto& card : cards_) root_.add(card);
        root_.add(footer_panel_);

        configure(kind);
    }

    void render(Canvas& canvas, FontRenderer& font) noexcept override {
        canvas.fill_vertical_gradient(0, 0, 1920, 1080, kNavyTop, kNavyBottom);
        canvas.fill_rect_alpha(120, 185, 1680, 1, {34, 74, 105}, 130);
        canvas.fill_rect_alpha(120, 966, 1680, 1, {34, 74, 105}, 105);
        root_.render(canvas, font);
    }

    bool handle(Action action) noexcept override {
        if (action == Action::Back) {
            if (kind_ == PageKind::Home) return false;
            return manager_.show("home");
        }
        if (action == Action::Confirm) {
            switch (focus_group().focused_index()) {
                case 0: return manager_.show("home");
                case 1: return manager_.show("graphics");
                case 2: return manager_.show("developer");
                default: return false;
            }
        }
        return Screen::handle(action);
    }

private:
    void configure(PageKind kind) noexcept {
        const char* eyebrow = "HOME / COMMAND CENTER";
        const char* title = "ORBIT";
        const char* subtitle = "Your console, clearly in view.";
        const char* detail = "A controller-first command center built on native PS4 video output,\nwith cached typography and transparent PNG composition.";
        const char* headings[3] = {"VIDEO OUTPUT", "TYPOGRAPHY", "IMAGE PIPELINE"};
        const char* bodies[3] = {"SceVideoOut / 1920 x 1080\nDouble buffered / V-sync",
                                 "FreeType / Quicksand\nCached glyphs / kerning",
                                 "PNG via stb_image\nBounded cache / alpha blend"};
        if (kind == PageKind::Graphics) {
            eyebrow = "GRAPHICS / VERIFIED PIPELINE";
            title = "Rendering";
            subtitle = "Native scanout, composed in memory.";
            detail = "SceVideoOut presents two 1920 x 1080 buffers at v-sync. The software\ncanvas clips panels, text, textures, and alpha composition to the screen.";
            headings[0] = "PRESENTATION"; bodies[0] = "SceVideoOut\nDouble-buffered / v-sync";
            headings[1] = "TEXTURES"; bodies[1] = "PNG decoded at load time\n16 MiB per image maximum";
            headings[2] = "RESOURCE BOUNDS"; bodies[2] = "16 cached images maximum\n32 MiB decoded pixels total";
        } else if (kind == PageKind::Developer) {
            eyebrow = "DEVELOPER / ORBIT 0.4.0";
            title = "Build & diagnostics";
            subtitle = "Details reported by this build.";
            detail = "This page reports the renderer and input implementation. Live system\nmemory, network state, and installed-app discovery are not queried here.";
            headings[0] = "PACKAGE IDENTITY"; bodies[0] = "Title ID: ORBT00001\nVersion: 0.4.0";
            headings[1] = "CONTROLLER"; bodies[1] = "OpenOrbis ScePad\nUserService initial user";
            headings[2] = "ENGINEERING"; bodies[2] = "Fixed-capacity UI tree\nNo frame-loop allocations";
        }
        eyebrow_.set_text(eyebrow); title_.set_text(title);
        subtitle_.set_text(subtitle); detail_.set_text(detail);
        card_title_a_.set_text(headings[0]); card_body_a_.set_text(bodies[0]);
        card_title_b_.set_text(headings[1]); card_body_b_.set_text(bodies[1]);
        card_title_c_.set_text(headings[2]); card_body_c_.set_text(bodies[2]);
    }

    ScreenManager& manager_;
    PageKind kind_;
    Panel root_{{0, 0, 1920, 1080}};
    Panel header_{};
    Image logo_;
    Label brand_, tagline_;
    Button nav_home_, nav_graphics_, nav_developer_;
    Panel hero_{};
    Label eyebrow_, title_, subtitle_, detail_;
    Image hero_mark_;
    std::array<Card, 3> cards_{};
    Label card_title_a_, card_body_a_, card_title_b_, card_body_b_, card_title_c_, card_body_c_;
    Panel footer_panel_{};
    Label footer_;
};

} // namespace

struct Dashboard::Pages {
    DashboardPage home;
    DashboardPage graphics;
    DashboardPage developer;
    Pages(ScreenManager& screens, const Texture* mark) noexcept
        : home(screens, mark, "home", PageKind::Home),
          graphics(screens, mark, "graphics", PageKind::Graphics),
          developer(screens, mark, "developer", PageKind::Developer) {
        home.focus_first(); graphics.focus_first(); developer.focus_first();
    }
};

Dashboard::~Dashboard() { delete pages_; }

bool Dashboard::initialize(const graphics::Texture* mark) noexcept {
    if (pages_) return false;
    pages_ = new (std::nothrow) Pages(screens_, mark);
    if (!pages_) return false;
    if (!screens_.register_screen(pages_->home) ||
        !screens_.register_screen(pages_->graphics) ||
        !screens_.register_screen(pages_->developer)) return false;

    controls_.set_background({12, 24, 43}, 255);
    controls_.set_border({48, 207, 255}, 2);
    controls_.set_radius(24);
    controls_.set_padding({36, 88, 36, 30});
    controls_.set_title("CONTROLLER GUIDE");
    controls_.add(control_move_); controls_.add(control_confirm_);
    controls_.add(control_tabs_); controls_.add(control_close_);
    control_focus_.add(control_close_);
    return true;
}

bool Dashboard::handle(std::uint16_t events) noexcept {
    if (controls_visible_) {
        if (events & input::Back || events & input::Context || events & input::Confirm) {
            controls_visible_ = false;
            return true;
        }
        if (events & input::Up) return control_focus_.move(ui::Direction::Up);
        if (events & input::Down) return control_focus_.move(ui::Direction::Down);
        return false;
    }
    if (events & input::Context) {
        controls_visible_ = true;
        return true;
    }
    if (events & input::Back) return screens_.handle(ui::Action::Back);
    if (events & input::Confirm) return screens_.handle(ui::Action::Confirm);
    if (events & input::TabPrevious) return screens_.handle(ui::Action::TabPrevious);
    if (events & input::TabNext) return screens_.handle(ui::Action::TabNext);
    if (events & input::Up) return screens_.handle(ui::Action::Up);
    if (events & input::Down) return screens_.handle(ui::Action::Down);
    if (events & input::Left) return screens_.handle(ui::Action::Left);
    if (events & input::Right) return screens_.handle(ui::Action::Right);
    return false;
}

bool Dashboard::tick(std::uint16_t elapsed_ms) noexcept {
    return screens_.update(elapsed_ms);
}

void Dashboard::render(graphics::Canvas& canvas, graphics::FontRenderer& font) noexcept {
    screens_.render(canvas, font);
    if (controls_visible_) controls_.render(canvas, font);
}

} // namespace orbit::modules
