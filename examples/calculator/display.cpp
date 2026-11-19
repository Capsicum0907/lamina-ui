#include "calculator/display.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::examples::calculator {

namespace {

using tokens::motion_scheme;

}

Display::Display(DisplayStyle style, const text::FontStack& fonts)
    : style_(style),
      fonts_(fonts),
      text_style_(tokens::text_style(tokens::type_style(style.focused_type))),
      secondary_ratio_(tokens::type_style(style.secondary_type).size_sp / tokens::type_style(style.focused_type).size_sp),
      lines_{Line{{}, text::layout_line(fonts, "", text_style_), true, motion::Animated(1.0, motion_scheme.default_effects()),
                  motion::Animated(1.0, motion_scheme.default_effects())},
             Line{{}, text::layout_line(fonts, "", text_style_), false,
                  motion::Animated(secondary_ratio_, motion_scheme.default_effects()),
                  motion::Animated(0.0, motion_scheme.default_effects())}} {}

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
}

bool Display::animating() const {
    return std::any_of(lines_.begin(), lines_.end(), [](const Line& l) { return !l.scale.settled() || !l.emphasis.settled(); });
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
    canvas.pop_clip();
}

}
