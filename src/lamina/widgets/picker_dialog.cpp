#include "lamina/style/style.hpp"
#include "lamina/widgets/picker_dialog.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/picker_dialog.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

namespace {

const tokens::PickerDialogTokens& t() {
    return current_style().picker_dialog;
}
const tokens::DialogTokens& d() {
    return current_style().dialog;
}

text::TextStyle title_style() {
    return tokens::text_style(tokens::type_style(t().time_title_type));
}

}

PickerDialog::PickerDialog(PickerDialogOptions options, std::unique_ptr<DialogPane> pane, const text::FontStack& fonts)
    : options_(std::move(options)),
      pane_(std::move(pane)),
      fonts_(fonts),
      confirm_({options_.confirm_label, t().action_style, t().action_size}, fonts),
      dismiss_({options_.dismiss_label, t().action_style, t().action_size}, fonts),
      popup_(d().closed_scale, tokens::motion_scheme.default_spatial(), tokens::motion_scheme.default_effects()) {
    if (options_.leading) {
        leading_ = std::make_unique<IconButton>(IconButtonOptions{options_.leading->icon, options_.leading->accessible_name,
                                                                  tokens::IconButtonStyle::standard, tokens::IconButtonSize::small,
                                                                  tokens::ButtonShape::round, tokens::IconButtonWidth::uniform, false, false,
                                                                  true, color::Role::on_surface_variant});
    }
    pane_->add_focus_stops(focus_);
    if (leading_) {
        focus_.add(*leading_);
    }
    focus_.add(dismiss_);
    focus_.add(confirm_);
}

void PickerDialog::set_title(std::string title) {
    options_.title = std::move(title);
}

void PickerDialog::set_labels(std::string confirm, std::string dismiss) {
    confirm_.set_label(std::move(confirm));
    dismiss_.set_label(std::move(dismiss));
}

double PickerDialog::padding() const {
    return options_.layout == PickerDialogLayout::time ? t().time_padding : 0.0;
}

double PickerDialog::title_height() const {
    if (options_.layout != PickerDialogLayout::time || options_.title.empty()) {
        return 0.0;
    }
    return title_style().line_height + t().time_title_bottom;
}

double PickerDialog::actions_height() const {
    const double buttons = std::max(confirm_.height(), leading_ ? leading_->height() : 0.0);
    return options_.layout == PickerDialogLayout::date ? buttons + t().date_actions_bottom : buttons;
}

void PickerDialog::place() {
    const double p = padding();
    const double w = pane_->width() + 2.0 * p;
    const double h = p + title_height() + pane_->height() + actions_height() + p;
    surface_ = {area_.x + (area_.width - w) / 2.0, area_.y + (area_.height - h) / 2.0, w, h};
    pane_->set_origin({surface_.x + p, surface_.y + p + title_height()});
    const double actions_top = surface_.y + p + title_height() + pane_->height();
    const double end = options_.layout == PickerDialogLayout::date ? surface_.x + w - t().date_actions_end : surface_.x + w - p;
    const double row = std::max(confirm_.height(), leading_ ? leading_->height() : 0.0);
    confirm_.set_origin({end - confirm_.width(), actions_top + (row - confirm_.height()) / 2.0});
    dismiss_.set_origin({end - confirm_.width() - t().actions_spacing - dismiss_.width(), actions_top + (row - dismiss_.height()) / 2.0});
    if (leading_) {
        leading_->set_origin({surface_.x + p, actions_top + (row - leading_->height()) / 2.0});
    }
}

void PickerDialog::open_in(const render::Rect& area, input::InputSource source) {
    area_ = area;
    popup_.open();
    place();
    focus_.clear_focus();
    if (source == input::InputSource::keyboard) {
        focus_.move_focus(FocusDirection::next);
    }
}

void PickerDialog::close() {
    popup_.close();
    focus_.clear_focus();
    pane_->clear_focus();
    cancel_interaction();
}

void PickerDialog::set_area(const render::Rect& area) {
    area_ = area;
    place();
}

DialogResult PickerDialog::finish(DialogResult result) {
    if (result != DialogResult::none) {
        close();
    }
    return result;
}

