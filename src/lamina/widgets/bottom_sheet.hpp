#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/anchored_draggable.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/bottom_sheet.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/content_host.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/pattern_strings.hpp"

namespace lamina::widgets {

enum class SheetValue { hidden, partially_expanded, expanded };

enum class BottomSheetKind { modal, standard };

class SheetDragHandle {
public:
    explicit SheetDragHandle(std::string name) : name_(std::move(name)) {}

    render::Rect bounds() const { return bounds_; }
    void set_bounds(const render::Rect& r) { bounds_ = r; }
    render::Rect handle_rect() const;
    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool can_focus() const { return shown_; }
    void set_shown(bool shown);
    void set_description(std::string description) { description_ = std::move(description); }
    const std::string& accessible_name() const { return name_; }
    AccessibleNode accessible() const {
        return {AccessibleRole::button, name_, bounds_, {.focusable = shown_, .focused = focused()}, {}, description_};
    }

    bool handle(const input::PointerEvent& event);
    bool activates(const input::KeyEvent& event) const { return focus_.activates(event, KeyActivation::space_or_enter); }
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const { return !state_layer_.settled() || !focus_.settled(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void retarget();

    std::string name_;
    std::string description_;
    render::Rect bounds_{0.0, 0.0, 0.0, 0.0};
    bool shown_ = false;
    PressState press_;
    FocusIndicator focus_;
    motion::Animated state_layer_{0.0, tokens::motion_scheme.fast_effects()};
};

struct BottomSheetOptions {
    BottomSheetKind kind = BottomSheetKind::modal;
    std::optional<ContentSlot> content = std::nullopt;
    bool drag_handle = true;
    bool skip_partially_expanded = false;
    bool skip_hidden = true;
    double peek_height = current_style().bottom_sheet.peek_height;
    bool dismiss_on_scrim = true;
    bool gestures = true;
    SheetValue initial = SheetValue::hidden;
    const PatternStrings* strings = &english_us_patterns();
    std::string accessible_name = {};
};

struct BottomSheetResponse {
    bool dismissed = false;
    bool consumed = false;
};

class BottomSheet {
public:
    explicit BottomSheet(BottomSheetOptions options);
    BottomSheet(const BottomSheet&) = delete;
    BottomSheet& operator=(const BottomSheet&) = delete;

    void set_area(const render::Rect& area);
    render::Rect area() const { return area_; }
    render::Rect bounds() const;
    render::Rect body_area() const;
    double sheet_height() const { return sheet_height_; }
    double offset() const { return drag_.offset(); }
    double visible_height() const;
    double scrim_alpha() const { return std::clamp(scrim_.value(), 0.0, 1.0); }
    bool modal() const { return options_.kind == BottomSheetKind::modal; }

    void show(input::InputSource source = input::InputSource::pointer);
    void hide();
    void expand();
    void partial_expand();
    void snap_to(SheetValue value);
    SheetValue current_value() const { return drag_.settled_value(); }
    SheetValue target_value() const { return drag_.target_value(); }
    bool has_state(SheetValue value) const { return drag_.has(value); }
    bool is_open() const;
    bool visible() const;
    bool dragging() const { return drag_.dragging(); }
    bool holds_pointer() const { return (modal() && visible()) || drag_.dragging() || content_.holds_pointer(); }
    void set_gestures_enabled(bool enabled);
    bool gestures_enabled() const { return options_.gestures; }

    SheetDragHandle* drag_handle() { return options_.drag_handle ? &handle_ : nullptr; }
    const ContentHost& content() const { return content_; }
    bool focused() const;
    void set_focused(bool focused);
    bool can_focus() const;
    std::optional<render::Rect> caret_area() const { return content_.caret_area(); }
    std::optional<double> next_wake_seconds() const { return content_.next_wake_seconds(); }

    BottomSheetResponse handle(const input::PointerEvent& event);
    BottomSheetResponse handle(const input::KeyEvent& event);
    bool handle(const input::TextEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const;

private:
    double handle_height() const;
    void layout_sheet();
    void update_anchors();
    void place_parts();
    void update_handle();
    bool dismiss_after_settle() const;
    BottomSheetResponse click_handle();
    BottomSheetResponse settle_to_dismiss();
    BottomSheetResponse animate_to_dismiss();
    BottomSheetResponse finish_drag(double time_seconds);
    bool content_first(const input::PointerEvent& event) const;
    double damped(double velocity) const;
    void tab(bool backward);

    BottomSheetOptions options_;
    SheetDragHandle handle_;
    ContentHost content_;
    FocusManager own_focus_;
    motion::AnchoredDraggable<SheetValue> drag_;
    motion::Animated scrim_;
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    double sheet_width_ = 0.0;
    double sheet_height_ = 0.0;
    bool scrim_pressed_ = false;
    bool content_claimed_ = false;
    bool tracking_ = false;
    double press_y_ = 0.0;
    bool press_on_content_ = false;
};

}
