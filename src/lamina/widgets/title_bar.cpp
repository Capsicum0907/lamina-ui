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

using tokens::motion_scheme;
using tokens::title_bar_tokens;

constexpr std::array<TitleBarAction, 3> button_order{TitleBarAction::minimize, TitleBarAction::toggle_maximize, TitleBarAction::close};

double resting_corner() {
    return tokens::corner_radius(title_bar_tokens.indicator_corner, title_bar_tokens.indicator_width, title_bar_tokens.indicator_height);
}

double pressed_corner() {
    return tokens::corner_radius(title_bar_tokens.pressed_corner, title_bar_tokens.indicator_width, title_bar_tokens.indicator_height);
}

}

TitleBar::TitleBar(TitleBarOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      title_({options_.title, title_bar_tokens.title, title_bar_tokens.title_color, false}, fonts),
      buttons_{{
          {button_order[0], {}, {0.0, motion_scheme.fast_effects()}, {0.0, motion_scheme.fast_effects()}, {resting_corner(), motion_scheme.fast_spatial()}},
          {button_order[1], {}, {0.0, motion_scheme.fast_effects()}, {0.0, motion_scheme.fast_effects()}, {resting_corner(), motion_scheme.fast_spatial()}},
          {button_order[2], {}, {0.0, motion_scheme.fast_effects()}, {0.0, motion_scheme.fast_effects()}, {resting_corner(), motion_scheme.fast_spatial()}},
      }},
      active_(1.0, motion_scheme.default_effects()) {}

double TitleBar::height() const {
    return title_bar_tokens.height;
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
    const double x = origin_.x + width_ - title_bar_tokens.trailing_space - from_end * title_bar_tokens.button_width;
    return {x, origin_.y, title_bar_tokens.button_width, height()};
}

void TitleBar::set_origin(render::Point origin) {
    origin_ = origin;
    const double h = height();
    const double buttons_left = button_rect(button_order.front()).x;
    const double title_x = origin_.x + title_bar_tokens.leading_space;
    const layout::Size title_size = title_.measure({0.0, std::max(0.0, buttons_left - title_x), 0.0, h});
    title_.set_origin({title_x, origin_.y + (h - title_size.height) / 2.0});
    if (options_.content) {
        const double x = title_x + title_size.width + title_bar_tokens.title_content_space;
        const double w = std::max(0.0, buttons_left - title_bar_tokens.content_buttons_space - x);
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
    const render::Rect indicator{cell.x + (cell.width - title_bar_tokens.indicator_width) / 2.0,
                                 cell.y + (cell.height - title_bar_tokens.indicator_height) / 2.0, title_bar_tokens.indicator_width,
                                 title_bar_tokens.indicator_height};
    const bool close = b.action == TitleBarAction::close;
    color::Argb content = scheme[title_bar_tokens.icon];
    color::Argb fill = 0;
    if (close) {
        const double k = b.emphasis.value();
        content = color::mix(content, scheme[title_bar_tokens.close_icon], k);
        fill = color::scale_alpha(scheme[title_bar_tokens.close_container], k);
        if (b.press.pressed) {
            fill = color::state_layer(fill, scheme[title_bar_tokens.close_icon], tokens::state_layer_opacity::pressed);
        }
    } else {
        fill = color::state_layer(0, scheme[title_bar_tokens.state_layer], b.state_layer.value());
    }
    if (color::alpha(fill) != 0) {
        canvas.fill_path(render::rounded_rect_path(indicator, render::uniform_radii(b.corner.value())), fill);
    }
    paint_glyph(canvas, indicator, b.action, content);
}

void TitleBar::paint_glyph(render::Canvas& canvas, const render::Rect& indicator, TitleBarAction action, color::Argb color) const {
    const auto& t = title_bar_tokens;
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
                           : color::mix(scheme[title_bar_tokens.inactive_container], scheme[title_bar_tokens.container], active_.value());
    canvas.fill_rect(bounds(), container);
    title_.paint(canvas, scheme);
    for (const Button& b : buttons_) {
        paint_button(canvas, scheme, b);
    }
}

}
