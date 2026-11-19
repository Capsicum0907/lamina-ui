#include <doctest/doctest.h>

#include <optional>
#include <vector>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/overlay_stack.hpp"
#include "lamina/widgets/protocol.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;

static_assert(widgets::Overlay<widgets::Menu> && widgets::Overlay<widgets::Dialog>);
static_assert(!widgets::Overlay<widgets::Button>);

namespace {

constexpr layout::Size window{800.0, 600.0};
constexpr input::KeyEvent escape{input::Key::escape, false};

widgets::Menu make_menu() {
    return widgets::Menu({{{"Edit"}, {"Share"}}}, fonts());
}

render::Point center(const render::Rect& r) {
    return {r.x + r.width / 2.0, r.y + r.height / 2.0};
}

}

TEST_CASE("an empty overlay stack leaves every event to the program") {
    widgets::OverlayStack stack;
    CHECK(stack.empty());
    CHECK_FALSE(stack.handle(input::PointerEvent{PointerAction::down, 1.0, 1.0}));
    CHECK_FALSE(stack.handle(escape));
    CHECK_FALSE(stack.handle(input::TextEvent{input::TextEventKind::commit, "a", 0}));
}

TEST_CASE("a modal overlay takes every event, reports its results, and leaves the stack when it closes") {
    auto menu = make_menu();
    widgets::OverlayStack stack;
    std::vector<std::size_t> chosen;
    menu.open({20.0, 20.0, 40.0, 40.0}, window);
    settle(menu);
    stack.push(menu, [&](const widgets::MenuResponse& r) {
        if (r.chosen) {
            chosen.push_back(*r.chosen);
        }
    });
    CHECK_FALSE(stack.empty());
    CHECK(stack.handle(input::PointerEvent{PointerAction::leave, 0.0, 0.0}));
    CHECK(stack.handle(input::TextEvent{input::TextEventKind::commit, "a", 0}));
    const render::Point p = center(menu.item_rect(1));
    CHECK(stack.handle(input::PointerEvent{PointerAction::down, p.x, p.y}));
    CHECK(stack.handle(input::PointerEvent{PointerAction::up, p.x, p.y}));
    CHECK(chosen == std::vector<std::size_t>{1});
    CHECK_FALSE(menu.is_open());
    CHECK(stack.empty());
    CHECK_FALSE(stack.handle(input::PointerEvent{PointerAction::move, p.x, p.y}));
}

TEST_CASE("pushing a modal overlay cancels the press in progress on the base") {
    widgets::Button button({"Open"}, fonts());
    auto menu = make_menu();
    widgets::OverlayStack stack;
    stack.add_base(button);
    const render::Point p = center(button.bounds());
    button.handle({PointerAction::down, p.x, p.y});
    menu.open(button.bounds(), window);
    stack.push(menu, [](const widgets::MenuResponse&) {});
    CHECK_FALSE(button.handle({PointerAction::up, p.x, p.y}));
}

TEST_CASE("the topmost overlay gets the keys first and the one below takes over when it closes") {
    auto menu = make_menu();
    widgets::Dialog dialog({"Title", "Text", "OK", "Cancel"}, fonts());
    widgets::OverlayStack stack;
    std::vector<widgets::DialogResult> results;
    menu.open({20.0, 20.0, 40.0, 40.0}, window);
    stack.push(menu, [](const widgets::MenuResponse&) {});
    dialog.open(window);
    stack.push(dialog, [&](widgets::DialogResult r) { results.push_back(r); });
    CHECK(stack.handle(escape));
    CHECK(results == std::vector<widgets::DialogResult>{widgets::DialogResult::dismissed});
    CHECK_FALSE(dialog.is_open());
    CHECK(menu.is_open());
    CHECK(stack.handle(escape));
    CHECK_FALSE(menu.is_open());
    CHECK(stack.empty());
    CHECK(results.size() == 1);
}

TEST_CASE("a modeless overlay passes on the events it does not use") {
    auto menu = make_menu();
    widgets::OverlayStack stack;
    menu.open({20.0, 20.0, 40.0, 40.0}, window);
    settle(menu);
    stack.push(menu, [](const widgets::MenuResponse&) {}, widgets::Modality::modeless);
    CHECK_FALSE(stack.handle(input::PointerEvent{PointerAction::leave, 0.0, 0.0}));
    CHECK_FALSE(stack.handle(input::KeyEvent{input::Key::a, false}));
    const render::Point p = center(menu.item_rect(0));
    CHECK(stack.handle(input::PointerEvent{PointerAction::move, p.x, p.y}));
}

TEST_CASE("pushing an overlay that is already on the stack moves it to the top without delivering twice") {
    auto menu = make_menu();
    widgets::OverlayStack stack;
    int calls = 0;
    menu.open({20.0, 20.0, 40.0, 40.0}, window);
    stack.push(menu, [&](const widgets::MenuResponse&) { ++calls; });
    stack.push(menu, [&](const widgets::MenuResponse&) { ++calls; });
    stack.handle(input::PointerEvent{PointerAction::move, 30.0, 30.0});
    CHECK(calls == 1);
}

