#include "calculator/display.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::calculator {

Display::Display(DisplayStyle style, const text::FontStack& fonts)
    : style_(style),
      fonts_(fonts),
      text_style_(current_style().type.text(style.focused_type)),
      secondary_ratio_(current_style().type.style(style.secondary_type).size_sp / current_style().type.style(style.focused_type).size_sp),
      lines_{Line{{}, text::layout_line(fonts, "", text_style_), true, motion::Animated(1.0, style.size_spring),
                  motion::Animated(1.0, style.color_spring)},
             Line{{}, text::layout_line(fonts, "", text_style_), false, motion::Animated(secondary_ratio_, style.size_spring),
                  motion::Animated(0.0, style.color_spring)}},
      badge_(text::layout_line(fonts, "", current_style().type.text(style.badge_type))),
      badge_opacity_(0.0, style.color_spring) {}

void Display::set_badge(const std::string& text, bool visible) {
    if (text != badge_text_) {
        badge_text_ = text;
        badge_ = text::layout_line(fonts_, badge_text_, current_style().type.text(style_.badge_type));
    }
    badge_opacity_.animate_to(visible ? 1.0 : 0.0);
}

void Display::show(const std::string& formula, const std::string& result, bool result_focused, bool error) {
    const std::array<const std::string*, 2> texts{&formula, &result};
    const std::array<bool, 2> focused{!result_focused, result_focused};
    error_ = error;
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        Line& line = lines_[i];
        if (line.text != *texts[i]) {
            line.text = *texts[i];
            line.layout = text::layout_line(fonts_, line.text, text_style_);
        }
        line.focused = focused[i];
        retarget(line, !shown_);
    }
    shown_ = true;
}

void Display::arrange(const render::Rect& rect) {
    rect_ = rect;
    for (Line& line : lines_) {
        retarget(line, line.scale.settled());
    }
}

double Display::target_scale(const Line& line) const {
    const double role = line.focused ? 1.0 : secondary_ratio_;
    const double natural = line.layout.width * role;
    const double available = std::max(0.0, rect_.width - 2.0 * style_.inset);
    const double fit = natural > available && natural > 0.0 ? available / natural : 1.0;
    return role * std::max(style_.min_fit, fit);
}

void Display::retarget(Line& line, bool snap) {
    const double scale = target_scale(line);
    const double emphasis = line.focused ? 1.0 : 0.0;
    if (snap) {
        line.scale.snap_to(scale);
        line.emphasis.snap_to(emphasis);
    } else {
        line.scale.animate_to(scale);
        line.emphasis.animate_to(emphasis);
    }
}

void Display::step(double dt) {
    for (Line& line : lines_) {
        line.scale.step(dt);
        line.emphasis.step(dt);
    }
    badge_opacity_.step(dt);
}

bool Display::animating() const {
    return !badge_opacity_.settled() ||
           std::any_of(lines_.begin(), lines_.end(), [](const Line& l) { return !l.scale.settled() || !l.emphasis.settled(); });
}

void Display::paint_line(render::Canvas& canvas, const Line& line, double bottom, color::Argb color) const {
    const double s = line.scale.value();
    const double x = rect_.x + rect_.width - style_.inset - line.layout.width * s;
    const double y = bottom - line.layout.box_height() * s;
    canvas.save();
    canvas.translate(x, y);
    canvas.scale(s);
    text::draw_line(canvas, fonts_, line.layout, {0.0, 0.0}, color);
    canvas.restore();
}

void Display::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const Line& formula = lines_[0];
    const Line& result = lines_[1];
    const color::Argb secondary = scheme[style_.secondary_color];
    const color::Argb focused = scheme[style_.focused_color];
    const color::Argb result_focused = scheme[error_ ? style_.error_color : style_.focused_color];

    canvas.push_clip({rect_.x + style_.inset, rect_.y, std::max(0.0, rect_.width - 2.0 * style_.inset), rect_.height});
    const double result_bottom = rect_.y + rect_.height;
    paint_line(canvas, result, result_bottom, color::mix(secondary, result_focused, result.emphasis.value()));
    const double formula_bottom = result_bottom - result.layout.box_height() * result.scale.value() - style_.line_gap;
    paint_line(canvas, formula, formula_bottom, color::mix(secondary, focused, formula.emphasis.value()));
    if (badge_opacity_.value() > 0.0) {
        text::draw_line(canvas, fonts_, badge_, {rect_.x + style_.inset, rect_.y + style_.inset},
                        color::scale_alpha(scheme[style_.badge_color], badge_opacity_.value()));
    }
    canvas.pop_clip();
}

}
