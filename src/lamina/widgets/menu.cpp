#include "lamina/style/style.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/scrim.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/item_focus.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

const tokens::MenuTokens& t() {
    return current_style().menu;
}

template <std::size_t N>
double pick(const std::array<double, N>& candidates, double size, double space, double margin) {
    for (std::size_t i = 0; i < N; ++i) {
        const double c = candidates[i];
        if (c >= margin && c + size <= space - margin) {
            return c;
        }
    }
    if (size >= space - 2.0 * margin) {
        return (space - size) / 2.0;
    }
    return std::clamp(candidates[N - 1], margin, space - margin - size);
}

double pivot_fraction(double anchor_start, double anchor_end, double menu_start, double menu_end) {
    if (menu_start >= anchor_end) {
        return 0.0;
    }
    if (menu_end <= anchor_start) {
        return 1.0;
    }
    if (menu_end - menu_start <= 0.0) {
        return 0.0;
    }
    const double center = (std::max(anchor_start, menu_start) + std::min(anchor_end, menu_end)) / 2.0;
    return (center - menu_start) / (menu_end - menu_start);
}

const input::PointerEvent pointer_leave{input::PointerAction::leave, 0.0, 0.0};

}

render::Point menu_position(const render::Rect& anchor, layout::Size window, layout::Size menu) {
    return menu_position(anchor, window, menu, t().window_horizontal_margin, t().window_vertical_margin);
}

render::Point menu_position(const render::Rect& anchor, layout::Size window, layout::Size menu, double horizontal_margin,
                            double vertical_margin) {
    const double anchor_right = anchor.x + anchor.width;
    const double anchor_bottom = anchor.y + anchor.height;
    const std::array<double, 3> xs{
        anchor.x,
        anchor_right - menu.width,
        anchor.x + anchor.width / 2.0 < window.width / 2.0 ? 0.0 : window.width - menu.width,
    };
    const std::array<double, 4> ys{
        anchor_bottom,
        anchor.y - menu.height,
        anchor.y - menu.height / 2.0,
        anchor.y + anchor.height / 2.0 < window.height / 2.0 ? 0.0 : window.height - menu.height,
    };
    return {pick(xs, menu.width, window.width, horizontal_margin), pick(ys, menu.height, window.height, vertical_margin)};
}

render::Point menu_transform_origin(const render::Rect& anchor, const render::Rect& menu) {
    const double fx = pivot_fraction(anchor.x, anchor.x + anchor.width, menu.x, menu.x + menu.width);
    const double fy = pivot_fraction(anchor.y, anchor.y + anchor.height, menu.y, menu.y + menu.height);
    return {menu.x + fx * menu.width, menu.y + fy * menu.height};
}

render::Point context_menu_position(render::Point at, layout::Size window, layout::Size menu) {
    return context_menu_position(at, window, menu, t().context_window_margin);
}

render::Point context_menu_position(render::Point at, layout::Size window, layout::Size menu, double margin) {
    double x = at.x;
    double y = at.y;
    if (x + menu.width > window.width - margin) {
        x -= menu.width;
    }
    if (y + menu.height > window.height - margin) {
        y -= menu.height;
    }
    return {std::max(x, margin), std::max(y, margin)};
}

render::Point submenu_position(const render::Rect& item, layout::Size window, layout::Size menu) {
    const double margin = t().context_window_margin;
    double x = item.x + item.width;
    if (x + menu.width > window.width - margin) {
        x = item.x - menu.width;
    }
    double y = item.y;
    if (y + menu.height > window.height - margin) {
        y = window.height - margin - menu.height;
    }
    return {std::max(x, margin), std::max(y, margin)};
}

