#include "lamina/widgets/tooltip.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::TooltipTokens& t = tokens::tooltip_tokens;
constexpr const tokens::PlainTooltipTokens& plain = tokens::plain_tooltip_tokens;
constexpr const tokens::RichTooltipTokens& rich = tokens::rich_tooltip_tokens;
using tokens::motion_scheme;

double fit(double v, double lo, double hi) {
    return std::clamp(v, lo, std::max(lo, hi));
}

double lines_height(const Label& label, tokens::TypeRole role) {
    return static_cast<double>(std::max<std::size_t>(1, label.line_count())) * tokens::type_style(role).line_height_sp;
}

ButtonOptions action_options(std::string label) {
    ButtonOptions o{std::move(label), rich.action_style, rich.action_size};
    o.colors = ButtonColors{std::nullopt, rich.action_color};
    return o;
}

}

render::Point tooltip_position(const render::Rect& anchor, layout::Size window, layout::Size tooltip, TooltipPosition position,
                               double spacing) {
    const double right_limit = window.width - tooltip.width;
    const double bottom_limit = window.height - tooltip.height;
    switch (position) {
    case TooltipPosition::left:
    case TooltipPosition::right: {
        double x = position == TooltipPosition::left ? anchor.x - (tooltip.width + spacing) : anchor.x + anchor.width + spacing;
        if (position == TooltipPosition::left && x < 0.0) {
            x = anchor.x + anchor.width + spacing;
        } else if (position == TooltipPosition::right && x + tooltip.width > window.width) {
            x = anchor.x - (tooltip.width + spacing);
        }
        const double y = anchor.y + (anchor.height - tooltip.height) / 2.0;
        return {fit(x, 0.0, right_limit), fit(y, 0.0, bottom_limit)};
    }
    case TooltipPosition::below: {
        const double x = anchor.x + (anchor.width - tooltip.width) / 2.0;
        double y = anchor.y + anchor.height + spacing;
        if (y + tooltip.height > window.height) {
            y = anchor.y - tooltip.height - spacing;
        }
        return {fit(x, 0.0, right_limit), fit(y, 0.0, bottom_limit)};
    }
    case TooltipPosition::above:
        break;
    }
    const double x = anchor.x + (anchor.width - tooltip.width) / 2.0;
    double y = anchor.y - tooltip.height - spacing;
    if (y < 0.0) {
        y = anchor.y + anchor.height + spacing;
    }
    return {fit(x, 0.0, right_limit), fit(y, 0.0, bottom_limit)};
}

