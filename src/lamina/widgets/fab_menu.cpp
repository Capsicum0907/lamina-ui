#include "lamina/widgets/fab_menu.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/widgets/item_focus.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::ToggleFabTokens& tf = tokens::toggle_fab_tokens;
constexpr const tokens::FabMenuTokens& fm = tokens::fab_menu_tokens;

double initial_icon(tokens::FabSize size) {
    return tokens::fab_size(size).icon_size;
}

}

ToggleFab::ToggleFab(ToggleFabOptions options)
    : options_(std::move(options)), progress_(options_.checked ? 1.0 : 0.0, tf.progress), state_layer_(0.0, tokens::motion_scheme.fast_effects()) {
    options_.accessible_name = required_name(std::move(options_.accessible_name), "A toggle FAB");
}

render::Rect ToggleFab::container() const {
    const double initial = width();
    const double size = motion::lerp(initial, tf.final_size, progress_.value());
    return {origin_.x + initial - size, origin_.y, size, size};
}

double ToggleFab::corner_radius() const {
    const double initial = tf.initial_corner[static_cast<std::size_t>(options_.size)];
    return std::max(0.0, motion::lerp(initial, tf.final_size / 2.0, progress_.value()));
}

double ToggleFab::icon_size() const {
    return motion::lerp(initial_icon(options_.size), tf.final_icon_size, progress_.value());
}

void ToggleFab::set_checked(bool checked) {
    options_.checked = checked;
    progress_.animate_to(checked ? 1.0 : 0.0);
}

void ToggleFab::retarget() {
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
}

void ToggleFab::set_focused(bool focused) {
    focus_.set_focused(focused);
    retarget();
}

AccessibleNode ToggleFab::accessible() const {
    AccessibleState state{.focusable = true, .focused = focused()};
    state.toggle = checked() ? AccessibleToggle::on : AccessibleToggle::off;
    return {AccessibleRole::toggle_button, options_.accessible_name, container(), state};
}

bool ToggleFab::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    const bool clicked = press_.update(e, container());
    if (clicked) {
        set_checked(!checked());
    }
    retarget();
    return clicked;
}

bool ToggleFab::handle(const input::KeyEvent& e) {
    if (!focus_.activates(e, KeyActivation::space_or_enter)) {
        return false;
    }
    set_checked(!checked());
    return true;
}

void ToggleFab::cancel_interaction() {
    press_ = {};
    retarget();
}

void ToggleFab::step(double dt) {
    progress_.step(dt);
    state_layer_.step(dt);
    focus_.step(dt);
}

bool ToggleFab::animating() const {
    return !progress_.settled() || !state_layer_.settled() || !focus_.settled();
}

void ToggleFab::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double p = std::clamp(progress_.value(), 0.0, 1.0);
    const render::Rect r = container();
    const double radius = std::min(corner_radius(), std::min(r.width, r.height) / 2.0);
    const color::Argb content = color::mix(scheme[tf.content], scheme[tf.checked_content], p);
    const color::Argb fill = color::state_layer(color::mix(scheme[tf.container], scheme[tf.checked_container], p), content, state_layer_.value());
    paint_surface(canvas, r, radius, fill, tokens::elevation_dp[tf.elevation], scheme[color::Role::shadow]);
    focus_.paint(canvas, scheme, r, radius);
    const icons::Icon& icon = progress_.value() > tf.icon_swap_progress ? options_.checked_icon : options_.icon;
    icons::draw_centered(canvas, icon, r, icon_size(), content);
}

FabMenu::FabMenu(FabMenuOptions options, const text::FontStack& fonts)
    : options_(std::move(options)), fonts_(fonts), button_(options_.button), stagger_(0.0, fm.stagger) {
    const text::TextStyle style = tokens::text_style(tokens::type_style(fm.item_label));
    const bool open = options_.button.checked;
    for (FabMenuItem& spec : options_.items) {
        text::TextLine line = text::layout_line(fonts_, spec.label, style);
        const double full = std::max(fm.item_min_width, fm.item_leading_space + fm.item_icon_size + fm.item_icon_label_space + line.width +
                                                            fm.item_trailing_space);
        items_.push_back(Item{std::move(spec), std::move(line), full, {}, {}, motion::Animated(0.0, tokens::motion_scheme.fast_effects()),
                              motion::Animated(open ? 1.0 : 0.0, fm.item_width), motion::Animated(open ? 1.0 : 0.0, fm.item_alpha), open});
    }
    stagger_.snap_to(open ? static_cast<double>(items_.size()) : 0.0);
    place_button();
}

