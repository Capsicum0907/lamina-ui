#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/grouped_menu.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/item_focus.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::GroupedMenuTokens& t = tokens::grouped_menu_tokens;
using tokens::motion_scheme;

motion::AnimatedRadii corner_springs() {
    return motion::AnimatedRadii(render::uniform_radii(0.0), motion_scheme.fast_spatial());
}

}

tokens::Corners menu_group_corners(std::size_t index, std::size_t count) {
    if (count <= 1) {
        return t.standalone_group;
    }
    if (index == 0) {
        return t.leading_group;
    }
    return index + 1 == count ? t.trailing_group : t.middle_group;
}

tokens::Corners menu_item_corners(std::size_t index, std::size_t count) {
    if (count <= 1) {
        return t.standalone_item;
    }
    if (index == 0) {
        return t.leading_item;
    }
    return index + 1 == count ? t.trailing_item : t.middle_item;
}

GroupedMenu::GroupedMenu(GroupedMenuOptions options, const text::FontStack& fonts)
    : fonts_(fonts), popup_(t.closed_scale, motion_scheme.fast_spatial(), motion_scheme.fast_effects()) {
    const auto item_style = tokens::text_style(tokens::type_style(t.label));
    const auto group_label_style = tokens::text_style(tokens::type_style(t.group_label));
    double content = 0.0;
    double y = 0.0;
    for (std::size_t g = 0; g < options.groups.size(); ++g) {
        MenuGroup& spec = options.groups[g];
        Group group{std::nullopt, 0.0, 0.0, std::nullopt, spec.selection, {}, y, 0.0, false, false, corner_springs()};
        double gy = t.group_vertical_padding;
        if (!spec.label.empty()) {
            group.label = text::layout_line(fonts_, spec.label, group_label_style);
            group.label_y = gy;
            group.label_height = std::max(t.group_label_min_height, group.label->style.line_height);
            gy += group.label_height;
            content = std::max(content, std::clamp(t.group_label_start_padding + group.label->width + t.group_label_end_padding,
                                                   t.item_min_width, t.item_max_width));
        }
        if (spec.divider) {
            group.divider_y = gy + t.divider_vertical_padding;
            gy += 2.0 * t.divider_vertical_padding + t.divider_thickness;
        }
        for (GroupedMenuItem& item : spec.items) {
            std::optional<text::TextLine> label;
            std::optional<AccessKeyLabel> access;
            const std::optional<input::Key> key = access_key_for(item.label, item.access_key);
            double w = 2.0 * t.item_horizontal_padding;
            double h = 0.0;
            if (!item.label.empty()) {
                label = text::layout_line(fonts_, item.label, item_style);
                w += label->width;
                h = label->style.line_height;
                if (key) {
                    access = access_key_label(fonts_, item.label, *label, *key);
                    w += access->suffix_width();
                }
            }
            if (item.leading_icon) {
                w += t.leading_icon_size + t.icon_text_space;
                h = std::max(h, t.leading_icon_size);
            }
            if (item.trailing_icon) {
                w += t.icon_text_space + t.trailing_icon_size;
                h = std::max(h, t.trailing_icon_size);
            }
            content = std::max(content, std::clamp(w, t.item_min_width, t.item_max_width) + 2.0 * t.item_horizontal_inset);
            const double height = std::max(t.item_min_height, h + 2.0 * t.item_vertical_padding);
            const double on = item.selected ? 1.0 : 0.0;
            access_keys_.push_back(key);
            group.items.push_back({std::move(label), std::move(item.leading_icon), std::move(item.selected_leading_icon),
                                   std::move(item.trailing_icon), std::move(access), item.selected, gy, height, {},
                                   motion::Animated(0.0, motion_scheme.fast_effects()), motion::Animated(on, motion_scheme.fast_effects()),
                                   corner_springs()});
            gy += height;
        }
        group.height = gy + t.group_vertical_padding;
        y += group.height + t.group_gap;
        groups_.push_back(std::move(group));
    }
    width_ = content;
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        groups_[g].corners.snap_to(group_target(g));
        for (std::size_t i = 0; i < groups_[g].items.size(); ++i) {
            groups_[g].items[i].corners.snap_to(item_target(g, i));
        }
    }
}

