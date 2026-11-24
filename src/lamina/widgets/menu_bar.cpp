#include "lamina/style/style.hpp"
#include "lamina/widgets/menu_bar.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

namespace {

const tokens::MenuBarTokens& t() {
    return current_style().menu_bar;
}
using tokens::motion_scheme;

}

void apply_shortcuts(std::vector<MenuItem>& items, const ShortcutRegistry& commands) {
    for (MenuItem& item : items) {
        if (!item.command.empty() && !item.shortcut) {
            item.shortcut = commands.chord(item.command);
        }
        apply_shortcuts(item.submenu, commands);
    }
}

void sync_commands(Menu& menu, const ShortcutRegistry& commands) {
    for (std::size_t i = 0; i < menu.count(); ++i) {
        if (!menu.command(i).empty() && commands.contains(menu.command(i))) {
            menu.set_item_enabled(i, commands.enabled(menu.command(i)));
        }
        if (Menu* sub = menu.submenu(i)) {
            sync_commands(*sub, commands);
        }
    }
}

MenuBar::MenuBar(MenuBarOptions options, const text::FontStack& fonts)
    : fonts_(fonts), accessible_name_(required_name(options.accessible_name, "a menu bar")), commands_(options.commands),
      density_(options.density) {
    const auto style = tokens::text_style(tokens::type_style(t().label));
    double x = t().start_padding;
    for (MenuBarMenu& spec : options.menus) {
        text::TextLine label = text::layout_line(fonts_, spec.label, style);
        const std::optional<input::Key> key = access_key_for(spec.label, spec.access_key);
        std::optional<AccessKeyLabel> access;
        double w = 2.0 * t().item_horizontal_padding + label.width;
        if (key) {
            access = access_key_label(fonts_, spec.label, label, *key);
            w += access->suffix_width();
        }
        if (commands_) {
            apply_shortcuts(spec.items, *commands_);
        }
        menus_.push_back(std::make_unique<Menu>(
            MenuOptions{std::move(spec.items), spec.label, density_, options.icon_font, options.shortcut_format}, fonts_));
        names_.push_back(spec.label);
        access_keys_.push_back(key);
        items_.push_back({std::move(label), std::move(access), x, w, {}, motion::Animated(0.0, motion_scheme.fast_effects()),
                          motion::Animated(0.0, motion_scheme.default_effects())});
        x += w;
    }
    width_ = x + t().start_padding;
}

double MenuBar::height() const {
    return tokens::density_tokens.menu_bar.at(density_);
}

render::Rect MenuBar::item_rect(std::size_t index) const {
    const Item& item = items_[index];
    return {origin_.x + item.x, origin_.y + t().item_vertical_inset, item.width, height() - 2.0 * t().item_vertical_inset};
}

std::optional<std::size_t> MenuBar::item_at(double x, double y) const {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (contains(item_rect(i), x, y)) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> MenuBar::access_match(const input::KeyEvent& e) const {
    input::KeyEvent plain = e;
    plain.alt = false;
    const std::optional<AccessKeyMatch> m = match_access_key(plain, access_keys_, std::nullopt);
    return m ? std::optional<std::size_t>(m->index) : std::nullopt;
}

void MenuBar::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        cancel_interaction();
    }
}

void MenuBar::set_density(tokens::Density density) {
    density_ = density;
    for (auto& m : menus_) {
        m->set_density(density);
    }
}

void MenuBar::refresh_commands() {
    if (!commands_) {
        return;
    }
    for (auto& m : menus_) {
        sync_commands(*m, *commands_);
    }
}

void MenuBar::reveal_access_keys(bool shown) {
    access_keys_fade_.animate_to(shown ? 1.0 : 0.0);
}

void MenuBar::retarget() {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        double target = items_[i].press.state_layer_opacity();
        if (active_ && !open_ && highlight_ == i) {
            target = std::max(target, tokens::state_layer_opacity::focus);
        }
        items_[i].state_layer.animate_to(target);
        items_[i].open.animate_to(open_ == i ? 1.0 : 0.0);
    }
    if (highlight_) {
        ring_ = *highlight_;
    }
    focus_.set_focused(active_ && !open_ && highlight_.has_value());
}

