#include <doctest/doctest.h>

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;
using tokens::CornerSize;

namespace {

widgets::Button toggle(ButtonStyle style, bool checked = false, bool radio = false, ButtonSize size = ButtonSize::small) {
    widgets::ButtonOptions o{"Wi-Fi", style, size};
    o.toggle = widgets::ButtonToggle{.checked = checked, .radio = radio};
    widgets::Button b(std::move(o), fonts());
    b.set_origin({10.0, 10.0});
    return b;
}

void press_and_release(widgets::Button& b) {
    const render::Rect r = b.bounds();
    b.handle({PointerAction::down, r.x + r.width / 2.0, r.y + r.height / 2.0});
    b.handle({PointerAction::up, r.x + r.width / 2.0, r.y + r.height / 2.0});
    b.handle({PointerAction::leave, 0.0, 0.0});
}

}

static_assert(widgets::Focusable<widgets::Button>);
static_assert(widgets::Disableable<widgets::Button>);

TEST_CASE("toggle button colors follow ToggleButton.kt's defaults per style") {
    const auto& filled = *tokens::toggle_button_style(ButtonStyle::filled);
    CHECK(filled.container == color::Role::surface_container);
    CHECK(filled.content == color::Role::on_surface_variant);
    CHECK(filled.checked_container == color::Role::primary);
    CHECK(filled.checked_content == color::Role::on_primary);
    const auto& tonal = *tokens::toggle_button_style(ButtonStyle::tonal);
    CHECK(tonal.container == color::Role::secondary_container);
    CHECK(tonal.checked_container == color::Role::secondary);
    CHECK(tonal.checked_content == color::Role::on_secondary);
    const auto& outlined = *tokens::toggle_button_style(ButtonStyle::outlined);
    CHECK_FALSE(outlined.container);
    CHECK(outlined.checked_container == color::Role::inverse_surface);
    CHECK(outlined.checked_content == color::Role::inverse_on_surface);
    CHECK(outlined.disabled_container.role == color::Role::outline_variant);
    CHECK(outlined.disabled_container.opacity == doctest::Approx(0.1));
    const auto& elevated = *tokens::toggle_button_style(ButtonStyle::elevated);
    CHECK(elevated.container == color::Role::surface_container_low);
    CHECK(elevated.content == color::Role::primary);
    CHECK_FALSE(tokens::toggle_button_style(ButtonStyle::text));
    CHECK(tokens::toggle_button_motion.shape.stiffness == tokens::motion_scheme.fast_spatial().stiffness);
}

TEST_CASE("a text button cannot toggle") {
    widgets::ButtonOptions o{"Text", ButtonStyle::text};
    o.toggle = widgets::ButtonToggle{};
    CHECK_THROWS_AS(widgets::Button(std::move(o), fonts()), std::invalid_argument);
}

TEST_CASE("a click flips a toggle button and its accessible state") {
    auto b = toggle(ButtonStyle::filled);
    CHECK(b.toggleable());
    CHECK(b.accessible().role == widgets::AccessibleRole::toggle_button);
    CHECK(b.accessible().state.toggle == widgets::AccessibleToggle::off);
    press_and_release(b);
    CHECK(b.checked());
    CHECK(b.accessible().state.toggle == widgets::AccessibleToggle::on);
    press_and_release(b);
    CHECK_FALSE(b.checked());
}

TEST_CASE("Space and Enter toggle a focused toggle button, and a disabled one ignores them") {
    auto b = toggle(ButtonStyle::tonal);
    b.set_focused(true);
    CHECK(b.handle(input::KeyEvent{Key::space}));
    CHECK(b.checked());
    CHECK(b.handle(input::KeyEvent{Key::enter}));
    CHECK_FALSE(b.checked());
    b.set_enabled(false);
    CHECK_FALSE(b.handle(input::KeyEvent{Key::space}));
    press_and_release(b);
    CHECK_FALSE(b.checked());
}

TEST_CASE("a radio toggle button only checks itself and reports a selected radio button") {
    auto b = toggle(ButtonStyle::filled, true, true);
    CHECK(b.accessible().role == widgets::AccessibleRole::radio_button);
    CHECK(b.accessible().state.selected == true);
    press_and_release(b);
    CHECK(b.checked());
}

TEST_CASE("a toggle button is round, squares its corners when checked and morphs on the fast spatial spring") {
    auto b = toggle(ButtonStyle::filled);
    const double h = b.height();
    CHECK(b.corner_radii().top_left == doctest::Approx(h / 2.0));
    b.set_checked(true);
    double lowest = h;
    for (int i = 0; i < settle_frames; ++i) {
        b.step(frame_seconds);
        lowest = std::min(lowest, b.corner_radii().top_left);
    }
    const double square = tokens::corner_radius(tokens::button_size(ButtonSize::small).square_corner, b.width(), h);
    CHECK(b.corner_radii().top_left == doctest::Approx(square));
    CHECK(lowest < square - 0.5);
}

