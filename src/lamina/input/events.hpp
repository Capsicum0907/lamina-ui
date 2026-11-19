#pragma once

#include <optional>
#include <string>

namespace lamina::input {

enum class PointerAction { move, down, up, leave };

struct PointerEvent {
    PointerAction action;
    double x_dp;
    double y_dp;
    int clicks = 0;
};

enum class Key {
    backspace,
    delete_forward,
    left,
    right,
    up,
    down,
    home,
    end,
    enter,
    escape,
    tab,
    a,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z,
    digit_0,
    digit_1,
    digit_2,
    digit_3,
    digit_4,
    digit_5,
    digit_6,
    digit_7,
    digit_8,
    digit_9,
    plus,
    minus,
    asterisk,
    slash,
    period,
    percent,
    equals,
};

constexpr std::optional<Key> key_for_character(char32_t c) {
    if (c >= U'a' && c <= U'z') {
        return static_cast<Key>(static_cast<int>(Key::a) + static_cast<int>(c - U'a'));
    }
    if (c >= U'0' && c <= U'9') {
        return static_cast<Key>(static_cast<int>(Key::digit_0) + static_cast<int>(c - U'0'));
    }
    switch (c) {
    case U'+': return Key::plus;
    case U'-': return Key::minus;
    case U'*': return Key::asterisk;
    case U'/': return Key::slash;
    case U'.': return Key::period;
    case U'%': return Key::percent;
    case U'=': return Key::equals;
    default: return std::nullopt;
    }
}

struct KeyEvent {
    Key key;
    bool shift;
    bool ctrl = false;
};

enum class TextEventKind { commit, compose };

struct TextEvent {
    TextEventKind kind;
    std::string text;
    int cursor;
};

}