Tooltip::Tooltip(TooltipOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      text_({options_.text, options_.kind == TooltipKind::rich ? rich.text : plain.text,
             options_.kind == TooltipKind::rich ? rich.text_color : plain.content, true},
            fonts),
      title_({options_.title, rich.title, rich.title_color, true}, fonts),
      popup_(t.closed_scale, motion_scheme.fast_spatial(), motion_scheme.fast_effects()) {
    if (options_.kind == TooltipKind::rich && !options_.action_label.empty()) {
        action_.emplace(action_options(options_.action_label), fonts);
    }
    relayout();
}

void Tooltip::set_anchor(const render::Rect& bounds) {
    anchor_ = [bounds] { return TooltipAnchor{bounds}; };
}

void Tooltip::set_text(std::string text) {
    options_.text = std::move(text);
    text_.set_text(options_.text);
    relayout();
}

TooltipAnchor Tooltip::anchor() const {
    return anchor_ ? anchor_() : TooltipAnchor{placed_anchor_};
}

double Tooltip::spacing() const {
    return t.anchor_spacing + (options_.caret ? t.caret_height : 0.0);
}

void Tooltip::relayout() {
    if (options_.kind == TooltipKind::plain) {
        const layout::Size s = text_.measure({0.0, plain.max_width - 2.0 * plain.horizontal_padding, 0.0, layout::unbounded});
        size_ = {std::clamp(s.width + 2.0 * plain.horizontal_padding, t.min_width, plain.max_width),
                 std::max(t.min_height, lines_height(text_, plain.text) + 2.0 * plain.vertical_padding)};
        return;
    }
    const double inner = rich.max_width - 2.0 * rich.horizontal_padding;
    const bool has_title = !options_.title.empty();
    double width = 0.0;
    double height = 0.0;
    if (has_title) {
        const layout::Size s = title_.measure({0.0, inner, 0.0, layout::unbounded});
        width = std::max(width, s.width);
        height += std::max(0.0, rich.title_first_baseline - title_.first_baseline()) + lines_height(title_, rich.title);
    }
    const layout::Size s = text_.measure({0.0, inner, 0.0, layout::unbounded});
    width = std::max(width, s.width);
    if (has_title || action_) {
        height += std::max(0.0, rich.text_first_baseline - text_.first_baseline()) + lines_height(text_, rich.text) + rich.text_bottom_padding;
    } else {
        height += 2.0 * rich.plain_vertical_padding + lines_height(text_, rich.text);
    }
    if (action_) {
        width = std::max(width, action_->width());
        height += std::max(rich.action_min_height, action_->height()) + rich.action_bottom_padding;
    }
    size_ = {std::clamp(width + 2.0 * rich.horizontal_padding, t.min_width, rich.max_width), std::max(t.min_height, height)};
}

render::Rect Tooltip::bounds() const {
    return {origin_.x, origin_.y, size_.width, size_.height};
}

std::optional<render::Rect> Tooltip::action_bounds() const {
    if (!action_) {
        return std::nullopt;
    }
    return action_->bounds();
}

std::optional<render::Rect> Tooltip::caret() const {
    if (!options_.caret) {
        return std::nullopt;
    }
    const render::Rect b = bounds();
    const render::Rect a = placed_anchor_;
    if (options_.position == TooltipPosition::left || options_.position == TooltipPosition::right) {
        const bool to_the_right = b.x > a.x;
        const double x = to_the_right ? b.x - t.caret_height : b.x + b.width;
        return render::Rect{x, b.y + b.height / 2.0 - t.caret_width / 2.0, t.caret_height, t.caret_width};
    }
    const bool below = b.y > a.y;
    const double mid = std::clamp(a.x + a.width / 2.0, b.x, b.x + b.width);
    const double y = below ? b.y - t.caret_height : b.y + b.height;
    return render::Rect{mid - t.caret_width / 2.0, y, t.caret_width, t.caret_height};
}

void Tooltip::open(TooltipTrigger trigger) {
    placed_anchor_ = anchor().bounds;
    origin_ = tooltip_position(placed_anchor_, window_, size_, options_.position, spacing());
    if (options_.kind == TooltipKind::plain) {
        const double slack = lines_height(text_, plain.text) - text_.bounds().height;
        text_.set_origin({origin_.x + plain.horizontal_padding, origin_.y + plain.vertical_padding + slack / 2.0});
    } else {
        const double x = origin_.x + rich.horizontal_padding;
        double y = origin_.y;
        if (!options_.title.empty()) {
            y += std::max(0.0, rich.title_first_baseline - title_.first_baseline());
            title_.set_origin({x, y});
            y += lines_height(title_, rich.title);
        }
        if (!options_.title.empty() || action_) {
            y += std::max(0.0, rich.text_first_baseline - text_.first_baseline());
        } else {
            y += rich.plain_vertical_padding + (lines_height(text_, rich.text) - text_.bounds().height) / 2.0;
        }
        text_.set_origin({x, y});
    }
    if (action_) {
        const double inner_bottom = origin_.y + size_.height - rich.action_bottom_padding;
        const double slot = std::max(rich.action_min_height, action_->height());
        action_->set_origin({origin_.x + rich.horizontal_padding, inner_bottom - slot + (slot - action_->height()) / 2.0});
    }
    popup_.open();
    trigger_ = trigger;
    hover_wait_ = -1.0;
    long_press_wait_ = -1.0;
    const bool times_out = !options_.persistent && (trigger == TooltipTrigger::focus || trigger == TooltipTrigger::long_press);
    time_left_ = times_out ? t.timeout_seconds : -1.0;
}

void Tooltip::show() {
    open(TooltipTrigger::program);
}

void Tooltip::dismiss() {
    if (popup_.is_open()) {
        popup_.close();
    }
    trigger_ = TooltipTrigger::none;
    time_left_ = -1.0;
    if (action_) {
        action_->set_focused(false);
        action_->cancel_interaction();
    }
}

void Tooltip::set_focused(bool focused) {
    if (action_) {
        action_->set_focused(focused && can_focus());
    }
}

TooltipResponse Tooltip::handle(const input::PointerEvent& e) {
    TooltipResponse response;
    const TooltipAnchor a = anchor();
    const bool live = e.action != input::PointerAction::leave;
    const bool over_anchor = live && contains(a.bounds, e.x_dp, e.y_dp);
    const bool over_tooltip = live && popup_.is_open() && contains(bounds(), e.x_dp, e.y_dp);

    if (action_ && popup_.is_open()) {
        if (action_->handle(e)) {
            response.action = true;
            response.consumed = true;
            dismiss();
            return response;
        }
        response.consumed = over_tooltip;
    }

    if (e.kind == input::PointerKind::mouse) {
        if (over_anchor && !hovered_) {
            hovered_ = true;
            if (!popup_.is_open()) {
                if (options_.hover_delay_seconds <= 0.0) {
                    open(TooltipTrigger::hover);
                } else {
                    hover_wait_ = options_.hover_delay_seconds;
                }
            }
        } else if (!over_anchor && hovered_) {
            hovered_ = false;
            hover_wait_ = -1.0;
            if (popup_.is_open() && trigger_ == TooltipTrigger::hover && !options_.persistent) {
                dismiss();
            }
        }
    } else {
        if (e.action == input::PointerAction::down && over_anchor) {
            long_press_wait_ = t.long_press_seconds;
        } else if (e.action == input::PointerAction::up || e.action == input::PointerAction::leave ||
                   (e.action == input::PointerAction::move && !over_anchor)) {
            long_press_wait_ = -1.0;
            if (popup_.is_open() && trigger_ == TooltipTrigger::long_press && e.action != input::PointerAction::move) {
                dismiss();
            }
        }
    }
    if (e.action == input::PointerAction::down && popup_.is_open() && !over_anchor && !over_tooltip) {
        dismiss();
    }
    return response;
}

TooltipResponse Tooltip::handle(const input::KeyEvent& e) {
    TooltipResponse response;
    if (!popup_.is_open()) {
        return response;
    }
    if (e.key == input::Key::escape) {
        dismiss();
        response.consumed = true;
        return response;
    }
    if (action_ && action_->handle(e)) {
        response.action = true;
        response.consumed = true;
        dismiss();
    }
    return response;
}

void Tooltip::cancel_interaction() {
    hovered_ = false;
    hover_wait_ = -1.0;
    long_press_wait_ = -1.0;
    if (action_) {
        action_->cancel_interaction();
    }
}

void Tooltip::step(double dt) {
    const bool focused_now = anchor().focused;
    if (focused_now && !anchor_focused_) {
        open(TooltipTrigger::focus);
    } else if (!focused_now && anchor_focused_ && trigger_ == TooltipTrigger::focus && !focused()) {
        dismiss();
    }
    anchor_focused_ = focused_now;
    if (hover_wait_ >= 0.0) {
        hover_wait_ -= dt;
        if (hover_wait_ <= 0.0) {
            open(TooltipTrigger::hover);
        }
    }
    if (long_press_wait_ >= 0.0) {
        long_press_wait_ -= dt;
        if (long_press_wait_ <= 0.0) {
            open(TooltipTrigger::long_press);
        }
    }
    if (time_left_ >= 0.0 && !focused()) {
        time_left_ -= dt;
        if (time_left_ <= 0.0) {
            dismiss();
        }
    }
    popup_.step(dt);
    if (action_) {
        action_->step(dt);
    }
}

bool Tooltip::animating() const {
    return !popup_.settled() || (action_ && action_->animating());
}

std::optional<double> Tooltip::next_wake_seconds() const {
    std::optional<double> wake;
    for (double w : {hover_wait_, long_press_wait_, focused() ? -1.0 : time_left_}) {
        if (w >= 0.0) {
            wake = wake ? std::min(*wake, w) : w;
        }
    }
    return wake;
}

void Tooltip::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    const color::ColorScheme s = color::faded(scheme, opacity());
    const render::Rect r = bounds();
    canvas.save();
    popup_.scale_about(canvas, {r.x + r.width / 2.0, r.y + r.height / 2.0});
    const bool is_rich = options_.kind == TooltipKind::rich;
    const color::Argb container = s[is_rich ? rich.container : plain.container];
    const double radius = tokens::corner_radius(is_rich ? rich.corner : plain.corner, r.width, r.height);
    if (is_rich) {
        paint_surface(canvas, r, radius, container, tokens::elevation_dp[rich.elevation], s[color::Role::shadow]);
    } else {
        canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(radius)), container);
    }
    if (const std::optional<render::Rect> c = caret()) {
        render::Path tri;
        const bool sideways = options_.position == TooltipPosition::left || options_.position == TooltipPosition::right;
        if (sideways) {
            const bool points_left = c->x < r.x;
            const double base = points_left ? c->x + c->width : c->x;
            const double tip = points_left ? c->x : c->x + c->width;
            tri.move_to({base, c->y});
            tri.line_to({tip, c->y + c->height / 2.0});
            tri.line_to({base, c->y + c->height});
        } else {
            const bool points_up = c->y < r.y;
            const double base = points_up ? c->y + c->height : c->y;
            const double tip = points_up ? c->y : c->y + c->height;
            tri.move_to({c->x, base});
            tri.line_to({c->x + c->width / 2.0, tip});
            tri.line_to({c->x + c->width, base});
        }
        tri.close();
        canvas.fill_path(tri, container);
    }
    if (!is_rich) {
        text_.paint(canvas, s);
        canvas.restore();
        return;
    }
    if (action_) {
        action_->paint(canvas, s);
    }
    title_.paint(canvas, s);
    text_.paint(canvas, s);
    canvas.restore();
}

std::optional<AccessibleNode> Tooltip::accessible() const {
    if (!popup_.is_open()) {
        return std::nullopt;
    }
    AccessibleNode node{AccessibleRole::tooltip, options_.title.empty() ? options_.text : options_.title, bounds()};
    if (!options_.title.empty()) {
        node.description = options_.text;
    }
    if (action_) {
        node.children.push_back(action_->accessible());
    }
    return node;
}

}
