#include <doctest/doctest.h>

#include <cstdlib>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/text_field.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;

namespace {

widgets::TextField field(std::string label, std::string placeholder) {
    widgets::TextField f({.label = std::move(label), .placeholder = std::move(placeholder)}, fonts());
    f.set_origin({10.0, 20.0});
    return f;
}

void type(widgets::TextField& f, const char* text) {
    f.handle(input::TextEvent{input::TextEventKind::commit, text, 0});
}

int placeholder_pixels(widgets::TextField& f) {
    thorvg::Canvas canvas(320, 100);
    canvas.clear(0xffffffff);
    f.paint(canvas, scheme());
    int n = 0;
    const color::Argb c = scheme()[tokens::outlined_text_field.placeholder];
    for (int y = 30; y < 70; ++y) {
        for (int x = 20; x < 280; ++x) {
            n += canvas.pixel(x, y) == c ? 1 : 0;
        }
    }
    return n;
}

}

TEST_CASE("with a label, the placeholder shows only while the empty field has the focus") {
    auto f = field("Name", "Jane Doe");
    CHECK(f.placeholder_opacity() == 0.0);
    CHECK_FALSE(f.placeholder_shown());
    f.set_focused(true);
    CHECK(f.animating());
    settle(f);
    CHECK(f.placeholder_opacity() == doctest::Approx(1.0));
    CHECK(f.placeholder_shown());
    CHECK(placeholder_pixels(f) > 0);
    f.set_focused(false);
    settle(f);
    CHECK(f.placeholder_opacity() == doctest::Approx(0.0));
    CHECK_FALSE(f.placeholder_shown());
}

TEST_CASE("without a label, the placeholder shows whenever the field is empty") {
    auto f = field("", "Search");
    CHECK(f.placeholder_opacity() == 1.0);
    CHECK(f.placeholder_shown());
    CHECK(placeholder_pixels(f) > 0);
    f.set_focused(true);
    settle(f);
    CHECK(f.placeholder_shown());
}

TEST_CASE("the placeholder goes away with the first character and comes back when the text is cleared") {
    auto f = field("Name", "Jane Doe");
    f.set_focused(true);
    settle(f);
    type(f, "J");
    CHECK_FALSE(f.placeholder_shown());
    CHECK(placeholder_pixels(f) == 0);
    f.handle(input::KeyEvent{input::Key::backspace});
    CHECK(f.placeholder_shown());
}

TEST_CASE("an IME composition counts as text, so the placeholder is not drawn under it") {
    auto f = field("", "Search");
    f.set_focused(true);
    f.handle(input::TextEvent{input::TextEventKind::compose, "か", 1});
    CHECK_FALSE(f.placeholder_shown());
}

TEST_CASE("the placeholder fades in on the slow effects spring when an empty field takes the focus") {
    auto slow = field("Name", "Jane Doe");
    slow.set_focused(true);
    slow.step(frame_seconds * 4);
    const double after_focus = slow.placeholder_opacity();

    auto fast = field("Name", "Jane Doe");
    fast.set_focused(true);
    settle(fast);
    fast.set_focused(false);
    fast.step(frame_seconds * 4);
    const double after_blur = 1.0 - fast.placeholder_opacity();
    CHECK(after_focus > 0.0);
    CHECK(after_blur > after_focus);
}

TEST_CASE("a disabled text field cannot take the focus or be edited") {
    widgets::TextField f({.label = "Name", .placeholder = "Jane Doe"}, fonts());
    f.set_origin({10.0, 20.0});
    f.set_focused(true);
    type(f, "a");
    f.set_enabled(false);
    CHECK_FALSE(f.focused());
    CHECK_FALSE(f.can_focus());
    CHECK_FALSE(f.handle({PointerAction::down, 40.0, 40.0}));
    CHECK_FALSE(f.focused());
    type(f, "b");
    CHECK(f.text() == "a");
    widgets::FocusManager focus;
    focus.add(f);
    CHECK_FALSE(focus.move_focus(widgets::FocusDirection::next));
    f.set_enabled(true);
    CHECK(f.handle({PointerAction::down, 40.0, 40.0}));
    CHECK(f.focused());
}

TEST_CASE("a disabled text field draws its outline in on-surface at 12 %") {
    widgets::TextField f({.label = "Name", .enabled = false}, fonts());
    f.set_origin({10.0, 20.0});
    thorvg::Canvas canvas(320, 100);
    canvas.clear(0xffffffff);
    f.paint(canvas, scheme());
    const color::Argb expected = color::source_over(color::scale_alpha(scheme()[color::Role::on_surface], 0.12), 0xffffffff);
    const color::Argb side = canvas.pixel(10, 48);
    for (int shift : {0, 8, 16}) {
        CHECK(std::abs(static_cast<int>((side >> shift) & 0xff) - static_cast<int>((expected >> shift) & 0xff)) <= 2);
    }
}