void MenuBar::open(std::size_t index, input::InputSource source) {
    if (open_ && *open_ != index) {
        menus_[*open_]->close();
    }
    if (commands_) {
        sync_commands(*menus_[index], *commands_);
    }
    menus_[index]->open(item_rect(index), window_, source, t().window_margin, t().window_margin);
    open_ = index;
    highlight_ = index;
    if (source == input::InputSource::keyboard) {
        active_ = true;
        reveal_access_keys(true);
    }
    retarget();
}

void MenuBar::close_menu() {
    if (open_) {
        menus_[*open_]->close();
        open_.reset();
    }
    retarget();
}

void MenuBar::deactivate() {
    active_ = false;
    highlight_.reset();
    drag_open_ = false;
    reveal_access_keys(false);
    retarget();
}

void MenuBar::cancel_interaction() {
    cancel_presses(items_);
    close_menu();
    deactivate();
}

MenuBarResponse MenuBar::finish(std::size_t index, MenuResponse r) {
    open_.reset();
    deactivate();
    MenuBarResponse out{true, index, std::move(r.path), std::move(r.command)};
    if (commands_ && !out.command.empty() && commands_->contains(out.command)) {
        commands_->run(out.command);
    }
    return out;
}

MenuBarResponse MenuBar::handle(const input::PointerEvent& e) {
    MenuBarResponse response;
    if (!enabled()) {
        return response;
    }
    const std::optional<std::size_t> over = e.action == input::PointerAction::leave ? std::nullopt : item_at(e.x_dp, e.y_dp);
    if (open_) {
        Menu& m = *menus_[*open_];
        if (over && !m.contains_point(e.x_dp, e.y_dp)) {
            update_presses(items_, e, [this](std::size_t i) { return item_rect(i); });
            if (e.action == input::PointerAction::down) {
                if (*over == *open_) {
                    close_menu();
                    deactivate();
                } else {
                    open(*over, input::InputSource::pointer);
                    drag_open_ = true;
                }
            } else if (e.action == input::PointerAction::move && *over != *open_) {
                open(*over, input::InputSource::pointer);
            } else if (e.action == input::PointerAction::up) {
                drag_open_ = false;
            }
            retarget();
            response.consumed = true;
            return response;
        }
        update_presses(items_, input::PointerEvent{input::PointerAction::leave, 0.0, 0.0}, [this](std::size_t i) { return item_rect(i); });
        if (e.action == input::PointerAction::up && drag_open_) {
            drag_open_ = false;
            MenuResponse r = m.activate_at(e.x_dp, e.y_dp);
            if (!r.path.empty()) {
                return finish(*open_, std::move(r));
            }
        }
        MenuResponse r = m.handle(e);
        if (!r.path.empty()) {
            return finish(*open_, std::move(r));
        }
        if (!m.is_open()) {
            open_.reset();
            deactivate();
        }
        retarget();
        response.consumed = r.consumed || e.action != input::PointerAction::leave;
        return response;
    }
    update_presses(items_, e, [this](std::size_t i) { return item_rect(i); });
    if (e.action == input::PointerAction::down) {
        if (over) {
            open(*over, input::InputSource::pointer);
            drag_open_ = true;
        } else if (active_) {
            deactivate();
        }
    }
    retarget();
    response.consumed = over.has_value();
    return response;
}

