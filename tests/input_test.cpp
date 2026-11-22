#include <doctest/doctest.h>

#include <string_view>

#include "lamina/input/events.hpp"

using lamina::input::Key;
using lamina::input::key_for_character;

TEST_CASE("key_for_character maps digits, letters and the operator characters") {
    CHECK(key_for_character(U'0') == Key::digit_0);
    CHECK(key_for_character(U'5') == Key::digit_5);
    CHECK(key_for_character(U'9') == Key::digit_9);
    CHECK(key_for_character(U'a') == Key::a);
    CHECK(key_for_character(U'v') == Key::v);
    CHECK(key_for_character(U'z') == Key::z);
    CHECK(key_for_character(U'+') == Key::plus);
    CHECK(key_for_character(U'-') == Key::minus);
    CHECK(key_for_character(U'*') == Key::asterisk);
    CHECK(key_for_character(U'/') == Key::slash);
    CHECK(key_for_character(U'.') == Key::period);
    CHECK(key_for_character(U'%') == Key::percent);
    CHECK(key_for_character(U'=') == Key::equals);
    CHECK(key_for_character(U'(') == Key::paren_left);
    CHECK(key_for_character(U')') == Key::paren_right);
    CHECK(key_for_character(U'^') == Key::caret);
    CHECK(key_for_character(U'!') == Key::exclamation);
}

TEST_CASE("key_for_character leaves other characters unmapped") {
    CHECK_FALSE(key_for_character(U'A').has_value());
    CHECK_FALSE(key_for_character(U' ').has_value());
    CHECK_FALSE(key_for_character(U',').has_value());
    CHECK_FALSE(key_for_character(U'\u00d7').has_value());
    CHECK_FALSE(key_for_character(U'\u3042').has_value());
}

TEST_CASE("character_for_key is the inverse of key_for_character") {
    using lamina::input::character_for_key;
    for (char32_t c : std::u32string_view(U"abcxyz0189+-*/.%=()^!")) {
        CHECK(character_for_key(*key_for_character(c)) == c);
    }
    CHECK_FALSE(character_for_key(Key::enter).has_value());
    CHECK_FALSE(character_for_key(Key::space).has_value());
    CHECK_FALSE(character_for_key(Key::page_down).has_value());
}

TEST_CASE("only a touch is followed by a leave when it lifts, so the mouse and the pen keep their hover") {
    CHECK(lamina::input::leaves_after_up(lamina::input::PointerKind::touch));
    CHECK_FALSE(lamina::input::leaves_after_up(lamina::input::PointerKind::mouse));
    CHECK_FALSE(lamina::input::leaves_after_up(lamina::input::PointerKind::pen));
}