DialogResult PickerDialog::handle(const input::PointerEvent& e) {
    if (!popup_.is_open()) {
        return DialogResult::none;
    }
    if (popup_.dismisses(e, surface_) && contains(area_, e.x_dp, e.y_dp)) {
        return finish(DialogResult::dismissed);
    }
    pane_->handle(e);
    if (leading_ && leading_->handle(e) && options_.leading->on_click) {
        options_.leading->on_click(*leading_);
    }
    if (dismiss_.handle(e)) {
        return finish(DialogResult::dismissed);
    }
    if (confirm_.handle(e)) {
        return finish(DialogResult::confirmed);
    }
    return DialogResult::none;
}

DialogResult PickerDialog::handle(const input::KeyEvent& e) {
    if (!popup_.is_open()) {
        return DialogResult::none;
    }
    if (popup_.dismisses(e)) {
        return finish(DialogResult::dismissed);
    }
    if (pane_->handle(e)) {
        return DialogResult::none;
    }
    if (focus_.handle(e)) {
        return DialogResult::none;
    }
    if (leading_ && leading_->focused() && leading_->handle(e) && options_.leading->on_click) {
        options_.leading->on_click(*leading_);
        return DialogResult::none;
    }
    if (dismiss_.focused() && dismiss_.handle(e)) {
        return finish(DialogResult::dismissed);
    }
    if (confirm_.focused() && confirm_.handle(e)) {
        return finish(DialogResult::confirmed);
    }
    return DialogResult::none;
}

bool PickerDialog::handle(const input::TextEvent& e) {
    if (!popup_.is_open()) {
        return false;
    }
    pane_->handle(e);
    return true;
}

bool PickerDialog::handle(const input::WheelEvent& e) {
    if (!popup_.is_open()) {
        return false;
    }
    pane_->handle(e);
    return true;
}

void PickerDialog::cancel_interaction() {
    pane_->cancel_interaction();
    confirm_.cancel_interaction();
    dismiss_.cancel_interaction();
    if (leading_) {
        leading_->cancel_interaction();
    }
}

void PickerDialog::step(double dt) {
    popup_.step(dt);
    pane_->step(dt);
    confirm_.step(dt);
    dismiss_.step(dt);
    if (leading_) {
        leading_->step(dt);
    }
    if (popup_.visible()) {
        place();
    }
}

bool PickerDialog::animating() const {
    return !popup_.settled() || (popup_.visible() && (pane_->animating() || confirm_.animating() || dismiss_.animating() ||
                                                      (leading_ && leading_->animating())));
}

std::optional<render::Rect> PickerDialog::caret_area() const {
    return popup_.is_open() ? pane_->caret_area() : std::nullopt;
}

std::optional<double> PickerDialog::next_wake_seconds() const {
    return popup_.is_open() ? pane_->next_wake_seconds() : std::nullopt;
}

void PickerDialog::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!popup_.visible()) {
        return;
    }
    const double a = popup_.opacity();
    canvas.fill_rect(area_, color::with_alpha(scheme[d().scrim], d().scrim_opacity * a));
    const color::ColorScheme faded = color::faded(scheme, a);
    canvas.save();
    popup_.scale_about(canvas, {surface_.x + surface_.width / 2.0, surface_.y + surface_.height / 2.0});
    const double radius = tokens::corner_radius(options_.corner, surface_.width, surface_.height);
    const render::Path shape = render::rounded_rect_path(surface_, render::uniform_radii(radius));
    canvas.fill_path(shape, faded[d().container]);
    canvas.push_clip(shape);
    if (title_height() > 0.0) {
        const text::TextLine line = text::layout_line(fonts_, options_.title, title_style());
        text::draw_line(canvas, fonts_, line, {surface_.x + padding(), surface_.y + padding()}, faded[t().time_title_color]);
    }
    pane_->paint(canvas, faded);
    if (leading_) {
        leading_->paint(canvas, faded);
    }
    dismiss_.paint(canvas, faded);
    confirm_.paint(canvas, faded);
    canvas.pop_clip();
    canvas.restore();
}

std::optional<AccessibleNode> PickerDialog::accessible() const {
    if (!popup_.is_open()) {
        return std::nullopt;
    }
    AccessibleNode node{AccessibleRole::dialog, options_.accessible_name.empty() ? options_.title : options_.accessible_name, surface_,
                        {.modal = true}};
    node.children.push_back(pane_->accessible());
    if (leading_) {
        node.children.push_back(leading_->accessible());
    }
    node.children.push_back(dismiss_.accessible());
    node.children.push_back(confirm_.accessible());
    return node;
}

}
