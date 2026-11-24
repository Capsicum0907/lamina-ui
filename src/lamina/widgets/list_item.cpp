#include "lamina/style/style.hpp"
#include "lamina/widgets/list_item.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"

namespace lamina::widgets {

namespace {

const tokens::ListTokens& t() {
    return current_style().list;
}

text::TextStyle style_of(tokens::TypeRole role) {
    return tokens::text_style(tokens::type_style(role));
}

const tokens::DenseLength& line_density(ListLines lines) {
    switch (lines) {
    case ListLines::three:
        return tokens::density_tokens.list_three_line;
    case ListLines::two:
        return tokens::density_tokens.list_two_line;
    case ListLines::one:
        break;
    }
    return tokens::density_tokens.list_one_line;
}

double min_height(ListLines lines, tokens::Density density) {
    return line_density(lines).at(density);
}

double vertical_space(double space, ListLines lines, tokens::Density density) {
    const tokens::DenseLength& l = line_density(lines);
    return std::max(0.0, space - (l.base - l.at(density)) / 2.0);
}

bool takes_toggle(ListItemAction action) {
    return action == ListItemAction::toggle;
}

}

double list_item_estimate(const ListItemSpec& spec) {
    if (!spec.overline.empty() && !spec.supporting.empty()) {
        return min_height(ListLines::three, spec.density);
    }
    if (!spec.overline.empty() || !spec.supporting.empty()) {
        return min_height(ListLines::two, spec.density);
    }
    return min_height(ListLines::one, spec.density);
}

void ListItem::set_density(tokens::Density density) {
    spec_.density = density;
    text_width_ = -1.0;
}

ListItem::ListItem(ListItemSpec spec, const text::FontStack& fonts)
    : spec_(std::move(spec)),
      fonts_(fonts),
      enabled_(spec_.enabled),
      selected_(spec_.selected ? 1.0 : 0.0, t().color_spring),
      corner_(0.0, t().shape_spring),
      state_layer_(0.0, t().state_layer_spring) {
    if (spec_.headline.empty()) {
        spec_.accessible_name = required_name(std::move(spec_.accessible_name), "A list item without a headline");
    }
    if (spec_.action == ListItemAction::none) {
        spec_.selected = false;
        selected_.snap_to(0.0);
    }
    if (!spec_.overline.empty()) {
        overline_ = text::layout_line(fonts_, spec_.overline, style_of(t().overline));
    }
    build_slot(leading_, spec_.leading);
    build_slot(trailing_, spec_.trailing);
    corner_.snap_to(target_corner());
}

void ListItem::build_slot(Slot& slot, const ListSlot& spec) {
    slot.spec = spec;
    const bool on = spec_.selected;
    const std::string name = spec_.accessible_name.empty() ? spec_.headline : spec_.accessible_name;
    switch (spec.kind) {
    case ListSlotKind::checkbox:
        slot.checkbox.emplace(CheckboxOptions{on ? tokens::CheckboxState::on : tokens::CheckboxState::off, spec_.enabled, name});
        break;
    case ListSlotKind::switch_control:
        slot.switch_control.emplace(SwitchOptions{.checked = on, .enabled = spec_.enabled, .accessible_name = name});
        break;
    case ListSlotKind::radio_button:
        slot.radio.emplace(RadioButtonOptions{on, spec_.enabled, name});
        break;
    case ListSlotKind::avatar:
        if (!spec.image) {
            slot.text = text::layout_line(fonts_, spec.text, style_of(t().avatar_label));
        }
        break;
    case ListSlotKind::text:
        slot.text = text::layout_line(fonts_, spec.text, style_of(t().trailing_text));
        break;
    case ListSlotKind::none:
    case ListSlotKind::icon:
    case ListSlotKind::image:
        break;
    }
}

layout::Size ListItem::slot_size(const Slot& slot) {
    switch (slot.spec.kind) {
    case ListSlotKind::none:
        return {0.0, 0.0};
    case ListSlotKind::icon:
        return {t().icon_size, t().icon_size};
    case ListSlotKind::avatar:
        return {t().avatar_size, t().avatar_size};
    case ListSlotKind::image:
        return {t().image_size, t().image_size};
    case ListSlotKind::checkbox:
        return {t().checkbox_slot, t().checkbox_slot};
    case ListSlotKind::radio_button:
        return {t().radio_slot, t().radio_slot};
    case ListSlotKind::switch_control:
        return {t().switch_width, t().switch_height};
    case ListSlotKind::text:
        return {slot.text->width, slot.text->box_height()};
    }
    return {0.0, 0.0};
}

void ListItem::layout_text(double width) {
    if (width == text_width_) {
        return;
    }
    text_width_ = width;
    headline_ = text::layout_paragraph(fonts_, spec_.headline, style_of(t().headline), width);
    supporting_ = spec_.supporting.empty() ? text::TextBlock{} : text::layout_paragraph(fonts_, spec_.supporting, style_of(t().supporting), width);
    const bool overline = overline_.has_value();
    const bool supporting = !spec_.supporting.empty();
    if ((overline && supporting) || supporting_.lines.size() > 1) {
        lines_ = ListLines::three;
    } else if (overline || supporting) {
        lines_ = ListLines::two;
    } else {
        lines_ = ListLines::one;
    }
    const double text_h = (overline_ ? overline_->box_height() : 0.0) + headline_.height + supporting_.height;
    const double content = std::max({slot_size(leading_).height, text_h, slot_size(trailing_).height});
    height_ = std::max(min_height(lines_, spec_.density),
                       vertical_space(t().top_space, lines_, spec_.density) + vertical_space(t().bottom_space, lines_, spec_.density) + content);
}

double ListItem::height_for(double width) {
    const layout::Size lead = slot_size(leading_);
    const layout::Size trail = slot_size(trailing_);
    double text_w = width - t().leading_space - t().trailing_space;
    if (lead.width > 0.0) {
        text_w -= lead.width + t().between_space;
    }
    if (trail.width > 0.0) {
        text_w -= trail.width + t().between_space;
    }
    layout_text(std::max(0.0, text_w));
    return height_;
}

layout::Size ListItem::measure(const layout::Constraints& c) {
    const double width = std::isfinite(c.max_width) ? c.max_width : c.min_width;
    return c.constrain({width, height_for(width)});
}

void ListItem::place_slot(Slot& slot, double x, double top, double inner, bool top_aligned) {
    const layout::Size s = slot_size(slot);
    const double y = top_aligned ? top : top + (inner - s.height) / 2.0;
    slot.rect = {x, y, s.width, s.height};
    const render::Point c{x + s.width / 2.0, y + s.height / 2.0};
    if (slot.checkbox) {
        slot.checkbox->set_origin({c.x - slot.checkbox->width() / 2.0, c.y - slot.checkbox->height() / 2.0});
    }
    if (slot.switch_control) {
        slot.switch_control->set_origin({c.x - slot.switch_control->width() / 2.0, c.y - slot.switch_control->height() / 2.0});
    }
    if (slot.radio) {
        slot.radio->set_origin({c.x - slot.radio->width() / 2.0, c.y - slot.radio->height() / 2.0});
    }
}

void ListItem::arrange(const render::Rect& r) {
    height_for(r.width);
    rect_ = r;
    const double top_space = vertical_space(t().top_space, lines_, spec_.density);
    const double top = r.y + top_space;
    const double inner = r.height - top_space - vertical_space(t().bottom_space, lines_, spec_.density);
    const bool top_aligned = inner >= t().top_align_breakpoint;
    double x = r.x + t().leading_space;
    if (leading_.spec.kind != ListSlotKind::none) {
        place_slot(leading_, x, top, inner, top_aligned);
        x += leading_.rect.width + t().between_space;
    }
    if (trailing_.spec.kind != ListSlotKind::none) {
        place_slot(trailing_, r.x + r.width - t().trailing_space - slot_size(trailing_).width, top, inner, top_aligned);
    }
    const double text_h = (overline_ ? overline_->box_height() : 0.0) + headline_.height + supporting_.height;
    text_at_ = {x, top_aligned ? top : top + (inner - text_h) / 2.0};
}

render::Rect ListItem::leading_rect() const {
    return leading_.rect;
}

render::Rect ListItem::trailing_rect() const {
    return trailing_.rect;
}

void ListItem::sync_controls() {
    const bool on = spec_.selected;
    for (Slot* slot : {&leading_, &trailing_}) {
        if (slot->checkbox) {
            slot->checkbox->set_state(on ? tokens::CheckboxState::on : tokens::CheckboxState::off);
        }
        if (slot->switch_control) {
            slot->switch_control->set_checked(on);
        }
        if (slot->radio) {
            slot->radio->set_selected(on);
        }
    }
}

void ListItem::set_selected(bool selected) {
    if (!interactive() || selected == spec_.selected) {
        return;
    }
    spec_.selected = selected;
    selected_.animate_to(selected ? 1.0 : 0.0);
    sync_controls();
    retarget();
}

double ListItem::target_corner() const {
    tokens::CornerSize c = t().corner;
    if (interactive() && enabled()) {
        if (press_.pressed) {
            c = t().pressed_corner;
        } else if (spec_.selected) {
            c = t().selected_corner;
        } else if (focus_.focused()) {
            c = t().focused_corner;
        } else if (press_.hovered) {
            c = t().hovered_corner;
        }
    } else if (interactive() && spec_.selected) {
        c = t().selected_corner;
    }
    return tokens::corner_radius_dp[static_cast<std::size_t>(c)];
}

void ListItem::retarget() {
    state_layer_.animate_to(interactive() ? focus_.state_layer_opacity(press_.state_layer_opacity()) : 0.0);
    corner_.animate_to(target_corner());
}

void ListItem::set_focused(bool focused) {
    focus_.set_focused(focused && can_focus());
    retarget();
}

void ListItem::set_enabled(bool enabled) {
    for (Slot* slot : {&leading_, &trailing_}) {
        if (slot->checkbox) {
            slot->checkbox->set_enabled(enabled);
        }
        if (slot->switch_control) {
            slot->switch_control->set_enabled(enabled);
        }
        if (slot->radio) {
            slot->radio->set_enabled(enabled);
        }
    }
    if (!enabled_.set(enabled)) {
        return;
    }
    spec_.enabled = enabled;
    if (!enabled) {
        press_ = {};
        focus_.set_focused(false);
    }
    retarget();
}

AccessibleNode ListItem::accessible() const {
    AccessibleRole role = AccessibleRole::list_item;
    AccessibleState state{.enabled = enabled(), .focusable = can_focus(), .focused = focused()};
    if (spec_.action == ListItemAction::toggle) {
        role = AccessibleRole::checkbox;
        state.toggle = spec_.selected ? AccessibleToggle::on : AccessibleToggle::off;
    } else if (spec_.action == ListItemAction::select) {
        role = AccessibleRole::radio_button;
        state.selected = spec_.selected;
    }
    AccessibleNode node{role, spec_.accessible_name.empty() ? spec_.headline : spec_.accessible_name, rect_, state};
    node.description = spec_.supporting;
    if (trailing_.spec.kind == ListSlotKind::text) {
        node.value = trailing_.spec.text;
    }
    return node;
}

void ListItem::activate(ListItemResponse& response) {
    response.clicked = true;
    response.consumed = true;
    if (spec_.action == ListItemAction::toggle) {
        set_selected(!spec_.selected);
    } else if (spec_.action == ListItemAction::select) {
        set_selected(true);
    }
}

ListItemResponse ListItem::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    ListItemResponse response;
    if (!interactive() || !enabled()) {
        return response;
    }
    if (press_.update(e, rect_)) {
        activate(response);
    }
    retarget();
    return response;
}

