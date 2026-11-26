#include "lamina/style/style.hpp"
#include "lamina/widgets/wavy_progress.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <mutex>
#include <numbers>
#include <tuple>

#include "lamina/motion/easing.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/render/path.hpp"
#include "lamina/shapes/morph.hpp"
#include "lamina/shapes/shapes.hpp"

namespace lamina::widgets {

namespace {

const tokens::WavyProgressTokens& t() {
    return current_style().wavy_progress;
}
const tokens::WavyProgressMotion& m() {
    return current_style().wavy_progress_motion;
}
constexpr double tau = 2.0 * std::numbers::pi;

double radians(double degrees) {
    return degrees * std::numbers::pi / 180.0;
}

render::Point rotate_about(render::Point p, render::Point c, double a) {
    const double cs = std::cos(a);
    const double sn = std::sin(a);
    const render::Point d = p - c;
    return {c.x + cs * d.x - sn * d.y, c.y + sn * d.x + cs * d.y};
}

void rotate_all(std::vector<render::Point>& points, render::Point c, double a) {
    if (a == 0.0) {
        return;
    }
    for (render::Point& p : points) {
        p = rotate_about(p, c, a);
    }
}

struct CircularShapes {
    shapes::RoundedPolygon circle;
    shapes::RoundedPolygon star;
    shapes::Morph morph;
};

const CircularShapes& circular_shapes(int n) {
    using Key = std::tuple<int, float, float, float, float, float>;
    static std::mutex lock;
    static std::map<Key, CircularShapes> cache;
    const Key key{n, t().star_inner_radius, t().star_rounding.radius, t().star_rounding.smoothing, t().star_inner_rounding.radius,
                  t().star_inner_rounding.smoothing};
    const std::scoped_lock hold(lock);
    auto it = cache.find(key);
    if (it == cache.end()) {
        shapes::RoundedPolygon circle = shapes::circle(n).normalized();
        shapes::RoundedPolygon star = shapes::star(n, 1.0f, t().star_inner_radius, t().star_rounding, t().star_inner_rounding).normalized();
        shapes::Morph morph(circle, star);
        it = cache.emplace(key, CircularShapes{std::move(circle), std::move(star), std::move(morph)}).first;
    }
    return it->second;
}

void stroke(render::Canvas& canvas, const std::vector<render::Point>& line, double width, color::Argb color) {
    if (line.size() >= 2) {
        canvas.fill_path(render::stroke_path(line, width), color);
    }
}

}

double wavy_indicator_amplitude(double progress) {
    return progress <= t().wave_start_progress || progress >= t().wave_end_progress ? 0.0 : 1.0;
}

double linear_indeterminate_fraction(const tokens::KeyframeSegment& segment, double play_ms) {
    const double ms = std::fmod(play_ms, current_style().linear_indeterminate.cycle_ms);
    if (ms <= segment.delay_ms) {
        return 0.0;
    }
    if (ms >= segment.delay_ms + segment.duration_ms) {
        return 1.0;
    }
    return motion::ease(current_style().linear_indeterminate.easing, (ms - segment.delay_ms) / segment.duration_ms);
}

double circular_indeterminate_sweep(double play_ms) {
    const auto& c = current_style().circular_indeterminate;
    const double ms = std::fmod(play_ms, c.cycle_ms);
    const double half = c.cycle_ms / 2.0;
    if (ms <= half) {
        return motion::lerp(c.min_progress, c.max_progress, ms / half);
    }
    return motion::lerp(c.max_progress, c.min_progress, motion::ease(c.progress_easing, (ms - half) / half));
}

double circular_indeterminate_rotation(double play_ms) {
    const auto& c = current_style().circular_indeterminate;
    const double ms = std::fmod(play_ms, c.cycle_ms);
    const double step = std::floor(ms / c.turn_every_ms);
    const double into = ms - step * c.turn_every_ms;
    const double turn = std::clamp(into / c.turn_ms, 0.0, 1.0);
    const double extra = c.turn_degrees * (step + turn);
    return c.global_degrees * ms / c.cycle_ms + extra + c.base_degrees;
}

WavyProgressCore::WavyProgressCore(WavyProgressOptions options)
    : options_(std::move(options)),
      target_(options_.progress ? std::optional(std::clamp(*options_.progress, 0.0, 1.0)) : std::nullopt),
      shown_(target_.value_or(0.0)),
      amplitude_(0.0),
      handoff_(1.0) {
    if (!options_.amplitude) {
        options_.amplitude = wavy_indicator_amplitude;
    }
    amplitude_.snap_to(target_amplitude());
}

double WavyProgressCore::progress() const {
    return std::clamp(shown_.value(), 0.0, 1.0);
}

double WavyProgressCore::target_amplitude() const {
    const double value = determinate() ? options_.amplitude(progress()) : options_.indeterminate_amplitude;
    return std::isnan(value) ? 1.0 : std::clamp(value, 0.0, 1.0);
}

void WavyProgressCore::set_progress(std::optional<double> progress) {
    const bool was = determinate();
    target_ = progress ? std::optional(std::clamp(*progress, 0.0, 1.0)) : std::nullopt;
    if (target_) {
        shown_.snap_to(*target_);
    }
    if (!was && target_) {
        amplitude_.snap_to(target_amplitude());
        on_handoff();
    } else if (was && !target_) {
        play_ms_ = 0.0;
        handoff_.snap_to(1.0);
    }
}

void WavyProgressCore::ease_progress(double progress) {
    if (!determinate()) {
        set_progress(progress);
        return;
    }
    target_ = std::clamp(progress, 0.0, 1.0);
    shown_.animate_to(*target_, m().progress);
}

AccessibleNode WavyProgressCore::accessible(const render::Rect& bounds) const {
    AccessibleNode node{AccessibleRole::progress_indicator, options_.accessible_name, bounds};
    if (determinate()) {
        node.state.range = AccessibleRange{progress(), 0.0, 1.0};
    }
    return node;
}

void WavyProgressCore::step(double dt, double offset_period_seconds) {
    shown_.step(dt);
    handoff_.step(dt);
    const double target = target_amplitude();
    if (target != amplitude_.target()) {
        amplitude_.animate_to(target, amplitude_.value() < target ? m().amplitude_up : m().amplitude_down);
    }
    amplitude_.step(dt);
    if (offset_period_seconds > 0.0 && (amplitude_.value() > 0.0 || target > 0.0)) {
        offset_ = std::fmod(offset_ + dt / offset_period_seconds, 1.0);
    }
    if (!determinate()) {
        play_ms_ += dt * 1000.0;
    }
}

bool WavyProgressCore::animating() const {
    const bool waving = (amplitude_.value() > 0.0 || target_amplitude() > 0.0) && options_.wave_speed.value_or(1.0) > 0.0;
    return !determinate() || waving || !shown_.settled() || !amplitude_.settled() || !handoff_.settled();
}

LinearWavyProgressIndicator::LinearWavyProgressIndicator(WavyProgressOptions options) : WavyProgressCore(std::move(options)) {}

double LinearWavyProgressIndicator::wavelength() const {
    if (options_.wavelength) {
        return *options_.wavelength;
    }
    return determinate() ? t().linear_wavelength : t().linear_indeterminate_wavelength;
}

void LinearWavyProgressIndicator::on_handoff() {
    handoff_.snap_to(0.0);
    handoff_.animate_to(1.0, m().progress);
}

double LinearWavyProgressIndicator::cap() const {
    return height() > width_ ? 0.0 : std::max(t().linear_thickness, t().linear_track_thickness) / 2.0;
}

const render::Polyline& LinearWavyProgressIndicator::wave() const {
    const double lambda = wavelength();
    const bool flat = amplitude() == 0.0;
    if (wave_width_ == width_ && wave_length_ == lambda && wave_flat_ == flat && !wave_.points().empty()) {
        return wave_;
    }
    const double h = height();
    std::vector<render::Point> pts;
    double right = width_;
    if (flat) {
        pts = {{0.0, h / 2.0}, {width_, h / 2.0}};
    } else {
        const double half = lambda / 2.0;
        double anchor = half;
        double control_x = half / 2.0;
        double control_y = h - t().linear_thickness;
        render::Point from{0.0, 0.0};
        while (anchor <= width_ + lambda * 2.0) {
            const render::Point to{anchor, 0.0};
            append_quadratic(pts, from, {control_x, control_y}, to, t().flatten_segments);
            from = to;
            right = anchor;
            anchor += half;
            control_x += half;
            control_y = -control_y;
        }
        for (render::Point& p : pts) {
            p.y += h / 2.0;
        }
    }
    wave_ = render::Polyline(std::move(pts));
    wave_scale_ = wave_.length() / (right + 1e-8);
    wave_width_ = width_;
    wave_length_ = lambda;
    wave_flat_ = flat;
    return wave_;
}

std::array<double, 4> LinearWavyProgressIndicator::fractions() const {
    if (determinate()) {
        return {0.0, progress() * handoff(), 0.0, 0.0};
    }
    const auto& l = current_style().linear_indeterminate;
    return {linear_indeterminate_fraction(l.first_tail, play_ms_), linear_indeterminate_fraction(l.first_head, play_ms_),
            linear_indeterminate_fraction(l.second_tail, play_ms_), linear_indeterminate_fraction(l.second_head, play_ms_)};
}

std::vector<std::vector<render::Point>> LinearWavyProgressIndicator::active_lines() const {
    std::vector<std::vector<render::Point>> out;
    const render::Polyline& full = wave();
    const double w = width_;
    const double h = height();
    const double c = cap();
    const double amp = amplitude();
    const std::array<double, 4> f = fractions();
    const std::size_t pairs = determinate() ? 1 : 2;
    for (std::size_t i = 0; i < pairs; ++i) {
        const double tail = f[i * 2] * w;
        const double head = f[i * 2 + 1] * w;
        if (std::abs(f[i * 2 + 1] - f[i * 2]) <= 0.0) {
            continue;
        }
        const double a_head = std::clamp(head, c, w - c);
        const double a_tail = std::clamp(tail, c, w - c);
        const double shift = amp != 0.0 ? wave_offset() * wavelength() : 0.0;
        std::vector<render::Point> seg = full.segment((a_tail + shift) * wave_scale_, (a_head + shift) * wave_scale_);
        for (render::Point& p : seg) {
            p.x = p.x - (shift > 0.0 ? shift : 0.0) + origin_.x;
            p.y = (amp != 1.0 ? p.y * amp : p.y) + (1.0 - amp) * h / 2.0 + origin_.y;
        }
        if (!seg.empty()) {
            out.push_back(std::move(seg));
        }
    }
    return out;
}

std::vector<std::vector<render::Point>> LinearWavyProgressIndicator::track_lines() const {
    std::vector<std::vector<render::Point>> out;
    const double w = width_;
    const double y = origin_.y + height() / 2.0;
    const double c = cap();
    const std::array<double, 4> f = fractions();
    const std::size_t pairs = determinate() ? 1 : 2;
    double gap = t().linear_gap;
    bool visible = false;
    double next_end = w - c;
    for (std::size_t i = 0; i < pairs; ++i) {
        const double tail = f[i * 2] * w;
        const double head = f[i * 2 + 1] * w;
        if (i == 0) {
            gap = head < c ? 0.0 : std::min(head - c, t().linear_gap);
            visible = head >= c;
        }
        const double a_head = std::clamp(head, c, w - c);
        const double a_tail = std::clamp(tail, c, w - c);
        const double spacing = visible ? gap + c * 2.0 : gap;
        if (next_end > a_head + spacing) {
            out.push_back({{origin_.x + next_end, y}, {origin_.x + std::max(c, a_head + spacing), y}});
        }
        if (head > tail) {
            next_end = std::max(c, a_tail - spacing);
        }
    }
    if (next_end > c) {
        out.push_back({{origin_.x + next_end, y}, {origin_.x + c, y}});
    }
    return out;
}

std::optional<render::Rect> LinearWavyProgressIndicator::stop_indicator() const {
    if (!determinate()) {
        return std::nullopt;
    }
    const double track = t().linear_track_thickness;
    double size = std::min(track, t().linear_stop_size * handoff());
    const double offset = size == track ? 0.0 : track / 4.0;
    double x = width_ - size - offset;
    const double progress_x = width_ * fractions()[1] + cap();
    if (x <= progress_x) {
        size = std::max(0.0, size - (progress_x - x));
        x = progress_x;
    }
    if (size <= 0.0) {
        return std::nullopt;
    }
    return render::Rect{origin_.x + x, origin_.y + (height() - size) / 2.0, size, size};
}

void LinearWavyProgressIndicator::step(double dt) {
    const double speed = options_.wave_speed.value_or(wavelength());
    const double lambda = wavelength();
    const double period = speed > 0.0 && lambda > 0.0 ? std::max(t().min_offset_seconds, lambda / speed) : 0.0;
    WavyProgressCore::step(dt, period);
}

void LinearWavyProgressIndicator::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const color::Argb track = scheme[options_.track];
    const color::Argb indicator = scheme[options_.indicator];
    canvas.save();
    canvas.push_clip(bounds());
    for (const auto& line : track_lines()) {
        stroke(canvas, line, t().linear_track_thickness, track);
    }
    for (const auto& line : active_lines()) {
        stroke(canvas, line, t().linear_thickness, indicator);
    }
    if (const auto dot = stop_indicator()) {
        canvas.fill_path(render::circle_path({dot->x + dot->width / 2.0, dot->y + dot->height / 2.0}, dot->width / 2.0), indicator);
    }
    canvas.pop_clip();
    canvas.restore();
}

