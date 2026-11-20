#include <doctest/doctest.h>

#include "lamina/tokens/focus.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;
using widgets::DialogResult;

namespace {

constexpr layout::Size window{800.0, 600.0};

widgets::DialogOptions options() {
    return {"Discard draft?", "The message will be deleted from this device.", "Discard", "Cancel"};
}

DialogResult click(widgets::Dialog& d, render::Rect r) {
    const double x = r.x + r.width / 2.0;
    const double y = r.y + r.height / 2.0;
    d.handle({PointerAction::down, x, y});
    return d.handle({PointerAction::up, x, y});
}

}

TEST_CASE("dialog tokens follow DialogTokens.kt, ScrimTokens.kt and AlertDialog.kt") {
    const auto& t = tokens::dialog_tokens;
    CHECK(t.container == color::Role::surface_container_high);
    CHECK(t.corner == tokens::CornerSize::extra_large);
    CHECK(t.padding == 24.0);
    CHECK(t.headline_bottom_space == 16.0);
    CHECK(t.supporting_text_bottom_space == 24.0);
    CHECK(t.min_width == 280.0);
    CHECK(t.max_width == 560.0);
    CHECK(t.headline == tokens::TypeRole::headline_small);
    CHECK(t.supporting_text == tokens::TypeRole::body_medium);
    CHECK(t.scrim_opacity == doctest::Approx(0.32));
}

TEST_CASE("the dialog is centered, at least 280 dp wide, with the confirm button at the end") {
    widgets::Dialog d(options(), fonts());
    d.open(window);
    const render::Rect r = d.bounds();
    CHECK(r.width >= 280.0);
    CHECK(r.width <= 560.0);
    CHECK(r.x + r.width / 2.0 == doctest::Approx(400.0));
    CHECK(r.y + r.height / 2.0 == doctest::Approx(300.0));
    const render::Rect confirm = d.confirm_bounds();
    CHECK(confirm.x + confirm.width == doctest::Approx(r.x + r.width - 24.0));
    CHECK(confirm.y + confirm.height == doctest::Approx(r.y + r.height - 24.0));
    REQUIRE(d.dismiss_bounds());
    CHECK(d.dismiss_bounds()->x + d.dismiss_bounds()->width + 8.0 == doctest::Approx(confirm.x));
}

TEST_CASE("the dialog fades and scales in, and its buttons confirm or dismiss it") {
    widgets::Dialog d(options(), fonts());
    CHECK_FALSE(d.visible());
    d.open(window);
    CHECK(d.opacity() == 0.0);
    CHECK(d.scale() == doctest::Approx(0.8));
    settle(d);
    CHECK(d.opacity() == doctest::Approx(1.0));
    CHECK(d.scale() == doctest::Approx(1.0));
    CHECK(click(d, d.confirm_bounds()) == DialogResult::confirmed);
    CHECK_FALSE(d.is_open());
    CHECK(d.visible());
    settle(d);
    CHECK_FALSE(d.visible());

    d.open(window);
    settle(d);
    CHECK(click(d, *d.dismiss_bounds()) == DialogResult::dismissed);
}

TEST_CASE("a press on the scrim or Escape dismisses the dialog") {
    widgets::Dialog d(options(), fonts());
    d.open(window);
    settle(d);
    CHECK(d.handle({PointerAction::down, d.bounds().x + 10.0, d.bounds().y + 10.0}) == DialogResult::none);
    CHECK(d.handle({PointerAction::up, d.bounds().x + 10.0, d.bounds().y + 10.0}) == DialogResult::none);
    CHECK(d.is_open());
    CHECK(d.handle({PointerAction::down, 5.0, 5.0}) == DialogResult::dismissed);
    CHECK_FALSE(d.is_open());
    d.open(window);
    CHECK(d.handle(input::KeyEvent{input::Key::escape, false}) == DialogResult::dismissed);
    CHECK(d.handle({PointerAction::down, 5.0, 5.0}) == DialogResult::none);
}

TEST_CASE("an open dialog paints the scrim and its container") {
    widgets::Dialog d(options(), fonts());
    d.open({400.0, 300.0});
    settle(d);
    thorvg::Canvas canvas(400, 300);
    canvas.clear(0xffffffff);
    d.paint(canvas, scheme());
    const color::Argb scrim = canvas.pixel(2, 2);
    CHECK(scrim != 0xffffffff);
    CHECK(((scrim >> 16) & 0xff) == doctest::Approx(255.0 * 0.68).epsilon(0.02));
    const render::Rect r = d.bounds();
    CHECK(canvas.pixel(static_cast<int>(r.x + 10.0), static_cast<int>(r.y + 30.0)) == scheme()[color::Role::surface_container_high]);
}

namespace {

widgets::DialogOptions with_icon() {
    widgets::DialogOptions o = options();
    o.icon = icons::Icon{icon_font(), res::icons::delete_};
    return o;
}

}

