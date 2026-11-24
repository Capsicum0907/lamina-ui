#include "lamina/style/style.hpp"
#include "lamina/widgets/title_bar.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/title_bar.hpp"

namespace lamina::widgets {

namespace {


constexpr std::array<TitleBarAction, 3> button_order{TitleBarAction::minimize, TitleBarAction::toggle_maximize, TitleBarAction::close};

double resting_corner() {
    return current_style().shape.radius(current_style().title_bar.indicator_corner, current_style().title_bar.indicator_width, current_style().title_bar.indicator_height);
}

double pressed_corner() {
    return current_style().shape.radius(current_style().title_bar.pressed_corner, current_style().title_bar.indicator_width, current_style().title_bar.indicator_height);
}

}

TitleBar::TitleBar(TitleBarOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      title_({options_.title, current_style().title_bar.title, current_style().title_bar.title_color, false}, fonts),
      buttons_{{
          {button_order[0], {}, {0.0, current_style().motion.fast_effects()}, {0.0, current_style().motion.fast_effects()}, {resting_corner(), current_style().motion.fast_spatial()}},
          {button_order[1], {}, {0.0, current_style().motion.fast_effects()}, {0.0, current_style().motion.fast_effects()}, {resting_corner(), current_style().motion.fast_spatial()}},
          {button_order[2], {}, {0.0, current_style().motion.fast_effects()}, {0.0, current_style().motion.fast_effects()}, {resting_corner(), current_style().motion.fast_spatial()}},
      }},
      active_(1.0, current_style().motion.default_effects()) {}

AccessibleNode TitleBar::accessible() const {
    AccessibleNode bar{AccessibleRole::title_bar, title_.text(), bounds()};
    const TitleBarNames& n = options_.names;
    bar.children.push_back({AccessibleRole::button, n.minimize, button_rect(TitleBarAction::minimize)});
    bar.children.push_back({AccessibleRole::button, maximized_ ? n.restore : n.maximize, button_rect(TitleBarAction::toggle_maximize)});
    bar.children.push_back({AccessibleRole::button, n.close, button_rect(TitleBarAction::close)});
    return bar;
}

double TitleBar::height() const {
    return current_style().title_bar.height;
}

void TitleBar::set_title(std::string title) {
    title_.set_text(std::move(title));
}

void TitleBar::set_active(bool active) {
    active_.animate_to(active ? 1.0 : 0.0);
}

render::Rect TitleBar::button_rect(TitleBarAction action) const {
    const auto it = std::find(button_order.begin(), button_order.end(), action);
    if (it == button_order.end()) {
        return {origin_.x, origin_.y, 0.0, 0.0};
    }
    const auto from_end = static_cast<double>(button_order.end() - it);
    const double x = origin_.x + width_ - current_style().title_bar.trailing_space - from_end * current_style().title_bar.button_width;
    return {x, origin_.y, current_style().title_bar.button_width, height()};
}

void TitleBar::set_origin(render::Point origin) {
    origin_ = origin;
    const double h = height();
    const double buttons_left = button_rect(button_order.front()).x;
    const double title_x = origin_.x + current_style().title_bar.leading_space;
    const layout::Size title_size = title_.measure({0.0, std::max(0.0, buttons_left - title_x), 0.0, h});
    title_.set_origin({title_x, origin_.y + (h - title_size.height) / 2.0});
    if (options_.content) {
        const double x = title_x + title_size.width + current_style().title_bar.title_content_space;
        const double w = std::max(0.0, buttons_left - current_style().title_bar.content_buttons_space - x);
        const layout::Size s = options_.content->measure({0.0, w, 0.0, h});
        options_.content->arrange({x, origin_.y + (h - s.height) / 2.0, s.width, s.height});
    }
}

input::CaptionRegions TitleBar::regions(std::span<const render::Rect> interactive) const {
    input::CaptionRegions r;
    r.drag = bounds();
    r.minimize = button_rect(TitleBarAction::minimize);
    r.maximize = button_rect(TitleBarAction::toggle_maximize);
    r.close = button_rect(TitleBarAction::close);
    r.interactive.assign(interactive.begin(), interactive.end());
    return r;
}

void TitleBar::retarget(Button& b) {
    b.state_layer.animate_to(b.press.state_layer_opacity());
    b.emphasis.animate_to(b.press.hovered || b.press.pressed ? 1.0 : 0.0);
    b.corner.animate_to(b.press.pressed ? pressed_corner() : resting_corner());
}

TitleBarAction TitleBar::handle(const input::PointerEvent& e) {
    TitleBarAction clicked = TitleBarAction::none;
    for (Button& b : buttons_) {
        if (b.press.update(e, button_rect(b.action))) {
            clicked = b.action;
        }
        retarget(b);
    }
    return clicked;
}

void TitleBar::cancel_interaction() {
    for (Button& b : buttons_) {
        b.press = {};
        retarget(b);
    }
}

void TitleBar::step(double dt) {
    active_.step(dt);
    for (Button& b : buttons_) {
        b.state_layer.step(dt);
        b.emphasis.step(dt);
        b.corner.step(dt);
    }
}

bool TitleBar::animating() const {
    return !active_.settled() || motion::any_unsettled(buttons_, &Button::state_layer, &Button::emphasis, &Button::corner);
}

void TitleBar::paint_button(render::Canvas& canvas, const color::ColorScheme& scheme, const Button& b) const {
    const render::Rect cell = button_rect(b.action);
    const render::Rect indicator{cell.x + (cell.width - current_style().title_bar.indicator_width) / 2.0,
                                 cell.y + (cell.height - current_style().title_bar.indicator_height) / 2.0, current_style().title_bar.indicator_width,
                                 current_style().title_bar.indicator_height};
    const bool close = b.action == TitleBarAction::close;
    color::Argb content = scheme[current_style().title_bar.icon];
    color::Argb fill = 0;
    if (close) {
        const double k = b.emphasis.value();
        content = color::mix(content, scheme[current_style().title_bar.close_icon], k);
        fill = color::scale_alpha(scheme[current_style().title_bar.close_container], k);
        if (b.press.pressed) {
            fill = color::state_layer(fill, scheme[current_style().title_bar.close_icon], current_style().state_layer.pressed);
        }
    } else {
        fill = color::state_layer(0, scheme[current_style().title_bar.state_layer], b.state_layer.value());
    }
    if (color::alpha(fill) != 0) {
        canvas.fill_path(render::rounded_rect_path(indicator, render::uniform_radii(b.corner.value())), fill);
    }
    paint_glyph(canvas, indicator, b.action, content);
}

void TitleBar::paint_glyph(render::Canvas& canvas, const render::Rect& indicator, TitleBarAction action, color::Argb color) const {
    const auto& t = current_style().title_bar;
    if (action == TitleBarAction::close) {
        icons::draw_centered(canvas, {options_.icons, t.close_symbol}, indicator, t.icon_size, color);
        return;
    }
    const double box = t.glyph_box;
    const double half = t.glyph_stroke / 2.0;
    const render::Rect g{indicator.x + (indicator.width - box) / 2.0, indicator.y + (indicator.height - box) / 2.0, box, box};
    auto outline = [&](const render::Rect& r) {
        canvas.fill_path(render::ring_path(r, t.glyph_corner, t.glyph_stroke), color, render::FillRule::even_odd);
    };
    if (action == TitleBarAction::minimize) {
        canvas.fill_rect({g.x, g.y + box / 2.0 - half, box, t.glyph_stroke}, color);
    } else if (!maximized_) {
        outline(g);
    } else {
        const double offset = t.restore_offset;
        const double inner = box - offset;
        outline({g.x, g.y + offset, inner, inner});
        const render::Rect back{g.x + offset, g.y, inner, inner};
        canvas.push_clip({g.x, g.y, box, offset});
        outline(back);
        canvas.pop_clip();
        canvas.push_clip({g.x + inner, g.y + offset, offset, inner - offset});
        outline(back);
        canvas.pop_clip();
    }
}

void TitleBar::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const color::Argb container =
        options_.container ? scheme[*options_.container]
                           : color::mix(scheme[current_style().title_bar.inactive_container], scheme[current_style().title_bar.container], active_.value());
    canvas.fill_rect(bounds(), container);
    title_.paint(canvas, scheme);
    for (const Button& b : buttons_) {
        paint_button(canvas, scheme, b);
    }
}

}
