#pragma once

#include <functional>
#include <optional>

#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/widgets/menu.hpp"

namespace lamina::widgets {

constexpr bool requests_context_menu(const input::KeyEvent& e) {
    if (e.ctrl || e.alt) {
        return false;
    }
    return (e.key == input::Key::f10 && e.shift) || (e.key == input::Key::menu && !e.shift);
}

struct ContextMenuAreaOptions {
    std::function<render::Rect()> area;
    std::function<bool()> focused = {};
    std::function<std::optional<render::Point>()> keyboard_point = {};
    std::function<void(render::Point)> before_open = {};
};

class ContextMenuArea {
public:
    ContextMenuArea(Menu& menu, ContextMenuAreaOptions options);

    void set_window(layout::Size window) { window_ = window; }
    Menu& menu() { return menu_; }
    const Menu& menu() const { return menu_; }
    render::Rect area() const { return options_.area(); }

    bool handle(const input::ContextClick& event);
    bool handle(const input::KeyEvent& event);

private:
    void open(render::Point at, input::InputSource source);

    Menu& menu_;
    ContextMenuAreaOptions options_;
    layout::Size window_{0.0, 0.0};
};

}
