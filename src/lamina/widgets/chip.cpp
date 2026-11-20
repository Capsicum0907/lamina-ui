#include "lamina/widgets/chip.hpp"

#include <algorithm>
#include <stdexcept>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/render/path.hpp"
#include "lamina/widgets/paint.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::ChipTokens& t = tokens::chip_tokens;
constexpr const tokens::ChipMotion& m = tokens::chip_motion;

text::TextStyle label_style() {
    return tokens::text_style(tokens::type_style(t.label));
}

bool is_selectable(const ChipOptions& o) {
    return tokens::chip_selectable(o.kind);
}

}

Chip::Chip(ChipOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      label_(text::layout_line(fonts_, options_.label, label_style())),
      enabled_(options_.enabled),
      elevation_(tokens::chip_style(options_.kind, options_.elevated).elevation, options_.enabled),
      selected_(options_.selected ? 1.0 : 0.0, m.colors),
      state_layer_(0.0, m.state_layer),
      leading_size_(0.0, m.slot_expand),
      leading_alpha_(0.0, m.slot_fade_in) {
    if (options_.label.empty()) {
        options_.accessible_name = required_name(std::move(options_.accessible_name), "A chip without a label");
    }
    if (!is_selectable(options_)) {
        options_.selected = false;
        selected_.snap_to(0.0);
    }
    if (options_.kind != tokens::ChipKind::input) {
        options_.avatar.reset();
        options_.removable = false;
    }
    if (options_.kind == tokens::ChipKind::suggestion) {
        options_.trailing_icon.reset();
    }
    if (options_.removable && !options_.trailing_icon) {
        throw std::invalid_argument("A removable chip needs a trailing icon to remove it with");
    }
    const double on = has_leading() ? 1.0 : 0.0;
    leading_size_.snap_to(on);
    leading_alpha_.snap_to(on);
}

const std::optional<icons::Icon>& Chip::shown_leading() const {
    if (options_.selected && options_.selected_leading_icon) {
        return options_.selected_leading_icon;
    }
    return options_.leading_icon;
}

bool Chip::has_leading() const {
    return options_.avatar.has_value() || shown_leading().has_value();
}

double Chip::leading_width() const {
    return options_.avatar ? t.avatar_size : t.icon_size;
}

double Chip::start_padding() const {
    if (options_.kind != tokens::ChipKind::input) {
        return t.padding;
    }
    return options_.avatar || !shown_leading() ? t.input_edge_padding : t.input_icon_padding;
}

double Chip::end_padding() const {
    if (options_.kind != tokens::ChipKind::input) {
        return t.padding;
    }
    return options_.trailing_icon ? t.input_icon_padding : t.input_edge_padding;
}

double Chip::trailing_width() const {
    return options_.trailing_icon ? t.icon_size : 0.0;
}

double Chip::content_width(double leading_fraction) const {
    return start_padding() + leading_width() * std::max(0.0, leading_fraction) + t.element_spacing + label_.width + t.element_spacing +
           trailing_width() + end_padding();
}

double Chip::width() const {
    return content_width(leading_size_.value());
}

double Chip::target_width() const {
    return content_width(leading_size_.target());
}

double Chip::height() const {
    return t.height;
}

render::Rect Chip::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

std::optional<render::Rect> Chip::leading_rect() const {
    if (leading_size_.value() <= 0.0) {
        return std::nullopt;
    }
    const double w = leading_width();
    return render::Rect{origin_.x + start_padding(), origin_.y + (t.height - w) / 2.0, w * std::min(1.0, leading_size_.value()), w};
}

std::optional<render::Rect> Chip::trailing_rect() const {
    if (!options_.trailing_icon) {
        return std::nullopt;
    }
    const render::Rect r = bounds();
    return render::Rect{r.x + r.width - end_padding() - t.icon_size, r.y + (t.height - t.icon_size) / 2.0, t.icon_size, t.icon_size};
}

std::optional<render::Rect> Chip::remove_target() const {
    if (!options_.removable) {
        return std::nullopt;
    }
    const render::Rect icon = *trailing_rect();
    const render::Rect target = touch_target(bounds());
    const double x = icon.x - t.element_spacing / 2.0;
    return render::Rect{x, target.y, target.x + target.width - x, target.height};
}

void Chip::update_leading() {
    const bool on = has_leading();
    if (on == (leading_size_.target() > 0.5)) {
        return;
    }
    if (on) {
        exiting_leading_.reset();
        leading_size_.set_spec(m.slot_expand);
        leading_alpha_.set_spec(m.slot_fade_in);
        leading_size_.animate_to(1.0);
        leading_alpha_.animate_to(1.0);
        return;
    }
    leading_size_.set_spec(m.slot_shrink);
    leading_alpha_.set_spec(m.slot_fade_out);
    leading_size_.animate_to(0.0);
    leading_alpha_.animate_to(0.0);
}

void Chip::set_selected(bool selected) {
    if (!is_selectable(options_) || selected == options_.selected) {
        return;
    }
    const std::optional<icons::Icon> before = shown_leading();
    options_.selected = selected;
    selected_.animate_to(selected ? 1.0 : 0.0);
    if (before && !shown_leading()) {
        exiting_leading_ = before;
    }
    update_leading();
}

AccessibleNode Chip::accessible() const {
    AccessibleState state{.enabled = enabled(), .focusable = enabled(), .focused = focused()};
    if (is_selectable(options_)) {
        state.toggle = options_.selected ? AccessibleToggle::on : AccessibleToggle::off;
        state.selected = options_.selected;
    }
    AccessibleNode node{is_selectable(options_) ? AccessibleRole::checkbox : AccessibleRole::button, accessible_name(), bounds(), state};
    if (options_.removable) {
        node.children.push_back(
            {AccessibleRole::button, options_.remove_name + " " + accessible_name(), *remove_target(), {.enabled = enabled()}});
    }
    return node;
}

void Chip::retarget() {
    const PressState combined{press_.hovered || remove_press_.hovered, press_.pressed || remove_press_.pressed};
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
    elevation_.update(combined.hovered, focus_.focused(), combined.pressed, enabled());
}

void Chip::set_focused(bool focused) {
    focus_.set_focused(focused && enabled());
    retarget();
}

void Chip::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        press_ = {};
        remove_press_ = {};
        focus_.set_focused(false);
    }
    retarget();
}

