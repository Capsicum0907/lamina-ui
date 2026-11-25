#include "timer/readout.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::timer {

namespace {

std::size_t utf8_length(unsigned char lead) {
    if (lead >= 0xf0) {
        return 4;
    }
    if (lead >= 0xe0) {
        return 3;
    }
    return lead >= 0xc0 ? 2 : 1;
}

bool is_digit(std::string_view c) {
    return c.size() == 1 && c[0] >= '0' && c[0] <= '9';
}

}

std::vector<std::string_view> split_characters(std::string_view utf8) {
    std::vector<std::string_view> out;
    for (std::size_t i = 0; i < utf8.size();) {
        const std::size_t n = std::min(utf8_length(static_cast<unsigned char>(utf8[i])), utf8.size() - i);
        out.push_back(utf8.substr(i, n));
        i += n;
    }
    return out;
}

DigitFace::DigitFace(const text::FontStack& fonts, tokens::TypeRole role)
    : fonts_(fonts), style_(current_style().type.text(role)) {
    for (char c = '0'; c <= '9'; ++c) {
        const Glyph& g = glyph(std::string_view(&c, 1));
        digit_width_ = std::max(digit_width_, g.line.width);
        height_ = std::max(height_, g.line.box_height());
        baseline_ = std::max(baseline_, g.line.baseline);
    }
}

const DigitFace::Glyph& DigitFace::glyph(std::string_view character) const {
    for (const Glyph& g : glyphs_) {
        if (g.text == character) {
            return g;
        }
    }
    glyphs_.push_back({std::string(character), text::layout_line(fonts_, character, style_)});
    return glyphs_.back();
}

double DigitFace::advance(const Glyph& g) const {
    return is_digit(g.text) ? digit_width_ : g.line.width;
}

double DigitFace::width(std::string_view text) const {
    double w = 0.0;
    for (std::string_view c : split_characters(text)) {
        w += advance(glyph(c));
    }
    return w;
}

void DigitFace::draw(render::Canvas& canvas, std::string_view text, render::Point top_left,
                     const std::function<color::Argb(std::size_t)>& color_of) const {
    double x = top_left.x;
    std::size_t index = 0;
    for (std::string_view c : split_characters(text)) {
        const Glyph& g = glyph(c);
        const double cell = advance(g);
        const color::Argb color = color_of(index++);
        if (color::alpha(color) != 0) {
            text::draw_line(canvas, fonts_, g.line, {x + (cell - g.line.width) / 2.0, top_left.y + baseline_ - g.line.baseline},
                            color);
        }
        x += cell;
    }
}

EntryReadout::EntryReadout(EntryReadoutStyle style, const text::FontStack& fonts)
    : style_(style), fonts_(fonts), digits_(fonts, style.digit_type), units_(fonts, style.unit_type) {}

void EntryReadout::show(const std::string& digits, std::size_t entered) {
    const bool first = emphasis_.empty();
    shown_ = digits;
    while (emphasis_.size() < digits.size()) {
        emphasis_.emplace_back(0.0, style_.fade_spring);
    }
    for (std::size_t i = 0; i < digits.size(); ++i) {
        const double target = i + entered >= digits.size() ? 1.0 : 0.0;
        if (first) {
            emphasis_[i].snap_to(target);
        } else {
            emphasis_[i].animate_to(target);
        }
    }
}

void EntryReadout::step(double dt) {
    for (motion::Animated& a : emphasis_) {
        a.step(dt);
    }
}

bool EntryReadout::animating() const {
    return std::any_of(emphasis_.begin(), emphasis_.end(), [](const motion::Animated& a) { return !a.settled(); });
}

void EntryReadout::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const std::size_t fields = style_.units.size();
    if (shown_.empty() || fields == 0 || shown_.size() % fields != 0) {
        return;
    }
    const std::size_t per_field = shown_.size() / fields;
    double natural = 0.0;
    for (std::size_t f = 0; f < fields; ++f) {
        natural += digits_.width(std::string_view(shown_).substr(f * per_field, per_field)) + style_.unit_gap +
                   units_.width(style_.units[f]);
    }
    natural += style_.field_gap * static_cast<double>(fields - 1);
    const double height = digits_.height();
    if (natural <= 0.0 || height <= 0.0) {
        return;
    }
    const double scale = std::max(0.0, std::min({rect_.width / natural, rect_.height / height, style_.max_scale}));
    const color::Argb entered = scheme[style_.entered];
    const color::Argb pending = scheme[style_.pending];
    const color::Argb unit = scheme[style_.unit];

    canvas.save();
    canvas.translate(rect_.x + (rect_.width - natural * scale) / 2.0, rect_.y + (rect_.height - height * scale) / 2.0);
    canvas.scale(scale);
    double x = 0.0;
    for (std::size_t f = 0; f < fields; ++f) {
        const std::string_view part = std::string_view(shown_).substr(f * per_field, per_field);
        digits_.draw(canvas, part, {x, 0.0}, [&](std::size_t i) {
            return color::mix(pending, entered, emphasis_[f * per_field + i].value());
        });
        x += digits_.width(part) + style_.unit_gap;
        units_.draw(canvas, style_.units[f], {x, digits_.baseline() - units_.baseline()}, [&](std::size_t) { return unit; });
        x += units_.width(style_.units[f]) + style_.field_gap;
    }
    canvas.restore();
}

TimeText::TimeText(TimeTextStyle style, const text::FontStack& fonts)
    : style_(style), face_(fonts, style.type), scale_(1.0, style.scale_spring) {}

double TimeText::fit() const {
    const double w = face_.width(text_);
    const double h = face_.height();
    if (w <= 0.0 || h <= 0.0) {
        return 0.0;
    }
    return std::max(0.0, std::min(rect_.width / w, rect_.height / h));
}

void TimeText::show(const std::string& text) {
    if (text == text_) {
        return;
    }
    const double before = face_.width(text_);
    text_ = text;
    if (face_.width(text_) != before && placed_) {
        scale_.animate_to(fit());
    }
}

void TimeText::arrange(const render::Rect& rect) {
    rect_ = rect;
    if (!placed_ || scale_.settled()) {
        scale_.snap_to(fit());
    } else {
        scale_.animate_to(fit());
    }
    placed_ = !text_.empty();
}

void TimeText::step(double dt) {
    scale_.step(dt);
}

bool TimeText::animating() const {
    return !scale_.settled();
}

void TimeText::paint(render::Canvas& canvas, color::Argb color) const {
    const double s = scale_.value();
    const double w = face_.width(text_) * s;
    const double h = face_.height() * s;
    canvas.save();
    canvas.translate(rect_.x + (rect_.width - w) / 2.0, rect_.y + (rect_.height - h) / 2.0);
    canvas.scale(s);
    face_.draw(canvas, text_, {0.0, 0.0}, [&](std::size_t) { return color; });
    canvas.restore();
}

}