double GroupedMenu::height() const {
    if (groups_.empty()) {
        return 0.0;
    }
    return groups_.back().y + groups_.back().height;
}

render::Rect GroupedMenu::bounds() const {
    return {origin_.x, origin_.y, width_, height()};
}

render::Rect GroupedMenu::group_rect(std::size_t group) const {
    const Group& g = groups_[group];
    return {origin_.x, origin_.y + g.y, width_, g.height};
}

render::Rect GroupedMenu::item_rect(std::size_t group, std::size_t item) const {
    const Group& g = groups_[group];
    const Item& i = g.items[item];
    return {origin_.x + t.item_horizontal_inset, origin_.y + g.y + i.y, width_ - 2.0 * t.item_horizontal_inset, i.height};
}

render::CornerRadii GroupedMenu::group_radii(std::size_t group) const {
    return groups_[group].corners.value();
}

render::CornerRadii GroupedMenu::item_radii(std::size_t group, std::size_t item) const {
    return groups_[group].items[item].corners.value();
}

render::CornerRadii GroupedMenu::group_target(std::size_t group) const {
    const Group& g = groups_[group];
    const render::Rect r = group_rect(group);
    if (g.has_been_hovered && !g.hovered) {
        return tokens::corner_radii(tokens::all_corners(t.inactive_group), r.width, r.height);
    }
    return tokens::corner_radii(menu_group_corners(group, groups_.size()), r.width, r.height);
}

render::CornerRadii GroupedMenu::item_target(std::size_t group, std::size_t item) const {
    const Group& g = groups_[group];
    const render::Rect r = item_rect(group, item);
    if (g.items[item].selected) {
        return tokens::corner_radii(tokens::all_corners(t.selected_item), r.width, r.height);
    }
    return tokens::corner_radii(menu_item_corners(item, g.items.size()), r.width, r.height);
}

void GroupedMenu::retarget_group(std::size_t group) {
    groups_[group].corners.animate_to(group_target(group));
}

void GroupedMenu::retarget_item(std::size_t group, std::size_t item) {
    Item& i = groups_[group].items[item];
    i.corners.animate_to(item_target(group, item));
    i.selection.animate_to(i.selected ? 1.0 : 0.0);
}

void GroupedMenu::set_selected(std::size_t group, std::size_t item, bool selected) {
    groups_[group].items[item].selected = selected;
    retarget_item(group, item);
}

void GroupedMenu::choose(std::size_t group, std::size_t item) {
    Group& g = groups_[group];
    switch (g.selection) {
    case MenuSelection::none:
        close();
        break;
    case MenuSelection::single:
        for (std::size_t i = 0; i < g.items.size(); ++i) {
            set_selected(group, i, i == item);
        }
        break;
    case MenuSelection::multiple:
        set_selected(group, item, !g.items[item].selected);
        break;
    }
}

void GroupedMenu::open(const render::Rect& anchor, layout::Size window, input::InputSource source) {
    origin_ = menu_position(anchor, window, {width_, height()}, t.window_horizontal_margin, t.window_vertical_margin);
    pivot_ = menu_transform_origin(anchor, bounds());
    if (popup_.open()) {
        for (std::size_t g = 0; g < groups_.size(); ++g) {
            groups_[g].hovered = false;
            groups_[g].has_been_hovered = false;
            groups_[g].corners.snap_to(group_target(g));
            for (std::size_t i = 0; i < groups_[g].items.size(); ++i) {
                Item& item = groups_[g].items[i];
                item.corners.snap_to(item_target(g, i));
                item.selection.snap_to(item.selected ? 1.0 : 0.0);
            }
        }
    }
    access_keys_shown_ = source == input::InputSource::keyboard;
    access_keys_fade_.snap_to(access_keys_shown_ ? 1.0 : 0.0);
    set_active(source == input::InputSource::keyboard && flat_count() > 0 ? std::optional<std::size_t>(0) : std::nullopt);
}

void GroupedMenu::close() {
    popup_.close();
    set_active(std::nullopt);
    cancel_interaction();
}

