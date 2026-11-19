#pragma once

#include <string>

namespace lamina::input {

enum class PointerAction { move, down, up, leave };

struct PointerEvent {
    PointerAction action;
    double x_dp;
    double y_dp;
    int clicks = 0;
};

enum class Key { backspace, delete_forward, left, right, up, down, home, end, enter, escape, tab, a, c, v, x };

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