TEST_CASE("dialog icon tokens follow DialogTokens.kt and AlertDialog.kt") {
    const auto& t = tokens::dialog_tokens;
    CHECK(t.icon_size == 24.0);
    CHECK(t.icon_color == color::Role::secondary);
    CHECK(t.icon_bottom_space == 16.0);
    CHECK(t.button_line_space == 8.0);
}

TEST_CASE("an icon sits centered above a centered headline") {
    widgets::Dialog plain(options(), fonts());
    plain.open(window);
    CHECK_FALSE(plain.icon_bounds());
    CHECK(plain.headline_bounds().x == doctest::Approx(plain.bounds().x + 24.0));

    widgets::Dialog d(with_icon(), fonts());
    d.open(window);
    const render::Rect r = d.bounds();
    REQUIRE(d.icon_bounds());
    const render::Rect icon = *d.icon_bounds();
    CHECK(icon.width == 24.0);
    CHECK(icon.y == doctest::Approx(r.y + 24.0));
    CHECK(icon.x + icon.width / 2.0 == doctest::Approx(r.x + r.width / 2.0));
    const render::Rect headline = d.headline_bounds();
    CHECK(headline.y == doctest::Approx(icon.y + 24.0 + 16.0));
    CHECK(headline.x + headline.width / 2.0 == doctest::Approx(r.x + r.width / 2.0));
    CHECK(r.height == doctest::Approx(plain.bounds().height + 24.0 + 16.0));
}

TEST_CASE("the dialog paints its icon in the secondary color") {
    widgets::Dialog d(with_icon(), fonts());
    d.open(window);
    settle(d);
    thorvg::Canvas canvas(800, 600);
    canvas.clear(0xffffffff);
    d.paint(canvas, scheme());
    const render::Rect icon = *d.icon_bounds();
    int ink = 0;
    for (int y = static_cast<int>(icon.y); y < static_cast<int>(icon.y + icon.height); ++y) {
        for (int x = static_cast<int>(icon.x); x < static_cast<int>(icon.x + icon.width); ++x) {
            ink += canvas.pixel(x, y) == scheme()[color::Role::secondary] ? 1 : 0;
        }
    }
    CHECK(ink > 20);
}

TEST_CASE("buttons that do not fit wrap with confirm on the first line, both at the end") {
    widgets::Dialog one(options(), fonts());
    one.open(window);
    CHECK(one.button_lines() == 1);

    widgets::DialogOptions o = options();
    o.confirm_label = "Discard the draft and every attachment in it";
    o.dismiss_label = "Keep editing the draft for now";
    widgets::Dialog d(o, fonts());
    d.open(window);
    CHECK(d.button_lines() == 2);
    const render::Rect r = d.bounds();
    const render::Rect confirm = d.confirm_bounds();
    REQUIRE(d.dismiss_bounds());
    const render::Rect dismiss = *d.dismiss_bounds();
    CHECK(confirm.x + confirm.width == doctest::Approx(r.x + r.width - 24.0));
    CHECK(dismiss.x + dismiss.width == doctest::Approx(r.x + r.width - 24.0));
    CHECK(dismiss.y == doctest::Approx(confirm.y + confirm.height + 8.0));
    CHECK(dismiss.y + dismiss.height == doctest::Approx(r.y + r.height - 24.0));
}

TEST_CASE("a dialog opened on a window size covers the whole window") {
    widgets::Dialog by_size(options(), fonts());
    widgets::Dialog by_area(options(), fonts());
    by_size.open(window);
    by_area.open_in({0.0, 0.0, window.width, window.height});
    const render::Rect a = by_size.bounds();
    const render::Rect b = by_area.bounds();
    CHECK(a.x == b.x);
    CHECK(a.y == b.y);
    CHECK(a.width == b.width);
    CHECK(a.height == b.height);
    CHECK(by_size.area().width == window.width);
    CHECK(by_size.area().height == window.height);
}

TEST_CASE("a dialog given an area centers in it, dims only it and is dismissed only by presses inside it") {
    constexpr double bar = 40.0;
    const render::Rect area{0.0, bar, window.width, window.height - bar};
    widgets::Dialog d(options(), fonts());
    d.open_in(area);
    settle(d);
    const render::Rect r = d.bounds();
    CHECK(r.x + r.width / 2.0 == doctest::Approx(area.x + area.width / 2.0));
    CHECK(r.y + r.height / 2.0 == doctest::Approx(area.y + area.height / 2.0));

    thorvg::Canvas canvas(static_cast<int>(window.width), static_cast<int>(window.height));
    constexpr color::Argb background = 0xffffffff;
    canvas.clear(background);
    d.paint(canvas, scheme());
    CHECK(canvas.pixel(10, static_cast<int>(bar / 2.0)) == background);
    CHECK(canvas.pixel(10, static_cast<int>(bar + 10.0)) != background);

    CHECK(d.handle({PointerAction::down, 10.0, bar / 2.0}) == DialogResult::none);
    d.handle({PointerAction::up, 10.0, bar / 2.0});
    CHECK(d.is_open());
    CHECK(d.handle({PointerAction::down, 10.0, bar + 10.0}) == DialogResult::dismissed);
    CHECK_FALSE(d.is_open());
}

