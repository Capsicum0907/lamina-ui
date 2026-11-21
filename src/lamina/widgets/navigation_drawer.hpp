#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/anchored_draggable.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/tokens/navigation_drawer.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/badge.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/pattern_strings.hpp"
#include "lamina/widgets/scroll_area.hpp"

namespace lamina::widgets {

enum class DrawerEntryKind { item, headline, divider };

struct DrawerEntry {
    DrawerEntryKind kind = DrawerEntryKind::item;
    std::string label = {};
    std::optional<icons::Icon> icon = std::nullopt;
    std::optional<BadgeSpec> badge = std::nullopt;
    bool enabled = true;
};

inline DrawerEntry drawer_headline(std::string text) {
    return {DrawerEntryKind::headline, std::move(text)};
}

inline DrawerEntry drawer_divider() {
    return {DrawerEntryKind::divider};
}

enum class DrawerSheetStyle { modal, standard };

struct DrawerSheetOptions {
    std::vector<DrawerEntry> entries;
    std::optional<std::size_t> selected = std::nullopt;
    DrawerSheetStyle style = DrawerSheetStyle::modal;
    const PatternStrings* strings = &english_us_patterns();
    std::string accessible_name = {};
    tokens::Density density = {};
};

struct DrawerResponse {
    std::optional<std::size_t> selected = std::nullopt;
    bool consumed = false;
    bool took_over = false;
    bool closed = false;
};

class DrawerSheet {
public:
    DrawerSheet(DrawerSheetOptions options, const text::FontStack& fonts);

    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& rect);
    render::Rect bounds() const { return rect_; }
    double content_height() const;
    double width_for(double available) const;
    tokens::Density density() const { return options_.density; }
    void set_density(tokens::Density density);

    std::size_t count() const { return options_.entries.size(); }
    const DrawerEntry& entry(std::size_t index) const { return options_.entries[index]; }
    std::optional<std::size_t> selected() const { return options_.selected; }
    void select(std::size_t index);
    render::Rect entry_rect(std::size_t index) const;
    render::Rect item_rect(std::size_t index) const;
    void set_item_enabled(std::size_t index, bool enabled);
    bool item_enabled(std::size_t index) const;
    void set_item_badge(std::size_t index, std::optional<BadgeSpec> badge);
    const ScrollArea& scroll() const { return scroll_; }

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    std::optional<std::size_t> focused_item() const { return focused() ? std::optional<std::size_t>(focus_index_) : std::nullopt; }
    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    bool can_focus() const;
    AccessibleNode accessible() const;

