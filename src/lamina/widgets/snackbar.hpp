#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/snackbar.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/popup_transition.hpp"

namespace lamina::widgets {

struct SnackbarOptions {
    std::string action_label;
    double duration_seconds = tokens::snackbar_tokens.short_duration_seconds;
};

struct SnackbarResponse {
    bool action = false;
    bool consumed = false;
};

class Snackbar {
public:
    Snackbar(SnackbarOptions options, const text::FontStack& fonts);
    Snackbar(const Snackbar&) = delete;
    Snackbar& operator=(const Snackbar&) = delete;

    void show(std::string message);
    void dismiss();
    void set_area(const render::Rect& area);
    bool is_open() const { return popup_.is_open(); }
    bool visible() const { return popup_.visible(); }
    double opacity() const { return popup_.opacity(); }
    double scale() const { return popup_.scale(); }
    render::Rect bounds() const { return surface_; }
    std::optional<render::Rect> action_bounds() const;
    double remaining_seconds() const { return remaining_; }
    bool focused() const { return action_ && action_->focused(); }
    void set_focused(bool focused);
    bool can_focus() const { return popup_.is_open() && action_.has_value(); }

    SnackbarResponse handle(const input::PointerEvent& event);
    SnackbarResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    double next_wake_seconds() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const {
        if (!popup_.is_open()) {
            return std::nullopt;
        }
        AccessibleNode node{AccessibleRole::status, message_.text(), surface_};
        if (action_) {
            node.children.push_back(action_->accessible());
        }
        return node;
    }

private:
    void relayout();

    SnackbarOptions options_;
    Label message_;
    std::optional<Button> action_;
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    render::Rect surface_{0.0, 0.0, 0.0, 0.0};
    PopupTransition popup_;
    double remaining_ = 0.0;
    bool hovered_ = false;
};

}