ListItemResponse ListItem::handle(const input::KeyEvent& e) {
    ListItemResponse response;
    if (can_focus() && focus_.activates(e, KeyActivation::space_or_enter)) {
        activate(response);
        retarget();
    }
    return response;
}

void ListItem::cancel_interaction() {
    press_ = {};
    retarget();
}

void ListItem::step(double dt) {
    focus_.step(dt);
    enabled_.step(dt);
    selected_.step(dt);
    corner_.step(dt);
    state_layer_.step(dt);
    for (Slot* slot : {&leading_, &trailing_}) {
        if (slot->checkbox) {
            slot->checkbox->step(dt);
        }
        if (slot->switch_control) {
            slot->switch_control->step(dt);
        }
        if (slot->radio) {
            slot->radio->step(dt);
        }
    }
}

bool ListItem::animating() const {
    bool controls = false;
    for (const Slot* slot : {&leading_, &trailing_}) {
        controls = controls || (slot->checkbox && slot->checkbox->animating()) || (slot->switch_control && slot->switch_control->animating()) ||
                   (slot->radio && slot->radio->animating());
    }
    return controls || !focus_.settled() || !enabled_.settled() || !selected_.settled() || !corner_.settled() || !state_layer_.settled();
}

void ListItem::paint_slot(render::Canvas& canvas, const color::ColorScheme& scheme, const Slot& slot, color::Argb color) const {
    const render::Rect& r = slot.rect;
    const ListSlot& s = slot.spec;
    switch (s.kind) {
    case ListSlotKind::none:
        return;
    case ListSlotKind::icon:
        s.icon->font.draw(canvas, s.icon->symbol, {r.x, r.y}, t().icon_size, color);
        return;
    case ListSlotKind::avatar: {
        const render::Point c{r.x + r.width / 2.0, r.y + r.height / 2.0};
        const double fade = enabled_.amount() + (1.0 - enabled_.amount()) * t().disabled_content.opacity;
        if (s.image) {
            canvas.push_clip(render::circle_path(c, r.width / 2.0));
            canvas.draw_image(*s.image, r, fade);
            canvas.pop_clip();
            return;
        }
        canvas.fill_path(render::circle_path(c, r.width / 2.0), color::scale_alpha(scheme[t().avatar_container], fade));
        text::draw_line(canvas, fonts_, *slot.text, {c.x - slot.text->width / 2.0, c.y - slot.text->box_height() / 2.0},
                        color::scale_alpha(scheme[t().avatar_content], fade));
        return;
    }
    case ListSlotKind::image: {
        const double fade = enabled_.amount() + (1.0 - enabled_.amount()) * t().disabled_content.opacity;
        canvas.push_clip(render::rounded_rect_path(r, render::uniform_radii(tokens::corner_radius(t().image_corner, r.width, r.height))));
        canvas.draw_image(*s.image, r, fade);
        canvas.pop_clip();
        return;
    }
    case ListSlotKind::checkbox:
        slot.checkbox->paint(canvas, scheme);
        return;
    case ListSlotKind::switch_control:
        slot.switch_control->paint(canvas, scheme);
        return;
    case ListSlotKind::radio_button:
        slot.radio->paint(canvas, scheme);
        return;
    case ListSlotKind::text:
        text::draw_line(canvas, fonts_, *slot.text, {r.x, r.y}, color);
        return;
    }
}

