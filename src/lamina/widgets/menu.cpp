#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/menu.hpp"

#include <algorithm>
#include <array>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/item_focus.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::MenuTokens& t = tokens::menu_tokens;
using tokens::motion_scheme;

template <std::size_t N>
double pick(const std::array<double, N>& candidates, double size, double space, double margin) {
    for (std::size_t i = 0; i < N; ++i) {
        const double c = candidates[i];
        if (c >= margin && c + size <= space - margin) {
            return c;
        }
    }
    if (size >= space - 2.0 * margin) {
        return (space - size) / 2.0;
    }
    return std::clamp(candidates[N - 1], margin, space - margin - size);
}

double pivot_fraction(double anchor_start, double anchor_end, double menu_start, double menu_end) {
    if (menu_start >= anchor_end) {
        return 0.0;
    }
    if (menu_end <= anchor_start) {
        return 1.0;
    }
    if (menu_end - menu_start <= 0.0) {
        return 0.0;
    }
    const double center = (std::max(anchor_start, menu_start) + std::min(anchor_end, menu_end)) / 2.0;
    return (center - menu_start) / (menu_end - menu_start);
}

}

render::Point menu_position(const render::Rect& anchor, layout::Size window, layout::Size menu) {
    return menu_position(anchor, window, menu, t.window_horizontal_margin, t.window_vertical_margin);
}

render::Point menu_position(const render::Rect& anchor, layout::Size window, layout::Size menu, double horizontal_margin,
                            double vertical_margin) {
    const double anchor_right = anchor.x + anchor.width;
    const double anchor_bottom = anchor.y + anchor.height;
    const std::array<double, 3> xs{
        anchor.x,
        anchor_right - menu.width,
        anchor.x + anchor.width / 2.0 < window.width / 2.0 ? 0.0 : window.width - menu.width,
    };
    const std::array<double, 4> ys{
        anchor_bottom,
        anchor.y - menu.height,
        anchor.y - menu.height / 2.0,
        anchor.y + anchor.height / 2.0 < window.height / 2.0 ? 0.0 : window.height - menu.height,
    };
    return {pick(xs, menu.width, window.width, horizontal_margin), pick(ys, menu.height, window.height, vertical_margin)};
}

render::Point menu_transform_origin(const render::Rect& anchor, const render::Rect& menu) {
    const double fx = pivot_fraction(anchor.x, anchor.x + anchor.width, menu.x, menu.x + menu.width);
    const double fy = pivot_fraction(anchor.y, anchor.y + anchor.height, menu.y, menu.y + menu.height);
    return {menu.x + fx * menu.width, menu.y + fy * menu.height};
}

Menu::Menu(MenuOptions options, const text::FontStack& fonts)
    : fonts_(fonts), popup_(t.closed_scale, motion_scheme.fast_spatial(), motion_scheme.fast_effects()) {
    const auto style = tokens::text_style(tokens::type_style(t.label));
    double content = 0.0;
    for (auto& item : options.items) {
        std::optional<text::TextLine> label;
        std::optional<AccessKeyLabel> access;
        const std::optional<input::Key> key = access_key_for(item.label, item.access_key);
        double w = 2.0 * t.item_horizontal_padding;
        if (!item.label.empty()) {
            label = text::layout_line(fonts_, item.label, style);
            w += label->width;
            if (key) {
                access = access_key_label(fonts_, item.label, *label, *key);
                w += access->suffix_width();
            }
        }
        if (item.leading_icon) {
            w += t.icon_size + t.icon_text_space;
        }
        content = std::max(content, w);
        access_keys_.push_back(key);
        items_.push_back({std::move(label), std::move(item.leading_icon), std::move(access), {},
                          motion::Animated(0.0, motion_scheme.fast_effects()), Enablement(item.enabled)});
    }
    width_ = std::clamp(content, t.item_min_width, t.item_max_width);
}

double Menu::height() const {
    return 2.0 * t.vertical_padding + t.item_height * static_cast<double>(items_.size());
}

render::Rect Menu::bounds() const {
    return {origin_.x, origin_.y, width_, height()};
}

render::Rect Menu::item_rect(std::size_t index) const {
    return {origin_.x, origin_.y + t.vertical_padding + t.item_height * static_cast<double>(index), width_, t.item_height};
}

void Menu::open(const render::Rect& anchor, layout::Size window, input::InputSource source) {
    origin_ = menu_position(anchor, window, {width_, height()});
    pivot_ = menu_transform_origin(anchor, bounds());
    popup_.open();
    access_keys_shown_ = source == input::InputSource::keyboard;
    access_keys_fade_.snap_to(access_keys_shown_ ? 1.0 : 0.0);
    set_active(source == input::InputSource::keyboard
                   ? first_enabled_item(items_.size(), [this](std::size_t i) { return item_enabled(i); })
                   : std::nullopt);
}

