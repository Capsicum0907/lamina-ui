#include "lamina/widgets/dialog.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/nodes.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::DialogTokens& t = tokens::dialog_tokens;
using tokens::motion_scheme;

ButtonOptions action(std::string label) {
    return {std::move(label), t.action_style, t.action_size};
}

constexpr input::PointerEvent press_elsewhere{input::PointerAction::down, -std::numeric_limits<double>::infinity(),
                                              -std::numeric_limits<double>::infinity()};

}

Dialog::Dialog(DialogOptions options, const text::FontStack& fonts)
    : headline_({options.headline, t.headline, t.headline_color}, fonts),
      supporting_({options.supporting_text, t.supporting_text, t.supporting_text_color}, fonts),
      confirm_(action(options.confirm_label), fonts),
      icon_(std::move(options.icon)),
      content_(std::move(options.content)),
      popup_(t.closed_scale, motion_scheme.default_spatial(), motion_scheme.default_effects()),
      default_action_(options.default_action) {
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
    std::vector<layout::NodePtr> slot_children;
    if (!options.supporting_text.empty()) {
        slot_children.push_back(node(supporting_));
    }
    if (content_ && content_->node) {
        if (!slot_children.empty()) {
            slot_children.push_back(layout::spacer({0.0, t.supporting_text_content_space}));
        }
        slot_children.push_back(content_->node);
    }
    if (!slot_children.empty()) {
        auto inner = layout::column(std::move(slot_children));
        inner->fill_cross(true);
        slot_inner_ = inner;
        slot_ = std::make_shared<layout::Leaf>(
            [this](const layout::Constraints& c) {
                const layout::Size s = slot_inner_->measure({0.0, c.max_width, 0.0, layout::unbounded});
                slot_content_height_ = s.height;
                return layout::Size{s.width, std::min(s.height, slot_limit_)};
            },
            [this](const render::Rect& r) {
                scroll_.set_viewport(r);
                scroll_.set_content_height(slot_content_height_);
                slot_inner_->arrange({0.0, 0.0, r.width, slot_content_height_});
            });
        slot_->self_align = layout::CrossAlign::stretch;
        children.push_back(slot_);
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

std::optional<render::Rect> Dialog::content_viewport() const {
    if (!slot_) {
        return std::nullopt;
    }
    return scroll_.viewport();
}

layout::Size Dialog::measure_root(const render::Rect& area) {
    const double max_width = std::max(t.min_width, std::min(t.max_width, area.width));
    return root_->measure({t.min_width, max_width, 0.0, layout::unbounded});
}

void Dialog::set_area(const render::Rect& area) {
    area_ = area;
    slot_limit_ = layout::unbounded;
    layout::Size s = measure_root(area);
    const double max_height = std::max(0.0, area.height - 2.0 * t.window_margin);
    if (slot_ && s.height > max_height) {
        slot_limit_ = std::max(0.0, slot_content_height_ - (s.height - max_height));
        s = measure_root(area);
    }
    surface_ = {area.x + (area.width - s.width) / 2.0, area.y + (area.height - s.height) / 2.0, s.width, s.height};
    root_->arrange(surface_);
}

void Dialog::set_window(layout::Size window) {
    set_area({0.0, 0.0, window.width, window.height});
}

void Dialog::open_in(const render::Rect& area, input::InputSource source) {
    set_area(area);
    popup_.open();
    clear_focus();
    scroll_.jump_to(0.0);
    const std::vector<DialogAction> order = enabled_order();
    const bool has_default = default_action_ && (*default_action_ == DialogAction::confirm || dismiss_) && action_enabled(*default_action_);
    if (has_default) {
        focus_ = *default_action_;
    } else if (!order.empty()) {
        focus_ = order.front();
    }
    if (source == input::InputSource::keyboard && focus_) {
        show_focus(*focus_);
    }
}

void Dialog::open(layout::Size window, input::InputSource source) {
    open_in({0.0, 0.0, window.width, window.height}, source);
}

void Dialog::close() {
    popup_.close();
    cancel_interaction();
    clear_focus();
}

std::optional<DialogAction> Dialog::focused_action() const {
    return popup_.is_open() ? focus_ : std::nullopt;
}

bool Dialog::content_focused() const {
    return content_ && content_->focus && content_->focus->has_focus();
}

bool Dialog::content_has_stops() const {
    return content_ && content_->focus && content_->focus->any_can_focus();
}

std::vector<DialogAction> Dialog::reading_order() const {
    std::vector<DialogAction> order{DialogAction::confirm};
    if (dismiss_) {
        const render::Rect c = confirm_.bounds();
        const render::Rect d = dismiss_->bounds();
        const bool dismiss_first = d.y < c.y || (d.y == c.y && d.x < c.x);
        order.insert(dismiss_first ? order.begin() : order.end(), DialogAction::dismiss);
    }
    return order;
}

std::vector<DialogAction> Dialog::enabled_order() const {
    std::vector<DialogAction> order = reading_order();
    std::erase_if(order, [this](DialogAction a) { return !action_enabled(a); });
    return order;
}

Button& Dialog::button(DialogAction action) {
    return action == DialogAction::dismiss && dismiss_ ? *dismiss_ : confirm_;
}

const Button& Dialog::button(DialogAction action) const {
    return action == DialogAction::dismiss && dismiss_ ? *dismiss_ : confirm_;
}

bool Dialog::action_enabled(DialogAction action) const {
    if (action == DialogAction::dismiss && !dismiss_) {
        return false;
    }
    return button(action).enabled();
}

void Dialog::set_action_enabled(DialogAction action, bool enabled) {
    if (action == DialogAction::dismiss && !dismiss_) {
        return;
    }
    button(action).set_enabled(enabled);
    if (!enabled && focus_ == action) {
        const bool shown = focus_shown_;
        clear_action_focus();
        const std::vector<DialogAction> order = enabled_order();
        if (!order.empty() && popup_.is_open()) {
            focus_ = order.front();
            if (shown) {
                show_focus(*focus_);
            }
        }
    }
}

DialogResult Dialog::result_of(DialogAction action) {
    close();
    return action == DialogAction::confirm ? DialogResult::confirmed : DialogResult::dismissed;
}

void Dialog::show_focus(DialogAction action) {
    clear_focus();
    focus_ = action;
    focus_shown_ = true;
    button(action).set_focused(true);
}

void Dialog::clear_action_focus() {
    confirm_.set_focused(false);
    if (dismiss_) {
        dismiss_->set_focused(false);
    }
    focus_.reset();
    focus_shown_ = false;
}

void Dialog::clear_focus() {
    clear_action_focus();
    if (content_ && content_->focus) {
        content_->focus->clear_focus();
    }
}

void Dialog::reveal_content_focus() {
    if (!content_ || !content_->focus) {
        return;
    }
    if (const std::optional<render::Rect> b = content_->focus->focused_bounds()) {
        scroll_.ensure_visible(b->y, b->y + b->height);
    }
}

void Dialog::tab(bool backward) {
    const FocusDirection direction = backward ? FocusDirection::previous : FocusDirection::next;
    const std::vector<DialogAction> order = enabled_order();
    auto enter_content = [&] {
        clear_action_focus();
        content_->focus->clear_focus();
        content_->focus->advance(direction);
        reveal_content_focus();
    };
    if (content_focused()) {
        if (content_->focus->advance(direction)) {
            reveal_content_focus();
            return;
        }
        content_->focus->clear_focus();
        if (order.empty()) {
            content_->focus->advance(direction);
            reveal_content_focus();
            return;
        }
        show_focus(backward ? order.back() : order.front());
        return;
    }
    if (order.empty()) {
        if (content_has_stops()) {
            enter_content();
        }
        return;
    }
    const auto at = focus_ ? std::find(order.begin(), order.end(), *focus_) : order.end();
    const std::size_t n = order.size();
    if (at == order.end()) {
        if (!backward && content_has_stops()) {
            enter_content();
            return;
        }
        show_focus(backward ? order.back() : order.front());
        return;
    }
    const auto i = static_cast<std::size_t>(at - order.begin());
    const bool leaves = backward ? i == 0 : i + 1 == n;
    if (leaves && content_has_stops()) {
        enter_content();
        return;
    }
    show_focus(order[backward ? (i + n - 1) % n : (i + 1) % n]);
}

void Dialog::deliver_to_content(const input::PointerEvent& e) {
    if (!content_ || !content_->widgets) {
        return;
    }
    const input::PointerEvent inside = scroll_.to_content(e);
    if (e.action == input::PointerAction::down && inside.action == input::PointerAction::leave) {
        content_->widgets->handle(press_elsewhere);
        return;
    }
    content_->widgets->handle(inside);
}

DialogResult Dialog::handle(const input::PointerEvent& e) {
    if (!popup_.is_open()) {
        return DialogResult::none;
    }
    if (popup_.dismisses(e, surface_) && contains(area_, e.x_dp, e.y_dp)) {
        close();
        return DialogResult::dismissed;
    }
    if (e.action == input::PointerAction::down) {
        focus_shown_ = false;
    }
    if (slot_) {
        scroll_.route(e, [&](const input::PointerEvent& pe) {
            deliver_to_content(pe);
            return content_ && content_->widgets && content_->widgets->holds_pointer();
        });
        if (e.action == input::PointerAction::down && content_focused()) {
            clear_action_focus();
        }
    }
    if (confirm_.handle(e)) {
        return result_of(DialogAction::confirm);
    }
    if (dismiss_ && dismiss_->handle(e)) {
        return result_of(DialogAction::dismiss);
    }
    return DialogResult::none;
}

DialogResult Dialog::handle(const input::KeyEvent& e) {
    if (popup_.dismisses(e)) {
        close();
        return DialogResult::dismissed;
    }
    if (!popup_.is_open()) {
        return DialogResult::none;
    }
    if (e.key == input::Key::tab && !e.ctrl) {
        tab(e.shift);
        return DialogResult::none;
    }
    if (content_focused()) {
        content_->widgets->handle(e);
        reveal_content_focus();
        return DialogResult::none;
    }
    if (slot_ && scroll_.handle(e)) {
        return DialogResult::none;
    }
    if (focus_ && activates(e, KeyActivation::space_or_enter)) {
        return result_of(*focus_);
    }
    return DialogResult::none;
}

bool Dialog::handle(const input::TextEvent& e) {
    if (!popup_.is_open() || !content_focused() || !content_->widgets) {
        return false;
    }
    content_->widgets->handle(e);
    reveal_content_focus();
    return true;
}

bool Dialog::handle(const input::WheelEvent& e) {
    if (!popup_.is_open() || !slot_) {
        return false;
    }
    return scroll_.handle(e);
}

std::optional<render::Rect> Dialog::caret_area() const {
    if (!popup_.is_open() || !content_focused()) {
        return std::nullopt;
    }
    const std::optional<render::Rect> caret = content_->focus->caret_area();
    return caret ? std::optional<render::Rect>(scroll_.to_window(*caret)) : std::nullopt;
}

std::optional<double> Dialog::next_wake_seconds() const {
    std::optional<double> wake;
    if (content_ && content_->focus && popup_.is_open()) {
        wake = content_->focus->next_wake_seconds();
    }
    if (const double bar = scroll_.next_wake_seconds(); bar >= 0.0) {
        wake = wake ? std::min(*wake, bar) : bar;
    }
    return wake;
}

void Dialog::cancel_interaction() {
    confirm_.cancel_interaction();
    if (dismiss_) {
        dismiss_->cancel_interaction();
    }
    scroll_.cancel_interaction();
    if (content_ && content_->widgets) {
        content_->widgets->cancel_interaction();
    }
}

void Dialog::step(double dt) {
    popup_.step(dt);
    confirm_.step(dt);
    if (dismiss_) {
        dismiss_->step(dt);
    }
    if (slot_) {
        scroll_.step(dt);
        if (const std::optional<input::PointerEvent> hover = scroll_.rehover(); hover && content_ && content_->widgets) {
            content_->widgets->handle(*hover);
        }
    }
    if (content_ && content_->widgets) {
        content_->widgets->step(dt);
    }
}

bool Dialog::animating() const {
    return !popup_.settled() || confirm_.animating() || (dismiss_ && dismiss_->animating()) || (slot_ && scroll_.animating()) ||
           (content_ && content_->widgets && content_->widgets->animating());
}

std::optional<AccessibleNode> Dialog::accessible() const {
    if (!popup_.is_open()) {
        return std::nullopt;
    }
    AccessibleNode dialog{AccessibleRole::dialog, headline_.text(), surface_, {.modal = true}};
    if (!supporting_.text().empty()) {
        AccessibleNode text = supporting_.accessible();
        text.bounds = scroll_.to_window(text.bounds);
        dialog.children.push_back(std::move(text));
    }
    if (content_ && content_->widgets) {
        std::vector<AccessibleNode> nodes = content_->widgets->accessible_nodes();
        std::function<void(AccessibleNode&)> shift = [&](AccessibleNode& n) {
            n.bounds = scroll_.to_window(n.bounds);
            for (AccessibleNode& c : n.children) {
                shift(c);
            }
        };
        for (AccessibleNode& n : nodes) {
            shift(n);
            dialog.children.push_back(std::move(n));
        }
    }
    for (DialogAction a : reading_order()) {
        dialog.children.push_back(button(a).accessible());
    }
    return dialog;
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
    if (slot_) {
        scroll_.begin_content(canvas);
        supporting_.paint(canvas, s);
        if (content_ && content_->widgets) {
            content_->widgets->paint(canvas, s);
        }
        scroll_.end_content(canvas);
        scroll_.paint(canvas, s);
    }
    if (dismiss_) {
        dismiss_->paint(canvas, s);
    }
    confirm_.paint(canvas, s);
    canvas.restore();
}

}