void ListItem::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double k = std::clamp(selected_.value(), 0.0, 1.0);
    const color::Argb disabled = resolve(scheme, t().disabled_content);
    const color::Argb headline = enabled_.mix(disabled, color::mix(scheme[t().headline_color], scheme[t().selected_content], k));
    const color::Argb content = enabled_.mix(disabled, color::mix(scheme[t().content_color], scheme[t().selected_content], k));
    const render::CornerRadii radii = render::uniform_radii(std::clamp(corner_.value(), 0.0, std::min(rect_.width, rect_.height) / 2.0));
    if (spec_.transparent) {
        const render::Path shape = render::rounded_rect_path(rect_, radii);
        canvas.fill_path(shape, color::scale_alpha(scheme[t().selected_container], k * enabled_.amount()));
        canvas.fill_path(shape, color::with_alpha(headline, std::clamp(state_layer_.value(), 0.0, 1.0)));
    } else {
        const color::Argb container = color::mix(scheme[t().container], scheme[t().selected_container], k * enabled_.amount());
        canvas.fill_path(render::rounded_rect_path(rect_, radii), color::state_layer(container, headline, state_layer_.value()));
    }

    paint_slot(canvas, scheme, leading_, content);
    paint_slot(canvas, scheme, trailing_, content);
    double y = text_at_.y;
    if (overline_) {
        text::draw_line(canvas, fonts_, *overline_, {text_at_.x, y}, content);
        y += overline_->box_height();
    }
    text::draw_block(canvas, fonts_, headline_, {text_at_.x, y}, headline);
    y += headline_.height;
    if (!supporting_.lines.empty()) {
        text::draw_block(canvas, fonts_, supporting_, {text_at_.x, y}, content);
    }
    focus_.paint_inward(canvas, scheme, rect_, radii);
}

}
