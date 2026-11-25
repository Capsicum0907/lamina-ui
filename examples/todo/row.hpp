#pragma once

#include <optional>
#include <string>

#include "common/animated_box.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/text_field.hpp"
#include "res.hpp"
#include "todo/engine.hpp"
#include "todo/layout.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::todo {

struct RowStyle {
    tokens::TypeRole text_type = tokens::TypeRole::body_large;
    color::Role text = color::Role::on_surface;
    color::Role done_text = color::Role::on_surface_variant;
    double min_height = 48.0;
    double text_padding = 12.0;
    double edit_padding = 4.0;
    double strike_thickness = 1.0;
    double strike_rise = 0.3;
    color::Role state_layer = color::Role::on_surface;
    tokens::CornerSize corner = tokens::CornerSize::medium;
    color::Role focus_ring = color::Role::secondary;
    double focus_ring_width = 2.0;
    icons::Symbol remove_icon = res::icons::delete_;
    std::string remove_name = "Delete";
    motion::SpringSpec move_spring = current_style().motion.default_spatial();
    motion::SpringSpec fade_spring = current_style().motion.default_effects();
};

enum class RowPart { check, text, remove };

struct RowAction {
    bool toggled = false;
    bool remove = false;
    std::optional<input::PointerEvent> edit;
};

class TaskRow {
public:
    TaskRow(const Item& item, const RowStyle& style, const text::FontStack& fonts, const icons::IconFont& icon_font,
            input::Clipboard* clipboard);

    ItemId id() const { return id_; }
    bool done() const { return done_; }
    void sync(const Item& item);
    void set_column(const Column& column);
    double target_height() const;

    void appear(double y, double height);
    void snap(const RowPlace& place);
    void place(const RowPlace& place, bool animate);
    void leave();
    void revive();
    bool leaving() const { return leaving_; }
    bool shown() const { return !leaving_ && presence_.target() > 0.0; }
    bool gone() const { return leaving_ && box_.settled() && presence_.settled(); }
    bool moving() const { return !box_.settled(); }
    render::Rect rect() const { return box_.rect(); }
    double target_y() const { return target_y_; }

    RowAction handle(const input::PointerEvent& event);
    RowAction handle(const input::KeyEvent& event);
    void cancel_interaction();

    bool focused() const;
    void set_focused(bool focused);
    std::optional<RowPart> focused_part() const;
    void focus_part(RowPart part);
    void clear_focus();
    widgets::Checkbox& checkbox() { return check_; }
    widgets::IconButton& remove_button() { return remove_; }
    bool editing() const { return editor_.has_value(); }
    void begin_edit(const std::optional<input::PointerEvent>& press);
    widgets::TextField* editor() { return editor_ ? &*editor_ : nullptr; }
    const widgets::TextField* editor() const { return editor_ ? &*editor_ : nullptr; }
    std::string end_edit();

    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void relayout();
    void arrange_widgets();
    render::Rect text_rect() const;

    const RowStyle& style_;
    text::FontStack fonts_;
    input::Clipboard* clipboard_;
    ItemId id_;
    std::string text_;
    bool done_;
    Column column_{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    text::TextBlock block_;
    double laid_out_width_ = -1.0;
    widgets::Checkbox check_;
    widgets::IconButton remove_;
    std::optional<widgets::TextField> editor_;
    std::optional<widgets::TextField> closing_editor_;
    widgets::PressState press_;
    widgets::PressState text_press_;
    bool focused_ = false;
    bool keep_focus_ = false;
    bool leaving_ = false;
    double target_y_ = 0.0;
    AnimatedBox box_;
    motion::Animated presence_;
    motion::Animated done_amount_;
    motion::Animated hover_;
    motion::Animated focus_;
    motion::Animated remove_visibility_;
    motion::Animated edit_;
};

struct HeaderStyle {
    std::string title_format = "Completed ({})";
    std::string clear_label = "Clear completed";
    tokens::TypeRole title_type = tokens::TypeRole::title_small;
    color::Role title_color = color::Role::on_surface_variant;
    icons::Symbol chevron = res::icons::expand_more;
    double chevron_size = 24.0;
    color::Role chevron_color = color::Role::on_surface_variant;
};

struct HeaderAction {
    bool toggled = false;
    bool clear = false;
};

class SectionHeader {
public:
    SectionHeader(const HeaderStyle& style, const RowStyle& row_style, const text::FontStack& fonts,
                  const icons::IconFont& icon_font);

    void set_count(std::size_t count);
    void set_expanded(bool expanded);
    bool expanded() const { return expanded_; }
    void place(const std::optional<Box>& box, const Column& column, bool animate);
    render::Rect rect() const { return box_.rect(); }

    HeaderAction handle(const input::PointerEvent& event);
    HeaderAction handle(const input::KeyEvent& event);
    void cancel_interaction();
    bool focused() const { return fold_focus_.focused(); }
    void set_focused(bool focused) { fold_focus_.set_focused(focused && shown()); }
    bool can_focus() const { return shown(); }
    widgets::Button& clear_button() { return clear_; }
    bool shown() const { return presence_.target() > 0.0; }
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    const HeaderStyle& style_;
    const RowStyle& row_style_;
    icons::IconFont icon_font_;
    widgets::Label title_;
    widgets::Button clear_;
    std::size_t count_ = 0;
    bool expanded_ = true;
    bool placed_ = false;
    Column column_{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    widgets::PressState press_;
    AnimatedBox box_;
    motion::Animated presence_;
    motion::Animated turn_;
    motion::Animated hover_;
    widgets::FocusIndicator fold_focus_;

    render::Rect toggle_rect() const;
};

}
