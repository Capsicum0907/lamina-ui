#include "lamina/widgets/snackbar.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::SnackbarTokens& t = tokens::snackbar_tokens;
using tokens::motion_scheme;

}

Snackbar::Snackbar(SnackbarOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      message_({"", t.supporting_text, t.supporting_text_color}, fonts),
      popup_(t.closed_scale, motion_scheme.fast_spatial(), motion_scheme.fast_effects()) {
    if (!options_.action_label.empty()) {
        action_.emplace(ButtonOptions{options_.action_label, t.action_style, t.action_size, tokens::ButtonShape::round,
                                      std::nullopt, ButtonColors{std::nullopt, t.action_color}},
                        fonts);
    }
}

void Snackbar::show(std::string message) {
    message_.set_text(std::move(message));
    relayout();
    popup_.open();
    remaining_ = options_.duration_seconds;
}

void Snackbar::dismiss() {
    popup_.close();
    cancel_interaction();
}

void Snackbar::set_area(const render::Rect& area) {
    area_ = area;
    relayout();
}

void Snackbar::relayout() {
    const double width = std::clamp(area_.width - 2.0 * t.outer_padding, 0.0, t.max_width);
    const double action_width = action_ ? action_->width() : 0.0;
    const double text_width = std::max(0.0, width - t.start_padding - t.end_padding - action_width - t.text_end_spacing);
    const layout::Size text = message_.measure({0.0, text_width, 0.0, std::numeric_limits<double>::infinity()});
    const double lines = static_cast<double>(std::max<std::size_t>(1, message_.line_count()));
    const double line_height = tokens::type_style(t.supporting_text).line_height_sp;
    const double height = std::max(t.single_line_height, lines * line_height + 2.0 * t.text_vertical_padding);
    surface_ = {area_.x + (area_.width - width) / 2.0, area_.y + area_.height - t.outer_padding - height, width, height};
    message_.set_origin({surface_.x + t.start_padding, surface_.y + (height - text.height) / 2.0});
    if (action_) {
        action_->set_origin({surface_.x + width - t.end_padding - action_width, surface_.y + (height - action_->height()) / 2.0});
    }
}

std::optional<render::Rect> Snackbar::action_bounds() const {
    if (!action_) {
        return std::nullopt;
    }
    return action_->bounds();
}

SnackbarResponse Snackbar::handle(const input::PointerEvent& e) {
    if (!popup_.is_open()) {
        return {};
    }
    hovered_ = e.action != input::PointerAction::leave && contains(surface_, e.x_dp, e.y_dp);
    if (action_ && action_->handle(e)) {
        dismiss();
        return {true, true};
    }
    return {false, hovered_};
}

bool Snackbar::handle(const input::KeyEvent&) {
    return false;
}

void Snackbar::cancel_interaction() {
    hovered_ = false;
    if (action_) {
        action_->cancel_interaction();
    }
}

void Snackbar::step(double dt) {
    popup_.step(dt);
    if (action_) {
        action_->step(dt);
    }
    if (popup_.is_open() && !hovered_) {
        remaining_ -= dt;
        if (remaining_ <= 0.0) {
            remaining_ = 0.0;
            dismiss();
        }
    }
}

bool Snackbar::animating() const {
    return !popup_.settled() || (action_ && action_->animating());
}

double Snackbar::next_wake_seconds() const {
    if (!popup_.is_open() || hovered_) {
        return -1.0;
    }
    return remaining_;
}

void Snackbar::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    const color::ColorScheme s = color::faded(scheme, opacity());
    canvas.save();
    popup_.scale_about(canvas, {surface_.x + surface_.width / 2.0, surface_.y + surface_.height / 2.0});
    const double radius = tokens::corner_radius(t.corner, surface_.width, surface_.height);
    paint_surface(canvas, surface_, radius, s[t.container], tokens::elevation_dp[t.elevation], s[color::Role::shadow]);
    message_.paint(canvas, s);
    if (action_) {
        action_->paint(canvas, s);
    }
    canvas.restore();
}

}