Menu::Menu(MenuOptions options, const text::FontStack& fonts)
    : fonts_(fonts), accessible_name_(options.accessible_name), icon_font_(options.icon_font),
      popup_(t().closed_scale, current_style().motion.fast_spatial(), current_style().motion.fast_effects()) {
    density_ = options.density;
    const tokens::ShortcutFormat& format = options.shortcut_format ? *options.shortcut_format : tokens::default_shortcut_format();
    for (auto& item : options.items) {
        std::unique_ptr<Menu> child;
        if (!item.submenu.empty()) {
            if (!icon_font_) {
                throw std::invalid_argument("a menu with a submenu needs an icon font for the submenu arrow");
            }
            child = std::make_unique<Menu>(MenuOptions{std::move(item.submenu), item.label, density_, icon_font_, options.shortcut_format}, fonts_);
        }
        names_.push_back(item.label);
        commands_.push_back(item.command);
        shortcut_labels_.push_back(item.shortcut ? format_chord(*item.shortcut, format) : std::string{});
        access_keys_.push_back(access_key_for(item.label, item.access_key));
        children_.push_back(std::move(child));
        items_.push_back({std::nullopt, std::move(item.leading_icon), std::nullopt, std::nullopt, {},
                          motion::Animated(0.0, current_style().motion.fast_effects()), Enablement(item.enabled), item.divider_before});
    }
    layout_items();
}

void Menu::layout_items() {
    const auto style = current_style().type.text(t().label);
    double content = 0.0;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        Item& item = items_[i];
        double w = 2.0 * t().item_horizontal_padding;
        if (!names_[i].empty()) {
            item.label = text::layout_line(fonts_, names_[i], style);
            w += item.label->width;
            if (access_keys_[i]) {
                item.access = access_key_label(fonts_, names_[i], *item.label, *access_keys_[i]);
                w += item.access->suffix_width();
            }
        }
        if (item.icon) {
            w += t().icon_size + t().icon_text_space;
        }
        if (children_[i]) {
            w += t().trailing_space + t().trailing_icon_size;
        } else if (!shortcut_labels_[i].empty()) {
            item.shortcut = text::layout_line(fonts_, shortcut_labels_[i], style);
            w += t().trailing_space + std::max(t().trailing_icon_size, item.shortcut->width);
        }
        content = std::max(content, w);
    }
    width_ = std::clamp(content, t().item_min_width, t().item_max_width);
}

void Menu::restyle() {
    style_stamp_.refresh();
    popup_.restyle(t().closed_scale, current_style().motion.fast_spatial(), current_style().motion.fast_effects());
    focus_.restyle();
    access_keys_fade_.set_spec(current_style().motion.fast_effects());
    for (Item& item : items_) {
        item.state_layer.set_spec(current_style().motion.fast_effects());
        item.enabled.restyle();
    }
    for (auto& c : children_) {
        if (c) {
            c->restyle();
        }
    }
    layout_items();
    if (position_ && popup_.is_open()) {
        reanchor(position_, anchor_);
    }
}

void Menu::reanchor(std::function<render::Point(layout::Size)> position, const render::Rect& anchor) {
    position_ = std::move(position);
    anchor_ = anchor;
    origin_ = position_({width_, height()});
    pivot_ = menu_transform_origin(anchor_, bounds());
    reanchor_child();
}

void Menu::reanchor_child() {
    if (!open_child_) {
        return;
    }
    const render::Rect item = item_rect(*open_child_);
    children_[*open_child_]->reanchor([item, window = window_](layout::Size size) { return submenu_position(item, window, size); }, item);
}

void Menu::set_density(tokens::Density density) {
    density_ = density;
    for (auto& c : children_) {
        if (c) {
            c->set_density(density);
        }
    }
}

double Menu::item_height() const {
    return current_style().density.menu_item.at(density_);
}

double Menu::divider_height() const {
    return 2.0 * t().divider_vertical_padding + t().divider_thickness;
}

double Menu::item_top(std::size_t index) const {
    std::size_t dividers = 0;
    for (std::size_t i = 0; i <= index && i < items_.size(); ++i) {
        dividers += items_[i].divider_before ? 1 : 0;
    }
    return t().vertical_padding + item_height() * static_cast<double>(index) + divider_height() * static_cast<double>(dividers);
}

