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