MenuBarResponse MenuBar::handle(const input::KeyEvent& e) {
    MenuBarResponse response;
    if (!enabled() || items_.empty()) {
        return response;
    }
    const bool plain = !e.ctrl && !e.alt && !e.shift;
    const bool toggle = (e.key == input::Key::alt && !e.ctrl && !e.shift) || (e.key == input::Key::f10 && plain);
    response.consumed = true;
    const std::size_t n = items_.size();
    if (open_) {
        const std::size_t index = *open_;
        if (toggle) {
            close_menu();
            deactivate();
            return response;
        }
        if (e.alt && !e.ctrl) {
            if (const std::optional<std::size_t> j = access_match(e)) {
                open(*j, input::InputSource::keyboard);
                return response;
            }
        }
        Menu& m = *menus_[index];
        MenuResponse r = m.handle(e);
        if (!r.path.empty()) {
            return finish(index, std::move(r));
        }
        if (!m.is_open()) {
            open_.reset();
            if (e.key == input::Key::escape) {
                active_ = true;
                highlight_ = index;
                reveal_access_keys(true);
                retarget();
            } else {
                deactivate();
            }
            return response;
        }
        if (r.consumed) {
            return response;
        }
        if ((e.key == input::Key::left || e.key == input::Key::right) && !e.ctrl && !e.alt) {
            open(e.key == input::Key::right ? (index + 1) % n : (index + n - 1) % n, input::InputSource::keyboard);
        }
        return response;
    }
    if (active_) {
        const std::size_t h = highlight_.value_or(0);
        if (toggle || e.key == input::Key::escape || e.key == input::Key::tab) {
            deactivate();
            return response;
        }
        switch (e.key) {
        case input::Key::left: highlight_ = (h + n - 1) % n; break;
        case input::Key::right: highlight_ = (h + 1) % n; break;
        case input::Key::home: highlight_ = 0; break;
        case input::Key::end: highlight_ = n - 1; break;
        case input::Key::down:
        case input::Key::up:
        case input::Key::enter:
        case input::Key::space: open(h, input::InputSource::keyboard); return response;
        default:
            if (!e.ctrl) {
                if (const std::optional<std::size_t> j = access_match(e)) {
                    open(*j, input::InputSource::keyboard);
                }
            }
            break;
        }
        retarget();
        return response;
    }
    if (toggle) {
        active_ = true;
        highlight_ = 0;
        reveal_access_keys(true);
        retarget();
        return response;
    }
    if (e.alt && !e.ctrl) {
        if (const std::optional<std::size_t> j = access_match(e)) {
            open(*j, input::InputSource::keyboard);
            return response;
        }
    }
    response.consumed = false;
    return response;
}

void MenuBar::step(double dt) {
    for (Item& item : items_) {
        item.state_layer.step(dt);
        item.open.step(dt);
    }
    for (auto& m : menus_) {
        m->step(dt);
    }
    access_keys_fade_.step(dt);
    focus_.step(dt);
    enabled_.step(dt);
}

bool MenuBar::animating() const {
    if (!access_keys_fade_.settled() || !focus_.settled() || !enabled_.settled()) {
        return true;
    }
    for (const auto& m : menus_) {
        if (m->animating()) {
            return true;
        }
    }
    return motion::any_unsettled(items_, &Item::state_layer, &Item::open);
}

AccessibleNode MenuBar::accessible() const {
    AccessibleNode bar{AccessibleRole::menu_bar, accessible_name_, bounds(), {.enabled = enabled()}};
    for (std::size_t i = 0; i < items_.size(); ++i) {
        AccessibleNode item{AccessibleRole::menu_item, names_[i], item_rect(i),
                            {.enabled = enabled(), .focused = active_ && !open_ && highlight_ == i}};
        item.value = open_ == i ? "Expanded" : "Collapsed";
        if (open_ == i) {
            if (auto m = menus_[i]->accessible()) {
                item.children.push_back(std::move(*m));
            }
        }
        bar.children.push_back(std::move(item));
    }
    return bar;
}

void MenuBar::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    canvas.fill_rect(bounds(), scheme[t().container]);
    const double fade = std::clamp(access_keys_fade_.value(), 0.0, 1.0);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const render::Rect r = item_rect(i);
        const render::CornerRadii radii = render::uniform_radii(tokens::corner_radius(t().item_corner, r.width, r.height));
        const double open = std::clamp(item.open.value(), 0.0, 1.0);
        if (open > 0.0) {
            canvas.fill_path(render::rounded_rect_path(r, radii), color::with_alpha(scheme[t().open_container], open));
        }
        if (item.state_layer.value() > 0.0) {
            canvas.fill_path(render::rounded_rect_path(r, radii), color::with_alpha(scheme[t().state_layer], item.state_layer.value()));
        }
        const color::Argb label = enabled_.mix(resolve(scheme, t().disabled_label), color::mix(scheme[t().label_color], scheme[t().open_label], open));
        const render::Point top_left{r.x + t().item_horizontal_padding, r.y + (r.height - item.label.box_height()) / 2.0};
        text::draw_line(canvas, fonts_, item.label, top_left, label);
        if (item.access && fade > 0.0) {
            paint_access_key(canvas, fonts_, *item.access, item.label, top_left, color::scale_alpha(label, fade));
        }
        if (ring_ == i) {
            focus_.paint_inward(canvas, scheme, r, radii);
        }
    }
}

void MenuBar::paint_menus(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    for (const auto& m : menus_) {
        m->paint(canvas, scheme);
    }
}

}
