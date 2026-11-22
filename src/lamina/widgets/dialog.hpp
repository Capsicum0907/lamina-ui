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
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/dialog.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/content_slot.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/popup_transition.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/widget_set.hpp"

namespace lamina::widgets {

enum class DialogAction { confirm, dismiss };

using DialogContent = ContentSlot;

struct DialogOptions {
    std::string headline;
    std::string supporting_text;
    std::string confirm_label;
    std::string dismiss_label;
    std::optional<icons::Icon> icon = std::nullopt;
    std::optional<DialogAction> default_action = std::nullopt;
    std::optional<DialogContent> content = std::nullopt;
};

enum class DialogResult { none, confirmed, dismissed };

class Dialog {
public:
    Dialog(DialogOptions options, const text::FontStack& fonts);
    Dialog(const Dialog&) = delete;
    Dialog& operator=(const Dialog&) = delete;

    void open_in(const render::Rect& area, input::InputSource source = input::InputSource::pointer);
    void open(layout::Size window, input::InputSource source = input::InputSource::pointer);
    void close();
    void set_area(const render::Rect& area);
    void set_window(layout::Size window);
    render::Rect area() const { return area_; }
    bool is_open() const { return popup_.is_open(); }
    bool visible() const { return popup_.visible(); }
    bool holds_pointer() const { return is_open(); }
    double opacity() const { return popup_.opacity(); }
    double scale() const { return popup_.scale(); }
    render::Rect bounds() const { return surface_; }
    render::Rect confirm_bounds() const { return confirm_.bounds(); }
    std::optional<render::Rect> dismiss_bounds() const;
    std::optional<render::Rect> icon_bounds() const;
    render::Rect headline_bounds() const { return headline_.bounds(); }
    std::optional<render::Rect> content_viewport() const;
    double content_height() const { return slot_content_height_; }
    bool content_scrolls() const { return slot_ && scroll_.max_offset() > 0.0; }
    const ScrollArea& content_scroll() const { return scroll_; }
    std::size_t button_lines() const { return actions_->line_count(); }
    std::optional<DialogAction> focused_action() const;
    bool focus_shown() const { return focus_shown_; }
    bool content_focused() const;
    bool action_enabled(DialogAction action) const;
    void set_action_enabled(DialogAction action, bool enabled);

    std::optional<render::Rect> caret_area() const;
    std::optional<double> next_wake_seconds() const;

    DialogResult handle(const input::PointerEvent& event);
    DialogResult handle(const input::KeyEvent& event);
    bool handle(const input::TextEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const;

private:
    std::vector<DialogAction> reading_order() const;
    std::vector<DialogAction> enabled_order() const;
    Button& button(DialogAction action);
    const Button& button(DialogAction action) const;
    DialogResult result_of(DialogAction action);
    void show_focus(DialogAction action);
    void clear_focus();
    void clear_action_focus();
    void tab(bool backward);
    bool content_has_stops() const;
    void reveal_content_focus();
    void deliver_to_content(const input::PointerEvent& e);
    layout::Size measure_root(const render::Rect& area);

    Label headline_;
    Label supporting_;
    Button confirm_;
    std::optional<Button> dismiss_;
    std::optional<icons::Icon> icon_;
    std::optional<DialogContent> content_;
    render::Point icon_origin_{0.0, 0.0};
    std::shared_ptr<layout::Flow> actions_;
    layout::NodePtr slot_;
    layout::NodePtr slot_inner_;
    double slot_limit_ = layout::unbounded;
    double slot_content_height_ = 0.0;
    ScrollArea scroll_;
    layout::NodePtr root_;
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    render::Rect surface_{0.0, 0.0, 0.0, 0.0};
    PopupTransition popup_;
    std::optional<DialogAction> default_action_;
    std::optional<DialogAction> focus_;
    bool focus_shown_ = false;
};

}