CircularWavyProgressIndicator::CircularWavyProgressIndicator(WavyProgressOptions options) : WavyProgressCore(std::move(options)) {}

double CircularWavyProgressIndicator::wavelength() const {
    return options_.wavelength.value_or(t().circular_wavelength);
}

int CircularWavyProgressIndicator::vertex_count() const {
    const double r = size_ / 2.0 - t().circular_thickness / 2.0;
    return std::max(t().min_circular_vertices, static_cast<int>(std::lround(tau * r / wavelength())));
}

double CircularWavyProgressIndicator::sweep() const {
    if (!determinate()) {
        return circular_indeterminate_sweep(play_ms_);
    }
    if (handoff() < 1.0) {
        return motion::lerp(handoff_sweep_, progress(), handoff());
    }
    return progress();
}

double CircularWavyProgressIndicator::rotation_degrees() const {
    if (!determinate()) {
        return circular_indeterminate_rotation(play_ms_);
    }
    if (handoff() < 1.0) {
        return std::fmod(motion::lerp(handoff_rotation_, 360.0, handoff()), 360.0);
    }
    return 0.0;
}

void CircularWavyProgressIndicator::on_handoff() {
    handoff_sweep_ = circular_indeterminate_sweep(play_ms_);
    const double r = std::fmod(circular_indeterminate_rotation(play_ms_), 360.0);
    handoff_rotation_ = r == 0.0 ? 360.0 : r;
    const double remaining = (360.0 - handoff_rotation_) / 360.0;
    motion::TweenSpec spec = m().circular_handoff_min;
    spec.duration_seconds = motion::lerp(m().circular_handoff_min.duration_seconds, m().circular_handoff_max.duration_seconds, remaining);
    handoff_.snap_to(0.0);
    handoff_.animate_to(1.0, spec);
}

