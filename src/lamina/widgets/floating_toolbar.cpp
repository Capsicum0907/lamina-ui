#include "lamina/widgets/floating_toolbar.hpp"

#include <algorithm>
#include <stdexcept>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/fab.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::FloatingToolbarTokens& t = tokens::floating_toolbar_tokens;

}

ToolbarScrollExpansion::ToolbarScrollExpansion(bool expanded, double threshold) : expanded_(expanded), threshold_distance_(threshold) {
    update_threshold();
}

void ToolbarScrollExpansion::update_threshold() {
    threshold_ = expanded_ ? content_offset_ - threshold_distance_ : content_offset_ + threshold_distance_;
}

void ToolbarScrollExpansion::set_expanded(bool expanded) {
    if (expanded_ == expanded) {
        return;
    }
    expanded_ = expanded;
    update_threshold();
}

std::optional<bool> ToolbarScrollExpansion::scrolled_to(double offset) {
    const double delta = -(offset - last_);
    last_ = offset;
    if (delta == 0.0) {
        return std::nullopt;
    }
    content_offset_ += delta;
    if (delta < 0.0 && content_offset_ <= threshold_) {
        threshold_ = content_offset_ + threshold_distance_;
        const bool changed = expanded_;
        expanded_ = false;
        return changed ? std::optional(false) : std::nullopt;
    }
    if (delta > 0.0 && content_offset_ >= threshold_) {
        threshold_ = content_offset_ - threshold_distance_;
        const bool changed = !expanded_;
        expanded_ = true;
        return changed ? std::optional(true) : std::nullopt;
    }
    return std::nullopt;
}

FloatingToolbar::FloatingToolbar(FloatingToolbarOptions options, const icons::IconFont&)
    : options_(std::move(options)),
      leading_{{}, motion::Animated(options_.expanded ? 1.0 : 0.0, t.expand)},
      content_{{}, motion::Animated(1.0, t.expand)},
      trailing_{{}, motion::Animated(options_.expanded ? 1.0 : 0.0, t.expand)},
      progress_(options_.expanded ? 1.0 : 0.0, t.expand),
      fab_state_layer_(0.0, tokens::motion_scheme.fast_effects()),
      fab_elevation_(tokens::elevation_dp[t.fab_elevation], tokens::motion_scheme.default_effects()) {
    if (options_.fab && (!options_.leading.empty() || !options_.trailing.empty())) {
        throw std::invalid_argument("A floating toolbar with a FAB has no leading or trailing content");
    }
    auto build = [this](Group& g, const std::vector<ToolbarAction>& actions) {
        for (const ToolbarAction& a : actions) {
            IconButtonOptions o{a.icon, a.accessible_name, tokens::IconButtonStyle::standard};
            o.enabled = a.enabled;
            o.content = colors().content;
            g.buttons.push_back(std::make_unique<IconButton>(std::move(o)));
        }
    };
    build(leading_, options_.leading);
    build(content_, options_.actions);
    build(trailing_, options_.trailing);
    if (options_.fab) {
        options_.fab->accessible_name = required_name(std::move(options_.fab->accessible_name), "A toolbar FAB");
    }
    place();
}

const tokens::ToolbarColorTokens& FloatingToolbar::colors() const {
    return options_.colors == ToolbarColorStyle::vibrant ? t.vibrant : t.standard;
}

FloatingToolbar::Group& FloatingToolbar::group(ToolbarSlot slot) {
    return slot == ToolbarSlot::leading ? leading_ : slot == ToolbarSlot::trailing ? trailing_ : content_;
}

const FloatingToolbar::Group& FloatingToolbar::group(ToolbarSlot slot) const {
    return slot == ToolbarSlot::leading ? leading_ : slot == ToolbarSlot::trailing ? trailing_ : content_;
}

std::size_t FloatingToolbar::action_count(ToolbarSlot slot) const {
    return group(slot).buttons.size();
}

IconButton& FloatingToolbar::action(ToolbarSlot slot, std::size_t index) {
    return *group(slot).buttons[index];
}

const IconButton& FloatingToolbar::action(ToolbarSlot slot, std::size_t index) const {
    return *group(slot).buttons[index];
}

double FloatingToolbar::extent(const Group& g) const {
    return static_cast<double>(g.buttons.size()) * t.item_slot * std::max(0.0, g.shown.value());
}