void GroupedMenu::cancel_interaction() {
    for (Group& g : groups_) {
        cancel_presses(g.items);
    }
    retarget_state_layers();
}

std::size_t GroupedMenu::flat_count() const {
    std::size_t n = 0;
    for (const Group& g : groups_) {
        n += g.items.size();
    }
    return n;
}

MenuItemIndex GroupedMenu::item_at(std::size_t flat) const {
    std::size_t g = 0;
    while (flat >= groups_[g].items.size()) {
        flat -= groups_[g].items.size();
        ++g;
    }
    return {g, flat};
}

std::optional<input::Key> GroupedMenu::access_key(std::size_t group, std::size_t item) const {
    std::size_t flat = item;
    for (std::size_t g = 0; g < group; ++g) {
        flat += groups_[g].items.size();
    }
    return access_keys_[flat];
}

std::optional<MenuItemIndex> GroupedMenu::focused_item() const {
    return active_ ? std::optional<MenuItemIndex>(item_at(*active_)) : std::nullopt;
}

void GroupedMenu::set_active(std::optional<std::size_t> flat) {
    active_ = flat;
    focus_.set_focused(flat.has_value());
    retarget_state_layers();
}

void GroupedMenu::retarget_state_layers() {
    const std::optional<MenuItemIndex> focused = focused_item();
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        widgets::retarget_state_layers(groups_[g].items, focused && focused->group == g ? std::optional<std::size_t>(focused->item) : std::nullopt);
    }
}

GroupedMenuResponse GroupedMenu::handle(const input::PointerEvent& e) {
    GroupedMenuResponse response;
    if (!popup_.is_open()) {
        return response;
    }
    if (popup_.dismisses(e, bounds())) {
        close();
        response.consumed = true;
        return response;
    }
    if (e.action == input::PointerAction::down) {
        set_active(std::nullopt);
    }
    response.consumed = e.action != input::PointerAction::leave;
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        Group& group = groups_[g];
        const bool hovered = e.action != input::PointerAction::leave && contains(group_rect(g), e.x_dp, e.y_dp);
        if (hovered != group.hovered) {
            group.hovered = hovered;
            group.has_been_hovered = group.has_been_hovered || hovered;
            retarget_group(g);
        }
        for (std::size_t i = 0; i < group.items.size(); ++i) {
            Item& item = group.items[i];
            if (item.press.update(e, item_rect(g, i))) {
                response.chosen = MenuItemIndex{g, i};
            }
        }
    }
    retarget_state_layers();
    if (response.chosen) {
        choose(response.chosen->group, response.chosen->item);
    }
    return response;
}

GroupedMenuResponse GroupedMenu::handle(const input::KeyEvent& e) {
    GroupedMenuResponse response;
    if (!popup_.is_open()) {
        return response;
    }
    if (closes_menu(e)) {
        close();
        response.consumed = true;
        return response;
    }
    if (activates(e, KeyActivation::space_or_enter)) {
        response.consumed = true;
        if (active_) {
            const MenuItemIndex chosen = item_at(*active_);
            response.chosen = chosen;
            choose(chosen.group, chosen.item);
        }
        return response;
    }
    if (const std::optional<std::size_t> target = menu_key_target(e, active_, flat_count())) {
        reveal_access_keys();
        set_active(target);
        response.consumed = true;
        return response;
    }
    if (const std::optional<AccessKeyMatch> match = match_access_key(e, access_keys_, active_)) {
        response.consumed = true;
        reveal_access_keys();
        if (match->unique && e.repeat) {
            return response;
        }
        set_active(match->index);
        if (match->unique) {
            const MenuItemIndex chosen = item_at(match->index);
            response.chosen = chosen;
            choose(chosen.group, chosen.item);
        }
    }
    return response;
}

void GroupedMenu::reveal_access_keys() {
    access_keys_shown_ = true;
    access_keys_fade_.animate_to(1.0);
}

void GroupedMenu::step(double dt) {
    popup_.step(dt);
    focus_.step(dt);
    access_keys_fade_.step(dt);
    for (Group& g : groups_) {
        g.corners.step(dt);
        for (Item& item : g.items) {
            item.state_layer.step(dt);
            item.selection.step(dt);
            item.corners.step(dt);
        }
    }
}

