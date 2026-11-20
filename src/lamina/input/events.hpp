#pragma once

#include <optional>
#include <string>

namespace lamina::input {

enum class PointerAction { move, down, up, leave };

enum class InputSource { pointer, keyboard };

enum class PointerKind { mouse, touch, pen };

struct PointerEvent {
    PointerAction action;
    double x_dp;
    double y_dp;
    int clicks = 0;
    PointerKind kind = PointerKind::mouse;
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
    paren_left,
    paren_right,
    caret,
    exclamation,
    space,
    page_up,
    page_down,
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
    case U'(': return Key::paren_left;
    case U')': return Key::paren_right;
    case U'^': return Key::caret;
    case U'!': return Key::exclamation;
    default: return std::nullopt;
    }
}

constexpr std::optional<char32_t> character_for_key(Key key) {
    const int k = static_cast<int>(key);
    if (k >= static_cast<int>(Key::a) && k <= static_cast<int>(Key::z)) {
        return static_cast<char32_t>(U'a' + (k - static_cast<int>(Key::a)));
    }
    if (k >= static_cast<int>(Key::digit_0) && k <= static_cast<int>(Key::digit_9)) {
        return static_cast<char32_t>(U'0' + (k - static_cast<int>(Key::digit_0)));
    }
    switch (key) {
    case Key::plus: return U'+';
    case Key::minus: return U'-';
    case Key::asterisk: return U'*';
    case Key::slash: return U'/';
    case Key::period: return U'.';
    case Key::percent: return U'%';
    case Key::equals: return U'=';
    case Key::paren_left: return U'(';
    case Key::paren_right: return U')';
    case Key::caret: return U'^';
    case Key::exclamation: return U'!';
    default: return std::nullopt;
    }
}

struct WheelEvent {
    double x_dp;
    double y_dp;
    double dx;
    double dy;
};

struct KeyEvent {
    Key key;
    bool shift;
    bool ctrl = false;
    bool repeat = false;
    bool alt = false;
};

enum class TextEventKind { commit, compose };

struct TextEvent {
    TextEventKind kind;
    std::string text;
    int cursor;
};

}
