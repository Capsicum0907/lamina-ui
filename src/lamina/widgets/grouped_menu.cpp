#include "lamina/widgets/grouped_menu.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::GroupedMenuTokens& t = tokens::grouped_menu_tokens;

constexpr tokens::SpringSpec scale_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::fast);
constexpr tokens::SpringSpec alpha_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);
constexpr tokens::SpringSpec morph_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::fast);
constexpr tokens::SpringSpec color_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);
constexpr tokens::SpringSpec state_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);

render::CornerRadii radii_of(const tokens::Corners& c, double w, double h) {
    return {tokens::corner_radius(c.top_start, w, h), tokens::corner_radius(c.top_end, w, h),
            tokens::corner_radius(c.bottom_end, w, h), tokens::corner_radius(c.bottom_start, w, h)};
}

std::array<double, 4> as_array(const render::CornerRadii& r) {
    return {r.top_left, r.top_right, r.bottom_right, r.bottom_left};
}

template <typename Springs>
render::CornerRadii current(const Springs& s) {
    return {std::max(0.0, s[0].value()), std::max(0.0, s[1].value()), std::max(0.0, s[2].value()), std::max(0.0, s[3].value())};
}

template <typename Springs>
void animate(Springs& s, const render::CornerRadii& target) {
    const auto v = as_array(target);
    for (std::size_t i = 0; i < 4; ++i) {
        s[i].animate_to(v[i]);
    }
}

template <typename Springs>
void snap(Springs& s, const render::CornerRadii& target) {
    const auto v = as_array(target);
    for (std::size_t i = 0; i < 4; ++i) {
        s[i].snap_to(v[i]);
    }
}

template <typename Springs>
bool springs_settled(const Springs& s) {
    return std::all_of(s.begin(), s.end(), [](const motion::Animated& a) { return a.settled(); });
}

std::array<motion::Animated, 4> springs() {
    return {motion::Animated(0.0, morph_spring), motion::Animated(0.0, morph_spring), motion::Animated(0.0, morph_spring),
            motion::Animated(0.0, morph_spring)};
}

double text_box(const text::TextLine& line) {
    return std::max(line.style.line_height, line.ascent + line.descent);
}

