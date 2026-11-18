#include <doctest/doctest.h>

#include <string>

#include "lamina/render/thorvg_canvas.hpp"
#include "lamina/widgets/dialog.hpp"

using namespace lamina;
using platform::PointerAction;
using widgets::DialogResult;

namespace {

const text::FontStack& fonts() {
    static const text::FontStack stack({text::Font::from_file(std::string(LAMINA_TEST_FONTS_DIR) + "/lamina-test-sans.ttf")});
    return stack;
}

const color::ColorScheme& scheme() {
    static const color::ColorScheme s = color::spec_2021().build(color::default_scheme_options);
    return s;
}

void settle(widgets::Dialog& d) {
    for (int i = 0; i < 240; ++i) {
        d.step(1.0 / 60.0);
    }
}

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
    CHECK(d.handle(platform::KeyEvent{platform::Key::escape, false}) == DialogResult::dismissed);
    CHECK(d.handle({PointerAction::down, 5.0, 5.0}) == DialogResult::none);
}

TEST_CASE("an open dialog paints the scrim and its container") {
    widgets::Dialog d(options(), fonts());
    d.open({400.0, 300.0});
    settle(d);
    render::ThorVGCanvas canvas(400, 300);
    canvas.clear(0xffffffff);
    d.paint(canvas, scheme());
    const color::Argb scrim = canvas.pixel(2, 2);
    CHECK(scrim != 0xffffffff);
    CHECK(((scrim >> 16) & 0xff) == doctest::Approx(255.0 * 0.68).epsilon(0.02));
    const render::Rect r = d.bounds();
    CHECK(canvas.pixel(static_cast<int>(r.x + 10.0), static_cast<int>(r.y + 30.0)) == scheme()[color::Role::surface_container_high]);
}