TEST_CASE("set_area moves an open dialog to the new area") {
    widgets::Dialog d(options(), fonts());
    d.open(window);
    const render::Rect area{100.0, 50.0, 400.0, 300.0};
    d.set_area(area);
    const render::Rect r = d.bounds();
    CHECK(r.x + r.width / 2.0 == doctest::Approx(300.0));
    CHECK(r.y + r.height / 2.0 == doctest::Approx(200.0));
    CHECK(d.area().x == area.x);
    CHECK(d.area().y == area.y);
}

namespace {

constexpr input::KeyEvent key_tab{input::Key::tab, false};
constexpr input::KeyEvent key_shift_tab{input::Key::tab, true};
constexpr input::KeyEvent key_enter{input::Key::enter, false};
constexpr input::KeyEvent key_space{input::Key::space, false};

}

TEST_CASE("a dialog opened from the keyboard shows the focus on its first action in reading order") {
    widgets::Dialog d(options(), fonts());
    d.open(window, input::InputSource::keyboard);
    REQUIRE(d.focused_action());
    CHECK(*d.focused_action() == widgets::DialogAction::dismiss);
    CHECK(d.focus_shown());
    CHECK(d.handle(key_enter) == DialogResult::dismissed);
    CHECK_FALSE(d.is_open());
    CHECK_FALSE(d.focused_action());
}

TEST_CASE("a dialog opened by the pointer holds the focus without showing it until Tab") {
    widgets::Dialog d(options(), fonts());
    d.open(window);
    REQUIRE(d.focused_action());
    CHECK(*d.focused_action() == widgets::DialogAction::dismiss);
    CHECK_FALSE(d.focus_shown());
    CHECK(d.handle(key_tab) == DialogResult::none);
    CHECK(d.focus_shown());
    CHECK(*d.focused_action() == widgets::DialogAction::confirm);
    CHECK(d.handle(key_space) == DialogResult::confirmed);
}

TEST_CASE("Tab and Shift+Tab cycle only through the dialog's actions") {
    widgets::Dialog d(options(), fonts());
    d.open(window, input::InputSource::keyboard);
    d.handle(key_tab);
    CHECK(*d.focused_action() == widgets::DialogAction::confirm);
    d.handle(key_tab);
    CHECK(*d.focused_action() == widgets::DialogAction::dismiss);
    d.handle(key_shift_tab);
    CHECK(*d.focused_action() == widgets::DialogAction::confirm);
    CHECK(d.is_open());
    CHECK(d.handle(input::KeyEvent{input::Key::enter, false, false, true}) == DialogResult::none);
    CHECK(d.handle(input::KeyEvent{input::Key::escape, false}) == DialogResult::dismissed);
}

TEST_CASE("the default action takes the first focus, and a dialog with one action focuses it") {
    widgets::DialogOptions o = options();
    o.default_action = widgets::DialogAction::confirm;
    widgets::Dialog d(o, fonts());
    d.open(window, input::InputSource::keyboard);
    CHECK(*d.focused_action() == widgets::DialogAction::confirm);

    widgets::DialogOptions single{"Saved", "", "OK", ""};
    single.default_action = widgets::DialogAction::dismiss;
    widgets::Dialog s(single, fonts());
    s.open(window, input::InputSource::keyboard);
    CHECK(*s.focused_action() == widgets::DialogAction::confirm);
    s.handle(key_tab);
    CHECK(*s.focused_action() == widgets::DialogAction::confirm);
    CHECK(s.handle(key_enter) == DialogResult::confirmed);
}

TEST_CASE("wrapped actions keep the reading order, confirm first, and a pointer press hides the focus") {
    widgets::DialogOptions o = options();
    o.confirm_label = "Discard the draft and every attachment in it";
    o.dismiss_label = "Keep editing the draft for now";
    widgets::Dialog d(o, fonts());
    d.open(window, input::InputSource::keyboard);
    REQUIRE(d.button_lines() == 2);
    CHECK(*d.focused_action() == widgets::DialogAction::confirm);
    const render::Rect r = d.headline_bounds();
    d.handle({PointerAction::down, r.x + 1.0, r.y + 1.0});
    CHECK_FALSE(d.focus_shown());
    CHECK(d.is_open());
}

TEST_CASE("the focused action draws the focus ring") {
    widgets::Dialog d(options(), fonts());
    thorvg::Canvas canvas(800, 600);
    auto ring = [&] {
        settle(d);
        canvas.clear(0xffffffff);
        d.paint(canvas, scheme());
        return count_color(canvas, scheme()[tokens::focus_indicator.color]);
    };
    d.open(window);
    const int hidden = ring();
    d.handle(key_tab);
    CHECK(ring() > hidden);
}
