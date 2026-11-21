#include "lamina/widgets/split_button.hpp"

#include <algorithm>

#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::SplitButtonTokens& t = tokens::split_button_tokens;

}

SplitButton::SplitButton(SplitButtonOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font)
    : options_(std::move(options)), rotation_(0.0, t.chevron_spring) {
    const tokens::SplitButtonSizeTokens& s = tokens::split_button_size(options_.size);
    ButtonOptions lead{options_.label, options_.style, options_.size, tokens::ButtonShape::round, options_.icon};
    lead.enabled = options_.enabled;
    lead.accessible_name = options_.label.empty() ? options_.accessible_name : std::string();
    lead.shapes = ButtonShapes{tokens::split_leading_corners(s.inner_corner), tokens::split_leading_corners(s.inner_pressed_corner),
                               std::nullopt, t.shape_spring};
    lead.padding = ButtonPadding{s.leading_leading_space, s.leading_trailing_space};
    lead.min_width = t.min_width;
    leading_ = std::make_unique<Button>(std::move(lead), fonts);

    ButtonOptions trail{"", options_.style, options_.size, tokens::ButtonShape::round, icons::Icon{icon_font, t.chevron}};
    trail.enabled = options_.enabled;
    trail.accessible_name = required_name(options_.trailing_name, "The trailing button of a split button");
    trail.toggle = ButtonToggle{.checked = false, .radio = false, .checked_colors = false, .checked_overlay = t.checked_overlay,
                                .fill_checked_icon = false};
    trail.shapes = ButtonShapes{tokens::split_trailing_corners(s.inner_corner), tokens::split_trailing_corners(s.inner_pressed_corner),
                                tokens::all_corners(tokens::CornerSize::full), t.shape_spring};
    trail.center_optically = true;
    trail.padding = ButtonPadding{s.trailing_leading_space, s.trailing_trailing_space};
    trail.min_width = t.min_width;
    trail.icon_size = s.trailing_icon_size;
    trailing_ = std::make_unique<Button>(std::move(trail), fonts);
    if (!options_.menu.empty()) {
        menu_ = std::make_unique<Menu>(MenuOptions{options_.menu, options_.trailing_name}, fonts);
    }
    set_origin(origin_);
}

double SplitButton::width() const {
    return leading_->width() + t.between_space + trailing_->width();
}

double SplitButton::height() const {
    return std::max(leading_->height(), trailing_->height());
}

void SplitButton::set_origin(render::Point origin) {
    origin_ = origin;
    const double h = height();
    leading_->set_origin({origin.x, origin.y + (h - leading_->height()) / 2.0});
    trailing_->set_origin({origin.x + leading_->width() + t.between_space, origin.y + (h - trailing_->height()) / 2.0});
}

void SplitButton::set_checked(bool checked) {
    trailing_->set_checked(checked);
    sync_menu(input::InputSource::pointer);
}

void SplitButton::sync_menu(input::InputSource source) {
    if (!menu_) {
        return;
    }
    if (trailing_->checked() && !menu_->is_open()) {
        menu_->open(bounds(), window_, source);
    } else if (!trailing_->checked() && menu_->is_open()) {
        menu_->close();
    }
}

void SplitButton::set_enabled(bool enabled) {
    leading_->set_enabled(enabled);
    trailing_->set_enabled(enabled);
    if (!enabled) {
        set_checked(false);
    }
}

AccessibleNode SplitButton::accessible() const {
    AccessibleNode node{AccessibleRole::group, options_.accessible_name.empty() ? options_.label : options_.accessible_name, bounds(),
                        {.enabled = enabled()}};
    node.children.push_back(leading_->accessible());
    AccessibleNode trail = trailing_->accessible();
    trail.value = checked() ? options_.expanded_state : options_.collapsed_state;
    node.children.push_back(std::move(trail));
    if (menu_) {
        if (auto m = menu_->accessible()) {
            node.children.push_back(std::move(*m));
        }
    }
    return node;
}

SplitButtonResponse SplitButton::handle(const input::PointerEvent& e) {
    SplitButtonResponse response;
    if (menu_open()) {
        const bool inside = contains(trailing_->bounds(), e.x_dp, e.y_dp);
        const MenuResponse m = menu_->handle(e);
        if (m.chosen) {
            response.chosen = m.chosen;
            response.consumed = true;
            trailing_->set_checked(false);
            return response;
        }
        if (!menu_->is_open() && !inside) {
            trailing_->set_checked(false);
        }
        if (m.consumed && !inside) {
            response.consumed = true;
            return response;
        }
    }
    if (leading_->handle(e)) {
        response.leading_clicked = true;
        response.consumed = true;
    }
    if (trailing_->handle(e)) {
        response.trailing_toggled = true;
        response.consumed = true;
        sync_menu(input::InputSource::pointer);
    }
    return response;
}

SplitButtonResponse SplitButton::handle(const input::KeyEvent& e) {
    SplitButtonResponse response;
    if (menu_open()) {
        const MenuResponse m = menu_->handle(e);
        response.consumed = m.consumed;
        if (m.chosen) {
            response.chosen = m.chosen;
        }
        if (!menu_->is_open()) {
            trailing_->set_checked(false);
            trailing_->set_focused(true);
        }
        return response;
    }
    if (leading_->handle(e)) {
        response.leading_clicked = true;
        response.consumed = true;
    }
    if (trailing_->handle(e)) {
        response.trailing_toggled = true;
        response.consumed = true;
        sync_menu(input::InputSource::keyboard);
    }
    return response;
}

void SplitButton::cancel_interaction() {
    leading_->cancel_interaction();
    trailing_->cancel_interaction();
    if (menu_) {
        menu_->cancel_interaction();
    }
}

void SplitButton::step(double dt) {
    leading_->step(dt);
    trailing_->step(dt);
    if (menu_) {
        menu_->step(dt);
    }
    rotation_.animate_to(trailing_->checked() ? t.chevron_turn : 0.0);
    rotation_.step(dt);
    trailing_->set_icon_rotation(rotation_.value());
    set_origin(origin_);
}

bool SplitButton::animating() const {
    const double target = trailing_->checked() ? t.chevron_turn : 0.0;
    return leading_->animating() || trailing_->animating() || !rotation_.settled() || rotation_.target() != target ||
           (menu_ && menu_->animating());
}

void SplitButton::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    leading_->paint(canvas, scheme);
    trailing_->paint(canvas, scheme);
}

void SplitButton::paint_overlay(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (menu_ && menu_->visible()) {
        menu_->paint(canvas, scheme);
    }
}

}