double Menu::height() const {
    std::size_t dividers = 0;
    for (const Item& item : items_) {
        dividers += item.divider_before ? 1 : 0;
    }
    return 2.0 * t().vertical_padding + item_height() * static_cast<double>(items_.size()) + divider_height() * static_cast<double>(dividers);
}

render::Rect Menu::bounds() const {
    return {origin_.x, origin_.y, width_, height()};
}

render::Rect Menu::item_rect(std::size_t index) const {
    return {origin_.x, origin_.y + item_top(index), width_, item_height()};
}

void Menu::place(std::function<render::Point(layout::Size)> position, const render::Rect& anchor, layout::Size window, input::InputSource source) {
    close_child();
    position_ = std::move(position);
    anchor_ = anchor;
    origin_ = position_({width_, height()});
    window_ = window;
    pivot_ = menu_transform_origin(anchor, bounds());
    popup_.open();
    hover_target_.reset();
    hover_timer_ = -1.0;
    access_keys_shown_ = source == input::InputSource::keyboard;
    access_keys_fade_.snap_to(access_keys_shown_ ? 1.0 : 0.0);
    set_active(source == input::InputSource::keyboard
                   ? first_enabled_item(items_.size(), [this](std::size_t i) { return item_enabled(i); })
                   : std::nullopt);
}

void Menu::reposition(const render::Rect& anchor, layout::Size window) {
    if (!popup_.is_open()) {
        return;
    }
    window_ = window;
    reanchor([anchor, window](layout::Size size) { return menu_position(anchor, window, size); }, anchor);
}

void Menu::reposition(const render::Rect& anchor, layout::Size window, double horizontal_margin, double vertical_margin) {
    if (!popup_.is_open()) {
        return;
    }
    window_ = window;
    reanchor([anchor, window, horizontal_margin, vertical_margin](layout::Size size) {
        return menu_position(anchor, window, size, horizontal_margin, vertical_margin);
    }, anchor);
}

void Menu::open(const render::Rect& anchor, layout::Size window, input::InputSource source) {
    place([anchor, window](layout::Size size) { return menu_position(anchor, window, size); }, anchor, window, source);
}

void Menu::open(const render::Rect& anchor, layout::Size window, input::InputSource source, double horizontal_margin, double vertical_margin) {
    place([anchor, window, horizontal_margin, vertical_margin](layout::Size size) {
        return menu_position(anchor, window, size, horizontal_margin, vertical_margin);
    }, anchor, window, source);
}

void Menu::open_at(render::Point at, layout::Size window, input::InputSource source) {
    place([at, window](layout::Size size) { return context_menu_position(at, window, size); }, {at.x, at.y, 0.0, 0.0}, window, source);
}

void Menu::open_child(std::size_t index, input::InputSource source) {
    if (open_child_ == index) {
        if (source == input::InputSource::keyboard) {
            Menu& c = *children_[index];
            c.reveal_access_keys();
            if (!c.active_) {
                c.set_active(first_enabled_item(c.items_.size(), [&c](std::size_t i) { return c.item_enabled(i); }));
            }
        }
        return;
    }
    close_child();
    Menu& c = *children_[index];
    const render::Rect item = item_rect(index);
    c.place([item, window = window_](layout::Size size) { return submenu_position(item, window, size); }, item, window_, source);
    open_child_ = index;
    retarget();
}

void Menu::close_child() {
    if (!open_child_) {
        return;
    }
    children_[*open_child_]->close();
    open_child_.reset();
    retarget();
}

void Menu::set_item_enabled(std::size_t index, bool enabled) {
    if (index >= items_.size() || !items_[index].enabled.set(enabled)) {
        return;
    }
    if (!enabled) {
        disable_item(items_, index);
        if (open_child_ == index) {
            close_child();
        }
        if (active_ == index) {
            set_active(std::nullopt);
        }
    }
    retarget();
}

