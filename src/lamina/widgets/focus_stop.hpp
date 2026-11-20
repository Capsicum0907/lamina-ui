#pragma once

#include <functional>
#include <optional>

#include "lamina/render/geometry.hpp"

namespace lamina::widgets {

class FocusStop {
public:
    struct Hooks {
        std::function<bool()> focused;
        std::function<void(bool)> set_focused;
        std::function<bool()> can_focus;
        std::function<render::Rect()> bounds;
    };

    explicit FocusStop(Hooks hooks) : hooks_(std::move(hooks)) {}
    FocusStop(const FocusStop&) = delete;
    FocusStop& operator=(const FocusStop&) = delete;

    bool focused() const { return hooks_.focused(); }
    void set_focused(bool focused) { hooks_.set_focused(focused); }
    bool can_focus() const { return hooks_.can_focus(); }
    render::Rect bounds() const { return hooks_.bounds(); }

private:
    Hooks hooks_;
};

template <typename W>
FocusStop::Hooks forward_focus(W& widget, std::function<bool()> shown) {
    return {[&widget] { return widget.focused(); }, [&widget](bool f) { widget.set_focused(f); },
            [&widget, shown] { return shown() && widget.can_focus(); }, [&widget] { return widget.bounds(); }};
}

}
