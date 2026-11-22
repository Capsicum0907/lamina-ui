#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/layout/window_size_class.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_item.hpp"
#include "lamina/widgets/pane_navigator.hpp"
#include "lamina/widgets/pattern_strings.hpp"
#include "lamina/widgets/side_navigation.hpp"

namespace lamina::widgets {

enum class NavigationSuiteType {
    short_navigation_bar_compact,
    short_navigation_bar_medium,
    wide_navigation_rail_collapsed,
    wide_navigation_rail_expanded,
    navigation_drawer,
    none,
};

NavigationSuiteType navigation_suite_type(const WindowAdaptiveInfo& info);

struct NavigationSuiteChoice {
    std::array<NavigationSuiteType, static_cast<std::size_t>(layout::WindowWidthClass::count)> by_width;
    NavigationSuiteType compact_height;
};

inline constexpr NavigationSuiteChoice widening_navigation_suite{
    {NavigationSuiteType::short_navigation_bar_compact, NavigationSuiteType::wide_navigation_rail_collapsed,
     NavigationSuiteType::wide_navigation_rail_expanded, NavigationSuiteType::navigation_drawer, NavigationSuiteType::navigation_drawer},
    NavigationSuiteType::short_navigation_bar_medium,
};

NavigationSuiteType navigation_suite_type(const WindowAdaptiveInfo& info, const NavigationSuiteChoice& choice);

struct NavigationSuiteScaffoldOptions {
    std::vector<NavigationItem> items;
    std::size_t selected = 0;
    layout::NodePtr content = nullptr;
    NavigationSuiteType type = NavigationSuiteType::wide_navigation_rail_collapsed;
    std::string accessible_name = {};
    const PatternStrings* strings = &english_us_patterns();
    tokens::Density density = {};
    bool labels = true;
};

struct NavigationSuiteResponse {
    std::optional<std::size_t> selected;
    bool consumed = false;
};

class NavigationSuiteScaffold {
public:
    NavigationSuiteScaffold(NavigationSuiteScaffoldOptions options, const text::FontStack& fonts);

    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& rect);
    render::Rect bounds() const { return rect_; }
    render::Rect content_rect() const { return content_; }

    NavigationSuiteType type() const { return type_; }
    void set_type(NavigationSuiteType type);
    void update(const WindowAdaptiveInfo& info, const NavigationSuiteChoice& choice = widening_navigation_suite);
    std::size_t selected() const { return selected_; }
    void select(std::size_t index);
    double bar_progress() const { return bar_.value(); }
    double side_progress() const { return side_.value(); }
    double drawer_amount() const { return side_widget_.drawer_amount(); }
    double expansion() const { return side_widget_.expansion(); }
    double side_width() const;
    bool labels() const { return labels_; }
    void set_labels(bool labels);
    void snap_to_targets();

    NavigationBar& bar() { return bar_widget_; }
    SideNavigation& side() { return side_widget_; }
    const NavigationBar& bar() const { return bar_widget_; }
    const SideNavigation& side() const { return side_widget_; }

    bool focused() const;
    void set_focused(bool focused);
    bool can_focus() const;
    render::Rect focus_bounds() const;
    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    tokens::Density density() const { return density_; }
    void set_density(tokens::Density density);

    NavigationSuiteResponse handle(const input::PointerEvent& event);
    NavigationSuiteResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::vector<AccessibleNode> accessible_nodes() const;

private:
    enum class Active { bar, side, none };

    static bool is_bar(NavigationSuiteType t);
    static bool is_drawer(NavigationSuiteType t);
    static bool is_expanded(NavigationSuiteType t);
    Active active() const;
    bool side_visible() const;
    bool bar_visible() const;
    void retarget_side();
    void place();
    void sync(std::size_t index);

    NavigationSuiteType type_;
    std::size_t selected_;
    layout::NodePtr content_node_;
    NavigationBar bar_widget_;
    SideNavigation side_widget_;
    motion::Animated bar_;
    motion::Animated side_;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
    render::Rect content_{0.0, 0.0, 0.0, 0.0};
    bool enabled_ = true;
    bool labels_ = true;
    tokens::Density density_{};
};

}