double FabMenu::items_width() const {
    double w = 0.0;
    for (const Item& item : items_) {
        w = std::max(w, item.full_width);
    }
    return w;
}

double FabMenu::items_height() const {
    if (items_.empty()) {
        return 0.0;
    }
    const double n = static_cast<double>(items_.size());
    return n * fm.item_height + (n - 1.0) * fm.item_between_space + button_.height() + fm.button_padding_bottom + fm.close_button_between_space;
}

double FabMenu::width() const {
    return 2.0 * fm.padding_horizontal + std::max(button_.width(), items_width());
}

double FabMenu::height() const {
    return std::max(button_.height() + fm.button_padding_bottom, items_height());
}

void FabMenu::place_button() {
    const double inner = std::max(button_.width(), items_width());
    const double x = options_.alignment == FabMenuAlignment::end ? origin_.x + fm.padding_horizontal + inner - button_.width()
                                                                 : origin_.x + fm.padding_horizontal;
    button_.set_origin({x, origin_.y + height() - button_.height() - fm.button_padding_bottom});
}

std::size_t FabMenu::visible_count() const {
    const double target = stagger_.target();
    const double v = stagger_.value();
    if (std::abs(v - target) < fm.stagger_threshold) {
        return static_cast<std::size_t>(std::lround(target));
    }
    return static_cast<std::size_t>(std::clamp(std::trunc(v), 0.0, static_cast<double>(items_.size())));
}

render::Rect FabMenu::item_rect(std::size_t index) const {
    const double inner = std::max(button_.width(), items_width());
    double y = origin_.y;
    for (std::size_t i = 0; i < index; ++i) {
        y += fm.item_height + fm.item_between_space;
    }
    const double w = items_[index].full_width * std::max(items_[index].width.value(), 0.0);
    const double left = origin_.x + fm.padding_horizontal;
    const double x = options_.alignment == FabMenuAlignment::end ? left + inner - w : left;
    return {x, y, w, fm.item_height};
}

void FabMenu::set_expanded(bool expanded) {
    button_.set_checked(expanded);
    stagger_.animate_to(expanded ? static_cast<double>(items_.size()) : 0.0);
    if (!expanded) {
        for (Item& item : items_) {
            item.focus.set_focused(false);
            item.press = {};
        }
    }
}

void FabMenu::retarget_items() {
    const std::size_t visible = visible_count();
    const std::size_t n = items_.size();
    for (std::size_t i = 0; i < n; ++i) {
        const bool on = i >= n - std::min(visible, n);
        Item& item = items_[i];
        if (on != item.target) {
            item.target = on;
            item.width.animate_to(on ? 1.0 : 0.0);
            item.alpha.animate_to(on ? 1.0 : 0.0);
        }
    }
}

std::vector<FocusStop*> FabMenu::focus_stops() {
    if (stops_.empty()) {
        stops_.push_back(std::make_unique<FocusStop>(forward_focus(button_, [] { return true; })));
        for (std::size_t i = 0; i < items_.size(); ++i) {
            stops_.push_back(std::make_unique<FocusStop>(FocusStop::Hooks{
                [this, i] { return items_[i].focus.focused(); },
                [this, i](bool f) {
                    items_[i].focus.set_focused(f);
                    retarget_state_layers(items_, focused_item());
                },
                [this, i] { return expanded() && items_[i].target; }, [this, i] { return item_rect(i); }}));
        }
    }
    std::vector<FocusStop*> out;
    for (auto& s : stops_) {
        out.push_back(s.get());
    }
    return out;
}

std::optional<std::size_t> FabMenu::focused_item() const {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].focus.focused()) {
            return i;
        }
    }
    return std::nullopt;
}

