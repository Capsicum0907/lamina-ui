#include "lamina/widgets/checkbox.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/paint.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::CheckboxTokens& t = tokens::checkbox_tokens;

constexpr tokens::SpringSpec check_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::standard);
constexpr tokens::SpringSpec box_in_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);
constexpr tokens::SpringSpec box_out_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);
constexpr tokens::SpringSpec state_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);

struct Vec {
    double x;
    double y;
};

Vec sub(render::Point a, render::Point b) {
    return {a.x - b.x, a.y - b.y};
}

render::Point offset(render::Point p, Vec v, double k) {
    return {p.x + v.x * k, p.y + v.y * k};
}

double length(Vec v) {
    return std::hypot(v.x, v.y);
}

Vec unit(Vec v) {
    const double l = length(v);
    return l > 0.0 ? Vec{v.x / l, v.y / l} : Vec{0.0, 0.0};
}

Vec left_normal(Vec d) {
    return {-d.y, d.x};
}

template <std::size_t N>
void add_polygon(render::Path& path, std::array<render::Point, N> pts) {
    double area = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        const auto& a = pts[i];
        const auto& b = pts[(i + 1) % N];
        area += a.x * b.y - b.x * a.y;
    }
    if (area < 0.0) {
        std::reverse(pts.begin(), pts.end());
    }
    path.move_to(pts[0]);
    for (std::size_t i = 1; i < N; ++i) {
        path.line_to(pts[i]);
    }
    path.close();
}

void add_segment(render::Path& path, render::Point a, render::Point b, double half, bool cap_start, bool cap_end) {
    const Vec d = unit(sub(b, a));
    if (d.x == 0.0 && d.y == 0.0) {
        return;
    }
    const Vec n = left_normal(d);
    const render::Point s = cap_start ? offset(a, d, -half) : a;
    const render::Point e = cap_end ? offset(b, d, half) : b;
    add_polygon<4>(path, {offset(s, n, half), offset(e, n, half), offset(e, n, -half), offset(s, n, -half)});
}

void add_miter(render::Path& path, render::Point corner, Vec d1, Vec d2, double half) {
    const double turn = d1.x * d2.y - d1.y * d2.x;
    const Vec n1 = turn > 0.0 ? Vec{d1.y, -d1.x} : left_normal(d1);
    const Vec n2 = turn > 0.0 ? Vec{d2.y, -d2.x} : left_normal(d2);
    const double denom = 1.0 + n1.x * n2.x + n1.y * n2.y;
    if (denom <= 1e-6) {
        return;
    }
    const Vec m{(n1.x + n2.x) / denom, (n1.y + n2.y) / denom};
    add_polygon<4>(path, {corner, offset(corner, n1, half), offset(corner, m, half), offset(corner, n2, half)});
}

double lerp(double a, double b, double f) {
    return a + (b - a) * f;
}

}

render::Path check_mark_path(const render::Rect& box, double draw_fraction, double center_gravitation, double stroke_width) {
    render::Path path;
    const double f = std::clamp(draw_fraction, 0.0, 1.0);
    if (f <= 0.0) {
        return path;
    }
    const double g = center_gravitation;
    const double c = t.indeterminate_center;
    auto at = [&](double ux, double uy) { return render::Point{box.x + box.width * ux, box.y + box.width * uy}; };
    const render::Point left = at(t.check_left.x, lerp(t.check_left.y, c, g));
    const render::Point cross = at(lerp(t.check_cross.x, c, g), lerp(t.check_cross.y, c, g));
    const render::Point right = at(t.check_right.x, lerp(t.check_right.y, c, g));

    const double half = stroke_width / 2.0;
    const Vec d1 = unit(sub(cross, left));
    const Vec d2 = unit(sub(right, cross));
    const double l1 = length(sub(cross, left));
    const double l2 = length(sub(right, cross));
    const double drawn = (l1 + l2) * f;
    if (drawn <= l1) {
        add_segment(path, left, offset(left, d1, drawn), half, true, true);
        return path;
    }
    add_segment(path, left, cross, half, true, false);
    add_segment(path, cross, offset(cross, d2, drawn - l1), half, false, true);
    add_miter(path, cross, d1, d2, half);
    return path;
}