bool Menu::contains_point(double x, double y) const {
    if (popup_.is_open() && contains(bounds(), x, y)) {
        return true;
    }
    return open_child_ && children_[*open_child_]->contains_point(x, y);
}

std::optional<std::size_t> Menu::item_at(double x, double y) const {
    if (!popup_.is_open() || !contains(bounds(), x, y)) {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (contains(item_rect(i), x, y)) {
            return i;
        }
    }
    return std::nullopt;
}

MenuResponse Menu::activate_at(double x, double y) {
    if (open_child_ && children_[*open_child_]->contains_point(x, y)) {
        return from_child(*open_child_, children_[*open_child_]->activate_at(x, y));
    }
    const std::optional<std::size_t> i = item_at(x, y);
    if (i && item_enabled(*i) && !has_submenu(*i)) {
        return chose(*i);
    }
    return {};
}

MenuResponse Menu::chose(std::size_t index) {
    MenuResponse r{true, index, {index}, commands_[index]};
    close();
    return r;
}

MenuResponse Menu::from_child(std::size_t index, MenuResponse child) {
    if (child.path.empty()) {
        return child;
    }
    child.path.insert(child.path.begin(), index);
    child.chosen.reset();
    child.consumed = true;
    close();
    return child;
}

std::optional<AccessibleNode> Menu::accessible() const {
    if (!popup_.is_open()) {
        return std::nullopt;
    }
    AccessibleNode menu{AccessibleRole::menu, accessible_name_, bounds()};
    for (std::size_t i = 0; i < items_.size(); ++i) {
        AccessibleNode item{AccessibleRole::menu_item, names_[i], item_rect(i), {.enabled = item_enabled(i), .focused = active_ == i}};
        item.description = shortcut_labels_[i];
        if (children_[i] && open_child_ == i) {
            if (auto c = children_[i]->accessible()) {
                item.children.push_back(std::move(*c));
            }
        }
        menu.children.push_back(std::move(item));
    }
    return menu;
}

std::vector<std::optional<input::Key>> Menu::enabled_access_keys() const {
    std::vector<std::optional<input::Key>> keys = access_keys_;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (!item_enabled(i)) {
            keys[i].reset();
        }
    }
    return keys;
}

void Menu::close() {
    close_child();
    popup_.close();
    hover_target_.reset();
    hover_timer_ = -1.0;
    set_active(std::nullopt);
    cancel_interaction();
}

void Menu::set_active(std::optional<std::size_t> index) {
    active_ = index;
    focus_.set_focused(index.has_value());
    retarget();
}

void Menu::retarget() {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        double target = items_[i].press.state_layer_opacity();
        if (open_child_ == i) {
            target = std::max(target, current_style().state_layer.hover);
        }
        if (active_ == i) {
            target = std::max(target, current_style().state_layer.focus);
        }
        items_[i].state_layer.animate_to(target);
    }
}

void Menu::cancel_interaction() {
    cancel_presses(items_);
    retarget();
}

void Menu::hover(std::optional<std::size_t> index) {
    if (index == hover_target_) {
        return;
    }
    hover_target_ = index;
    if (!index || index == open_child_) {
        hover_timer_ = -1.0;
        return;
    }
    if (has_submenu(*index)) {
        hover_timer_ = t().submenu_open_seconds;
    } else {
        hover_timer_ = open_child_ ? t().submenu_close_seconds : -1.0;
    }
}

