#include "todo/row.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/focus.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/text_field.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::todo {

namespace {

const tokens::OutlinedTextFieldTokens& field_tokens() {
    return current_style().text_field;
}

tokens::CheckboxState check_state(bool done) {
    return done ? tokens::CheckboxState::on : tokens::CheckboxState::off;
}

std::string title_text(const HeaderStyle& style, std::size_t count) {
    return std::vformat(style.title_format, std::make_format_args(count));
}

double unit(const motion::Animated& a) {
    return std::clamp(a.value(), 0.0, 1.0);
}

double focus_ring_reach() {
    return current_style().focus_indicator.outer_offset + current_style().focus_indicator.thickness;
}

}

TaskRow::TaskRow(const Item& item, const RowStyle& style, const text::FontStack& fonts, const icons::IconFont& icon_font,
                 input::Clipboard* clipboard)
    : style_(style),
      fonts_(fonts),
      clipboard_(clipboard),
      id_(item.id),
      text_(item.text),
      done_(item.done),
      check_({check_state(item.done)}),
      remove_({icons::Icon{icon_font, style.remove_icon}, style.remove_name, tokens::IconButtonStyle::standard, tokens::IconButtonSize::small}),
      box_(style.move_spring),
      presence_(0.0, style.fade_spring),
      done_amount_(item.done ? 1.0 : 0.0, style.fade_spring),
      hover_(0.0, style.fade_spring),
      focus_(0.0, style.fade_spring),
      remove_visibility_(0.0, style.fade_spring),
      edit_(0.0, style.fade_spring) {}

void TaskRow::sync(const Item& item) {
    if (item.text != text_) {
        text_ = item.text;
        laid_out_width_ = -1.0;
        relayout();
    }
    if (item.done != done_) {
        done_ = item.done;
        check_.set_state(check_state(done_));
    }
    done_amount_.animate_to(done_ ? 1.0 : 0.0);
}

void TaskRow::set_column(const Column& column) {
    column_ = column;
    relayout();
    for (auto* e : {&editor_, &closing_editor_}) {
        if (*e) {
            (*e)->set_width(column_.text_width + field_tokens().padding);
        }
    }
}

void TaskRow::relayout() {
    if (column_.text_width == laid_out_width_) {
        return;
    }
    laid_out_width_ = column_.text_width;
    block_ = text::layout_paragraph(fonts_, text_, current_style().type.text(style_.text_type), column_.text_width);
}

double TaskRow::target_height() const {
    if (editor_) {
        return std::max(style_.min_height, editor_->height() + 2.0 * style_.edit_padding);
    }
    return std::max(style_.min_height, block_.height + 2.0 * style_.text_padding);
}

void TaskRow::appear(double y, double height) {
    target_y_ = y;
    box_.snap_to(Box{column_.x, y, column_.width, 0.0});
    box_.move_to(Box{column_.x, y, column_.width, height}, true);
    presence_.snap_to(0.0);
    presence_.animate_to(1.0);
    arrange_widgets();
}

void TaskRow::snap(const RowPlace& p) {
    target_y_ = p.y;
    box_.snap_to(Box{column_.x, p.y, column_.width, p.height});
    presence_.snap_to(p.shown ? 1.0 : 0.0);
    arrange_widgets();
}

void TaskRow::place(const RowPlace& p, bool animate) {
    target_y_ = p.y;
    box_.move_to(Box{column_.x, p.y, column_.width, p.height}, animate);
    presence_.animate_to(p.shown ? 1.0 : 0.0);
    if (!p.shown) {
        cancel_interaction();
        clear_focus();
    }
}

void TaskRow::leave() {
    const render::Rect r = box_.rect();
    leaving_ = true;
    box_.move_to(Box{r.x, r.y, r.width, 0.0}, true);
    presence_.animate_to(0.0);
    cancel_interaction();
    clear_focus();
    editor_.reset();
    closing_editor_.reset();
    edit_.snap_to(0.0);
}