    DrawerResponse handle(const input::PointerEvent& event);
    DrawerResponse handle(const input::KeyEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_container(render::Canvas& canvas, const color::ColorScheme& scheme, const render::Rect& container) const;
    void paint_entries(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Row {
        text::TextLine label;
        std::optional<text::TextLine> badge;
        double top = 0.0;
        double height = 0.0;
        PressState press;
        motion::Animated state_layer;
        motion::Animated selection;
        Enablement enabled;
        bool allowed = true;
    };

    bool is_item(std::size_t index) const { return options_.entries[index].kind == DrawerEntryKind::item; }
    bool row_enabled(std::size_t index) const { return enabled_ && is_item(index) && rows_[index].enabled.enabled(); }
    void retarget();
    void layout_rows();
    std::optional<std::size_t> row_at(double x, double y) const;
    std::size_t focus_start() const;
    void reveal(std::size_t index);

    DrawerSheetOptions options_;
    text::FontStack fonts_;
    std::vector<Row> rows_;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
    ScrollArea scroll_;
    FocusIndicator focus_;
    std::size_t focus_index_ = 0;
    bool enabled_ = true;
};

enum class DrawerValue { closed, open };

struct NavigationDrawerOptions {
    DrawerSheetOptions sheet;
    bool gestures = true;
    DrawerValue initial = DrawerValue::closed;
};

class DrawerSlide {
public:
    explicit DrawerSlide(DrawerValue initial);

    void set_width(double width);
    double width() const { return width_; }
    double offset() const { return drag_.offset(); }
    double fraction() const;
    DrawerValue current_value() const { return drag_.settled_value(); }
    DrawerValue target_value() const { return drag_.target_value(); }
    bool open_or_opening() const { return drag_.target_value() == DrawerValue::open; }
    bool shown() const;
    bool pressed() const { return drag_.pressed(); }
    bool dragging() const { return drag_.dragging(); }
    bool animating() const { return drag_.animating(); }

    void open();
    void close();
    void snap_to(DrawerValue value) { drag_.snap_to(value); }
    void press(double x, double time_seconds) { drag_.press(x, time_seconds); }
    bool move(double x, double time_seconds) { return drag_.move(x, time_seconds); }
    DrawerValue release(double time_seconds);
    void cancel();
    void step(double dt_seconds) { drag_.step(dt_seconds); }

private:
    motion::AnchoredDraggable<DrawerValue> drag_;
    double width_ = 0.0;
};

class ModalNavigationDrawer {
public:
    ModalNavigationDrawer(NavigationDrawerOptions options, const text::FontStack& fonts);
    ModalNavigationDrawer(const ModalNavigationDrawer&) = delete;
    ModalNavigationDrawer& operator=(const ModalNavigationDrawer&) = delete;

    void set_area(const render::Rect& area);
    render::Rect area() const { return area_; }
    render::Rect sheet_bounds() const { return sheet_.bounds(); }
    DrawerSheet& sheet() { return sheet_; }
    const DrawerSheet& sheet() const { return sheet_; }

    void open(input::InputSource source = input::InputSource::pointer);
    void close();
    void snap_to(DrawerValue value);
    DrawerValue current_value() const { return slide_.current_value(); }
    DrawerValue target_value() const { return slide_.target_value(); }
    double offset() const { return slide_.offset(); }
    double scrim_alpha() const { return slide_.fraction(); }
    bool is_open() const { return slide_.shown(); }
    bool visible() const { return slide_.shown(); }
    void set_gestures_enabled(bool enabled) { options_.gestures = enabled; }

    DrawerResponse handle(const input::PointerEvent& event);
    DrawerResponse handle(const input::KeyEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const;

private:
    void place();

    NavigationDrawerOptions options_;
    DrawerSheet sheet_;
    DrawerSlide slide_;
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    bool scrim_pressed_ = false;
    bool tracking_ = false;
};

class DismissibleNavigationDrawer {
public:
    DismissibleNavigationDrawer(NavigationDrawerOptions options, layout::NodePtr content, const text::FontStack& fonts);
    DismissibleNavigationDrawer(const DismissibleNavigationDrawer&) = delete;
    DismissibleNavigationDrawer& operator=(const DismissibleNavigationDrawer&) = delete;

    layout::Size measure(const layout::Constraints& c);
    void arrange(const render::Rect& rect);
    render::Rect bounds() const { return area_; }
    render::Rect content_rect() const { return content_rect_; }
    DrawerSheet& sheet() { return sheet_; }
    const DrawerSheet& sheet() const { return sheet_; }

    void open(input::InputSource source = input::InputSource::pointer);
    void close();
    void snap_to(DrawerValue value);
    DrawerValue current_value() const { return slide_.current_value(); }
    DrawerValue target_value() const { return slide_.target_value(); }
    double offset() const { return slide_.offset(); }
    void set_gestures_enabled(bool enabled) { options_.gestures = enabled; }

    bool focused() const { return sheet_.focused(); }
    void set_focused(bool focused) { sheet_.set_focused(focused); }
    bool can_focus() const { return slide_.open_or_opening() && sheet_.can_focus(); }
    void set_enabled(bool enabled) { sheet_.set_enabled(enabled); }
    bool enabled() const { return sheet_.enabled(); }

    DrawerResponse handle(const input::PointerEvent& event);
    DrawerResponse handle(const input::KeyEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const;

private:
    void place();

    NavigationDrawerOptions options_;
    DrawerSheet sheet_;
    DrawerSlide slide_;
    layout::NodePtr content_;
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    render::Rect content_rect_{0.0, 0.0, 0.0, 0.0};
    bool tracking_ = false;
};

layout::NodePtr permanent_drawer(DrawerSheet& sheet, layout::NodePtr content);

}