MenuResponse Menu::handle(const input::PointerEvent& e) {
    MenuResponse response;
    if (!popup_.is_open()) {
        return response;
    }
    if (open_child_) {
        Menu& c = *children_[*open_child_];
        if (e.action != input::PointerAction::leave && c.contains_point(e.x_dp, e.y_dp)) {
            hover_target_.reset();
            hover_timer_ = -1.0;
            update_presses(items_, pointer_leave, [](std::size_t) { return render::Rect{}; });
            MenuResponse r = c.handle(e);
            if (!r.path.empty()) {
                return from_child(*open_child_, std::move(r));
            }
            retarget();
            r.consumed = true;
            return r;
        }
        c.handle(pointer_leave);
    }
    if (e.action == input::PointerAction::down && !contains_point(e.x_dp, e.y_dp)) {
        close();
        response.consumed = true;
        return response;
    }
    if (e.action == input::PointerAction::down) {
        set_active(std::nullopt);
    }
    response.consumed = e.action != input::PointerAction::leave;
    const std::optional<std::size_t> clicked =
        update_presses(items_, e, [this](std::size_t i) { return item_enabled(i) ? item_rect(i) : render::Rect{}; });
    if (e.action == input::PointerAction::move || e.action == input::PointerAction::down) {
        const std::optional<std::size_t> over = item_at(e.x_dp, e.y_dp);
        const bool usable = over && item_enabled(*over);
        hover(usable ? over : std::nullopt);
        if (e.action == input::PointerAction::down && usable && has_submenu(*over)) {
            open_child(*over, input::InputSource::pointer);
        }
    }
    retarget();
    if (clicked) {
        if (has_submenu(*clicked)) {
            open_child(*clicked, input::InputSource::pointer);
        } else {
            return chose(*clicked);
        }
    }
    return response;
}

MenuResponse Menu::handle(const input::KeyEvent& e) {
    MenuResponse response;
    if (!popup_.is_open()) {
        return response;
    }
    if (e.key == input::Key::tab) {
        close();
        response.consumed = true;
        return response;
    }
    if (open_child_ && children_[*open_child_]->active_) {
        const std::size_t index = *open_child_;
        Menu& c = *children_[index];
        MenuResponse r = c.handle(e);
        if (!r.path.empty()) {
            return from_child(index, std::move(r));
        }
        if (!c.is_open()) {
            open_child_.reset();
            set_active(index);
            r.consumed = true;
            return r;
        }
        if (r.consumed) {
            return r;
        }
        if (e.key == input::Key::left && !e.ctrl && !e.alt) {
            close_child();
            set_active(index);
            response.consumed = true;
        }
        return response;
    }
    if (closes_menu(e)) {
        close();
        response.consumed = true;
        return response;
    }
    if (activates(e, KeyActivation::space_or_enter)) {
        response.consumed = true;
        if (active_ && has_submenu(*active_)) {
            open_child(*active_, input::InputSource::keyboard);
        } else if (active_) {
            return chose(*active_);
        }
        return response;
    }
    if (e.key == input::Key::right && !e.ctrl && !e.alt && active_ && has_submenu(*active_)) {
        open_child(*active_, input::InputSource::keyboard);
        response.consumed = true;
        return response;
    }
    if (e.key == input::Key::left && !e.ctrl && !e.alt && open_child_) {
        const std::size_t index = *open_child_;
        close_child();
        set_active(index);
        response.consumed = true;
        return response;
    }
    if (const std::optional<std::size_t> target =
            menu_key_target(e, active_, items_.size(), [this](std::size_t i) { return item_enabled(i); })) {
        reveal_access_keys();
        if (open_child_ && open_child_ != target) {
            close_child();
        }
        set_active(target);
        response.consumed = true;
        return response;
    }
    if (const std::optional<AccessKeyMatch> match = match_access_key(e, enabled_access_keys(), active_)) {
        response.consumed = true;
        reveal_access_keys();
        if (match->unique && e.repeat) {
            return response;
        }
        set_active(match->index);
        if (match->unique) {
            if (has_submenu(match->index)) {
                open_child(match->index, input::InputSource::keyboard);
            } else {
                return chose(match->index);
            }
        }
    }
    return response;
}

void Menu::reveal_access_keys() {
    access_keys_shown_ = true;
    access_keys_fade_.animate_to(1.0);
}