void TaskRow::revive() {
    leaving_ = false;
    presence_.animate_to(1.0);
}

render::Rect TaskRow::text_rect() const {
    const render::Rect r = box_.rect();
    return {column_.text_x, r.y, column_.remove_x - column_.text_x, r.height};
}

void TaskRow::arrange_widgets() {
    const render::Rect r = box_.rect();
    check_.set_origin({column_.x, r.y + (style_.min_height - check_.height()) / 2.0});
    remove_.set_origin({column_.remove_x, r.y + (style_.min_height - remove_.height()) / 2.0});
    for (auto* e : {&editor_, &closing_editor_}) {
        if (*e) {
            (*e)->set_origin({column_.text_x - field_tokens().padding, r.y + style_.edit_padding});
        }
    }
}

RowAction TaskRow::handle(const input::PointerEvent& e) {
    RowAction a;
    if (!shown()) {
        return a;
    }
    press_.update(e, box_.rect());
    if (editor_) {
        editor_->handle(e);
    } else if (e.action == input::PointerAction::down && widgets::contains(text_rect(), e.x_dp, e.y_dp)) {
        a.edit = e;
    }
    if (check_.handle(e)) {
        a.toggled = true;
    }
    if (remove_visibility_.target() > 0.0) {
        a.remove = remove_.handle(e);
    } else {
        remove_.cancel_interaction();
    }
    return a;
}

RowAction TaskRow::handle(const input::KeyEvent& e) {
    RowAction a;
    if (!shown()) {
        return a;
    }
    a.toggled = check_.handle(e);
    a.remove = !a.toggled && remove_.handle(e);
    return a;
}

void TaskRow::cancel_interaction() {
    press_ = {};
    check_.cancel_interaction();
    remove_.cancel_interaction();
    if (editor_) {
        editor_->cancel_interaction();
    }
}

bool TaskRow::focused() const {
    return editor_ ? editor_->focused() : focused_;
}

void TaskRow::set_focused(bool focused) {
    if (editor_) {
        editor_->set_focused(focused);
    } else {
        focused_ = focused;
    }
}

std::optional<RowPart> TaskRow::focused_part() const {
    if (check_.focused()) {
        return RowPart::check;
    }
    if (remove_.focused()) {
        return RowPart::remove;
    }
    if (focused()) {
        return RowPart::text;
    }
    return std::nullopt;
}

void TaskRow::focus_part(RowPart part) {
    switch (part) {
    case RowPart::check: check_.set_focused(true); break;
    case RowPart::text: set_focused(true); break;
    case RowPart::remove: remove_.set_focused(true); break;
    }
}

void TaskRow::clear_focus() {
    check_.set_focused(false);
    remove_.set_focused(false);
    set_focused(false);
}

void TaskRow::begin_edit(const std::optional<input::PointerEvent>& press) {
    closing_editor_.reset();
    edit_.animate_to(1.0);
    editor_.emplace(widgets::TextFieldOptions{.label = "", .multiline = false, .clipboard = clipboard_}, fonts_);
    editor_->set_width(column_.text_width + field_tokens().padding);
    editor_->set_text(text_);
    arrange_widgets();
    editor_->set_focused(true);
    if (press && widgets::contains(editor_->bounds(), press->x_dp, press->y_dp)) {
        editor_->handle(*press);
    } else {
        editor_->handle(input::KeyEvent{input::Key::end, false, true});
    }
    focused_ = false;
    keep_focus_ = !press;
}

std::string TaskRow::end_edit() {
    if (!editor_) {
        return text_;
    }
    std::string text = editor_->text();
    focused_ = keep_focus_ && editor_->focused();
    closing_editor_ = std::move(editor_);
    editor_.reset();
    closing_editor_->set_focused(false);
    edit_.animate_to(0.0);
    return text;
}

