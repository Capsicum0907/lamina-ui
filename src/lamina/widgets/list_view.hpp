#pragma once

#include <cmath>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/list_item.hpp"
#include "lamina/widgets/scroll_area.hpp"

namespace lamina::widgets {

struct ListViewOptions {
    std::vector<ListItemSpec> items;
    bool single_selection = false;
    std::string accessible_name = {};
    double top_inset = 0.0;
};

struct ListViewResponse {
    std::optional<std::size_t> clicked;
    bool consumed = false;
};

struct ListViewStats {
    std::size_t live_rows = 0;
    std::size_t measured_rows = 0;
    std::size_t painted_rows = 0;
};

class ListView {
public:
    ListView(ListViewOptions options, const text::FontStack& fonts);

    void set_viewport(const render::Rect& viewport);
    const render::Rect& viewport() const { return scroll_.viewport(); }
    layout::Size measure(const layout::Constraints& c) const {
        return c.constrain({std::isfinite(c.max_width) ? c.max_width : c.min_width, std::isfinite(c.max_height) ? c.max_height : c.min_height});
    }
    void arrange(const render::Rect& r) { set_viewport(r); }
    render::Rect bounds() const { return scroll_.viewport(); }
    ScrollArea& scroll_area() { return scroll_; }
    const ScrollArea& scroll_area() const { return scroll_; }

    std::size_t size() const { return specs_.size(); }
    const ListItemSpec& item(std::size_t index) const { return specs_[index]; }
    void set_items(std::vector<ListItemSpec> items);
    double top_inset() const { return top_inset_; }
    void set_top_inset(double inset);
    void set_selected(std::size_t index, bool selected);
    void set_item_enabled(std::size_t index, bool enabled);
    double content_height() const;
    render::Rect row_rect(std::size_t index) const;
    std::optional<std::size_t> row_at(double x, double y) const;
    std::pair<std::size_t, std::size_t> visible_rows() const;
    const ListViewStats& stats() const { return stats_; }

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    std::optional<std::size_t> focused_row() const;
    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    bool can_focus() const;
    AccessibleNode accessible() const;

    ListViewResponse handle(const input::PointerEvent& event);
    ListViewResponse handle(const input::KeyEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    double next_wake_seconds() const { return scroll_.next_wake_seconds(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    bool row_enabled(std::size_t index) const;
    void rebuild_tops() const;
    ListItem& live(std::size_t index);
    void update_rows();
    void focus_row(std::size_t index);
    void after_click(std::size_t index, ListViewResponse& response);
    std::size_t row_for_offset(double y) const;

    std::vector<ListItemSpec> specs_;
    text::FontStack fonts_;
    bool single_selection_;
    std::string accessible_name_;
    double top_inset_;
    ScrollArea scroll_;
    std::map<std::size_t, std::unique_ptr<ListItem>> live_;
    std::vector<double> heights_;
    std::vector<double> measured_width_;
    mutable std::vector<double> tops_;
    mutable bool tops_dirty_ = true;
    FocusIndicator focus_;
    std::size_t focus_index_ = 0;
    bool enabled_ = true;
    std::optional<std::size_t> pressed_;
    std::pair<std::size_t, std::size_t> visible_{0, 0};
    mutable ListViewStats stats_;
};

}
