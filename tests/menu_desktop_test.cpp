#include <doctest/doctest.h>

#include <stdexcept>

#include "lamina/text/layout.hpp"
#include "lamina/widgets/menu.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::KeyChord;

namespace {

constexpr layout::Size window{800.0, 600.0};
const auto& t = tokens::menu_tokens;

widgets::MenuOptions file_menu() {
    return {.items = {{.label = "New", .shortcut = KeyChord{Key::n, true}, .command = "new"},
                      {.label = "Open", .shortcut = KeyChord{Key::o, true}, .command = "open"},
                      {.label = "Recent",
                       .submenu = {{.label = "notes.txt", .command = "recent-1"}, {.label = "plan.md", .command = "recent-2"}}},
                      {.label = "Save as", .shortcut = KeyChord{Key::s, true, true}, .command = "save-as", .divider_before = true},
                      {.label = "Quit", .command = "quit", .divider_before = true}},
            .accessible_name = "File",
            .icon_font = icon_font(),
            .shortcut_format = &tokens::pc_shortcut_format};
}

void press_key(widgets::Menu& m, Key k, bool shift = false) {
    m.handle(input::KeyEvent{k, shift});
}

}

TEST_CASE("a menu item shows its shortcut at the end in the trailing color and makes room for it") {
    widgets::Menu plain({{{"Save"}}}, fonts());
    widgets::Menu m({.items = {{.label = "Save", .shortcut = KeyChord{Key::s, true, true}}}, .shortcut_format = &tokens::pc_shortcut_format},
                    fonts());
    CHECK(m.shortcut_label(0) == "Ctrl+Shift+S");
    const auto style = tokens::text_style(tokens::type_style(t.label));
    const double label = text::layout_line(fonts(), "Save", style).width;
    const double shortcut = text::layout_line(fonts(), "Ctrl+Shift+S", style).width;
    CHECK(m.width() == doctest::Approx(std::max(t.item_min_width, 2.0 * t.item_horizontal_padding + label + t.trailing_space + shortcut)));
    CHECK(m.width() > plain.width());
    CHECK(t.trailing == color::Role::on_surface_variant);
    CHECK(t.trailing_space == 12.0);
    CHECK(t.trailing_icon_size == 24.0);
    m.open({10.0, 10.0, 40.0, 40.0}, window);
    settle(m);
    const auto node = m.accessible();
    REQUIRE(node);
    CHECK(node->children[0].description == "Ctrl+Shift+S");
}

TEST_CASE("dividers add their padding between items") {
    widgets::Menu m(file_menu(), fonts());
    const double divider = 2.0 * t.divider_vertical_padding + t.divider_thickness;
    CHECK(divider == 5.0);
    CHECK(m.height() == doctest::Approx(2.0 * t.vertical_padding + 5.0 * t.item_height + 2.0 * divider));
    m.open({10.0, 10.0, 40.0, 40.0}, window);
    CHECK(m.item_rect(3).y - (m.item_rect(2).y + m.item_rect(2).height) == doctest::Approx(divider));
    CHECK(m.item_rect(2).y - (m.item_rect(1).y + m.item_rect(1).height) == doctest::Approx(0.0));
}

TEST_CASE("a submenu needs an icon font for its arrow") {
    widgets::MenuOptions o = file_menu();
    o.icon_font.reset();
    CHECK_THROWS_AS(widgets::Menu(std::move(o), fonts()), std::invalid_argument);
}