void TaskRow::step(double dt) {
    const bool show_remove = !leaving_ && !editor_ && (press_.hovered || focused_ || check_.focused() || remove_.focused());
    remove_visibility_.animate_to(show_remove ? 1.0 : 0.0);
    hover_.animate_to(press_.hovered && !editor_ ? 1.0 : 0.0);
    focus_.animate_to(focused_ && !editor_ ? 1.0 : 0.0);
    box_.step(dt);
    for (motion::Animated* a : {&presence_, &done_amount_, &hover_, &focus_, &remove_visibility_, &edit_}) {
        a->step(dt);
    }
    if (closing_editor_ && edit_.settled()) {
        closing_editor_.reset();
    }
    arrange_widgets();
    check_.step(dt);
    remove_.step(dt);
    for (auto* e : {&editor_, &closing_editor_}) {
        if (*e) {
            (*e)->step(dt);
        }
    }
}

bool TaskRow::animating() const {
    return !box_.settled() || !presence_.settled() || !done_amount_.settled() || !hover_.settled() || !focus_.settled() ||
           !remove_visibility_.settled() || !edit_.settled() || check_.animating() || remove_.animating() ||
           (editor_ && editor_->animating()) || (closing_editor_ && closing_editor_->animating());
}

void TaskRow::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double p = unit(presence_);
    const render::Rect r = box_.rect();
    if (p <= 0.0 || r.height <= 0.0) {
        return;
    }
    const color::ColorScheme s = p < 1.0 ? color::faded(scheme, p) : scheme;
    const double reach = moving() ? 0.0 : focus_ring_reach();
    canvas.push_clip({r.x - reach, r.y - reach, r.width + 2.0 * reach, r.height + 2.0 * reach});
    const double radius = current_style().shape.radius(style_.corner, r.width, r.height);
    if (const double h = unit(hover_); h > 0.0) {
        canvas.fill_rounded_rect(r, render::uniform_radii(radius),
                                 color::with_alpha(s[style_.state_layer], h * current_style().state_layer.hover));
    }
    if (const double f = unit(focus_); f > 0.0) {
        canvas.fill_path(render::ring_path(r, radius, style_.focus_ring_width), color::scale_alpha(s[style_.focus_ring], f),
                         render::FillRule::even_odd);
    }
    check_.paint(canvas, s);
    const double e = unit(edit_);
    for (const auto* field : {&editor_, &closing_editor_}) {
        if (*field && e > 0.0) {
            (*field)->paint(canvas, e < 1.0 ? color::faded(s, e) : s);
        }
    }
    if (e < 1.0) {
        const double d = unit(done_amount_);
        const color::Argb ink = color::scale_alpha(color::mix(s[style_.text], s[style_.done_text], d), 1.0 - e);
        double y = r.y + style_.text_padding;
        for (const text::TextLine& line : block_.lines) {
            text::draw_line(canvas, fonts_, line, {column_.text_x, y}, ink);
            if (d > 0.0) {
                const double strike = y + line.baseline - style_.strike_rise * line.style.size;
                canvas.fill_rect({column_.text_x, strike - style_.strike_thickness / 2.0, line.width * d, style_.strike_thickness}, ink);
            }
            y += line.box_height();
        }
    }
    if (const double v = unit(remove_visibility_); v > 0.0) {
        remove_.paint(canvas, v < 1.0 ? color::faded(s, v) : s);
    }
    canvas.pop_clip();
}

SectionHeader::SectionHeader(const HeaderStyle& style, const RowStyle& row_style, const text::FontStack& fonts,
                             const icons::IconFont& icon_font)
    : style_(style),
      row_style_(row_style),
      icon_font_(icon_font),
      title_({title_text(style, 0), style.title_type, style.title_color, false}, fonts),
      clear_({style.clear_label, tokens::ButtonStyle::text, tokens::ButtonSize::small}, fonts),
      box_(row_style.move_spring),
      presence_(0.0, row_style.fade_spring),
      turn_(1.0, row_style.move_spring),
      hover_(0.0, row_style.fade_spring) {}

void SectionHeader::set_count(std::size_t count) {
    if (count == count_) {
        return;
    }
    count_ = count;
    title_.set_text(title_text(style_, count_));
}