void Chip::activate(ChipResponse& response) {
    response.clicked = true;
    response.consumed = true;
    if (is_selectable(options_)) {
        set_selected(!options_.selected);
    }
}

ChipResponse Chip::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    ChipResponse response;
    if (!enabled()) {
        return response;
    }
    const std::optional<render::Rect> remove = remove_target();
    const bool over_remove = remove && e.action != input::PointerAction::leave && contains(*remove, e.x_dp, e.y_dp);
    if (remove && remove_press_.update(e, *remove)) {
        response.removed = true;
        response.consumed = true;
    }
    input::PointerEvent body = e;
    if (over_remove || remove_press_.pressed) {
        body.action = input::PointerAction::leave;
    }
    if (press_.update(body, touch_target(bounds()))) {
        activate(response);
    }
    retarget();
    return response;
}

ChipResponse Chip::handle(const input::KeyEvent& e) {
    ChipResponse response;
    if (!enabled() || !focus_.focused()) {
        return response;
    }
    if (options_.removable && !e.ctrl && !e.repeat && (e.key == input::Key::delete_forward || e.key == input::Key::backspace)) {
        response.removed = true;
        response.consumed = true;
        return response;
    }
    if (focus_.activates(e, KeyActivation::space_or_enter)) {
        activate(response);
    }
    return response;
}

void Chip::cancel_interaction() {
    press_ = {};
    remove_press_ = {};
    retarget();
}

void Chip::step(double dt) {
    focus_.step(dt);
    enabled_.step(dt);
    elevation_.step(dt);
    selected_.step(dt);
    state_layer_.step(dt);
    leading_size_.step(dt);
    leading_alpha_.step(dt);
    if (leading_size_.settled() && leading_size_.target() <= 0.0) {
        exiting_leading_.reset();
    }
}

