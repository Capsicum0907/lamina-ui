#pragma once

#include <string>

namespace lamina::platform {

enum class PointerAction { move, down, up, leave };

struct PointerEvent {
    PointerAction action;
    double x_dp;
    double y_dp;
};

enum class Key { backspace, delete_forward, left, right, home, end, enter, escape, tab };

struct KeyEvent {
    Key key;
    bool shift;
};

enum class TextEventKind { commit, compose };

struct TextEvent {
    TextEventKind kind;
    std::string text;
    int cursor;
};

}
