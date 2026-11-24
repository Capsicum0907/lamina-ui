#include "lamina/widgets/loading_indicator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include "lamina/shapes/to_path.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::LoadingIndicatorTokens& t = tokens::loading_indicator_tokens;

double radians(double degrees) {
    return degrees * std::numbers::pi / 180.0;
}

std::vector<shapes::RoundedPolygon> polygons(bool determinate) {
    std::vector<shapes::RoundedPolygon> out;
    if (determinate) {
        const float a = static_cast<float>(radians(t.determinate_circle_rotation_degrees));
        const float c = std::cos(a);
        const float s = std::sin(a);
        out.push_back(shapes::material_shape(tokens::loading_indicator_determinate_shapes[0]).transformed([c, s](shapes::Point p) {
            return shapes::Point{c * p.x - s * p.y, s * p.x + c * p.y};
        }));
        out.push_back(shapes::material_shape(tokens::loading_indicator_determinate_shapes[1]));
        return out;
    }
    for (shapes::MaterialShape m : tokens::loading_indicator_indeterminate_shapes) {
        out.push_back(shapes::material_shape(m));
    }
    return out;
}

std::vector<shapes::Morph> build_morphs(bool determinate) {
    const std::vector<shapes::RoundedPolygon> p = polygons(determinate);
    std::vector<shapes::Morph> out;
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (i + 1 < p.size()) {
            out.emplace_back(p[i].normalized(), p[i + 1].normalized());
        } else if (!determinate) {
            out.emplace_back(p[i].normalized(), p[0].normalized());
        }
    }
    return out;
}

double scale_factor(bool determinate) {
    double factor = 1.0;
    for (const shapes::RoundedPolygon& polygon : polygons(determinate)) {
        const shapes::Bounds b = polygon.calculate_bounds(true);
        const shapes::Bounds m = polygon.calculate_max_bounds();
        const double sx = (b.right - b.left) / (m.right - m.left);
        const double sy = (b.bottom - b.top) / (m.bottom - m.top);
        factor = std::min(factor, std::max(sx, sy));
    }
    return factor * t.active_size / t.container_size;
}

}

const std::vector<shapes::Morph>& loading_indicator_morphs(bool determinate) {
    static const std::vector<shapes::Morph> indeterminate = build_morphs(false);
    static const std::vector<shapes::Morph> determinate_morphs = build_morphs(true);
    return determinate ? determinate_morphs : indeterminate;
}

double loading_indicator_scale(bool determinate) {
    static const double indeterminate = scale_factor(false);
    static const double determinate_scale = scale_factor(true);
    return determinate ? determinate_scale : indeterminate;
}

double spring_finish_seconds(const motion::SpringSpec& spec, double threshold) {
    const double omega = std::sqrt(spec.stiffness);
    const double zeta = spec.damping_ratio;
    if (zeta >= 1.0) {
        double time = 0.0;
        const double dt = t.estimate_step_seconds;
        motion::SpringState s{0.0, 0.0};
        while (std::abs(1.0 - s.value) >= threshold && time < t.estimate_limit_seconds) {
            s = motion::advance(spec, s, 1.0, dt);
            time += dt;
        }
        return time;
    }
    const double r = -zeta * omega;
    const double damped = omega * std::sqrt(1.0 - zeta * zeta);
    const double c1 = 1.0;
    const double c2 = -r * c1 / damped;
    return std::log(threshold / std::hypot(c1, c2)) / r;
}

LoadingIndicator::LoadingIndicator(LoadingIndicatorOptions options)
    : options_(std::move(options)),
      morph_(0.0, t.morph_spring),
      handoff_(0.0, t.handoff_spring),
      turn_(t.morph_turn_degrees) {
    if (options_.progress) {
        options_.progress = std::clamp(*options_.progress, 0.0, 1.0);
    }
    morph_seconds_ = spring_finish_seconds(t.morph_spring, t.morph_threshold);
    if (!determinate()) {
        start_morph();
    }
}

void LoadingIndicator::start_morph() {
    morph_.snap_to(0.0);
    morph_.animate_to(1.0);
    morph_elapsed_ = 0.0;
    morph_running_ = true;
}

void LoadingIndicator::set_progress(std::optional<double> progress) {
    const bool was = determinate();
    const double turned = rotation_degrees();
    const render::Point centre = shape_centre();
    options_.progress = progress ? std::optional(std::clamp(*progress, 0.0, 1.0)) : std::nullopt;
    if (was && !determinate()) {
        morph_index_ = 0;
        turn_ = turned;
        spin_ = 0.0;
        cycle_elapsed_ = 0.0;
        start_morph();
        const render::Point now = shape_centre();
        handoff_shift_ = {centre.x - now.x, centre.y - now.y};
        handoff_.snap_to(1.0);
        handoff_.animate_to(0.0);
    } else if (determinate()) {
        handoff_.snap_to(0.0);
    }
}

