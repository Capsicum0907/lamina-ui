#include <doctest/doctest.h>

#include "lamina/render/thorvg_canvas.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/switch.hpp"

using namespace lamina;
using platform::PointerAction;
using tokens::CheckboxState;

namespace {

const icons::IconFont& icon_font() {
    static const icons::IconFont font = icons::IconFont::from_directory(LAMINA_ICONS_DIR);
    return font;
}

const color::ColorScheme& scheme() {
    static const color::ColorScheme s = color::spec_2021().build(color::default_scheme_options);
    return s;
}

template <typename W>
void settle(W& w) {
    for (int i = 0; i < 240; ++i) {
        w.step(1.0 / 60.0);
    }
}

template <typename W>
bool click(W& w, double x, double y) {
    w.handle({PointerAction::down, x, y});
    const bool clicked = w.handle({PointerAction::up, x, y});
    w.handle({PointerAction::leave, 0.0, 0.0});
    return clicked;
}

int count_color(render::Canvas& canvas, color::Argb c) {
    int n = 0;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            n += canvas.pixel(x, y) == c ? 1 : 0;
        }
    }
    return n;
}

}

TEST_CASE("switch tokens") {
    const auto& t = tokens::switch_tokens;
    CHECK(t.track_width == 52.0);
    CHECK(t.track_height == 32.0);
    CHECK(t.unchecked_thumb == 16.0);
    CHECK(t.checked_thumb == 24.0);
    CHECK(t.pressed_thumb == 28.0);
    CHECK(t.checked.track == color::Role::primary);
    CHECK(t.unchecked.thumb == color::Role::outline);
    CHECK_FALSE(t.checked.border.has_value());
}

TEST_CASE("a switch toggles on click and moves its thumb to the end") {
    widgets::Switch s;
    s.set_origin({0.0, 8.0});
    CHECK_FALSE(s.checked());
    CHECK(s.thumb_center_x() == 16.0);
    CHECK(click(s, 26.0, 24.0));
    CHECK(s.checked());
    CHECK(s.animating());
    settle(s);
    CHECK(s.thumb_center_x() == doctest::Approx(36.0));
    CHECK(s.thumb_size() == doctest::Approx(24.0));
    CHECK(click(s, 26.0, 24.0));
    CHECK_FALSE(s.checked());
    settle(s);
    CHECK(s.thumb_center_x() == doctest::Approx(16.0));
    CHECK(s.thumb_size() == doctest::Approx(16.0));
}

TEST_CASE("the switch thumb grows while pressed and with an icon") {
    widgets::Switch s;
    s.handle({PointerAction::down, 10.0, 16.0});
    settle(s);
    CHECK(s.thumb_size() == doctest::Approx(28.0));
    s.handle({PointerAction::leave, 0.0, 0.0});
    settle(s);
    CHECK(s.thumb_size() == doctest::Approx(16.0));

    widgets::Switch with_icon({false, std::nullopt, icons::Icon{icon_font(), icons::Symbol::close}});
    CHECK(with_icon.thumb_size() == doctest::Approx(24.0));
}

TEST_CASE("a checked switch paints a primary track and its check icon") {
    widgets::Switch s({true, icons::Icon{icon_font(), icons::Symbol::check}});
    render::ThorVGCanvas canvas(52, 32);
    canvas.clear(0xffffffff);
    s.paint(canvas, scheme());
    CHECK(canvas.pixel(8, 16) == scheme()[color::Role::primary]);
    const int thumb_with_icon = count_color(canvas, scheme()[color::Role::on_primary]);
    CHECK(thumb_with_icon > 50);
    widgets::Switch plain({true});
    canvas.clear(0xffffffff);
    plain.paint(canvas, scheme());
    CHECK(count_color(canvas, scheme()[color::Role::on_primary]) > thumb_with_icon + 20);
}

TEST_CASE("checkbox tokens follow Checkbox.kt") {
    const auto& t = tokens::checkbox_tokens;
    CHECK(t.box_size == 20.0);
    CHECK(t.stroke_width == 2.0);
    CHECK(t.corner_radius == 2.0);
    CHECK(t.uncheck_snap_delay_seconds == doctest::Approx(0.1));
    CHECK(t.check_cross.y == doctest::Approx(0.7));
}