TEST_CASE("Right opens a submenu with its first item focused, Left and Escape close only the submenu") {
    widgets::Menu m(file_menu(), fonts());
    m.open({10.0, 10.0, 40.0, 40.0}, window, input::InputSource::keyboard);
    settle(m);
    REQUIRE(m.focused_item() == 0u);
    press_key(m, Key::down);
    press_key(m, Key::down);
    CHECK(m.focused_item() == 2u);
    CHECK(m.has_submenu(2));
    press_key(m, Key::right);
    REQUIRE(m.open_submenu() == 2u);
    const widgets::Menu& sub = *m.submenu(2);
    CHECK(sub.is_open());
    CHECK(sub.focused_item() == 0u);
    CHECK(sub.bounds().x == doctest::Approx(m.bounds().x + m.width()));
    CHECK(sub.bounds().y == doctest::Approx(m.item_rect(2).y));
    press_key(m, Key::left);
    CHECK_FALSE(m.open_submenu());
    CHECK(m.is_open());
    CHECK(m.focused_item() == 2u);
    press_key(m, Key::enter);
    REQUIRE(m.open_submenu() == 2u);
    press_key(m, Key::escape);
    CHECK_FALSE(m.open_submenu());
    CHECK(m.is_open());
    press_key(m, Key::right);
    press_key(m, Key::down);
    widgets::MenuResponse r = m.handle(input::KeyEvent{Key::enter, false});
    CHECK(r.consumed);
    CHECK_FALSE(r.chosen);
    CHECK(r.path == std::vector<std::size_t>{2, 1});
    CHECK(r.command == "recent-2");
    CHECK_FALSE(m.is_open());
    CHECK_FALSE(m.submenu(2)->is_open());
}

TEST_CASE("Tab closes the whole menu from inside a submenu") {
    widgets::Menu m(file_menu(), fonts());
    m.open({10.0, 10.0, 40.0, 40.0}, window, input::InputSource::keyboard);
    press_key(m, Key::r);
    REQUIRE(m.open_submenu() == 2u);
    press_key(m, Key::tab);
    CHECK_FALSE(m.is_open());
}

TEST_CASE("choosing a direct item reports its command and path") {
    widgets::Menu m(file_menu(), fonts());
    m.open({10.0, 10.0, 40.0, 40.0}, window, input::InputSource::keyboard);
    const widgets::MenuResponse r = m.handle(input::KeyEvent{Key::o, false});
    CHECK(r.chosen == 1u);
    CHECK(r.path == std::vector<std::size_t>{1});
    CHECK(r.command == "open");
}

TEST_CASE("hovering an item with a submenu opens it after the hover delay, and hovering another item closes it after the delay") {
    widgets::Menu m(file_menu(), fonts());
    m.open({10.0, 10.0, 40.0, 40.0}, window);
    settle(m);
    const render::Rect recent = m.item_rect(2);
    m.handle({PointerAction::move, recent.x + 20.0, recent.y + 10.0});
    for (double s = 0.0; s < t.submenu_open_seconds - 0.05; s += frame_seconds) {
        m.step(frame_seconds);
    }
    CHECK_FALSE(m.open_submenu());
    for (int i = 0; i < 6; ++i) {
        m.step(frame_seconds);
    }
    REQUIRE(m.open_submenu() == 2u);
    CHECK_FALSE(m.submenu(2)->focused_item());
    const render::Rect inside = m.submenu(2)->item_rect(0);
    m.handle({PointerAction::move, inside.x + 10.0, inside.y + 10.0});
    for (int i = 0; i < 60; ++i) {
        m.step(frame_seconds);
    }
    CHECK(m.open_submenu() == 2u);
    const render::Rect open = m.item_rect(1);
    m.handle({PointerAction::move, open.x + 10.0, open.y + 10.0});
    for (int i = 0; i < 20; ++i) {
        m.step(frame_seconds);
    }
    CHECK(m.open_submenu() == 2u);
    for (int i = 0; i < 10; ++i) {
        m.step(frame_seconds);
    }
    CHECK_FALSE(m.open_submenu());
    CHECK(t.submenu_open_seconds == 0.4);
    CHECK(t.submenu_close_seconds == 0.4);
}

TEST_CASE("pressing an item with a submenu opens it at once, and clicking in the submenu chooses through it") {
    widgets::Menu m(file_menu(), fonts());
    m.open({10.0, 10.0, 40.0, 40.0}, window);
    settle(m);
    const render::Rect recent = m.item_rect(2);
    m.handle({PointerAction::down, recent.x + 10.0, recent.y + 10.0});
    REQUIRE(m.open_submenu() == 2u);
    m.handle({PointerAction::up, recent.x + 10.0, recent.y + 10.0});
    CHECK(m.is_open());
    const render::Rect plan = m.submenu(2)->item_rect(1);
    CHECK(m.contains_point(plan.x + 5.0, plan.y + 5.0));
    m.handle({PointerAction::move, plan.x + 5.0, plan.y + 5.0});
    m.handle({PointerAction::down, plan.x + 5.0, plan.y + 5.0});
    const widgets::MenuResponse r = m.handle({PointerAction::up, plan.x + 5.0, plan.y + 5.0});
    CHECK(r.command == "recent-2");
    CHECK(r.path == std::vector<std::size_t>{2, 1});
    CHECK_FALSE(m.is_open());
}

