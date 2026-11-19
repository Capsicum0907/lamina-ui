#include <doctest/doctest.h>

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
}

TEST_CASE("key_for_character leaves other characters unmapped") {
    CHECK_FALSE(key_for_character(U'A').has_value());
    CHECK_FALSE(key_for_character(U' ').has_value());
    CHECK_FALSE(key_for_character(U',').has_value());
    CHECK_FALSE(key_for_character(U'\u00d7').has_value());
    CHECK_FALSE(key_for_character(U'\u3042').has_value());
}
