#include <doctest/doctest.h>

#include "lamina/tokens/focus.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;

namespace {

constexpr input::KeyEvent space{Key::space, false};
constexpr input::KeyEvent enter{Key::enter, false};
constexpr input::KeyEvent held_space{Key::space, false, false, true};
constexpr input::KeyEvent tab{Key::tab, false};

icons::Icon icon() {
    return {icon_font(), res::icons::favorite};
}

widgets::Button button() {
    return widgets::Button({"Send"}, fonts());
}

template <typename W>
void place(W& w) {
    w.set_origin({20.0, 20.0});
}

template <typename W>
int ring_pixels(W& w) {
    thorvg::Canvas canvas(160, 100);
    canvas.clear(0xffffffff);
    settle(w);
    w.paint(canvas, scheme());
    return count_color(canvas, scheme()[tokens::focus_indicator.color]);
}

template <typename W>
void check_focus_protocol(W w, bool enter_activates) {
    place(w);
    CHECK_FALSE(w.focused());
    CHECK_FALSE(w.handle(space));
    CHECK_FALSE(w.handle(enter));
    const int unfocused = ring_pixels(w);

    w.set_focused(true);
    CHECK(w.focused());
    CHECK(w.animating());
    CHECK(ring_pixels(w) > unfocused);
    CHECK_FALSE(w.handle(held_space));
    CHECK_FALSE(w.handle(tab));
    CHECK(w.handle(enter) == enter_activates);
    CHECK(w.handle(space));

    w.handle({PointerAction::down, 500.0, 500.0});
    CHECK_FALSE(w.focused());
    CHECK(ring_pixels(w) == unfocused);
    CHECK_FALSE(w.handle(space));
}

}

TEST_CASE("buttons, icon buttons and FABs take the focus, show the ring and activate on Space or Enter") {
    check_focus_protocol(button(), true);
    check_focus_protocol(widgets::IconButton({icon(), "Add", tokens::IconButtonStyle::filled}), true);
    check_focus_protocol(widgets::Fab({icon(), "Add"}), true);
}

TEST_CASE("checkboxes, switches and radio buttons take the focus, show the ring and activate on Space only") {
    check_focus_protocol(widgets::Checkbox(), false);
    check_focus_protocol(widgets::Switch(), false);
    check_focus_protocol(widgets::RadioButton(), false);
}

TEST_CASE("Space toggles a focused checkbox, switch and toggle icon button once per press") {
    widgets::Checkbox c;
    c.set_focused(true);
    CHECK(c.handle(space));
    CHECK(c.state() == tokens::CheckboxState::on);
    CHECK_FALSE(c.handle(held_space));
    CHECK(c.state() == tokens::CheckboxState::on);
    CHECK(c.handle(space));
    CHECK(c.state() == tokens::CheckboxState::off);

    widgets::Switch s;
    s.set_focused(true);
    CHECK(s.handle(space));
    CHECK(s.checked());
    CHECK_FALSE(s.handle(enter));
    CHECK(s.checked());

    widgets::IconButton t({icon(), "Favorite", tokens::IconButtonStyle::filled, tokens::IconButtonSize::small, tokens::ButtonShape::round,
                           tokens::IconButtonWidth::uniform, true});
    t.set_focused(true);
    CHECK(t.handle(enter));
    CHECK(t.checked());

    widgets::RadioButton r;
    r.set_focused(true);
    CHECK(r.handle(space));
    CHECK(r.selected());
}

TEST_CASE("focus raises the state layer of an idle button without a pointer") {
    auto b = button();
    place(b);
    thorvg::Canvas canvas(160, 100);
    auto center = [&] {
        canvas.clear(0xffffffff);
        settle(b);
        b.paint(canvas, scheme());
        const render::Rect r = b.bounds();
        return canvas.pixel(static_cast<int>(r.x + r.width / 2.0), static_cast<int>(r.y + 2.0));
    };
    const color::Argb idle = center();
    b.set_focused(true);
    CHECK(center() != idle);
    b.set_focused(false);
    CHECK(center() == idle);
}

TEST_CASE("a focus manager cycles through buttons and selection controls alike") {
    auto a = button();
    widgets::Checkbox c;
    widgets::Switch s;
    widgets::FocusManager focus;
    focus.add(a);
    focus.add(c);
    focus.add(s);
    CHECK(focus.handle(tab));
    CHECK(a.focused());
    CHECK(focus.handle(tab));
    CHECK(c.focused());
    CHECK_FALSE(a.focused());
    CHECK(focus.handle({Key::tab, true}));
    CHECK(a.focused());
    CHECK(focus.handle({Key::tab, true}));
    CHECK(s.focused());
    CHECK_FALSE(focus.caret_area());
}
