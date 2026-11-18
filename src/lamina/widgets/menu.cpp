#include "lamina/widgets/menu.hpp"

#include <algorithm>
#include <array>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::MenuTokens& t = tokens::menu_tokens;

constexpr tokens::SpringSpec scale_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::fast);
constexpr tokens::SpringSpec alpha_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);
constexpr tokens::SpringSpec state_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);

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

color::Argb faded(color::Argb c, double alpha) {
    return color::with_alpha(c, alpha * static_cast<double>(c >> 24) / 255.0);
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
    : fonts_(fonts), scale_(t.closed_scale, scale_spring), alpha_(0.0, alpha_spring) {
    const auto style = text::text_style(tokens::type_style(t.label));
    double content = 0.0;
    for (auto& item : options.items) {
        std::optional<text::TextLine> label;
        double w = 2.0 * t.item_horizontal_padding;
        if (!item.label.empty()) {
            label = text::layout_line(fonts_, item.label, style);
            w += label->width;
        }
        if (item.leading_icon) {
            w += t.icon_size + t.icon_text_space;
        }
        content = std::max(content, w);
        items_.push_back({std::move(label), std::move(item.leading_icon), {}, motion::Animated(0.0, state_spring)});
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

void Menu::open(const render::Rect& anchor, layout::Size window) {
    origin_ = menu_position(anchor, window, {width_, height()});
    pivot_ = menu_transform_origin(anchor, bounds());
    if (!visible()) {
        scale_.snap_to(t.closed_scale);
        alpha_.snap_to(0.0);
    }
    open_ = true;
    scale_.animate_to(1.0);
    alpha_.animate_to(1.0);
}

void Menu::close() {
    open_ = false;
    scale_.animate_to(t.closed_scale);
    alpha_.animate_to(0.0);
    for (Item& item : items_) {
        item.press = {};
        item.state_layer.animate_to(0.0);
    }
}

bool Menu::visible() const {
    return open_ || !alpha_.settled() || alpha_.value() > 0.0;
}

double Menu::opacity() const {
    return std::clamp(alpha_.value(), 0.0, 1.0);
}

MenuResponse Menu::handle(const platform::PointerEvent& e) {
    MenuResponse response;
    if (!open_) {
        return response;
    }
    const bool inside = contains(bounds(), e.x_dp, e.y_dp);
    if (e.action == platform::PointerAction::down && !inside) {
        close();
        response.consumed = true;
        return response;
    }
    response.consumed = e.action != platform::PointerAction::leave;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        Item& item = items_[i];
        if (item.press.update(e, item_rect(i))) {
            response.chosen = i;
        }
        item.state_layer.animate_to(item.press.state_layer_opacity());
    }
    if (response.chosen) {
        close();
    }
    return response;
}

bool Menu::handle(const platform::KeyEvent& e) {
    if (open_ && e.key == platform::Key::escape) {
        close();
        return true;
    }
    return false;
}

void Menu::step(double dt) {
    scale_.step(dt);
    alpha_.step(dt);
    for (Item& item : items_) {
        item.state_layer.step(dt);
    }
}

bool Menu::animating() const {
    if (!scale_.settled() || !alpha_.settled()) {
        return true;
    }
    return std::any_of(items_.begin(), items_.end(), [](const Item& item) { return !item.state_layer.settled(); });
}

void Menu::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    const double a = opacity();
    const render::Rect r = bounds();
    const double radius = tokens::corner_radius(t.corner, r.width, r.height);
    canvas.save();
    canvas.translate(pivot_.x, pivot_.y);
    canvas.scale(scale_.value());
    canvas.translate(-pivot_.x, -pivot_.y);
    paint_surface(canvas, r, radius, faded(scheme[t.container], a), tokens::elevation_dp[t.elevation],
                  faded(scheme[color::Role::shadow], a));
    const color::Argb label_color = faded(scheme[t.label_color], a);
    const color::Argb icon_color = faded(scheme[t.leading_icon], a);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const render::Rect ir = item_rect(i);
        if (item.state_layer.value() > 0.0) {
            canvas.fill_path(render::rounded_rect_path(ir, render::uniform_radii(0.0)),
                             faded(color::with_alpha(scheme[t.state_layer], item.state_layer.value()), a));
        }
        double x = ir.x + t.item_horizontal_padding;
        const double cy = ir.y + ir.height / 2.0;
        if (item.icon) {
            item.icon->font.draw(canvas, item.icon->symbol, {x, cy - t.icon_size / 2.0}, t.icon_size, icon_color);
            x += t.icon_size + t.icon_text_space;
        }
        if (item.label) {
            const double box = std::max(item.label->style.line_height, item.label->ascent + item.label->descent);
            text::draw_line(canvas, fonts_, *item.label, {x, cy - box / 2.0}, label_color);
        }
    }
    canvas.restore();
}

}