void Menu::set_item_enabled(std::size_t index, bool enabled) {
    if (index >= items_.size() || !items_[index].enabled.set(enabled)) {
        return;
    }
    if (!enabled) {
        disable_item(items_, index);
        if (active_ == index) {
            set_active(std::nullopt);
        }
    }
    retarget_state_layers(items_, active_);
}

std::vector<std::optional<input::Key>> Menu::enabled_access_keys() const {
    std::vector<std::optional<input::Key>> keys = access_keys_;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (!item_enabled(i)) {
            keys[i].reset();
        }
    }
    return keys;
}

void Menu::close() {
    popup_.close();
    set_active(std::nullopt);
    cancel_interaction();
}

void Menu::set_active(std::optional<std::size_t> index) {
    active_ = index;
    focus_.set_focused(index.has_value());
    retarget_state_layers(items_, active_);
}

void Menu::cancel_interaction() {
    cancel_presses(items_);
    retarget_state_layers(items_, active_);
}

MenuResponse Menu::handle(const input::PointerEvent& e) {
    MenuResponse response;
    if (!popup_.is_open()) {
        return response;
    }
    if (popup_.dismisses(e, bounds())) {
        close();
        response.consumed = true;
        return response;
    }
    if (e.action == input::PointerAction::down) {
        set_active(std::nullopt);
    }
    response.consumed = e.action != input::PointerAction::leave;
    response.chosen = update_presses(items_, e, [this](std::size_t i) { return item_enabled(i) ? item_rect(i) : render::Rect{}; });
    retarget_state_layers(items_, active_);
    if (response.chosen) {
        close();
    }
    return response;
}

MenuResponse Menu::handle(const input::KeyEvent& e) {
    MenuResponse response;
    if (!popup_.is_open()) {
        return response;
    }
    if (closes_menu(e)) {
        close();
        response.consumed = true;
        return response;
    }
    if (activates(e, KeyActivation::space_or_enter)) {
        response.consumed = true;
        if (active_) {
            response.chosen = active_;
            close();
        }
        return response;
    }
    if (const std::optional<std::size_t> target =
            menu_key_target(e, active_, items_.size(), [this](std::size_t i) { return item_enabled(i); })) {
        reveal_access_keys();
        set_active(target);
        response.consumed = true;
        return response;
    }
    if (const std::optional<AccessKeyMatch> match = match_access_key(e, enabled_access_keys(), active_)) {
        response.consumed = true;
        reveal_access_keys();
        if (match->unique && e.repeat) {
            return response;
        }
        set_active(match->index);
        if (match->unique) {
            response.chosen = match->index;
            close();
        }
    }
    return response;
}

void Menu::reveal_access_keys() {
    access_keys_shown_ = true;
    access_keys_fade_.animate_to(1.0);
}

void Menu::step(double dt) {
    popup_.step(dt);
    focus_.step(dt);
    access_keys_fade_.step(dt);
    for (Item& item : items_) {
        item.state_layer.step(dt);
        item.enabled.step(dt);
    }
}

bool Menu::animating() const {
    if (!popup_.settled() || !focus_.settled() || !access_keys_fade_.settled()) {
        return true;
    }
    return motion::any_unsettled(items_, &Item::state_layer, &Item::enabled);
}

void Menu::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    const double a = opacity();
    const render::Rect r = bounds();
    const double radius = tokens::corner_radius(t.corner, r.width, r.height);
    canvas.save();
    popup_.scale_about(canvas, pivot_);
    const color::ColorScheme s = color::faded(scheme, a);
    paint_surface(canvas, r, radius, s[t.container], tokens::elevation_dp[t.elevation], s[color::Role::shadow]);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const color::Argb label_color = item.enabled.mix(resolve(s, t.disabled_label), s[t.label_color]);
        const color::Argb icon_color = item.enabled.mix(resolve(s, t.disabled_leading_icon), s[t.leading_icon]);
        const render::Rect ir = item_rect(i);
        if (item.state_layer.value() > 0.0) {
            canvas.fill_rect(ir, color::scale_alpha(color::with_alpha(scheme[t.state_layer], item.state_layer.value()), a));
        }
        double x = ir.x + t.item_horizontal_padding;
        const double cy = ir.y + ir.height / 2.0;
        if (item.icon) {
            item.icon->font.draw(canvas, item.icon->symbol, {x, cy - t.icon_size / 2.0}, t.icon_size, icon_color);
            x += t.icon_size + t.icon_text_space;
        }
        if (item.label) {
            const render::Point top_left{x, cy - item.label->box_height() / 2.0};
            text::draw_line(canvas, fonts_, *item.label, top_left, label_color);
            if (item.access && access_keys_fade_.value() > 0.0) {
                paint_access_key(canvas, fonts_, *item.access, *item.label, top_left,
                                 color::scale_alpha(label_color, std::clamp(access_keys_fade_.value(), 0.0, 1.0)));
            }
        }
    }
    if (active_) {
        focus_.paint_inward(canvas, s, item_rect(*active_), t.focus_ring_corner);
    }
    canvas.restore();
}

}
