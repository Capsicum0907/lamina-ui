#include "lamina/widgets/progress_indicator.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

namespace {

using tokens::motion_scheme;

constexpr double tau = 2.0 * std::numbers::pi;
constexpr double quarter_turn = std::numbers::pi / 2.0;

}

CircularProgressIndicator::CircularProgressIndicator(CircularProgressOptions options)
    : options_(options),
      previous_colors_(options.colors),
      width_(options.diameter),
      height_(options.diameter),
      offset_(0.0, motion_scheme.default_spatial()),
      amplitude_(0.0, motion_scheme.slow_effects()),
      color_mix_(1.0, motion_scheme.default_effects()) {}

void CircularProgressIndicator::set_size(double width, double height) {
    width_ = width;
    height_ = height;
}

double CircularProgressIndicator::diameter() const {
    return std::max(0.0, std::min(width_, height_));
}

double CircularProgressIndicator::radius() const {
    const double reach = std::max(options_.thickness / 2.0 + options_.amplitude, options_.track_thickness / 2.0);
    return diameter() / 2.0 - reach;
}

double CircularProgressIndicator::progress() const {
    return std::clamp(target_ + offset_.value(), 0.0, 1.0);
}

void CircularProgressIndicator::set_progress(double progress) {
    target_ = std::clamp(progress, 0.0, 1.0);
}

void CircularProgressIndicator::ease_progress(double progress) {
    const double shown = this->progress();
    target_ = std::clamp(progress, 0.0, 1.0);
    offset_.snap_to(shown - target_);
    offset_.animate_to(0.0);
}

void CircularProgressIndicator::set_wavy(bool wavy) {
    options_.wavy = wavy;
}

void CircularProgressIndicator::set_colors(ProgressColors colors) {
    if (colors.indicator == options_.colors.indicator && colors.track == options_.colors.track) {
        return;
    }
    previous_colors_ = options_.colors;
    options_.colors = colors;
    color_mix_.snap_to(0.0);
    color_mix_.animate_to(1.0);
}

double CircularProgressIndicator::target_amplitude() const {
    const auto& t = tokens::circular_progress_tokens;
    const double p = progress();
    return options_.wavy && p > t.wave_start_progress && p < t.wave_end_progress ? 1.0 : 0.0;
}

void CircularProgressIndicator::step(double dt) {
    offset_.step(dt);
    amplitude_.animate_to(target_amplitude());
    amplitude_.step(dt);
    color_mix_.step(dt);
    if (amplitude_.value() > 0.0) {
        phase_ = std::fmod(phase_ + tau * options_.waves_per_second * dt, tau);
    }
}

bool CircularProgressIndicator::animating() const {
    const bool waving = (amplitude_.value() > 0.0 || target_amplitude() > 0.0) && options_.waves_per_second > 0.0;
    return waving || !offset_.settled() || !amplitude_.settled() || !color_mix_.settled();
}

void CircularProgressIndicator::stroke_arc(render::Canvas& canvas, double from, double to, double thickness,
                                           double amplitude, color::Argb color) const {
    const double length = to - from;
    const double r = radius();
    if (length <= 0.0 || thickness <= 0.0 || r <= 0.0) {
        return;
    }
    const render::Point center{origin_.x + width_ / 2.0, origin_.y + height_ / 2.0};
    const int segments = std::max(1, static_cast<int>(std::ceil(length / options_.sample_length)));
    std::vector<render::Point> line;
    line.reserve(static_cast<std::size_t>(segments) + 1);
    for (int i = 0; i <= segments; ++i) {
        const double s = from + length * i / segments;
        const double wave = amplitude > 0.0 ? amplitude * std::sin(tau * s / options_.wavelength - phase_) : 0.0;
        const double angle = s / r - quarter_turn;
        line.push_back(center + render::Point{std::cos(angle), std::sin(angle)} * (r + wave));
    }
    const double width = thickness * std::min(1.0, length / thickness);
    canvas.fill_path(render::stroke_path(line, width), color);
}

void CircularProgressIndicator::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double r = radius();
    if (r <= 0.0) {
        return;
    }
    const double k = color_mix_.value();
    const color::Argb indicator = color::mix(scheme[previous_colors_.indicator], scheme[options_.colors.indicator], k);
    const color::Argb track = color::mix(scheme[previous_colors_.track], scheme[options_.colors.track], k);
    const double circumference = tau * r;
    const double stop = progress() * circumference;
    const double cap = std::max(options_.thickness, options_.track_thickness) / 2.0;
    const double spacing = 2.0 * std::min(stop, cap) + std::min(stop, options_.gap);
    stroke_arc(canvas, stop + spacing, circumference - spacing, options_.track_thickness, 0.0, track);
    stroke_arc(canvas, 0.0, stop, options_.thickness, options_.amplitude * amplitude_.value(), indicator);
}

}