void SectionHeader::set_expanded(bool expanded) {
    expanded_ = expanded;
    turn_.animate_to(expanded ? 1.0 : 0.0);
}

void SectionHeader::place(const std::optional<Box>& box, const Column& column, bool animate) {
    column_ = column;
    if (!box) {
        presence_.animate_to(0.0);
        cancel_interaction();
        clear_.set_focused(false);
        fold_focus_.set_focused(false);
        return;
    }
    if (!placed_ || presence_.value() <= 0.0) {
        box_.snap_to(*box);
        placed_ = true;
    } else {
        box_.move_to(*box, animate);
    }
    presence_.animate_to(1.0);
}

HeaderAction SectionHeader::handle(const input::KeyEvent& e) {
    HeaderAction a;
    if (presence_.target() <= 0.0) {
        return a;
    }
    a.clear = clear_.handle(e);
    a.toggled = fold_focus_.activates(e, widgets::KeyActivation::space_or_enter);
    return a;
}

render::Rect SectionHeader::toggle_rect() const {
    const render::Rect r = box_.rect();
    return {r.x, r.y, clear_.bounds().x - r.x, r.height};
}

HeaderAction SectionHeader::handle(const input::PointerEvent& e) {
    HeaderAction a;
    if (presence_.target() <= 0.0) {
        return a;
    }
    fold_focus_.release_on_press(e);
    if (clear_.handle(e)) {
        a.clear = true;
        return a;
    }
    a.toggled = press_.update(e, toggle_rect());
    return a;
}

void SectionHeader::cancel_interaction() {
    press_ = {};
    clear_.cancel_interaction();
}

void SectionHeader::step(double dt) {
    hover_.animate_to(press_.hovered ? 1.0 : 0.0);
    box_.step(dt);
    for (motion::Animated* a : {&presence_, &turn_, &hover_}) {
        a->step(dt);
    }
    fold_focus_.step(dt);
    const render::Rect r = box_.rect();
    clear_.set_origin({r.x + r.width - clear_.width(), r.y + (r.height - clear_.height()) / 2.0});
    title_.measure({0.0, std::max(0.0, clear_.bounds().x - column_.text_x), 0.0, r.height});
    const render::Rect t = title_.bounds();
    title_.set_origin({column_.text_x, r.y + (r.height - t.height) / 2.0});
    clear_.step(dt);
}

bool SectionHeader::animating() const {
    return !box_.settled() || !presence_.settled() || !turn_.settled() || !hover_.settled() || !fold_focus_.settled() || clear_.animating();
}

void SectionHeader::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double p = unit(presence_);
    if (p <= 0.0) {
        return;
    }
    const color::ColorScheme s = p < 1.0 ? color::faded(scheme, p) : scheme;
    const render::Rect r = box_.rect();
    const render::Rect toggle = toggle_rect();
    const double radius = current_style().shape.radius(row_style_.corner, toggle.width, toggle.height);
    const double layer = std::max(unit(hover_) * current_style().state_layer.hover, fold_focus_.amount() * current_style().state_layer.focus);
    if (layer > 0.0) {
        canvas.fill_rounded_rect(toggle, render::uniform_radii(radius), color::with_alpha(s[row_style_.state_layer], layer));
    }
    const double size = style_.chevron_size;
    const render::Point center{column_.x + column_.check_width / 2.0, r.y + r.height / 2.0};
    const double angle = std::numbers::pi * turn_.value();
    const double c = std::cos(angle);
    const double sn = std::sin(angle);
    const render::Affine rotate{c, sn, -sn, c, center.x - c * center.x + sn * center.y, center.y - sn * center.x - c * center.y};
    const render::Path chevron = icon_font_.path(style_.chevron, {center.x - size / 2.0, center.y - size / 2.0}, size);
    canvas.fill_path(chevron.transformed(rotate), s[style_.chevron_color]);
    title_.paint(canvas, s);
    clear_.paint(canvas, s);
    fold_focus_.paint_inward(canvas, s, toggle, radius);
}

}