TEST_CASE("pressing a toggle button takes the pressed corners, which win over the checked ones") {
    auto b = toggle(ButtonStyle::filled, true);
    const render::Rect r = b.bounds();
    b.handle({PointerAction::down, r.x + 5.0, r.y + 5.0});
    settle(b);
    const double pressed = tokens::corner_radius(tokens::button_size(ButtonSize::small).pressed_corner, r.width, r.height);
    CHECK(b.corner_radii().top_left == doctest::Approx(pressed));
    b.cancel_interaction();
    settle(b);
    CHECK(b.corner_radii().top_left == doctest::Approx(tokens::corner_radius(CornerSize::medium, r.width, r.height)));
}

TEST_CASE("reversing a toggle half way runs the morph back from where it is") {
    auto b = toggle(ButtonStyle::filled);
    b.set_checked(true);
    for (int i = 0; i < 4; ++i) {
        b.step(frame_seconds);
    }
    const double mid = b.corner_radii().top_left;
    b.set_checked(false);
    b.step(frame_seconds);
    CHECK(std::abs(b.corner_radii().top_left - mid) < 2.0);
    settle(b);
    CHECK(b.corner_radii().top_left == doctest::Approx(b.height() / 2.0));
}

TEST_CASE("custom shapes give each corner its own size") {
    widgets::ButtonOptions o{"Lead", ButtonStyle::filled};
    o.toggle = widgets::ButtonToggle{};
    o.shapes = widgets::ButtonShapes{{CornerSize::full, CornerSize::small, CornerSize::small, CornerSize::full},
                                     {CornerSize::full, CornerSize::extra_small, CornerSize::extra_small, CornerSize::full},
                                     tokens::all_corners(CornerSize::full)};
    widgets::Button b(std::move(o), fonts());
    const render::CornerRadii r = b.corner_radii();
    CHECK(r.top_left == doctest::Approx(b.height() / 2.0));
    CHECK(r.top_right == doctest::Approx(8.0));
    CHECK(r.bottom_right == doctest::Approx(8.0));
    CHECK(r.bottom_left == doctest::Approx(b.height() / 2.0));
}

TEST_CASE("an outlined toggle button drops its outline and fills with inverse-surface when checked") {
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    auto b = toggle(ButtonStyle::outlined);
    b.set_size(120.0, 40.0);
    thorvg::Canvas canvas(140, 60);
    auto edge = [&] {
        canvas.clear(scheme[color::Role::surface]);
        b.paint(canvas, scheme);
        return canvas.pixel(70, 10);
    };
    auto centre = [&] { return canvas.pixel(30, 30); };
    CHECK(edge() == scheme[color::Role::outline_variant]);
    CHECK(centre() == scheme[color::Role::surface]);
    b.set_checked(true);
    settle(b);
    CHECK(edge() == scheme[color::Role::inverse_surface]);
    CHECK(centre() == scheme[color::Role::inverse_surface]);
}

TEST_CASE("the checked overlay paints the content color over a checked button") {
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    widgets::ButtonOptions o{"", ButtonStyle::filled, ButtonSize::small, ButtonShape::round, icons::Icon{icon_font(), res::icons::add}};
    o.accessible_name = "More";
    o.toggle = widgets::ButtonToggle{.checked = true, .checked_colors = false, .checked_overlay = 0.1};
    widgets::Button b(std::move(o), fonts());
    b.set_origin({0.0, 0.0});
    thorvg::Canvas canvas(80, 50);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme);
    const color::Argb expected = color::source_over(color::scale_alpha(scheme[color::Role::on_primary], 0.1), scheme[color::Role::primary]);
    const color::Argb got = canvas.pixel(4, 20);
    CHECK(std::abs(static_cast<int>(got & 0xff) - static_cast<int>(expected & 0xff)) <= 2);
}

TEST_CASE("an optically centered button shifts its content away from the rounder side") {
    widgets::ButtonOptions o{"", ButtonStyle::filled, ButtonSize::small, ButtonShape::round, icons::Icon{icon_font(), res::icons::add}};
    o.accessible_name = "More";
    o.shapes = widgets::ButtonShapes{{CornerSize::extra_small, CornerSize::full, CornerSize::full, CornerSize::extra_small},
                                     {CornerSize::extra_small, CornerSize::full, CornerSize::full, CornerSize::extra_small}};
    o.center_optically = true;
    widgets::Button b(std::move(o), fonts());
    b.set_size(48.0, 40.0);
    CHECK(b.content_offset() == doctest::Approx(0.11 * (4.0 - 20.0)));
}
