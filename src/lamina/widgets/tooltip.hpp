#pragma once

#include <functional>
#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/tooltip.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/popup_transition.hpp"
#include "lamina/widgets/protocol.hpp"

namespace lamina::widgets {

enum class TooltipKind { plain, rich };
enum class TooltipPosition { above, below, left, right };
enum class TooltipTrigger { none, hover, focus, long_press, program };

struct TooltipOptions {
    std::string text;
    TooltipKind kind = TooltipKind::plain;
    std::string title = {};
    std::string action_label = {};
    TooltipPosition position = TooltipPosition::above;
    bool caret = false;
    bool persistent = false;
    double hover_delay_seconds = tokens::tooltip_tokens.hover_show_delay_seconds;
};

struct TooltipResponse {
    bool action = false;
    bool consumed = false;
};

struct TooltipAnchor {
    render::Rect bounds;
    bool focused = false;
};

render::Point tooltip_position(const render::Rect& anchor, layout::Size window, layout::Size tooltip, TooltipPosition position,
                               double spacing);

class Tooltip {
public:
    Tooltip(TooltipOptions options, const text::FontStack& fonts);

    template <typename W>
    void anchor_to(const W& widget) {
        anchor_ = [&widget] {
            TooltipAnchor a{widget.bounds()};
            if constexpr (Focusable<W>) {
                a.focused = widget.focused();
            }
            return a;
        };
    }
    void set_anchor(const render::Rect& bounds);
    void set_window(layout::Size window) { window_ = window; }

    void show();
    void dismiss();
    bool is_open() const { return popup_.is_open(); }
    bool visible() const { return popup_.visible(); }
    double opacity() const { return popup_.opacity(); }
    double scale() const { return popup_.scale(); }
    TooltipTrigger trigger() const { return trigger_; }
    render::Rect bounds() const;
    std::optional<render::Rect> caret() const;
    std::optional<render::Rect> action_bounds() const;
    void set_text(std::string text);

    bool focused() const { return action_ && action_->focused(); }
    void set_focused(bool focused);
    bool can_focus() const { return popup_.is_open() && action_.has_value(); }

    TooltipResponse handle(const input::PointerEvent& event);
    TooltipResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    std::optional<double> next_wake_seconds() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const;

private:
    void open(TooltipTrigger trigger);
    void relayout();
    TooltipAnchor anchor() const;
    double spacing() const;

    TooltipOptions options_;
    Label text_;
    Label title_;
    std::optional<Button> action_;
    std::function<TooltipAnchor()> anchor_;
    layout::Size window_{layout::unbounded, layout::unbounded};
    layout::Size size_{0.0, 0.0};
    render::Point origin_{0.0, 0.0};
    render::Rect placed_anchor_{0.0, 0.0, 0.0, 0.0};
    PopupTransition popup_;
    TooltipTrigger trigger_ = TooltipTrigger::none;
    bool hovered_ = false;
    bool anchor_focused_ = false;
    double hover_wait_ = -1.0;
    double long_press_wait_ = -1.0;
    double time_left_ = -1.0;
};

template <typename W>
Tooltip name_tooltip(const W& anchor, const text::FontStack& fonts, TooltipOptions options = {}) {
    options.text = anchor.accessible().name;
    Tooltip tooltip(std::move(options), fonts);
    tooltip.anchor_to(anchor);
    return tooltip;
}

}
