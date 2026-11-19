#include "lamina/widgets/dialog.hpp"

#include <algorithm>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/nodes.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::DialogTokens& t = tokens::dialog_tokens;
using tokens::motion_scheme;

ButtonOptions action(std::string label) {
    return {std::move(label), t.action_style, t.action_size};
}

}

Dialog::Dialog(DialogOptions options, const text::FontStack& fonts)
    : headline_({options.headline, t.headline, t.headline_color}, fonts),
      supporting_({options.supporting_text, t.supporting_text, t.supporting_text_color}, fonts),
      confirm_(action(options.confirm_label), fonts),
      icon_(std::move(options.icon)),
      popup_(t.closed_scale, motion_scheme.default_spatial(), motion_scheme.default_effects()) {
    if (!options.dismiss_label.empty()) {
        dismiss_.emplace(action(options.dismiss_label), fonts);
    }
    std::vector<layout::NodePtr> buttons{node(confirm_)};
    if (dismiss_) {
        buttons.push_back(node(*dismiss_));
    }
    const double touch_slack = std::max(0.0, tokens::minimum_interactive_size - confirm_.height());
    actions_ = layout::flow(std::move(buttons));
    actions_->spacing(t.button_space)
        .line_spacing(std::clamp(t.button_line_space - touch_slack, 0.0, t.button_line_space) + touch_slack)
        .direction(layout::FlowDirection::end_to_start);
    actions_->self_align = layout::CrossAlign::end;

    std::vector<layout::NodePtr> children;
    if (icon_) {
        auto icon = layout::fixed({t.icon_size, t.icon_size}, [this](const render::Rect& r) { icon_origin_ = {r.x, r.y}; });
        icon->self_align = layout::CrossAlign::center;
        children.push_back(icon);
        children.push_back(layout::spacer({0.0, t.icon_bottom_space}));
    }
    if (!options.headline.empty()) {
        auto headline = node(headline_);
        if (icon_) {
            headline->self_align = layout::CrossAlign::center;
        }
        children.push_back(headline);
        children.push_back(layout::spacer({0.0, t.headline_bottom_space}));
    }
    if (!options.supporting_text.empty()) {
        children.push_back(node(supporting_));
        children.push_back(layout::spacer({0.0, t.supporting_text_bottom_space}));
    }
    children.push_back(actions_);
    auto column = layout::column(std::move(children));
    column->padding(layout::Insets::all(t.padding));
    root_ = column;
}

std::optional<render::Rect> Dialog::dismiss_bounds() const {
    if (!dismiss_) {
        return std::nullopt;
    }
    return dismiss_->bounds();
}

std::optional<render::Rect> Dialog::icon_bounds() const {
    if (!icon_) {
        return std::nullopt;
    }
    return render::Rect{icon_origin_.x, icon_origin_.y, t.icon_size, t.icon_size};
}

void Dialog::set_area(const render::Rect& area) {
    area_ = area;
    const double max_width = std::max(t.min_width, std::min(t.max_width, area.width));
    const layout::Size s = root_->measure({t.min_width, max_width, 0.0, area.height});
    surface_ = {area.x + (area.width - s.width) / 2.0, area.y + (area.height - s.height) / 2.0, s.width, s.height};
    root_->arrange(surface_);
}

void Dialog::set_window(layout::Size window) {
    set_area({0.0, 0.0, window.width, window.height});
}

void Dialog::open_in(const render::Rect& area) {
    set_area(area);
    popup_.open();
}

void Dialog::open(layout::Size window) {
    open_in({0.0, 0.0, window.width, window.height});
}

void Dialog::close() {
    popup_.close();
    cancel_interaction();
}

DialogResult Dialog::handle(const input::PointerEvent& e) {
    if (!popup_.is_open()) {
        return DialogResult::none;
    }
    if (popup_.dismisses(e, surface_) && contains(area_, e.x_dp, e.y_dp)) {
        close();
        return DialogResult::dismissed;
    }
    if (confirm_.handle(e)) {
        close();
        return DialogResult::confirmed;
    }
    if (dismiss_ && dismiss_->handle(e)) {
        close();
        return DialogResult::dismissed;
    }
    return DialogResult::none;
}

DialogResult Dialog::handle(const input::KeyEvent& e) {
    if (popup_.dismisses(e)) {
        close();
        return DialogResult::dismissed;
    }
    return DialogResult::none;
}

void Dialog::cancel_interaction() {
    confirm_.cancel_interaction();
    if (dismiss_) {
        dismiss_->cancel_interaction();
    }
}

void Dialog::step(double dt) {
    popup_.step(dt);
    confirm_.step(dt);
    if (dismiss_) {
        dismiss_->step(dt);
    }
}

bool Dialog::animating() const {
    return !popup_.settled() || confirm_.animating() || (dismiss_ && dismiss_->animating());
}

void Dialog::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    const double a = opacity();
    canvas.fill_rect(area_, color::with_alpha(scheme[t.scrim], t.scrim_opacity * a));
    const color::ColorScheme s = color::faded(scheme, a);
    canvas.save();
    popup_.scale_about(canvas, {surface_.x + surface_.width / 2.0, surface_.y + surface_.height / 2.0});
    const double radius = tokens::corner_radius(t.corner, surface_.width, surface_.height);
    canvas.fill_path(render::rounded_rect_path(surface_, render::uniform_radii(radius)), s[t.container]);
    if (icon_) {
        icon_->font.draw(canvas, icon_->symbol, icon_origin_, t.icon_size, s[t.icon_color]);
    }
    headline_.paint(canvas, s);
    supporting_.paint(canvas, s);
    if (dismiss_) {
        dismiss_->paint(canvas, s);
    }
    confirm_.paint(canvas, s);
    canvas.restore();
}

}