void Menu::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    popup_.step(dt);
    focus_.step(dt);
    access_keys_fade_.step(dt);
    for (Item& item : items_) {
        item.state_layer.step(dt);
        item.enabled.step(dt);
    }
    if (hover_timer_ >= 0.0) {
        hover_timer_ -= dt;
        if (hover_timer_ <= 0.0) {
            hover_timer_ = -1.0;
            if (open_child_ != hover_target_) {
                close_child();
            }
            if (hover_target_ && has_submenu(*hover_target_) && popup_.is_open()) {
                open_child(*hover_target_, input::InputSource::pointer);
            }
        }
    }
    for (auto& c : children_) {
        if (c) {
            c->step(dt);
        }
    }
}

bool Menu::animating() const {
    if (!popup_.settled() || !focus_.settled() || !access_keys_fade_.settled() || hover_timer_ >= 0.0) {
        return true;
    }
    for (const auto& c : children_) {
        if (c && c->animating()) {
            return true;
        }
    }
    return motion::any_unsettled(items_, &Item::state_layer, &Item::enabled);
}

void Menu::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    const double a = opacity();
    const render::Rect r = bounds();
    const double radius = current_style().shape.radius(t().corner, r.width, r.height);
    paint_receded_backdrop(canvas, {0.0, 0.0, window_.width, window_.height}, scheme, a);
    canvas.save();
    popup_.scale_about(canvas, pivot_);
    const color::ColorScheme s = color::faded(scheme, a);
    paint_container(canvas, r, render::uniform_radii(radius), s, t().container, current_style().elevation.levels[t().elevation], SurfaceLayer::overlay);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const color::Argb label_color = item.enabled.mix(resolve(s, t().disabled_label), s[t().label_color]);
        const color::Argb icon_color = item.enabled.mix(resolve(s, t().disabled_leading_icon), s[t().leading_icon]);
        const render::Rect ir = item_rect(i);
        if (item.divider_before) {
            canvas.fill_rect({r.x + t().divider_horizontal_padding, ir.y - divider_height() + t().divider_vertical_padding,
                              r.width - 2.0 * t().divider_horizontal_padding, t().divider_thickness},
                             s[t().divider_color]);
        }
        if (item.state_layer.value() > 0.0) {
            canvas.fill_rect(ir, color::scale_alpha(color::with_alpha(scheme[t().state_layer], item.state_layer.value()), a));
        }
        double x = ir.x + t().item_horizontal_padding;
        const double cy = ir.y + ir.height / 2.0;
        if (item.icon) {
            item.icon->font.draw(canvas, item.icon->symbol, {x, cy - t().icon_size / 2.0}, t().icon_size, icon_color);
            x += t().icon_size + t().icon_text_space;
        }
        if (item.label) {
            const render::Point top_left{x, cy - item.label->box_height() / 2.0};
            text::draw_line(canvas, fonts_, *item.label, top_left, label_color);
            if (item.access && access_keys_fade_.value() > 0.0) {
                paint_access_key(canvas, fonts_, *item.access, *item.label, top_left,
                                 color::scale_alpha(label_color, std::clamp(access_keys_fade_.value(), 0.0, 1.0)));
            }
        }
        const double end = ir.x + ir.width - t().item_horizontal_padding;
        const color::Argb trailing_color = item.enabled.mix(resolve(s, t().disabled_trailing), s[t().trailing]);
        if (children_[i]) {
            icon_font_->draw(canvas, t().submenu_symbol, {end - t().trailing_icon_size, cy - t().trailing_icon_size / 2.0}, t().trailing_icon_size,
                             trailing_color);
        } else if (item.shortcut) {
            text::draw_line(canvas, fonts_, *item.shortcut, {end - item.shortcut->width, cy - item.shortcut->box_height() / 2.0}, trailing_color);
        }
    }
    if (active_ && !(open_child_ && children_[*open_child_]->active_)) {
        focus_.paint_inward(canvas, s, item_rect(*active_), t().focus_ring_corner);
    }
    canvas.restore();
    for (const auto& c : children_) {
        if (c) {
            c->paint(canvas, scheme);
        }
    }
}

}
