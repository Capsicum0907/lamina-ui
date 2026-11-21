#include <doctest/doctest.h>

#include <stdexcept>
#include <string>

#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/shortcuts.hpp"
#include "lamina/widgets/text_field.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using widgets::KeyChord;

TEST_CASE("chords are written in one place per platform, modifiers in the order Ctrl, Alt, Shift") {
    const auto& pc = tokens::pc_shortcut_format;
    CHECK(widgets::format_chord({Key::s, true}, pc) == "Ctrl+S");
    CHECK(widgets::format_chord({Key::s, true, true}, pc) == "Ctrl+Shift+S");
    CHECK(widgets::format_chord({Key::delete_forward, true, false, true}, pc) == "Ctrl+Alt+Del");
    CHECK(widgets::format_chord({Key::enter, false, false, true}, pc) == "Alt+Enter");
    CHECK(widgets::format_chord({Key::f2}, pc) == "F2");
    CHECK(widgets::format_chord({Key::digit_0, true}, pc) == "Ctrl+0");
    CHECK(widgets::format_chord({Key::plus, true}, pc) == "Ctrl+Plus");
    CHECK(widgets::format_chord({Key::page_down, true}, pc) == "Ctrl+PgDn");
    const auto& mac = tokens::mac_shortcut_format;
    CHECK(widgets::format_chord({Key::s, true, true}, mac) == "⌃⇧S");
    CHECK(widgets::format_chord({Key::backspace, false, false, true}, mac) == "⌥⌫");
#ifndef __APPLE__
    CHECK(widgets::format_chord({Key::z, true}) == "Ctrl+Z");
#endif
}

TEST_CASE("a chord matches only its own modifiers") {
    const KeyChord save{Key::s, true};
    CHECK(save.matches({Key::s, false, true}));
    CHECK_FALSE(save.matches({Key::s, true, true}));
    CHECK_FALSE(save.matches({Key::s, false, false}));
    CHECK_FALSE(save.matches({Key::s, false, true, false, true}));
    CHECK(save.matches({Key::s, false, true, true}));
}

TEST_CASE("the registry runs the command of a chord and gives its label to a menu") {
    widgets::ShortcutRegistry r;
    int saved = 0;
    int opened = 0;
    r.add("save", KeyChord{Key::s, true}, [&] { ++saved; });
    r.add("open", KeyChord{Key::o, true}, [&] { ++opened; });
    r.add("about", std::nullopt, [] {});
    CHECK(r.size() == 3);
    CHECK(r.label("save", tokens::pc_shortcut_format) == "Ctrl+S");
    CHECK(r.label("about", tokens::pc_shortcut_format).empty());
    CHECK(r.match({Key::o, false, true}) == "open");
    CHECK_FALSE(r.match({Key::o, false}));
    CHECK(r.handle({Key::s, false, true}));
    CHECK(saved == 1);
    CHECK_FALSE(r.handle({Key::q, false, true}));
    CHECK(r.run("open"));
    CHECK(opened == 1);
}

TEST_CASE("a disabled command neither runs nor consumes its chord") {
    widgets::ShortcutRegistry r;
    int saved = 0;
    r.add("save", KeyChord{Key::s, true}, [&] { ++saved; });
    r.set_enabled("save", false);
    CHECK_FALSE(r.enabled("save"));
    CHECK_FALSE(r.handle({Key::s, false, true}));
    CHECK_FALSE(r.run("save"));
    CHECK(saved == 0);
    r.set_enabled("save", true);
    CHECK(r.handle({Key::s, false, true}));
    CHECK(saved == 1);
}

TEST_CASE("the registry refuses a second command with the same id or chord") {
    widgets::ShortcutRegistry r;
    r.add("save", KeyChord{Key::s, true}, [] {});
    CHECK_THROWS_AS(r.add("save", std::nullopt, [] {}), std::invalid_argument);
    CHECK_THROWS_AS(r.add("store", KeyChord{Key::s, true}, [] {}), std::invalid_argument);
    CHECK_THROWS_AS(r.add("", std::nullopt, [] {}), std::invalid_argument);
    CHECK_THROWS_AS(r.set_enabled("missing", true), std::out_of_range);
}

TEST_CASE("a focused text field keeps its own Ctrl+Z and clipboard keys and lets other chords through to the registry") {
    widgets::TextField f({.label = "Name"}, fonts());
    f.set_width(300.0);
    f.set_origin({0.0, 0.0});
    widgets::FocusManager focus;
    focus.add(f);
    f.handle({input::PointerAction::down, 10.0, 20.0});
    f.handle(input::TextEvent{input::TextEventKind::commit, "abc", 0});
    widgets::ShortcutRegistry r;
    int undone = 0;
    int saved = 0;
    int copied = 0;
    r.add("undo", KeyChord{Key::z, true}, [&] { ++undone; });
    r.add("save", KeyChord{Key::s, true}, [&] { ++saved; });
    r.add("copy", KeyChord{Key::c, true}, [&] { ++copied; });
    const auto route = [&](const input::KeyEvent& e) {
        const bool claimed = focus.claims_key(e);
        if (r.handle(e, focus)) {
            return true;
        }
        f.handle(e);
        return claimed;
    };
    CHECK(focus.claims_key({Key::z, false, true}));
    CHECK(route({Key::z, false, true}));
    CHECK(f.text().empty());
    CHECK(undone == 0);
    CHECK_FALSE(focus.claims_key({Key::z, false, true}));
    CHECK(route({Key::z, false, true}));
    CHECK(undone == 1);
    CHECK(route({Key::c, false, true}));
    CHECK(copied == 0);
    CHECK(route({Key::s, false, true}));
    CHECK(saved == 1);
    f.set_focused(false);
    CHECK(route({Key::c, false, true}));
    CHECK(copied == 1);
}