bool Chip::animating() const {
    return !focus_.settled() || !enabled_.settled() || !elevation_.settled() || !selected_.settled() || !state_layer_.settled() ||
           !leading_size_.settled() || !leading_alpha_.settled();
}

void Chip::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const tokens::ChipStyleTokens& s = tokens::chip_style(options_.kind, options_.elevated);
    const render::Rect r = bounds();
    const double radius = tokens::corner_radius(t.corner, r.width, r.height);
    const double k = std::clamp(selected_.value(), 0.0, 1.0);

    const color::Argb label_color =
        enabled_.mix(resolve(scheme, s.disabled_content), color::mix(scheme[s.label], scheme[s.selected_label], k));
    const color::Argb leading_color =
        enabled_.mix(resolve(scheme, s.disabled_content), color::mix(scheme[s.leading_icon], scheme[s.selected_leading_icon], k));
    const color::Argb trailing_color =
        enabled_.mix(resolve(scheme, s.disabled_content), color::mix(scheme[s.trailing_icon], scheme[s.selected_trailing_icon], k));

    const color::Argb container = mix_optional(scheme, s.container, s.selected_container, k);
    const color::Argb enabled_fill = color::state_layer(container, label_color, state_layer_.value());
    const color::Argb disabled_fill = mix_resolved(scheme, s.disabled_container, s.disabled_selected_container, k);
    const color::Argb fill = enabled_.mix(disabled_fill, enabled_fill);
    const double elevation = elevation_.value();
    if (elevation > 0.0 && color::alpha(fill) != 0) {
        paint_surface(canvas, r, radius, fill, elevation, scheme[color::Role::shadow]);
    } else if (color::alpha(fill) != 0) {
        canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(radius)), fill);
    }

    if (s.outline) {
        const double unselected = is_selectable(options_) ? 1.0 - k : 1.0;
        const color::Argb outline = enabled_.mix(resolve(scheme, s.disabled_outline), scheme[*s.outline]);
        if (unselected > 0.0) {
            canvas.fill_path(render::ring_path(r, radius, t.outline_width), color::scale_alpha(outline, unselected),
                             render::FillRule::even_odd);
        }
    }
    focus_.paint(canvas, scheme, r, radius);

    canvas.push_clip(render::rounded_rect_path(r, render::uniform_radii(radius)));
    if (const std::optional<render::Rect> lead = leading_rect()) {
        const double alpha = std::clamp(leading_alpha_.value(), 0.0, 1.0);
        const double w = leading_width();
        if (options_.avatar) {
            const double opacity = motion::lerp(t.disabled_avatar_opacity, 1.0, enabled_.amount()) * alpha;
            const render::Rect full{lead->x, lead->y, w, w};
            canvas.push_clip(render::circle_path({full.x + w / 2.0, full.y + w / 2.0}, w / 2.0));
            canvas.draw_image(*options_.avatar, full, opacity);
            canvas.pop_clip();
        } else {
            const std::optional<icons::Icon>& icon = shown_leading() ? shown_leading() : exiting_leading_;
            if (icon && alpha > 0.0) {
                canvas.push_clip(*lead);
                icon->font.draw(canvas, icon->symbol, {lead->x, lead->y}, w, color::scale_alpha(leading_color, alpha));
                canvas.pop_clip();
            }
        }
    }
    const double label_x = r.x + start_padding() + leading_width() * std::max(0.0, leading_size_.value()) + t.element_spacing;
    text::draw_line(canvas, fonts_, label_, {label_x, r.y + (r.height - label_.box_height()) / 2.0}, label_color);
    if (const std::optional<render::Rect> trail = trailing_rect()) {
        const double opacity = remove_press_.state_layer_opacity();
        if (options_.removable && opacity > 0.0) {
            const render::Point c{trail->x + trail->width / 2.0, trail->y + trail->height / 2.0};
            canvas.fill_path(render::circle_path(c, trail->width / 2.0 + t.input_edge_padding), color::with_alpha(trailing_color, opacity));
        }
        options_.trailing_icon->font.draw(canvas, options_.trailing_icon->symbol, {trail->x, trail->y}, t.icon_size, trailing_color);
    }
    canvas.pop_clip();
}

}
