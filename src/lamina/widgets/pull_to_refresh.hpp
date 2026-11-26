#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/pull_to_refresh.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/loading_indicator.hpp"

namespace lamina::widgets {

enum class PullIndicatorStyle { expressive, arrow };

struct PullToRefreshOptions {
    std::string accessible_name = {};
    bool enabled = true;
    const tokens::PullToRefreshTokens* tokens = nullptr;
    PullIndicatorStyle style = PullIndicatorStyle::expressive;
    const tokens::PullArrowTokens* arrow_tokens = nullptr;
};

struct PullArrow {
    double start_degrees = 0.0;
    double end_degrees = 0.0;
    double scale = 0.0;
};

PullArrow pull_arrow(double fraction, const tokens::PullArrowTokens& tokens);

struct PullToRefreshResponse {
    bool refresh = false;
    bool claimed = false;
    bool consumed = false;
};

double pull_offset(double pulled, double threshold, double max_tension);
double pull_distance(double offset, double threshold, double max_tension);

class PullToRefresh {
public:
    explicit PullToRefresh(PullToRefreshOptions options = {});

    void set_area(const render::Rect& area) { area_ = area; }
    const render::Rect& area() const { return area_; }

    PullToRefreshResponse handle(const input::PointerEvent& event, bool at_top);
    void cancel_interaction();

    void set_refreshing(bool refreshing);
    bool refreshing() const { return refreshing_; }
    bool spinning() const { return refreshing_ || holding_; }
    bool pulling() const { return pulling_; }
    bool holds_pointer() const { return pulling_; }
    bool tracking() const { return touch_.has_value(); }
    double distance_fraction() const { return distance_.value(); }
    bool shown() const;

    bool enabled() const { return options_.enabled; }
    void set_enabled(bool enabled);

    render::Rect container_rect() const;
    double indicator_scale() const;
    double indicator_turn_degrees() const { return turn_; }
    PullIndicatorStyle style() const { return options_.style; }
    void set_style(PullIndicatorStyle style);
    PullArrow arrow() const { return pull_arrow(distance_.value(), a()); }
    double arrow_alpha() const { return arrow_alpha_.value(); }
    double spinner_mix() const { return spinner_mix_.value(); }
    double spinner_sweep_degrees() const;
    double spinner_rotation_degrees() const;
    const LoadingIndicator& indicator() const { return indicator_; }

    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    std::optional<AccessibleNode> accessible() const;

    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Touch {
        double start_x = 0.0;
        double start_y = 0.0;
        std::optional<double> anchor_y;
        double anchor_x = 0.0;
    };

    const tokens::PullToRefreshTokens& t() const { return options_.tokens ? *options_.tokens : current_style().pull_to_refresh; }
    const tokens::PullArrowTokens& a() const { return options_.arrow_tokens ? *options_.arrow_tokens : current_style().pull_arrow; }
    void follow(double y);
    void release(bool commit, PullToRefreshResponse& response);
    void sync_indicator();
    void start_refreshing();
    double container_size() const;
    void paint_arrow(render::Canvas& canvas, color::Argb color) const;
    void paint_spinner(render::Canvas& canvas, color::Argb color) const;
    void leave();

    PullToRefreshOptions options_;
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    std::optional<Touch> touch_;
    bool pulling_ = false;
    bool refreshing_ = false;
    bool holding_ = false;
    double refresh_seconds_ = 0.0;
    motion::Animated distance_;
    LoadingIndicator indicator_;
    double turn_ = 0.0;
    motion::Tweened arrow_alpha_;
    motion::Tweened spinner_mix_{0.0};
    double spinner_ms_ = 0.0;
    StyleStamp style_stamp_;
};

}