Checkbox::Checkbox(CheckboxOptions options)
    : options_(options),
      check_fraction_(options_.state == tokens::CheckboxState::off ? 0.0 : 1.0, check_spring),
      gravitation_(options_.state == tokens::CheckboxState::indeterminate ? 1.0 : 0.0, check_spring),
      checked_(options_.state == tokens::CheckboxState::off ? 0.0 : 1.0, box_in_spring),
      state_layer_(0.0, state_spring) {}

double Checkbox::width() const {
    return t.state_layer_size;
}

double Checkbox::height() const {
    return t.state_layer_size;
}

render::Rect Checkbox::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

render::Rect Checkbox::box() const {
    const render::Rect r = bounds();
    return {r.x + (r.width - t.box_size) / 2.0, r.y + (r.height - t.box_size) / 2.0, t.box_size, t.box_size};
}

void Checkbox::set_state(tokens::CheckboxState next) {
    const tokens::CheckboxState prev = options_.state;
    if (next == prev) {
        return;
    }
    options_.state = next;
    const double gravity = next == tokens::CheckboxState::indeterminate ? 1.0 : 0.0;
    if (next == tokens::CheckboxState::off) {
        snap_off_in_ = t.uncheck_snap_delay_seconds;
        checked_.set_spec(box_out_spring);
        checked_.animate_to(0.0);
        return;
    }
    snap_off_in_ = -1.0;
    checked_.set_spec(box_in_spring);
    checked_.animate_to(1.0);
    check_fraction_.animate_to(1.0);
    if (prev == tokens::CheckboxState::off) {
        gravitation_.snap_to(gravity);
    } else {
        gravitation_.animate_to(gravity);
    }
}

bool Checkbox::handle(const input::PointerEvent& e) {
    const bool clicked = press_.update(e, touch_target(bounds()));
    if (clicked) {
        set_state(options_.state == tokens::CheckboxState::on ? tokens::CheckboxState::off : tokens::CheckboxState::on);
    }
    state_layer_.animate_to(press_.state_layer_opacity());
    return clicked;
}

void Checkbox::step(double dt) {
    if (snap_off_in_ >= 0.0) {
        snap_off_in_ -= dt;
        if (snap_off_in_ <= 0.0) {
            snap_off_in_ = -1.0;
            check_fraction_.snap_to(0.0);
            gravitation_.snap_to(0.0);
        }
    }
    check_fraction_.step(dt);
    gravitation_.step(dt);
    checked_.step(dt);
    state_layer_.step(dt);
}

bool Checkbox::animating() const {
    return snap_off_in_ >= 0.0 || !check_fraction_.settled() || !gravitation_.settled() || !checked_.settled() ||
           !state_layer_.settled();
}

void Checkbox::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect b = box();
    const double p = checked_.value();
    const color::Argb box_color = mix_optional(scheme, std::nullopt, t.checked_box, p);
    const color::Argb border_color = color::mix(scheme[t.unchecked_border], scheme[t.checked_box], p);
    const color::Argb check_color = mix_optional(scheme, std::nullopt, t.checkmark, p);

    if (box_color == border_color) {
        canvas.fill_rounded_rect(b, render::uniform_radii(t.corner_radius), box_color);
    } else {
        const render::Rect inner{b.x + t.stroke_width, b.y + t.stroke_width, b.width - 2 * t.stroke_width,
                                 b.height - 2 * t.stroke_width};
        if ((box_color >> 24) != 0) {
            canvas.fill_rounded_rect(inner, render::uniform_radii(std::max(0.0, t.corner_radius - t.stroke_width)), box_color);
        }
        canvas.fill_path(ring_path(b, t.corner_radius, t.stroke_width), border_color, render::FillRule::even_odd);
    }
    if ((check_color >> 24) != 0) {
        canvas.fill_path(check_mark_path(b, check_fraction_.value(), gravitation_.value(), t.stroke_width), check_color);
    }
    if (state_layer_.value() > 0.0) {
        const render::Rect r = bounds();
        canvas.fill_path(circle_path({r.x + r.width / 2.0, r.y + r.height / 2.0}, t.state_layer_size / 2.0),
                         color::with_alpha(scheme[t.state_layer], state_layer_.value()));
    }
}

}