bool GroupedMenu::animating() const {
    if (!popup_.settled() || !focus_.settled() || !access_keys_fade_.settled()) {
        return true;
    }
    return std::any_of(groups_.begin(), groups_.end(), [](const Group& g) {
        return !g.corners.settled() || std::any_of(g.items.begin(), g.items.end(), [](const Item& item) {
                   return !item.state_layer.settled() || !item.selection.settled() || !item.corners.settled();
               });
    });
}

void GroupedMenu::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    const double a = opacity();
    canvas.save();
    popup_.scale_about(canvas, pivot_);
    const color::ColorScheme s = color::faded(scheme, a);
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        const render::Rect gr = group_rect(g);
        paint_surface(canvas, gr, group_radii(g), s[t.group_container], tokens::elevation_dp[t.elevation], s[color::Role::shadow]);
    }
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        const Group& group = groups_[g];
        const render::Rect gr = group_rect(g);
        if (group.label) {
            const double y = gr.y + group.label_y + (group.label_height - group.label->box_height()) / 2.0;
            text::draw_line(canvas, fonts_, *group.label, {gr.x + t.group_label_start_padding, y}, s[t.group_label_color]);
        }
        if (group.divider_y) {
            const render::Rect line{gr.x + t.divider_horizontal_padding, gr.y + *group.divider_y,
                                    gr.width - 2.0 * t.divider_horizontal_padding, t.divider_thickness};
            canvas.fill_rect(line, s[t.divider_color]);
        }
        for (std::size_t i = 0; i < group.items.size(); ++i) {
            const Item& item = group.items[i];
            const render::Rect ir = item_rect(g, i);
            const render::CornerRadii radii = item_radii(g, i);
            const double k = std::clamp(item.selection.value(), 0.0, 1.0);
            if (k > 0.0) {
                canvas.fill_path(render::rounded_rect_path(ir, radii), color::mix(s[t.group_container], s[t.selected_container], k));
            }
            if (item.state_layer.value() > 0.0) {
                const color::Argb layer = color::mix(scheme[t.state_layer], scheme[t.selected_state_layer], k);
                canvas.fill_path(render::rounded_rect_path(ir, radii),
                                 color::scale_alpha(color::with_alpha(layer, item.state_layer.value()), a));
            }
            double x = ir.x + t.item_horizontal_padding;
            const double cy = ir.y + ir.height / 2.0;
            if (item.leading_icon) {
                const icons::Icon& icon = item.selected && item.selected_leading_icon ? *item.selected_leading_icon : *item.leading_icon;
                icon.font.draw(canvas, icon.symbol, {x, cy - t.leading_icon_size / 2.0}, t.leading_icon_size,
                               color::mix(s[t.leading_icon], s[t.selected_leading_icon], k));
                x += t.leading_icon_size + t.icon_text_space;
            }
            if (item.label) {
                const render::Point top_left{x, cy - item.label->box_height() / 2.0};
                const color::Argb label_color = color::mix(s[t.label_color], s[t.selected_label], k);
                text::draw_line(canvas, fonts_, *item.label, top_left, label_color);
                if (item.access && access_keys_fade_.value() > 0.0) {
                    paint_access_key(canvas, fonts_, *item.access, *item.label, top_left,
                                 color::scale_alpha(label_color, std::clamp(access_keys_fade_.value(), 0.0, 1.0)));
                }
            }
            if (item.trailing_icon) {
                const double tx = ir.x + ir.width - t.item_horizontal_padding - t.trailing_icon_size;
                item.trailing_icon->font.draw(canvas, item.trailing_icon->symbol, {tx, cy - t.trailing_icon_size / 2.0},
                                              t.trailing_icon_size,
                                              color::mix(s[t.trailing_icon], s[t.selected_trailing_icon], k));
            }
        }
    }
    if (const std::optional<MenuItemIndex> focused = focused_item()) {
        focus_.paint_inward(canvas, s, item_rect(focused->group, focused->item), item_radii(focused->group, focused->item));
    }
    canvas.restore();
}

}