double FloatingToolbar::full_main() const {
    return 2.0 * t.content_padding + static_cast<double>(content_.buttons.size()) * t.item_slot;
}

double FloatingToolbar::fab_size() const {
    return motion::lerp(t.fab_min_size, t.fab_max_size, 1.0 - progress_.value());
}

double FloatingToolbar::width() const {
    if (has_fab()) {
        return horizontal() ? full_main() + t.toolbar_to_fab_gap + t.fab_min_size : t.fab_max_size;
    }
    const double main = 2.0 * t.content_padding + extent(leading_) + extent(content_) + extent(trailing_);
    return horizontal() ? main : t.container_size;
}

double FloatingToolbar::height() const {
    if (has_fab()) {
        return horizontal() ? t.fab_max_size : full_main() + t.toolbar_to_fab_gap + t.fab_min_size;
    }
    const double main = 2.0 * t.content_padding + extent(leading_) + extent(content_) + extent(trailing_);
    return horizontal() ? t.container_size : main;
}

render::Rect FloatingToolbar::toolbar_rect() const {
    if (!has_fab()) {
        return bounds();
    }
    const double full = full_main();
    const double shown = full * std::clamp(progress_.value(), 0.0, 1.0);
    const double cross = (t.fab_max_size - t.container_size) / 2.0;
    const bool end = options_.fab_position == ToolbarFabPosition::end;
    const double total = full + t.toolbar_to_fab_gap + t.fab_min_size;
    const double main = end ? full - shown : total - full;
    if (horizontal()) {
        return {origin_.x + main, origin_.y + cross, shown, t.container_size};
    }
    return {origin_.x + cross, origin_.y + main, t.container_size, shown};
}

std::optional<render::Rect> FloatingToolbar::fab_rect() const {
    if (!has_fab()) {
        return std::nullopt;
    }
    const double s = fab_size();
    const bool end = options_.fab_position == ToolbarFabPosition::end;
    if (horizontal()) {
        return render::Rect{end ? origin_.x + width() - s : origin_.x, origin_.y + (height() - s) / 2.0, s, s};
    }
    return render::Rect{origin_.x + (width() - s) / 2.0, end ? origin_.y + height() - s : origin_.y, s, s};
}

void FloatingToolbar::set_origin(render::Point origin) {
    origin_ = origin;
    place();
}

void FloatingToolbar::place() {
    const render::Rect bar = toolbar_rect();
    const double slot = t.item_slot;
    const double cross = horizontal() ? bar.y + (bar.height - slot) / 2.0 : bar.x + (bar.width - slot) / 2.0;
    double main = (horizontal() ? bar.x : bar.y) + t.content_padding;
    auto lay = [&](Group& g, bool anchor_end) {
        const double full = static_cast<double>(g.buttons.size()) * slot;
        const double shown = extent(g);
        double at = anchor_end ? main + shown - full : main;
        for (auto& b : g.buttons) {
            const double ox = (slot - b->width()) / 2.0;
            const double oy = (slot - b->height()) / 2.0;
            if (horizontal()) {
                b->set_origin({at + ox, cross + oy});
            } else {
                b->set_origin({cross + ox, at + oy});
            }
            at += slot;
        }
        main += shown;
    };
    lay(leading_, !leading_.expanding);
    lay(content_, false);
    lay(trailing_, trailing_.expanding);
}

bool FloatingToolbar::shows(ToolbarSlot slot, std::size_t) const {
    if (has_fab()) {
        return expanded();
    }
    if (slot == ToolbarSlot::content) {
        return true;
    }
    return expanded();
}

void FloatingToolbar::set_expanded(bool expanded) {
    if (options_.expanded == expanded) {
        return;
    }
    options_.expanded = expanded;
    progress_.animate_to(expanded ? 1.0 : 0.0);
    for (Group* g : {&leading_, &trailing_}) {
        g->expanding = expanded;
        g->shown.animate_to(expanded ? 1.0 : 0.0);
        if (!expanded) {
            for (auto& b : g->buttons) {
                b->set_focused(false);
                b->cancel_interaction();
            }
        }
    }
    if (!expanded && has_fab()) {
        for (auto& b : content_.buttons) {
            b->set_focused(false);
            b->cancel_interaction();
        }
    }
}

