#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/tokens/top_app_bar.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/top_app_bar_state.hpp"

namespace lamina::widgets {

enum class TitleAlignment { start, center };

struct TopAppBarOptions {
    std::string title;
    tokens::TopAppBarKind kind = tokens::TopAppBarKind::small;
    std::string subtitle = {};
    std::optional<IconButtonOptions> navigation = std::nullopt;
    std::vector<IconButtonOptions> actions = {};
    TopAppBarScroll scroll = TopAppBarScroll::pinned;
    std::optional<TitleAlignment> alignment = std::nullopt;
    std::string accessible_name = {};
    tokens::Density density = {};
    bool container = true;
    std::optional<tokens::TypeRole> title_type = std::nullopt;
    std::optional<tokens::TypeRole> subtitle_type = std::nullopt;
    bool ellipsize = false;
};

struct TopAppBarResponse {
    bool navigation = false;
    std::optional<std::size_t> action;
    bool consumed = false;
};

class TopAppBar {
public:
    TopAppBar(TopAppBarOptions options, const text::FontStack& fonts);

    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& r);
    double width() const { return width_; }
    double height() const;
    double expanded_height() const;
    double collapsed_height() const;
    tokens::Density density() const { return options_.density; }
    void set_density(tokens::Density density);
    render::Rect bounds() const { return {origin_.x, origin_.y, width_, height()}; }
    render::Rect title_rect() const;
    double title_opacity() const;
    double collapsed_title_opacity() const;
    double container_amount() const;

    const TopAppBarState& state() const { return state_; }
    TopAppBarState& state() { return state_; }
    void follow(ScrollArea& scroll);
    void set_title(std::string title);

    IconButton* navigation_button() { return navigation_ ? &*navigation_ : nullptr; }
    std::size_t action_count() const { return actions_.size(); }
    IconButton& action(std::size_t index) { return actions_[index]; }
    const IconButton& action(std::size_t index) const { return actions_[index]; }

    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    AccessibleNode accessible() const;

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    TopAppBarResponse handle(const input::PointerEvent& event);
    TopAppBarResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    template <typename Offer>
    void offer_targets(const input::PointerEvent& event, Offer offer) const;
    const tokens::TopAppBarKindTokens& kind() const { return tokens::top_app_bar_kind(options_.kind); }
    bool two_rows() const { return kind().two_rows; }
    bool has_subtitle() const { return !options_.subtitle.empty(); }
    TitleAlignment alignment() const;
    double title_start() const;
    double actions_width() const;
    double title_x(double box_width, double start, double end) const;
    void place_buttons();
    void layout_titles();
    double block_height(const text::TextLine& title, const std::optional<text::TextLine>& subtitle) const;
    double text_room() const;
    text::TextLine title_line(const std::string& s, tokens::TypeRole role) const;
    void paint_title(render::Canvas& canvas, const text::TextLine& title, const std::optional<text::TextLine>& subtitle, render::Point at,
                     const color::ColorScheme& scheme, double opacity) const;

    TopAppBarOptions options_;
    text::FontStack fonts_;
    std::optional<IconButton> navigation_;
    std::vector<IconButton> actions_;
    TargetArbiter arbiter_;
    text::TextLine title_;
    std::optional<text::TextLine> subtitle_;
    text::TextLine small_title_;
    std::optional<text::TextLine> small_subtitle_;
    TopAppBarState state_;
    motion::Animated container_;
    render::Point origin_{0.0, 0.0};
    double width_ = 0.0;
    double driven_offset_ = 0.0;
    bool enabled_ = true;
};

}