TEST_CASE("a press outside the menu and its open submenu closes both") {
    widgets::Menu m(file_menu(), fonts());
    m.open({10.0, 10.0, 40.0, 40.0}, window, input::InputSource::keyboard);
    press_key(m, Key::r);
    REQUIRE(m.open_submenu());
    const widgets::MenuResponse r = m.handle({PointerAction::down, 790.0, 590.0});
    CHECK(r.consumed);
    CHECK_FALSE(m.is_open());
    CHECK_FALSE(m.submenu(2)->is_open());
}

TEST_CASE("activate_at chooses a leaf item under the point, for press, drag and release from a menu bar") {
    widgets::Menu m(file_menu(), fonts());
    m.open({10.0, 10.0, 40.0, 40.0}, window);
    const render::Rect save = m.item_rect(3);
    CHECK(m.item_at(save.x + 4.0, save.y + 4.0) == 3u);
    CHECK(m.activate_at(m.item_rect(2).x + 4.0, m.item_rect(2).y + 4.0).path.empty());
    const widgets::MenuResponse r = m.activate_at(save.x + 4.0, save.y + 4.0);
    CHECK(r.command == "save-as");
    CHECK_FALSE(m.is_open());
}

TEST_CASE("a context menu opens at the point and flips or clamps to stay 4 dp inside the window") {
    const layout::Size menu{200.0, 300.0};
    CHECK(t.context_window_margin == 4.0);
    const render::Point free = widgets::context_menu_position({100.0, 100.0}, window, menu);
    CHECK(free.x == 100.0);
    CHECK(free.y == 100.0);
    const render::Point right = widgets::context_menu_position({700.0, 100.0}, window, menu);
    CHECK(right.x == 500.0);
    CHECK(right.y == 100.0);
    const render::Point bottom = widgets::context_menu_position({100.0, 500.0}, window, menu);
    CHECK(bottom.y == 200.0);
    const render::Point corner = widgets::context_menu_position({799.0, 599.0}, window, menu);
    CHECK(corner.x == 599.0);
    CHECK(corner.y == 299.0);
    const render::Point tall = widgets::context_menu_position({100.0, 200.0}, {800.0, 250.0}, menu);
    CHECK(tall.y == 4.0);
    const render::Point narrow = widgets::context_menu_position({150.0, 10.0}, {300.0, 600.0}, menu);
    CHECK(narrow.x == 4.0);
    widgets::Menu m(file_menu(), fonts());
    m.open_at({790.0, 590.0}, window);
    CHECK(m.bounds().x + m.width() <= 790.0);
    CHECK(m.bounds().y + m.height() <= 590.0);
    CHECK(m.transform_origin().x == doctest::Approx(m.bounds().x + m.width()));
    CHECK(m.transform_origin().y == doctest::Approx(m.bounds().y + m.height()));
}

TEST_CASE("a submenu opens on the left of its item when there is no room on the right") {
    const render::Rect item{600.0, 100.0, 180.0, 48.0};
    const render::Point p = widgets::submenu_position(item, window, {150.0, 200.0});
    CHECK(p.x == 450.0);
    CHECK(p.y == 100.0);
    const render::Point low = widgets::submenu_position({10.0, 500.0, 180.0, 48.0}, window, {150.0, 200.0});
    CHECK(low.x == 190.0);
    CHECK(low.y == 396.0);
}

TEST_CASE("the density of a menu reaches its submenus") {
    widgets::Menu m(file_menu(), fonts());
    m.set_density(tokens::Density{-3});
    CHECK(m.item_height() == 36.0);
    CHECK(m.submenu(2)->item_height() == 36.0);
}
