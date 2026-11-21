#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/anchored_draggable.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/side_sheet.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/content_host.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/pattern_strings.hpp"

namespace lamina::widgets {

enum class SideSheetKind { modal, standard };
enum class SideSheetEdge { end, start };
enum class SideSheetValue { hidden, expanded };

struct SideSheetIcon {
    icons::Icon icon;
    std::string accessible_name;
};

struct SideSheetOptions {
    SideSheetKind kind = SideSheetKind::modal;
    bool detached = false;
    SideSheetEdge edge = SideSheetEdge::end;
    std::string headline = {};
    std::optional<SideSheetIcon> close = std::nullopt;
    std::optional<SideSheetIcon> back = std::nullopt;
    std::optional<ContentSlot> content = std::nullopt;
    std::vector<ButtonOptions> actions = {};
    bool dismiss_on_scrim = true;
    bool gestures = true;
    SideSheetValue initial = SideSheetValue::hidden;
    const PatternStrings* strings = &english_us_patterns();
    std::string accessible_name = {};
};

struct SideSheetResponse {
    bool dismissed = false;
    bool back = false;
    std::optional<std::size_t> action = std::nullopt;
    bool consumed = false;
};

class SideSheet {
public:
    SideSheet(SideSheetOptions options, const text::FontStack& fonts);
    SideSheet(const SideSheet&) = delete;
    SideSheet& operator=(const SideSheet&) = delete;

    void set_area(const render::Rect& area);
    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& rect);
    render::Rect area() const { return area_; }
    render::Rect bounds() const;
    double occupied_width() const;
    double travel() const;
    double progress() const;
    bool modal() const { return options_.kind == SideSheetKind::modal; }

    void open(input::InputSource source = input::InputSource::pointer);
    void close();
    void snap_to(SideSheetValue value);
    SideSheetValue current_value() const { return drag_.settled_value(); }
    SideSheetValue target_value() const { return drag_.target_value(); }
    bool is_open() const { return visible(); }
    bool visible() const;
    bool layout_changing() const { return !modal() && !drag_.idle(); }

    IconButton* close_button() { return close_ ? close_.get() : nullptr; }
    IconButton* back_button() { return back_ ? back_.get() : nullptr; }
    Button* action_button(std::size_t index) { return index < actions_.size() ? actions_[index].get() : nullptr; }
    void add_header_stops(FocusManager& focus);
    void add_action_stops(FocusManager& focus);
    const ContentHost& content() const { return content_; }
    bool focused() const;
    void set_focused(bool focused);
    std::optional<render::Rect> caret_area() const { return content_.caret_area(); }
    std::optional<double> next_wake_seconds() const { return content_.next_wake_seconds(); }

    SideSheetResponse handle(const input::PointerEvent& event);
    SideSheetResponse handle(const input::KeyEvent& event);
    bool handle(const input::TextEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const;

private:
    double sign() const { return options_.edge == SideSheetEdge::end ? 1.0 : -1.0; }
    double margin() const { return options_.detached ? tokens::side_sheet_tokens.detached_margin : 0.0; }
    double actions_height() const;
    render::Rect open_rect() const;
    void update_anchors();
    void place();
    void tab(bool backward);
    SideSheetValue release_target(double velocity) const;
    SideSheetResponse dismiss();
    template <typename F>
    void each_button(F&& f);

    SideSheetOptions options_;
    text::FontStack fonts_;
    text::TextLine headline_;
    std::unique_ptr<IconButton> close_;
    std::unique_ptr<IconButton> back_;
    std::vector<std::unique_ptr<Button>> actions_;
    ContentHost content_;
    FocusManager header_focus_;
    FocusManager action_focus_;
    motion::AnchoredDraggable<SideSheetValue> drag_;
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    bool scrim_pressed_ = false;
    bool tracking_ = false;
    bool content_claimed_ = false;
};

}
