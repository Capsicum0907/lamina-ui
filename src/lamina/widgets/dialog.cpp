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

constexpr tokens::SpringSpec scale_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::standard);
constexpr tokens::SpringSpec alpha_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);

ButtonOptions action(std::string label) {
    return {std::move(label), t.action_style, t.action_size};
}

}

color::ColorScheme faded(const color::ColorScheme& scheme, double opacity) {
    color::ColorScheme out = scheme;
    for (color::Argb& c : out.colors) {
        c = color::with_alpha(c, opacity * static_cast<double>(c >> 24) / 255.0);
    }
    return out;
}

Dialog::Dialog(DialogOptions options, const text::FontStack& fonts)
    : headline_(options.headline, t.headline, t.headline_color, fonts),
      supporting_(options.supporting_text, t.supporting_text, t.supporting_text_color, fonts),
      confirm_(action(options.confirm_label), fonts),
      icon_(std::move(options.icon)),
      scale_(t.closed_scale, scale_spring),
      alpha_(0.0, alpha_spring) {
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

void Dialog::set_window(layout::Size window) {
    window_ = window;
    const double max_width = std::max(t.min_width, std::min(t.max_width, window.width));
    const layout::Size s = root_->measure({t.min_width, max_width, 0.0, window.height});
    surface_ = {(window.width - s.width) / 2.0, (window.height - s.height) / 2.0, s.width, s.height};
    root_->arrange(surface_);
}

void Dialog::open(layout::Size window) {
    set_window(window);
    if (!visible()) {
        scale_.snap_to(t.closed_scale);
        alpha_.snap_to(0.0);
    }
    open_ = true;
    scale_.animate_to(1.0);
    alpha_.animate_to(1.0);
}

void Dialog::close() {
    open_ = false;
    scale_.animate_to(t.closed_scale);
    alpha_.animate_to(0.0);
    confirm_.handle({platform::PointerAction::leave, 0.0, 0.0});
    if (dismiss_) {
        dismiss_->handle({platform::PointerAction::leave, 0.0, 0.0});
    }
}

bool Dialog::visible() const {
    return open_ || !alpha_.settled() || alpha_.value() > 0.0;
}

double Dialog::opacity() const {
    return std::clamp(alpha_.value(), 0.0, 1.0);
}

DialogResult Dialog::handle(const platform::PointerEvent& e) {
    if (!open_) {
        return DialogResult::none;
    }
    if (e.action == platform::PointerAction::down && !contains(surface_, e.x_dp, e.y_dp)) {
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

DialogResult Dialog::handle(const platform::KeyEvent& e) {
    if (open_ && e.key == platform::Key::escape) {
        close();
        return DialogResult::dismissed;
    }
    return DialogResult::none;
}

void Dialog::step(double dt) {
    scale_.step(dt);
    alpha_.step(dt);
    confirm_.step(dt);
    if (dismiss_) {
        dismiss_->step(dt);
    }
}

bool Dialog::animating() const {
    return !scale_.settled() || !alpha_.settled() || confirm_.animating() || (dismiss_ && dismiss_->animating());
}

void Dialog::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    const double a = opacity();
    const render::Rect window{0.0, 0.0, window_.width, window_.height};
    canvas.fill_path(render::rounded_rect_path(window, render::uniform_radii(0.0)),
                     color::with_alpha(scheme[t.scrim], t.scrim_opacity * a));
    const color::ColorScheme s = faded(scheme, a);
    const double cx = surface_.x + surface_.width / 2.0;
    const double cy = surface_.y + surface_.height / 2.0;
    canvas.save();
    canvas.translate(cx, cy);
    canvas.scale(scale_.value());
    canvas.translate(-cx, -cy);
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