void FloatingToolbar::set_fab_focused(bool focused) {
    fab_focus_.set_focused(focused && enabled());
    fab_state_layer_.animate_to(fab_focus_.state_layer_opacity(fab_press_.state_layer_opacity()));
}

std::vector<FocusStop*> FloatingToolbar::focus_stops() {
    if (stops_.empty()) {
        for (ToolbarSlot slot : {ToolbarSlot::leading, ToolbarSlot::content, ToolbarSlot::trailing}) {
            Group& g = group(slot);
            for (std::size_t i = 0; i < g.buttons.size(); ++i) {
                stops_.push_back(std::make_unique<FocusStop>(forward_focus(*g.buttons[i], [this, slot, i] { return shows(slot, i); })));
            }
        }
        if (has_fab()) {
            stops_.push_back(std::make_unique<FocusStop>(FocusStop::Hooks{[this] { return fab_focused(); },
                                                                          [this](bool f) { set_fab_focused(f); },
                                                                          [this] { return enabled(); },
                                                                          [this] { return *fab_rect(); }}));
        }
    }
    std::vector<FocusStop*> out;
    for (auto& s : stops_) {
        out.push_back(s.get());
    }
    return out;
}

void FloatingToolbar::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    for (Group* g : {&leading_, &content_, &trailing_}) {
        for (auto& b : g->buttons) {
            b->set_enabled(enabled);
        }
    }
    if (!enabled) {
        fab_press_ = {};
        fab_focus_.set_focused(false);
        fab_state_layer_.animate_to(0.0);
    }
}

AccessibleNode FloatingToolbar::accessible() const {
    AccessibleNode node{AccessibleRole::toolbar, options_.accessible_name, bounds(), {.enabled = enabled()}};
    node.description = expanded() ? options_.strings->collapse_toolbar : options_.strings->expand_toolbar;
    for (ToolbarSlot slot : {ToolbarSlot::leading, ToolbarSlot::content, ToolbarSlot::trailing}) {
        const Group& g = group(slot);
        for (std::size_t i = 0; i < g.buttons.size(); ++i) {
            if (shows(slot, i)) {
                node.children.push_back(g.buttons[i]->accessible());
            }
        }
    }
    if (has_fab()) {
        node.children.push_back({AccessibleRole::button, options_.fab->accessible_name, *fab_rect(),
                                 {.enabled = enabled(), .focusable = enabled(), .focused = fab_focused()}});
    }
    return node;
}

FloatingToolbarResponse FloatingToolbar::handle(const input::PointerEvent& e) {
    FloatingToolbarResponse response;
    place();
    for (ToolbarSlot slot : {ToolbarSlot::leading, ToolbarSlot::content, ToolbarSlot::trailing}) {
        Group& g = group(slot);
        for (std::size_t i = 0; i < g.buttons.size(); ++i) {
            if (!shows(slot, i)) {
                continue;
            }
            if (g.buttons[i]->handle(e)) {
                response.clicked = ToolbarClick{slot, i};
                response.consumed = true;
            }
        }
    }
    if (has_fab()) {
        fab_focus_.release_on_press(e);
        if (enabled()) {
            if (fab_press_.update(e, *fab_rect())) {
                response.fab_clicked = true;
                response.consumed = true;
            }
            fab_state_layer_.animate_to(fab_focus_.state_layer_opacity(fab_press_.state_layer_opacity()));
            const std::size_t level = fab_press_.hovered && !fab_press_.pressed ? t.fab_hovered_elevation : t.fab_elevation;
            fab_elevation_.animate_to(tokens::elevation_dp[level]);
        }
    }
    const bool over = contains(toolbar_rect(), e.x_dp, e.y_dp) || (has_fab() && contains(*fab_rect(), e.x_dp, e.y_dp));
    if (e.action != input::PointerAction::leave && over) {
        response.consumed = true;
    }
    return response;
}

FloatingToolbarResponse FloatingToolbar::handle(const input::KeyEvent& e) {
    FloatingToolbarResponse response;
    if (!enabled()) {
        return response;
    }
    for (ToolbarSlot slot : {ToolbarSlot::leading, ToolbarSlot::content, ToolbarSlot::trailing}) {
        Group& g = group(slot);
        for (std::size_t i = 0; i < g.buttons.size(); ++i) {
            if (shows(slot, i) && g.buttons[i]->handle(e)) {
                response.clicked = ToolbarClick{slot, i};
                response.consumed = true;
            }
        }
    }
    if (has_fab() && fab_focus_.activates(e, KeyActivation::space_or_enter)) {
        response.fab_clicked = true;
        response.consumed = true;
    }
    return response;
}

