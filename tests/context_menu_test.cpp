#include <doctest/doctest.h>

#include "lamina/widgets/context_menu.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;

namespace {

constexpr layout::Size window{800.0, 600.0};

widgets::Menu make() {
    return widgets::Menu({.items = {{.label = "Cut", .command = "cut"}, {.label = "Copy", .command = "copy"}, {.label = "Paste", .command = "paste"}},
                          .accessible_name = "Context"},
                         fonts());
}

}

TEST_CASE("Shift+F10 and the Menu key ask for a context menu") {
    CHECK(widgets::requests_context_menu({Key::f10, true}));
    CHECK_FALSE(widgets::requests_context_menu({Key::f10, false}));
    CHECK_FALSE(widgets::requests_context_menu({Key::f10, true, true}));
    CHECK(widgets::requests_context_menu({Key::menu, false}));
    CHECK_FALSE(widgets::requests_context_menu({Key::menu, false, false, false, true}));
}

TEST_CASE("a right click inside the area opens the menu at the pointer") {
    widgets::Menu m = make();
    render::Point seen{0.0, 0.0};
    widgets::ContextMenuArea area(m, {.area = [] { return render::Rect{100.0, 100.0, 300.0, 200.0}; },
                                      .before_open = [&](render::Point p) { seen = p; }});
    area.set_window(window);
    CHECK_FALSE(area.handle(input::ContextClick{50.0, 50.0}));
    CHECK_FALSE(m.is_open());
    CHECK(area.handle(input::ContextClick{150.0, 160.0}));
    CHECK(m.is_open());
    CHECK(seen.x == 150.0);
    CHECK(m.bounds().x == 150.0);
    CHECK(m.bounds().y == 160.0);
    CHECK_FALSE(m.focused_item());
}

TEST_CASE("a right click near the window's corner keeps the whole menu inside the window") {
    widgets::Menu m = make();
    widgets::ContextMenuArea area(m, {.area = [] { return render::Rect{0.0, 0.0, 800.0, 600.0}; }});
    area.set_window(window);
    CHECK(area.handle(input::ContextClick{795.0, 598.0}));
    const render::Rect b = m.bounds();
    CHECK(b.x + b.width == doctest::Approx(795.0));
    CHECK(b.y + b.height == doctest::Approx(598.0));
    CHECK(b.x >= 4.0);
    CHECK(b.y >= 4.0);
    widgets::Menu n = make();
    widgets::ContextMenuArea small(n, {.area = [] { return render::Rect{0.0, 0.0, 120.0, 100.0}; }});
    small.set_window({120.0, 100.0});
    CHECK(small.handle(input::ContextClick{60.0, 50.0}));
    CHECK(n.bounds().x == 4.0);
    CHECK(n.bounds().y == 4.0);
}

TEST_CASE("the keyboard opens the menu only while the area has the focus, with the first item focused") {
    widgets::Menu m = make();
    bool focused = false;
    widgets::ContextMenuArea area(m, {.area = [] { return render::Rect{100.0, 100.0, 300.0, 200.0}; },
                                      .focused = [&] { return focused; },
                                      .keyboard_point = [] { return std::optional<render::Point>(render::Point{120.0, 140.0}); }});
    area.set_window(window);
    CHECK_FALSE(area.handle(input::KeyEvent{Key::menu, false}));
    focused = true;
    CHECK(area.handle(input::KeyEvent{Key::f10, true}));
    CHECK(m.is_open());
    CHECK(m.bounds().x == 120.0);
    CHECK(m.bounds().y == 140.0);
    CHECK(m.focused_item() == 0u);
    CHECK_FALSE(area.handle(input::KeyEvent{Key::menu, false}));
}

TEST_CASE("without a keyboard point the menu opens at the area's top left") {
    widgets::Menu m = make();
    widgets::ContextMenuArea area(m, {.area = [] { return render::Rect{100.0, 100.0, 300.0, 200.0}; }, .focused = [] { return true; }});
    area.set_window(window);
    CHECK(area.handle(input::KeyEvent{Key::menu, false}));
    CHECK(m.bounds().x == 100.0);
    CHECK(m.bounds().y == 100.0);
}