std::size_t LoadingIndicator::morph_count() const {
    return loading_indicator_morphs(determinate()).size();
}

double LoadingIndicator::morph_progress() const {
    if (!determinate()) {
        return morph_.value();
    }
    const double p = *options_.progress;
    const double n = static_cast<double>(morph_count());
    const std::size_t index = std::min(static_cast<std::size_t>(n * p), morph_count() - 1);
    if (p == 1.0 && index == morph_count() - 1) {
        return 1.0;
    }
    return std::fmod(p * n, 1.0);
}

double LoadingIndicator::rotation_degrees() const {
    if (determinate()) {
        return *options_.progress * t.determinate_turn_degrees;
    }
    return morph_.value() * t.morph_turn_degrees + turn_ + spin_;
}

namespace {

struct ControlBounds {
    double left;
    double top;
    double right;
    double bottom;
};

ControlBounds control_bounds(const std::vector<shapes::Cubic>& cubics) {
    ControlBounds b{std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
                    std::numeric_limits<double>::lowest()};
    for (const shapes::Cubic& c : cubics) {
        const auto& pts = c.points();
        for (std::size_t i = 0; i < pts.size(); i += 2) {
            b.left = std::min(b.left, static_cast<double>(pts[i]));
            b.right = std::max(b.right, static_cast<double>(pts[i]));
            b.top = std::min(b.top, static_cast<double>(pts[i + 1]));
            b.bottom = std::max(b.bottom, static_cast<double>(pts[i + 1]));
        }
    }
    return b;
}

}

std::vector<shapes::Cubic> LoadingIndicator::shape_cubics() const {
    const std::vector<shapes::Morph>& morphs = loading_indicator_morphs(determinate());
    std::size_t index = morph_index_;
    if (determinate()) {
        index = std::min(static_cast<std::size_t>(static_cast<double>(morphs.size()) * *options_.progress), morphs.size() - 1);
    }
    return morphs[index].as_cubics(static_cast<float>(morph_progress()));
}

render::Point LoadingIndicator::shape_centre() const {
    const ControlBounds b = control_bounds(shape_cubics());
    return {(b.left + b.right) / 2.0, (b.top + b.bottom) / 2.0};
}

render::Path LoadingIndicator::indicator_path() const {
    const std::vector<shapes::Cubic> cubics = shape_cubics();
    const double h = determinate() ? 0.0 : handoff_.value();
    const double size = options_.size;
    const double scale = size * (loading_indicator_scale(determinate()) + h * (loading_indicator_scale(true) - loading_indicator_scale(false)));
    const ControlBounds b = control_bounds(cubics);
    const render::Point centre{origin_.x + size / 2.0, origin_.y + size / 2.0};
    const double cx = (b.left + b.right) / 2.0 + h * handoff_shift_.x;
    const double cy = (b.top + b.bottom) / 2.0 + h * handoff_shift_.y;
    const double a = radians(rotation_degrees());
    const double cs = std::cos(a);
    const double sn = std::sin(a);
    const render::Affine m{scale * cs, scale * sn, -scale * sn, scale * cs, centre.x - scale * (cs * cx - sn * cy),
                           centre.y - scale * (sn * cx + cs * cy)};
    return shapes::to_path(cubics).transformed(m);
}

AccessibleNode LoadingIndicator::accessible() const {
    AccessibleNode node{AccessibleRole::progress_indicator, options_.accessible_name, bounds()};
    if (options_.progress) {
        node.state.range = AccessibleRange{*options_.progress, 0.0, 1.0};
    }
    return node;
}

void LoadingIndicator::step(double dt) {
    handoff_.step(dt);
    if (determinate()) {
        return;
    }
    spin_ = std::fmod(spin_ + 360.0 * dt / t.rotation_period_seconds, 360.0);
    cycle_elapsed_ += dt;
    if (morph_running_) {
        morph_.step(dt);
        morph_elapsed_ += dt;
        if (morph_elapsed_ >= morph_seconds_) {
            morph_running_ = false;
            morph_index_ = (morph_index_ + 1) % morph_count();
            morph_.snap_to(0.0);
            turn_ = std::fmod(turn_ + t.morph_turn_degrees, 360.0);
        }
    }
    if (!morph_running_ && cycle_elapsed_ >= t.morph_interval_seconds) {
        cycle_elapsed_ -= t.morph_interval_seconds;
        start_morph();
    }
}

void LoadingIndicator::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect r = bounds();
    const double radius = tokens::corner_radius(t.container_shape, r.width, r.height);
    const render::Path container = render::rounded_rect_path(r, render::uniform_radii(radius));
    if (options_.contained) {
        canvas.fill_path(container, scheme[t.contained_container]);
    }
    canvas.save();
    canvas.push_clip(container);
    canvas.fill_path(indicator_path(), scheme[options_.contained ? t.contained_indicator : t.indicator]);
    canvas.pop_clip();
    canvas.restore();
}

}
