#include <doctest/doctest.h>

#include <cstdlib>

#include "lamina/color/mix.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;

namespace {

bool near(color::Argb a, color::Argb b, int tolerance = 2) {
    for (int shift : {0, 8, 16, 24}) {
        const int x = static_cast<int>((a >> shift) & 0xff);
        const int y = static_cast<int>((b >> shift) & 0xff);
        if (std::abs(x - y) > tolerance) {
            return false;
        }
    }
    return true;
}

constexpr color::Argb white = 0xffffffff;

int count_near(render::Canvas& canvas, color::Argb c) {
    int n = 0;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            n += near(canvas.pixel(x, y), c) ? 1 : 0;
        }
    }
    return n;
}

color::Argb over_white(const color::ColorScheme& s, color::Role role, double opacity) {
    return color::source_over(color::scale_alpha(s[role], opacity), white);
}

widgets::Button button(tokens::ButtonStyle style = tokens::ButtonStyle::filled) {
    widgets::Button b({"Send", style}, fonts());
    b.set_origin({10.0, 10.0});
    return b;
}

widgets::IconButton icon_button(tokens::IconButtonStyle style = tokens::IconButtonStyle::filled) {
    widgets::IconButton b({{icon_font(), res::icons::add}, style});
    b.set_origin({10.0, 10.0});
    return b;
}

template <typename W>
void check_inert(W& w, double x, double y) {
    w.set_focused(true);
    CHECK(w.focused());
    w.set_enabled(false);
    CHECK_FALSE(w.enabled());
    CHECK_FALSE(w.can_focus());
    CHECK_FALSE(w.focused());
    w.set_focused(true);
    CHECK_FALSE(w.focused());
    CHECK_FALSE(w.handle(input::KeyEvent{Key::space}));
    CHECK_FALSE(w.handle(input::KeyEvent{Key::enter}));
    CHECK_FALSE(w.handle({PointerAction::move, x, y}));
    CHECK_FALSE(w.handle({PointerAction::down, x, y}));
    CHECK_FALSE(w.handle({PointerAction::up, x, y}));
    w.set_enabled(true);
    CHECK(w.can_focus());
}

}

static_assert(widgets::Disableable<widgets::Button>);
static_assert(widgets::Disableable<widgets::IconButton>);
static_assert(widgets::Disableable<widgets::Checkbox>);
static_assert(widgets::Disableable<widgets::RadioButton>);
static_assert(widgets::Disableable<widgets::Switch>);
static_assert(widgets::Disableable<widgets::Slider>);
static_assert(widgets::FocusGated<widgets::Button>);
static_assert(widgets::FocusGated<widgets::Slider>);

TEST_CASE("a disabled control ignores the pointer and the keys and gives up the focus") {
    auto b = button();
    check_inert(b, 20.0, 20.0);
    CHECK(click(b, 20.0, 20.0));
    auto ib = icon_button();
    check_inert(ib, 20.0, 20.0);
    widgets::Checkbox c;
    check_inert(c, 20.0, 20.0);
    CHECK(c.state() == tokens::CheckboxState::off);
    widgets::RadioButton r;
    check_inert(r, 20.0, 20.0);
    CHECK_FALSE(r.selected());
    widgets::Switch s;
    check_inert(s, 20.0, 16.0);
    CHECK_FALSE(s.checked());
}

TEST_CASE("a disabled slider keeps its value under the pointer and the keys") {
    widgets::Slider s({.value = 0.5});
    s.set_width(200.0);
    s.set_focused(true);
    s.set_enabled(false);
    CHECK_FALSE(s.focused());
    CHECK_FALSE(s.handle({PointerAction::down, 20.0, 20.0}));
    CHECK_FALSE(s.handle(input::KeyEvent{Key::right}));
    CHECK(s.value() == 0.5);
    CHECK_FALSE(s.dragging());
}

TEST_CASE("disabling ends a press in progress, so the release does not click") {
    auto b = button();
    b.handle({PointerAction::down, 20.0, 20.0});
    b.set_enabled(false);
    b.set_enabled(true);
    CHECK_FALSE(b.handle({PointerAction::up, 20.0, 20.0}));
}

TEST_CASE("the focus order skips disabled controls") {
    auto a = button();
    auto b = button();
    auto c = button();
    widgets::FocusManager focus;
    focus.add(a);
    focus.add(b);
    focus.add(c);
    b.set_enabled(false);
    focus.handle(input::KeyEvent{Key::tab});
    CHECK(a.focused());
    focus.handle(input::KeyEvent{Key::tab});
    CHECK(c.focused());
    CHECK_FALSE(b.focused());
    focus.handle(input::KeyEvent{Key::tab, true});
    CHECK(a.focused());
}