color::Argb faded(color::Argb c, double alpha) {
    return color::with_alpha(c, alpha * static_cast<double>(c >> 24) / 255.0);
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
    : fonts_(fonts), scale_(t.closed_scale, scale_spring), alpha_(0.0, alpha_spring) {
    const auto item_style = text::text_style(tokens::type_style(t.label));
    const auto group_label_style = text::text_style(tokens::type_style(t.group_label));
    double content = 0.0;
    double y = 0.0;
    for (std::size_t g = 0; g < options.groups.size(); ++g) {
        MenuGroup& spec = options.groups[g];
        Group group{std::nullopt, 0.0, 0.0, std::nullopt, spec.selection, {}, y, 0.0, false, false, springs()};
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
            double w = 2.0 * t.item_horizontal_padding;
            double h = 0.0;
            if (!item.label.empty()) {
                label = text::layout_line(fonts_, item.label, item_style);
                w += label->width;
                h = label->style.line_height;
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
            group.items.push_back({std::move(label), std::move(item.leading_icon), std::move(item.selected_leading_icon),
                                   std::move(item.trailing_icon), item.selected, gy, height, {}, motion::Animated(0.0, state_spring),
                                   motion::Animated(on, color_spring), springs()});
            gy += height;
        }
        group.height = gy + t.group_vertical_padding;
        y += group.height + t.group_gap;
        groups_.push_back(std::move(group));
    }
    width_ = content;
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        snap(groups_[g].corners, group_target(g));
        for (std::size_t i = 0; i < groups_[g].items.size(); ++i) {
            snap(groups_[g].items[i].corners, item_target(g, i));
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
    return current(groups_[group].corners);
}

render::CornerRadii GroupedMenu::item_radii(std::size_t group, std::size_t item) const {
    return current(groups_[group].items[item].corners);
}

render::CornerRadii GroupedMenu::group_target(std::size_t group) const {
    const Group& g = groups_[group];
    const render::Rect r = group_rect(group);
    if (g.has_been_hovered && !g.hovered) {
        return radii_of(tokens::all_corners(t.inactive_group), r.width, r.height);
    }
    return radii_of(menu_group_corners(group, groups_.size()), r.width, r.height);
}

render::CornerRadii GroupedMenu::item_target(std::size_t group, std::size_t item) const {
    const Group& g = groups_[group];
    const render::Rect r = item_rect(group, item);
    if (g.items[item].selected) {
        return radii_of(tokens::all_corners(t.selected_item), r.width, r.height);
    }
    return radii_of(menu_item_corners(item, g.items.size()), r.width, r.height);
}

void GroupedMenu::retarget_group(std::size_t group) {
    animate(groups_[group].corners, group_target(group));
}

void GroupedMenu::retarget_item(std::size_t group, std::size_t item) {
    Item& i = groups_[group].items[item];
    animate(i.corners, item_target(group, item));
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

void GroupedMenu::open(const render::Rect& anchor, layout::Size window) {
    origin_ = menu_position(anchor, window, {width_, height()}, t.window_horizontal_margin, t.window_vertical_margin);
    pivot_ = menu_transform_origin(anchor, bounds());
    if (!visible()) {
        scale_.snap_to(t.closed_scale);
        alpha_.snap_to(0.0);
        for (std::size_t g = 0; g < groups_.size(); ++g) {
            groups_[g].hovered = false;
            groups_[g].has_been_hovered = false;
            snap(groups_[g].corners, group_target(g));
            for (std::size_t i = 0; i < groups_[g].items.size(); ++i) {
                Item& item = groups_[g].items[i];
                snap(item.corners, item_target(g, i));
                item.selection.snap_to(item.selected ? 1.0 : 0.0);
            }
        }
    }
    open_ = true;
    scale_.animate_to(1.0);
    alpha_.animate_to(1.0);
}

void GroupedMenu::close() {
    open_ = false;
    scale_.animate_to(t.closed_scale);
    alpha_.animate_to(0.0);
    for (Group& g : groups_) {
        for (Item& item : g.items) {
            item.press = {};
            item.state_layer.animate_to(0.0);
        }
    }
}

bool GroupedMenu::visible() const {
    return open_ || !alpha_.settled() || alpha_.value() > 0.0;
}

double GroupedMenu::opacity() const {
    return std::clamp(alpha_.value(), 0.0, 1.0);
}

GroupedMenuResponse GroupedMenu::handle(const platform::PointerEvent& e) {
    GroupedMenuResponse response;
    if (!open_) {
        return response;
    }
    if (e.action == platform::PointerAction::down && !contains(bounds(), e.x_dp, e.y_dp)) {
        close();
        response.consumed = true;
        return response;
    }
    response.consumed = e.action != platform::PointerAction::leave;
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        Group& group = groups_[g];
        const bool hovered = e.action != platform::PointerAction::leave && contains(group_rect(g), e.x_dp, e.y_dp);
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
            item.state_layer.animate_to(item.press.state_layer_opacity());
        }
    }
    if (response.chosen) {
        choose(response.chosen->group, response.chosen->item);
    }
    return response;
}

bool GroupedMenu::handle(const platform::KeyEvent& e) {
    if (open_ && e.key == platform::Key::escape) {
        close();
        return true;
    }
    return false;
}

void GroupedMenu::step(double dt) {
    scale_.step(dt);
    alpha_.step(dt);
    for (Group& g : groups_) {
        for (motion::Animated& c : g.corners) {
            c.step(dt);
        }
        for (Item& item : g.items) {
            item.state_layer.step(dt);
            item.selection.step(dt);
            for (motion::Animated& c : item.corners) {
                c.step(dt);
            }
        }
    }
}

bool GroupedMenu::animating() const {
    if (!scale_.settled() || !alpha_.settled()) {
        return true;
    }
    return std::any_of(groups_.begin(), groups_.end(), [](const Group& g) {
        return !springs_settled(g.corners) || std::any_of(g.items.begin(), g.items.end(), [](const Item& item) {
                   return !item.state_layer.settled() || !item.selection.settled() || !springs_settled(item.corners);
               });
    });
}

void GroupedMenu::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    const double a = opacity();
    canvas.save();
    canvas.translate(pivot_.x, pivot_.y);
    canvas.scale(scale_.value());
    canvas.translate(-pivot_.x, -pivot_.y);
    const color::Argb shadow = faded(scheme[color::Role::shadow], a);
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        const render::Rect gr = group_rect(g);
        paint_surface(canvas, gr, group_radii(g), faded(scheme[t.group_container], a), tokens::elevation_dp[t.elevation], shadow);
    }
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        const Group& group = groups_[g];
        const render::Rect gr = group_rect(g);
        if (group.label) {
            const double y = gr.y + group.label_y + (group.label_height - text_box(*group.label)) / 2.0;
            text::draw_line(canvas, fonts_, *group.label, {gr.x + t.group_label_start_padding, y},
                            faded(scheme[t.group_label_color], a));
        }
        if (group.divider_y) {
            const render::Rect line{gr.x + t.divider_horizontal_padding, gr.y + *group.divider_y,
                                    gr.width - 2.0 * t.divider_horizontal_padding, t.divider_thickness};
            canvas.fill_path(render::rounded_rect_path(line, render::uniform_radii(0.0)), faded(scheme[t.divider_color], a));
        }
        for (std::size_t i = 0; i < group.items.size(); ++i) {
            const Item& item = group.items[i];
            const render::Rect ir = item_rect(g, i);
            const render::CornerRadii radii = item_radii(g, i);
            const double k = std::clamp(item.selection.value(), 0.0, 1.0);
            if (k > 0.0) {
                canvas.fill_path(render::rounded_rect_path(ir, radii),
                                 faded(color::mix(scheme[t.group_container], scheme[t.selected_container], k), a));
            }
            if (item.state_layer.value() > 0.0) {
                const color::Argb layer = color::mix(scheme[t.state_layer], scheme[t.selected_state_layer], k);
                canvas.fill_path(render::rounded_rect_path(ir, radii), faded(color::with_alpha(layer, item.state_layer.value()), a));
            }
            double x = ir.x + t.item_horizontal_padding;
            const double cy = ir.y + ir.height / 2.0;
            if (item.leading_icon) {
                const icons::Icon& icon = item.selected && item.selected_leading_icon ? *item.selected_leading_icon : *item.leading_icon;
                icon.font.draw(canvas, icon.symbol, {x, cy - t.leading_icon_size / 2.0}, t.leading_icon_size,
                               faded(color::mix(scheme[t.leading_icon], scheme[t.selected_leading_icon], k), a));
                x += t.leading_icon_size + t.icon_text_space;
            }
            if (item.label) {
                text::draw_line(canvas, fonts_, *item.label, {x, cy - text_box(*item.label) / 2.0},
                                faded(color::mix(scheme[t.label_color], scheme[t.selected_label], k), a));
            }
            if (item.trailing_icon) {
                const double tx = ir.x + ir.width - t.item_horizontal_padding - t.trailing_icon_size;
                item.trailing_icon->font.draw(canvas, item.trailing_icon->symbol, {tx, cy - t.trailing_icon_size / 2.0},
                                              t.trailing_icon_size,
                                              faded(color::mix(scheme[t.trailing_icon], scheme[t.selected_trailing_icon], k), a));
            }
        }
    }
    canvas.restore();
}

}
