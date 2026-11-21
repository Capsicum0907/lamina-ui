#include <doctest/doctest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "lamina/widgets/menu_bar.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::KeyChord;

namespace {

constexpr layout::Size window{1000.0, 700.0};

struct Fixture {
    widgets::ShortcutRegistry commands;
    std::vector<std::string> ran;
    widgets::MenuBar bar;

    Fixture() : bar(make()) {
        bar.set_width(1000.0);
        bar.set_origin({0.0, 0.0});
        bar.set_window(window);
    }

    widgets::MenuBar make() {
        for (const auto& [id, chord] : std::vector<std::pair<std::string, std::optional<KeyChord>>>{
                 {"new", KeyChord{Key::n, true}}, {"save", KeyChord{Key::s, true}}, {"quit", std::nullopt},
                 {"undo", KeyChord{Key::z, true}}, {"copy", KeyChord{Key::c, true}}, {"compact", std::nullopt}, {"dense", std::nullopt}}) {
            commands.add(id, chord, [this, id] { ran.push_back(id); });
        }
        return widgets::MenuBar({.menus = {{"File", {{.label = "New", .command = "new"}, {.label = "Save", .command = "save"}, {.label = "Quit", .command = "quit"}}},
                                           {"Edit", {{.label = "Undo", .command = "undo"}, {.label = "Copy", .command = "copy"}}},
                                           {"View", {{.label = "Density", .submenu = {{.label = "Compact", .command = "compact"}, {.label = "Dense", .command = "dense"}}}}}},
                                 .accessible_name = "Menu bar",
                                 .icon_font = icon_font(),
                                 .commands = &commands,
                                 .shortcut_format = &tokens::pc_shortcut_format},
                                fonts());
    }

    void key(Key k, bool shift = false, bool ctrl = false, bool alt = false) { bar.handle(input::KeyEvent{k, shift, ctrl, false, alt}); }
    void move(std::size_t i) {
        const render::Rect r = bar.item_rect(i);
        bar.handle({PointerAction::move, r.x + r.width / 2.0, r.y + r.height / 2.0});
    }
    void press(std::size_t i) {
        const render::Rect r = bar.item_rect(i);
        bar.handle({PointerAction::down, r.x + r.width / 2.0, r.y + r.height / 2.0});
        bar.handle({PointerAction::up, r.x + r.width / 2.0, r.y + r.height / 2.0});
    }
};

}

TEST_CASE("the menu bar lays out its menus in a row and takes its shortcut labels from the registry") {
    Fixture f;
    CHECK(f.bar.height() == tokens::menu_bar_tokens.height);
    CHECK(f.bar.count() == 3);
    CHECK(f.bar.item_rect(0).x == tokens::menu_bar_tokens.start_padding);
    CHECK(f.bar.item_rect(1).x == doctest::Approx(f.bar.item_rect(0).x + f.bar.item_rect(0).width));
    CHECK(f.bar.menu(0).shortcut_label(0) == "Ctrl+N");
    CHECK(f.bar.menu(0).shortcut_label(1) == "Ctrl+S");
    CHECK(f.bar.menu(0).shortcut_label(2).empty());
    CHECK(f.bar.menu(1).shortcut_label(0) == "Ctrl+Z");
    CHECK_THROWS_AS(widgets::MenuBar({.menus = {{"File", {{.label = "New"}}}}}, fonts()), std::invalid_argument);
}

TEST_CASE("Alt or F10 focuses the bar, arrows move along it and Down opens the menu with its first item focused") {
    Fixture f;
    f.key(Key::alt);
    CHECK(f.bar.active());
    CHECK(f.bar.highlighted() == 0u);
    CHECK(f.bar.access_keys_shown());
    f.key(Key::right);
    CHECK(f.bar.highlighted() == 1u);
    f.key(Key::left);
    f.key(Key::left);
    CHECK(f.bar.highlighted() == 2u);
    f.key(Key::home);
    f.key(Key::down);
    REQUIRE(f.bar.open_menu() == 0u);
    CHECK(f.bar.menu(0).focused_item() == 0u);
    CHECK(f.bar.menu(0).bounds().y == doctest::Approx(f.bar.item_rect(0).y + f.bar.item_rect(0).height));
    f.key(Key::f10);
    CHECK_FALSE(f.bar.open_menu());
    CHECK_FALSE(f.bar.active());
    f.key(Key::f10);
    CHECK(f.bar.active());
    f.key(Key::escape);
    CHECK_FALSE(f.bar.active());
}

TEST_CASE("Left and Right inside an open menu move to the next menu, and Right opens a submenu first") {
    Fixture f;
    f.key(Key::alt);
    f.key(Key::enter);
    REQUIRE(f.bar.open_menu() == 0u);
    f.key(Key::right);
    REQUIRE(f.bar.open_menu() == 1u);
    CHECK_FALSE(f.bar.menu(0).is_open());
    CHECK(f.bar.menu(1).focused_item() == 0u);
    f.key(Key::right);
    REQUIRE(f.bar.open_menu() == 2u);
    f.key(Key::right);
    CHECK(f.bar.open_menu() == 2u);
    CHECK(f.bar.menu(2).open_submenu() == 0u);
    f.key(Key::left);
    CHECK(f.bar.open_menu() == 2u);
    CHECK_FALSE(f.bar.menu(2).open_submenu());
    f.key(Key::left);
    CHECK(f.bar.open_menu() == 1u);
    f.key(Key::left);
    f.key(Key::left);
    CHECK(f.bar.open_menu() == 2u);
}