std::vector<render::Point> CircularWavyProgressIndicator::outline(bool active, bool repeat) const {
    const CircularShapes& shape = circular_shapes(vertex_count());
    std::vector<shapes::Cubic> cubics;
    shapes::Point pivot{0.5f, 0.5f};
    if (!active) {
        cubics = shape.circle.cubics();
        pivot = shape.circle.center();
    } else {
        const double amp = amplitude();
        if (amp >= 1.0) {
            cubics = shape.star.cubics();
        } else if (amp <= 0.0) {
            cubics = shape.circle.cubics();
        } else {
            cubics = shape.morph.as_cubics(static_cast<float>(amp));
        }
    }
    const double first = std::atan2(cubics.front().anchor0_y() - pivot.y, cubics.front().anchor0_x() - pivot.x);
    const double turn = radians(t().start_angle_degrees) - first;
    std::vector<render::Point> pts;
    auto add = [&](bool lead) {
        std::vector<render::Point> run;
        for (const shapes::Cubic& c : cubics) {
            append_cubic(run, {c.anchor0_x(), c.anchor0_y()}, {c.control0_x(), c.control0_y()}, {c.control1_x(), c.control1_y()},
                         {c.anchor1_x(), c.anchor1_y()}, t().flatten_segments);
        }
        pts.insert(pts.end(), run.begin() + (lead ? 0 : 1), run.end());
    };
    add(true);
    if (repeat) {
        add(false);
    }
    const render::Point p{pivot.x, pivot.y};
    double left = std::numeric_limits<double>::max();
    double top = left;
    double right = std::numeric_limits<double>::lowest();
    double bottom = right;
    const double scale = size_ - t().circular_thickness;
    for (render::Point& q : pts) {
        q = rotate_about(q, p, turn) * scale;
        left = std::min(left, q.x);
        right = std::max(right, q.x);
        top = std::min(top, q.y);
        bottom = std::max(bottom, q.y);
    }
    const render::Point c = centre();
    const render::Point shift{c.x - (left + right) / 2.0, c.y - (top + bottom) / 2.0};
    for (render::Point& q : pts) {
        q = q + shift;
    }
    return pts;
}

