#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/chip.hpp"

namespace lamina::widgets {

struct ChipGroupOptions {
    std::vector<ChipOptions> chips;
    bool single_selection = false;
    std::string accessible_name = {};
    double spacing = tokens::chip_tokens.group_spacing;
    double line_spacing = tokens::chip_tokens.group_line_spacing;
};

struct ChipGroupResponse {
    std::optional<std::size_t> clicked;
    std::optional<std::size_t> removed;
    std::string removed_label = {};
    bool consumed = false;
};

class ChipGroup {
public:
    ChipGroup(ChipGroupOptions options, const text::FontStack& fonts);

    layout::Size measure(const layout::Constraints& c);
    void arrange(const render::Rect& r);
    render::Rect bounds() const { return {origin_.x, origin_.y, width_, height_}; }
    std::size_t line_count() const { return line_count_; }

    std::size_t size() const;
    Chip& chip(std::size_t index);
    const Chip& chip(std::size_t index) const;
    std::optional<std::size_t> find(std::string_view label) const;
    void add(ChipOptions options);
    void remove(std::size_t index);
    std::vector<std::size_t> selected() const;

    bool focused() const { return focused_; }
    void set_focused(bool focused);
    std::optional<std::size_t> focused_chip() const;
    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    tokens::Density density() const { return density_; }
    void set_density(tokens::Density density);
    void set_chip_enabled(std::size_t index, bool enabled);
    bool can_focus() const;
    AccessibleNode accessible() const;

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    ChipGroupResponse handle(const input::PointerEvent& event);
    ChipGroupResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    template <typename Offer>
    void offer_targets(const input::PointerEvent& event, Offer offer) const;
    struct Entry {
        std::unique_ptr<Chip> chip;
        motion::Animated presence;
        motion::Animated dx;
        motion::Animated dy;
        render::Point at{0.0, 0.0};
        std::size_t line = 0;
        bool placed = false;
        bool leaving = false;
        bool allowed = true;
    };

    std::size_t entry_index(std::size_t index) const;
    std::vector<std::size_t> active() const;
    double flow(double max_width, bool place);
    void select_only(std::size_t entry);
    void focus_entry(std::optional<std::size_t> entry);
    void after_click(std::size_t entry, ChipGroupResponse& response);
    void start_leaving(std::size_t entry, ChipGroupResponse& response);

    ChipGroupOptions options_;
    tokens::Density density_{};
    text::FontStack fonts_;
    std::vector<Entry> entries_;
    TargetArbiter arbiter_;
    render::Point origin_{0.0, 0.0};
    double width_ = 0.0;
    double height_ = 0.0;
    double laid_out_width_ = 0.0;
    std::size_t line_count_ = 0;
    bool focused_ = false;
    std::size_t focus_entry_ = 0;
    bool enabled_ = true;
};

}