void FloatingToolbar::cancel_interaction() {
    for (Group* g : {&leading_, &content_, &trailing_}) {
        for (auto& b : g->buttons) {
            b->cancel_interaction();
        }
    }
    fab_press_ = {};
    fab_state_layer_.animate_to(fab_focus_.state_layer_opacity(0.0));
}

void FloatingToolbar::step(double dt) {
    progress_.step(dt);
    for (Group* g : {&leading_, &content_, &trailing_}) {
        g->shown.step(dt);
        for (auto& b : g->buttons) {
            b->step(dt);
        }
    }
    fab_state_layer_.step(dt);
    fab_elevation_.step(dt);
    fab_focus_.step(dt);
    enabled_.step(dt);
    place();
}

bool FloatingToolbar::animating() const {
    bool any = !progress_.settled() || !fab_state_layer_.settled() || !fab_elevation_.settled() || !fab_focus_.settled() || !enabled_.settled();
    for (const Group* g : {&leading_, &content_, &trailing_}) {
        any = any || !g->shown.settled();
        for (const auto& b : g->buttons) {
            any = any || b->animating();
        }
    }
    return any;
}

void FloatingToolbar::paint_group(render::Canvas& canvas, const color::ColorScheme& scheme, const Group& g) const {
    for (const auto& b : g.buttons) {
        b->paint(canvas, scheme);
    }
}

void FloatingToolbar::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect bar = toolbar_rect();
    const tokens::ToolbarColorTokens& c = colors();
    if (bar.width > 0.0 && bar.height > 0.0) {
        const double radius = tokens::corner_radius(t.shape, bar.width, bar.height);
        const double p = std::clamp(progress_.value(), 0.0, 1.0);
        const double elevation =
            has_fab() ? motion::lerp(tokens::elevation_dp[t.collapsed_elevation_with_fab], tokens::elevation_dp[t.expanded_elevation_with_fab], p)
                      : motion::lerp(tokens::elevation_dp[t.collapsed_elevation], tokens::elevation_dp[t.expanded_elevation], p);
        paint_surface(canvas, bar, radius, scheme[c.container], elevation, scheme[color::Role::shadow]);
        canvas.save();
        canvas.push_clip(render::rounded_rect_path(bar, render::uniform_radii(radius)));
        const render::Rect inner = horizontal()
                                       ? render::Rect{bar.x + t.content_padding, bar.y, std::max(0.0, bar.width - 2.0 * t.content_padding), bar.height}
                                       : render::Rect{bar.x, bar.y + t.content_padding, bar.width, std::max(0.0, bar.height - 2.0 * t.content_padding)};
        if (has_fab()) {
            canvas.push_clip(inner);
            paint_group(canvas, scheme, content_);
            canvas.pop_clip();
        } else {
            double main = horizontal() ? inner.x : inner.y;
            for (const Group* g : {&leading_, &content_, &trailing_}) {
                const double shown = extent(*g);
                if (shown > 0.0) {
                    canvas.push_clip(horizontal() ? render::Rect{main, bar.y, shown, bar.height} : render::Rect{bar.x, main, bar.width, shown});
                    paint_group(canvas, scheme, *g);
                    canvas.pop_clip();
                }
                main += shown;
            }
        }
        canvas.pop_clip();
        canvas.restore();
    }
    if (const auto fab = fab_rect()) {
        const double radius = std::min(tokens::corner_radius(t.fab_shape, fab->width, fab->height), fab->width / 2.0);
        const color::Argb content = enabled_.mix(color::scale_alpha(scheme[c.fab_content], tokens::disabled_alpha), scheme[c.fab_content]);
        const color::Argb fill = color::state_layer(scheme[c.fab_container], scheme[c.fab_content], fab_state_layer_.value());
        paint_surface(canvas, *fab, radius, fill, fab_elevation_.value(), scheme[color::Role::shadow]);
        fab_focus_.paint(canvas, scheme, *fab, radius);
        icons::draw_centered(canvas, options_.fab->icon, *fab, tokens::fab_size(tokens::FabSize::baseline).icon_size, content);
    }
}

}