TEST_CASE("Escape closes the menu and leaves the bar focused, a second Escape leaves the bar") {
    Fixture f;
    f.key(Key::e, false, false, true);
    REQUIRE(f.bar.open_menu() == 1u);
    f.key(Key::escape);
    CHECK_FALSE(f.bar.open_menu());
    CHECK(f.bar.active());
    CHECK(f.bar.highlighted() == 1u);
    f.key(Key::escape);
    CHECK_FALSE(f.bar.active());
}

TEST_CASE("Alt and a letter open that menu from anywhere, and a letter in the menu runs its command") {
    Fixture f;
    CHECK_FALSE(f.bar.handle(input::KeyEvent{Key::f, false}).consumed);
    CHECK_FALSE(f.bar.handle(input::KeyEvent{Key::x, false, false, false, true}).consumed);
    f.key(Key::f, false, false, true);
    REQUIRE(f.bar.open_menu() == 0u);
    const widgets::MenuBarResponse r = f.bar.handle(input::KeyEvent{Key::s, false});
    CHECK(r.menu == 0u);
    CHECK(r.command == "save");
    CHECK(f.ran == std::vector<std::string>{"save"});
    CHECK_FALSE(f.bar.open_menu());
    CHECK_FALSE(f.bar.active());
}

TEST_CASE("a submenu choice from the bar reports the full path and runs its command") {
    Fixture f;
    f.key(Key::v, false, false, true);
    f.key(Key::right);
    f.key(Key::down);
    const widgets::MenuBarResponse r = f.bar.handle(input::KeyEvent{Key::enter, false});
    CHECK(r.path == std::vector<std::size_t>{0, 1});
    CHECK(r.command == "dense");
    CHECK(f.ran == std::vector<std::string>{"dense"});
}

TEST_CASE("clicking a menu opens it, hovering another while one is open switches, clicking it again closes") {
    Fixture f;
    f.move(1);
    CHECK_FALSE(f.bar.open_menu());
    f.press(0);
    REQUIRE(f.bar.open_menu() == 0u);
    CHECK_FALSE(f.bar.active());
    CHECK_FALSE(f.bar.menu(0).focused_item());
    f.move(1);
    CHECK(f.bar.open_menu() == 1u);
    CHECK_FALSE(f.bar.menu(0).is_open());
    f.move(2);
    CHECK(f.bar.open_menu() == 2u);
    f.press(2);
    CHECK_FALSE(f.bar.open_menu());
}

TEST_CASE("pressing a menu, dragging to an item and releasing chooses it") {
    Fixture f;
    const render::Rect file = f.bar.item_rect(0);
    f.bar.handle({PointerAction::down, file.x + 5.0, file.y + 5.0});
    REQUIRE(f.bar.open_menu() == 0u);
    const render::Rect save = f.bar.menu(0).item_rect(1);
    f.bar.handle({PointerAction::move, save.x + 10.0, save.y + 10.0});
    const widgets::MenuBarResponse r = f.bar.handle({PointerAction::up, save.x + 10.0, save.y + 10.0});
    CHECK(r.command == "save");
    CHECK(f.ran == std::vector<std::string>{"save"});
}

TEST_CASE("a click outside an open menu closes it, and disabled commands show as disabled items") {
    Fixture f;
    f.commands.set_enabled("copy", false);
    f.press(1);
    REQUIRE(f.bar.open_menu() == 1u);
    CHECK(f.bar.menu(1).item_enabled(0));
    CHECK_FALSE(f.bar.menu(1).item_enabled(1));
    f.bar.handle({PointerAction::down, 900.0, 600.0});
    CHECK_FALSE(f.bar.open_menu());
    f.commands.set_enabled("copy", true);
    f.bar.refresh_commands();
    CHECK(f.bar.menu(1).item_enabled(1));
}

TEST_CASE("the menu bar reports a menu bar with an item per menu and the open menu inside it") {
    Fixture f;
    f.press(0);
    const widgets::AccessibleNode n = f.bar.accessible();
    CHECK(n.role == widgets::AccessibleRole::menu_bar);
    REQUIRE(n.children.size() == 3);
    CHECK(n.children[0].name == "File");
    CHECK(n.children[0].value == "Expanded");
    REQUIRE(n.children[0].children.size() == 1);
    CHECK(n.children[0].children[0].role == widgets::AccessibleRole::menu);
    CHECK(n.children[1].value == "Collapsed");
}

TEST_CASE("the density lowers the bar and its menus") {
    Fixture f;
    f.bar.set_density(tokens::Density{-3});
    CHECK(f.bar.height() == 28.0);
    CHECK(f.bar.menu(0).item_height() == 36.0);
    f.bar.set_density(tokens::Density{-9});
    CHECK(f.bar.height() == 28.0);
}

TEST_CASE("a disabled bar ignores keys and pointer") {
    Fixture f;
    f.bar.set_enabled(false);
    f.key(Key::alt);
    CHECK_FALSE(f.bar.active());
    f.press(0);
    CHECK_FALSE(f.bar.open_menu());
}