TEST_CASE("a checkbox toggles between off and on, and indeterminate goes to on") {
    widgets::Checkbox c;
    c.set_origin({0.0, 0.0});
    CHECK(click(c, 20.0, 20.0));
    CHECK(c.state() == CheckboxState::on);
    CHECK(click(c, 20.0, 20.0));
    CHECK(c.state() == CheckboxState::off);
    c.set_state(CheckboxState::indeterminate);
    CHECK(click(c, 20.0, 20.0));
    CHECK(c.state() == CheckboxState::on);
    CHECK_FALSE(click(c, 200.0, 200.0));
}

TEST_CASE("the check mark draws in, and snaps away after the delay") {
    widgets::Checkbox c;
    CHECK(c.check_fraction() == 0.0);
    c.set_state(CheckboxState::on);
    c.step(1.0 / 60.0);
    const double early = c.check_fraction();
    CHECK(early > 0.0);
    CHECK(early < 1.0);
    settle(c);
    CHECK(c.check_fraction() == doctest::Approx(1.0));
    c.set_state(CheckboxState::off);
    c.step(0.05);
    CHECK(c.check_fraction() == doctest::Approx(1.0));
    c.step(0.06);
    CHECK(c.check_fraction() == 0.0);
}

TEST_CASE("the check mark path grows with the draw fraction") {
    const render::Rect box{0.0, 0.0, 20.0, 20.0};
    CHECK(widgets::check_mark_path(box, 0.0, 0.0, 2.0).empty());
    render::ThorVGCanvas canvas(20, 20);
    canvas.clear(0xffffffff);
    canvas.fill_path(widgets::check_mark_path(box, 0.3, 0.0, 2.0), 0xff000000);
    const int partial = count_color(canvas, 0xff000000);
    canvas.clear(0xffffffff);
    canvas.fill_path(widgets::check_mark_path(box, 1.0, 0.0, 2.0), 0xff000000);
    const int full = count_color(canvas, 0xff000000);
    CHECK(partial > 0);
    CHECK(full > partial * 2);
    CHECK(canvas.pixel(8, 13) == 0xff000000);
}

TEST_CASE("a checked checkbox fills its box with primary and draws the check in on-primary") {
    widgets::Checkbox c({CheckboxState::on});
    render::ThorVGCanvas canvas(40, 40);
    canvas.clear(0xffffffff);
    c.paint(canvas, scheme());
    CHECK(canvas.pixel(12, 12) == scheme()[color::Role::primary]);
    CHECK(count_color(canvas, scheme()[color::Role::on_primary]) > 10);

    widgets::Checkbox off;
    canvas.clear(0xffffffff);
    off.paint(canvas, scheme());
    CHECK(canvas.pixel(20, 20) == 0xffffffff);
    CHECK(canvas.pixel(10, 20) == scheme()[color::Role::on_surface_variant]);
}

TEST_CASE("radio button tokens and selection") {
    CHECK(tokens::radio_button_tokens.icon_size == 20.0);
    CHECK(tokens::radio_button_tokens.dot_size == 12.0);
    widgets::RadioButton r;
    CHECK_FALSE(r.selected());
    CHECK(click(r, 20.0, 20.0));
    CHECK(r.selected());
    CHECK(click(r, 20.0, 20.0));
    CHECK(r.selected());
    settle(r);
    CHECK(r.dot_radius() == doctest::Approx(6.0));
    r.set_selected(false);
    settle(r);
    CHECK(r.dot_radius() == doctest::Approx(0.0).epsilon(0.01));
}

TEST_CASE("a selected radio button paints a ring and a dot") {
    widgets::RadioButton r({true});
    render::ThorVGCanvas canvas(40, 40);
    canvas.clear(0xffffffff);
    r.paint(canvas, scheme());
    const color::Argb primary = scheme()[color::Role::primary];
    CHECK(canvas.pixel(20, 20) == primary);
    CHECK(canvas.pixel(20, 11) == primary);
    CHECK(canvas.pixel(20, 13) == 0xffffffff);
}