AccessibleNode FabMenu::accessible() const {
    AccessibleNode node{AccessibleRole::group, options_.accessible_name, bounds()};
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (item_shown(i)) {
            node.children.push_back({AccessibleRole::button, items_[i].spec.label, item_rect(i),
                                     {.focusable = items_[i].target, .focused = items_[i].focus.focused()}});
        }
    }
    node.children.push_back(button_.accessible());
    return node;
}

FabMenuResponse FabMenu::handle(const input::PointerEvent& e) {
    FabMenuResponse response;
    place_button();
    if (e.action == input::PointerAction::down) {
        for (Item& item : items_) {
            item.focus.set_focused(false);
        }
    }
    const std::optional<std::size_t> clicked =
        update_presses(items_, e, [this](std::size_t i) { return items_[i].target ? item_rect(i) : render::Rect{0.0, 0.0, 0.0, 0.0}; });
    if (clicked) {
        response.chosen = clicked;
        response.consumed = true;
        set_expanded(false);
        return response;
    }
    if (button_.handle(e)) {
        response.toggled = true;
        response.consumed = true;
        set_expanded(button_.checked());
    }
    return response;
}

FabMenuResponse FabMenu::handle(const input::KeyEvent& e) {
    FabMenuResponse response;
    const std::optional<std::size_t> focused = focused_item();
    if (expanded() && e.key == input::Key::escape) {
        set_expanded(false);
        button_.set_focused(true);
        response.consumed = true;
        return response;
    }
    if (focused) {
        if (activates(e, KeyActivation::space_or_enter)) {
            response.chosen = focused;
            response.consumed = true;
            set_expanded(false);
            button_.set_focused(true);
        }
        return response;
    }
    if (button_.focused() && expanded() && !e.shift && (e.key == input::Key::tab || e.key == input::Key::down)) {
        button_.set_focused(false);
        items_.front().focus.set_focused(true);
        retarget_state_layers(items_, focused_item());
        response.consumed = true;
        return response;
    }
    if (button_.handle(e)) {
        response.toggled = true;
        response.consumed = true;
        set_expanded(button_.checked());
    }
    return response;
}

void FabMenu::cancel_interaction() {
    button_.cancel_interaction();
    cancel_presses(items_);
}

void FabMenu::step(double dt) {
    button_.step(dt);
    stagger_.step(dt);
    retarget_items();
    for (Item& item : items_) {
        item.width.step(dt);
        item.alpha.step(dt);
        item.state_layer.step(dt);
        item.focus.step(dt);
    }
    place_button();
}

bool FabMenu::animating() const {
    const bool items = std::any_of(items_.begin(), items_.end(), [](const Item& item) {
        return !item.width.settled() || !item.alpha.settled() || !item.state_layer.settled() || !item.focus.settled();
    });
    return items || button_.animating() || !stagger_.settled();
}

void FabMenu::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const double alpha = std::clamp(item.alpha.value(), 0.0, 1.0);
        const render::Rect r = item_rect(i);
        if (alpha <= 0.0 || r.width <= 0.0) {
            continue;
        }
        const double radius = tokens::corner_radius(fm.item_shape, r.width, r.height);
        const color::Argb content = scheme[fm.item_content];
        const color::Argb fill = color::state_layer(scheme[fm.item_container], content, item.state_layer.value());
        const render::Path pill = render::rounded_rect_path(r, render::uniform_radii(radius));
        canvas.fill_path(pill, color::scale_alpha(fill, alpha));
        item.focus.paint(canvas, scheme, r, radius);
        canvas.save();
        canvas.push_clip(pill);
        const double content_x = options_.alignment == FabMenuAlignment::end ? r.x + r.width - item.full_width : r.x;
        const double icon_x = content_x + fm.item_leading_space;
        icons::draw_centered(canvas, item.spec.icon, render::Rect{icon_x, r.y, fm.item_icon_size, r.height}, fm.item_icon_size,
                             color::scale_alpha(content, alpha));
        text::draw_line(canvas, fonts_, item.label,
                        {icon_x + fm.item_icon_size + fm.item_icon_label_space, r.y + (r.height - item.label.box_height()) / 2.0},
                        color::scale_alpha(content, alpha));
        canvas.pop_clip();
        canvas.restore();
    }
    button_.paint(canvas, scheme);
}

}