TEST_CASE("a modal overlay takes the focus away and gives it back when it closes") {
    widgets::Button a({"A"}, fonts());
    widgets::Button b({"B"}, fonts());
    widgets::FocusManager focus;
    focus.add(a);
    focus.add(b);
    widgets::OverlayStack stack;
    stack.set_focus(focus);
    focus.handle(input::KeyEvent{input::Key::tab, false});
    focus.handle(input::KeyEvent{input::Key::tab, false});
    REQUIRE(b.focused());

    widgets::Dialog dialog({"Delete?", "", "Delete", "Cancel"}, fonts());
    dialog.open(window, input::InputSource::keyboard);
    widgets::DialogResult result = widgets::DialogResult::none;
    stack.push(dialog, [&](widgets::DialogResult r) { result = r; });
    CHECK_FALSE(a.focused());
    CHECK_FALSE(b.focused());
    CHECK(stack.handle(input::KeyEvent{input::Key::tab, false}));
    CHECK_FALSE(a.focused());
    CHECK_FALSE(b.focused());
    CHECK(stack.handle(escape));
    CHECK(result == widgets::DialogResult::dismissed);
    CHECK(b.focused());
    CHECK(stack.empty());
}

TEST_CASE("a menu over a dialog gives the focus back in order, and a re-pushed overlay holds it once") {
    widgets::Button a({"A"}, fonts());
    widgets::FocusManager focus;
    focus.add(a);
    widgets::OverlayStack stack;
    stack.set_focus(focus);
    a.set_focused(true);

    widgets::Dialog dialog({"Delete?", "", "Delete", "Cancel"}, fonts());
    dialog.open(window);
    stack.push(dialog, [](widgets::DialogResult) {});
    auto menu = make_menu();
    menu.open({20.0, 20.0, 40.0, 40.0}, window);
    stack.push(menu, [](bool) {});
    stack.push(menu, [](bool) {});
    CHECK_FALSE(a.focused());
    CHECK(stack.handle(escape));
    CHECK_FALSE(menu.is_open());
    CHECK_FALSE(a.focused());
    CHECK(stack.handle(escape));
    CHECK_FALSE(dialog.is_open());
    CHECK(a.focused());
}

TEST_CASE("a modal overlay restores no focus when nothing had it, or when the widget left the order") {
    widgets::Button a({"A"}, fonts());
    widgets::FocusManager focus;
    focus.add(a);
    widgets::OverlayStack stack;
    stack.set_focus(focus);
    widgets::Dialog dialog({"Delete?", "", "Delete", "Cancel"}, fonts());
    dialog.open(window);
    stack.push(dialog, [](widgets::DialogResult) {});
    stack.handle(escape);
    CHECK_FALSE(a.focused());

    a.set_focused(true);
    dialog.open(window);
    stack.push(dialog, [](widgets::DialogResult) {});
    focus.remove(a);
    stack.handle(escape);
    CHECK_FALSE(a.focused());
}

TEST_CASE("an overlay removed from its own result handler leaves the stack after the event and gives the focus back") {
    widgets::Button a({"A"}, fonts());
    widgets::FocusManager focus;
    focus.add(a);
    a.set_focused(true);
    widgets::OverlayStack stack;
    stack.set_focus(focus);
    auto menu = make_menu();
    menu.open({20.0, 20.0, 40.0, 40.0}, window);
    int calls = 0;
    stack.push(menu, [&](bool) {
        ++calls;
        stack.remove(menu);
    });
    CHECK_FALSE(a.focused());
    CHECK(stack.handle(input::KeyEvent{input::Key::a, false}));
    CHECK(calls == 1);
    CHECK(menu.is_open());
    CHECK(stack.empty());
    CHECK(a.focused());
    CHECK_FALSE(stack.handle(input::KeyEvent{input::Key::a, false}));
    CHECK(calls == 1);
}

TEST_CASE("removing an overlay outside an event takes it off at once") {
    auto menu = make_menu();
    menu.open({20.0, 20.0, 40.0, 40.0}, window);
    widgets::OverlayStack stack;
    stack.push(menu, [](bool) {});
    CHECK_FALSE(stack.empty());
    stack.remove(menu);
    CHECK(stack.empty());
    CHECK_FALSE(stack.handle(escape));
    CHECK(menu.is_open());
}

TEST_CASE("an overlay closed by a pointer press drops the focus it held instead of showing it again") {
    widgets::Button a({"A"}, fonts());
    widgets::FocusManager focus;
    focus.add(a);
    a.set_focused(true);
    widgets::OverlayStack stack;
    stack.set_focus(focus);
    widgets::Dialog dialog({"Delete?", "", "Delete", "Cancel"}, fonts());
    dialog.open(window, input::InputSource::keyboard);
    stack.push(dialog, [](widgets::DialogResult) {});
    CHECK_FALSE(a.focused());
    stack.handle(input::PointerEvent{PointerAction::down, 1.0, 1.0});
    CHECK_FALSE(dialog.is_open());
    CHECK(stack.empty());
    CHECK_FALSE(a.focused());

    a.set_focused(true);
    dialog.open(window, input::InputSource::keyboard);
    stack.push(dialog, [](widgets::DialogResult) {});
    stack.handle(escape);
    CHECK(a.focused());
}