std::vector<render::Point> CircularWavyProgressIndicator::active_line() const {
    const bool motion = options_.wave_speed.value_or(wavelength()) > 0.0;
    const render::Polyline full(outline(true, motion));
    const double length = motion ? full.length() / 2.0 : full.length();
    const double stop = sweep() * length;
    const double shift = motion ? std::clamp(wave_offset(), 0.0, 1.0) * length : 0.0;
    std::vector<render::Point> line = full.segment(shift, stop + shift);
    if (motion) {
        rotate_all(line, centre(), -radians(std::fmod(wave_offset() * 360.0, 360.0)));
    }
    rotate_all(line, centre(), radians(rotation_degrees()));
    return line;
}

std::vector<render::Point> CircularWavyProgressIndicator::track_line() const {
    const render::Polyline full(outline(false, false));
    const double length = full.length();
    const double progress_length = render::Polyline(outline(true, false)).length();
    const double stop = sweep() * progress_length;
    const double gap = std::min(stop, t().circular_gap);
    const double insets = std::min(stop, std::max(t().circular_thickness, t().circular_track_thickness) / 2.0);
    const double spacing = insets * 2.0 + gap;
    std::vector<render::Point> line = full.segment(sweep() * length + spacing, length - spacing);
    rotate_all(line, centre(), radians(rotation_degrees()));
    return line;
}

void CircularWavyProgressIndicator::step(double dt) {
    const double speed = options_.wave_speed.value_or(wavelength());
    const double lambda = wavelength();
    const double period =
        speed > 0.0 && lambda > 0.0 ? std::max(t().min_offset_seconds, lambda / speed * static_cast<double>(vertex_count())) : 0.0;
    WavyProgressCore::step(dt, period);
}

void CircularWavyProgressIndicator::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    stroke(canvas, track_line(), t().circular_track_thickness, scheme[options_.track]);
    stroke(canvas, active_line(), t().circular_thickness, scheme[options_.indicator]);
}

}