TEST_CASE("the enabled state can be given in the options") {
    widgets::Button b({.label = "Send", .enabled = false}, fonts());
    CHECK_FALSE(b.enabled());
    CHECK_FALSE(b.animating());
    widgets::Checkbox c({.enabled = false});
    CHECK_FALSE(c.can_focus());
    widgets::Switch s({.enabled = false});
    CHECK_FALSE(s.can_focus());
    widgets::Slider sl({.enabled = false});
    CHECK_FALSE(sl.can_focus());
}

TEST_CASE("the colors move to the disabled ones continuously") {
    const auto& s = scheme();
    auto b = button();
    thorvg::Canvas canvas(160, 80);
    canvas.clear(white);
    b.paint(canvas, s);
    const color::Argb enabled = canvas.pixel(15, 30);
    CHECK(near(enabled, s[color::Role::primary]));

    b.set_enabled(false);
    CHECK(b.animating());
    b.step(frame_seconds * 3);
    canvas.clear(white);
    b.paint(canvas, s);
    const color::Argb middle = canvas.pixel(15, 30);
    const color::Argb disabled = over_white(s, color::Role::on_surface, 0.1);
    CHECK_FALSE(near(middle, enabled));
    CHECK_FALSE(near(middle, disabled));
    settle(b);
    CHECK_FALSE(b.animating());
    canvas.clear(white);
    b.paint(canvas, s);
    CHECK(near(canvas.pixel(15, 30), disabled));
}

TEST_CASE("disabled button containers follow the style tokens") {
    const auto& s = scheme();
    thorvg::Canvas canvas(160, 80);
    struct Case {
        tokens::ButtonStyle style;
        color::Argb container;
    };
    const Case cases[] = {
        {tokens::ButtonStyle::filled, over_white(s, color::Role::on_surface, 0.1)},
        {tokens::ButtonStyle::tonal, over_white(s, color::Role::on_surface, 0.12)},
        {tokens::ButtonStyle::elevated, over_white(s, color::Role::on_surface, 0.1)},
        {tokens::ButtonStyle::outlined, white},
        {tokens::ButtonStyle::text, white},
    };
    for (const Case& c : cases) {
        CAPTURE(static_cast<int>(c.style));
        auto b = button(c.style);
        b.set_enabled(false);
        settle(b);
        canvas.clear(white);
        b.paint(canvas, s);
        CHECK(near(canvas.pixel(16, 30), c.container));
    }
}

TEST_CASE("a disabled icon button draws its icon in on-surface at 38 %") {
    const auto& s = scheme();
    auto b = icon_button(tokens::IconButtonStyle::standard);
    b.set_enabled(false);
    settle(b);
    thorvg::Canvas canvas(80, 80);
    canvas.clear(white);
    b.paint(canvas, s);
    CHECK(count_near(canvas, over_white(s, color::Role::on_surface, 0.38)) > 20);
}

TEST_CASE("a disabled switch composites its colors over the surface, as Compose does") {
    const auto& s = scheme();
    widgets::Switch sw({.checked = true});
    sw.set_origin({10.0, 10.0});
    sw.set_enabled(false);
    settle(sw);
    thorvg::Canvas canvas(100, 60);
    canvas.clear(0xff000000);
    sw.paint(canvas, s);
    const color::Argb track = color::source_over(color::scale_alpha(s[color::Role::on_surface], 0.12), s[color::Role::surface]);
    CHECK(near(canvas.pixel(16, 26), track));
}

TEST_CASE("a disabled checked checkbox fills its box with on-surface at 38 %") {
    const auto& s = scheme();
    widgets::Checkbox c({tokens::CheckboxState::on});
    c.set_origin({0.0, 0.0});
    c.set_enabled(false);
    settle(c);
    thorvg::Canvas canvas(40, 40);
    canvas.clear(white);
    c.paint(canvas, s);
    CHECK(near(canvas.pixel(12, 12), over_white(s, color::Role::on_surface, 0.38)));
}

TEST_CASE("a widget set enables and disables every member that can be") {
    widgets::WidgetSet set;
    auto& b = set.emplace<widgets::Button>(widgets::ButtonOptions{"A"}, fonts());
    auto& c = set.emplace<widgets::Checkbox>();
    set.set_enabled(false);
    CHECK_FALSE(b.enabled());
    CHECK_FALSE(c.enabled());
    set.set_enabled(true);
    CHECK(b.enabled());
}
