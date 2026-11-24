#include "lamina/style/style.hpp"
#include "lamina/widgets/search_bar.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/divider.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/item_focus.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

const tokens::SearchTokens& t() {
    return current_style().search;
}

double slot_padding() {
    return t().icon_slot + (t().padding - t().icon_padding);
}

std::unique_ptr<IconButton> search_button(const SearchBarIcon& spec, color::Role content) {
    return std::make_unique<IconButton>(IconButtonOptions{.icon = spec.icon,
                                                          .accessible_name = spec.accessible_name,
                                                          .style = tokens::IconButtonStyle::standard,
                                                          .size = t().icon_button_size,
                                                          .content = content});
}

render::Point leading_centre(const render::Rect& field) {
    return {field.x + t().icon_slot / 2.0 + t().icon_offset, field.y + field.height / 2.0};
}

render::Point trailing_centre(const render::Rect& field) {
    return {field.x + field.width - t().icon_slot / 2.0 - t().icon_offset, field.y + field.height / 2.0};
}

void centre_button(IconButton& b, render::Point c) {
    b.set_origin({c.x - b.width() / 2.0, c.y - b.height() / 2.0});
}

}

SearchBar::SearchBar(SearchBarOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      field_(TextFieldOptions{.label = {},
                              .placeholder = options_.placeholder,
                              .clipboard = options_.clipboard,
                              .accessible_name = options_.accessible_name.empty() ? options_.strings->search : options_.accessible_name,
                              .min_width = 0.0,
                              .outlined = false,
                              .padding_start = slot_padding(),
                              .padding_end = options_.trailing ? slot_padding() : t().padding},
             fonts),
      back_(search_button(options_.back, t().leading_icon)),
      trailing_(options_.trailing ? search_button(*options_.trailing, t().trailing_icon) : nullptr),
      results_(ListViewOptions{.items = std::move(options_.results), .accessible_name = {}}, fonts),
      progress_(0.0, current_style().search_motion.expand) {
    if (options_.accessible_name.empty()) {
        options_.accessible_name = options_.strings->search;
    }
    top_focus_.add(*back_);
    top_focus_.add(field_);
    if (trailing_) {
        top_focus_.add(*trailing_);
    }
    list_focus_.add(results_);
}

layout::Size SearchBar::measure(const layout::Constraints& c) const {
    const double available = std::isfinite(c.max_width) ? c.max_width : t().min_width;
    const double w = std::min(t().max_width, std::max(available, std::min(t().min_width, available)));
    return c.constrain({w, t().height});
}

void SearchBar::arrange(const render::Rect& rect) {
    const layout::Size s = measure({0.0, rect.width, 0.0, rect.height});
    collapsed_ = {rect.x + (rect.width - s.width) / 2.0, rect.y + (rect.height - s.height) / 2.0, s.width, s.height};
    place();
}

void SearchBar::set_area(const render::Rect& area) {
    area_ = area;
    place();
}

bool SearchBar::full_screen() const {
    switch (options_.mode) {
    case SearchViewMode::full_screen:
        return true;
    case SearchViewMode::docked:
        return false;
    case SearchViewMode::automatic:
        break;
    }
    return area_.width < options_.compact_width;
}

bool SearchBar::showing_view() const {
    return progress_.target() == 1.0 || progress_.value() > t().expanded_threshold;
}

bool SearchBar::is_open() const {
    return showing_view();
}

render::Rect SearchBar::surface() const {
    const render::Rect& c = collapsed_;
    if (!showing_view()) {
        return c;
    }
    const double p = progress();
    if (full_screen()) {
        return {motion::lerp(c.x, area_.x, p), motion::lerp(c.y, area_.y, p), motion::lerp(c.width, area_.width, p),
                motion::lerp(c.height, area_.height, p)};
    }
    const double max_height = area_.height * t().docked_max_height_ratio;
    const double limit = motion::lerp(c.height, std::max(c.height, max_height), std::max(0.0, progress_.value()));
    const double natural = c.height + current_style().divider.thickness + results_.content_height();
    const double low = std::min(t().docked_min_height, limit);
    return {c.x, c.y, c.width, std::clamp(natural, low, std::max(low, limit))};
}

render::Rect SearchBar::field_rect() const {
    const render::Rect& c = collapsed_;
    if (!showing_view() || !full_screen()) {
        return c;
    }
    const render::Rect s = surface();
    const double raw = progress_.value();
    const double width = motion::lerp(c.width, s.width, raw);
    const double centre = motion::lerp(c.x + c.width / 2.0, s.x + s.width / 2.0, raw);
    const double top = s.y + motion::lerp(0.0, t().vertical_padding, progress());
    return {centre - width / 2.0, top, width, c.height};
}

render::Rect SearchBar::results_rect() const {
    const render::Rect s = surface();
    const render::Rect f = field_rect();
    const double bottom_padding = full_screen() ? motion::lerp(0.0, t().vertical_padding, progress()) : 0.0;
    const double top = f.y + f.height + bottom_padding + current_style().divider.thickness;
    return {s.x, top, s.width, std::max(0.0, s.y + s.height - top)};
}

double SearchBar::corner_radius() const {
    if (!showing_view()) {
        return tokens::corner_radius(t().collapsed_shape, collapsed_.width, collapsed_.height);
    }
    if (full_screen()) {
        return tokens::corner_radius(t().collapsed_shape, collapsed_.width, collapsed_.height) * (1.0 - progress());
    }
    return tokens::corner_radius(t().docked_shape, collapsed_.width, collapsed_.height);
}

void SearchBar::place() {
    const render::Rect f = field_rect();
    field_.set_width(f.width);
    field_.set_origin({f.x, f.y});
    centre_button(*back_, leading_centre(f));
    if (trailing_) {
        centre_button(*trailing_, trailing_centre(f));
    }
    results_.set_viewport(results_rect());
}

void SearchBar::expand() {
    claim_focus_ = true;
    if (progress_.target() != 1.0) {
        progress_.set_spec(current_style().search_motion.expand);
        progress_.animate_to(1.0);
        results_.scroll_area().jump_to(0.0);
    }
    if (!field_.focused() && !results_.focused()) {
        back_->set_focused(false);
        if (trailing_) {
            trailing_->set_focused(false);
        }
        field_.set_focused(true);
    }
}

void SearchBar::collapse() {
    claim_focus_ = false;
    if (progress_.target() == 0.0) {
        return;
    }
    progress_.set_spec(current_style().search_motion.collapse);
    progress_.animate_to(0.0);
    const bool had_focus = focused();
    results_.set_focused(false);
    back_->set_focused(false);
    results_.cancel_interaction();
    back_->cancel_interaction();
    if (had_focus) {
        if (trailing_) {
            trailing_->set_focused(false);
        }
        field_.set_focused(true);
    }
}

void SearchBar::set_query(std::string_view text) {
    field_.set_text(text);
}

void SearchBar::set_results(std::vector<ListItemSpec> results) {
    results_.set_items(std::move(results));
    place();
}

void SearchBar::add_focus_stops(FocusManager& focus) {
    focus.add(field_);
    if (trailing_) {
        focus.add(*trailing_);
    }
}

bool SearchBar::focused() const {
    return field_.focused() || results_.focused() || back_->focused() || (trailing_ && trailing_->focused());
}

void SearchBar::set_focused(bool focused) {
    if (!focused) {
        field_.set_focused(false);
        results_.set_focused(false);
        back_->set_focused(false);
        if (trailing_) {
            trailing_->set_focused(false);
        }
        return;
    }
    if (!this->focused()) {
        field_.set_focused(true);
    }
}

void SearchBar::set_enabled(bool enabled) {
    field_.set_enabled(enabled);
    if (trailing_) {
        trailing_->set_enabled(enabled);
    }
    if (!enabled) {
        collapse();
    }
}

std::optional<render::Rect> SearchBar::caret_area() const {
    if (!field_.focused()) {
        return std::nullopt;
    }
    return field_.caret_area();
}

std::optional<double> SearchBar::next_wake_seconds() const {
    std::optional<double> wake;
    if (field_.focused()) {
        const double blink = field_.next_blink_seconds();
        if (blink >= 0.0) {
            wake = blink;
        }
    }
    if (const double s = results_.next_wake_seconds(); s >= 0.0 && showing_view()) {
        wake = wake ? std::min(*wake, s) : s;
    }
    return wake;
}

void SearchBar::tab(bool backward) {
    chain_focus({&top_focus_, &list_focus_}, backward ? FocusDirection::previous : FocusDirection::next, true);
}

void SearchBar::focus_results() {
    if (!results_.can_focus()) {
        return;
    }
    field_.set_focused(false);
    results_.set_focused(true);
}

SearchBarResponse SearchBar::changed(const std::string& before, SearchBarResponse r) {
    const std::string now = field_.text();
    if (now == before) {
        return r;
    }
    r.query_changed = true;
    if (!showing_view() && field_.focused() && now.size() > before.size()) {
        expand();
        r.expanded = true;
    }
    return r;
}

SearchBarResponse SearchBar::handle(const input::PointerEvent& e) {
    SearchBarResponse r;
    const std::string before = field_.text();
    if (!showing_view()) {
        pressed_ = PointerPart::none;
        const bool inside = e.action != input::PointerAction::leave && contains(collapsed_, e.x_dp, e.y_dp);
        const bool on_trailing = trailing_ && inside && contains(trailing_->bounds(), e.x_dp, e.y_dp);
        if (trailing_ && trailing_->handle(e)) {
            r.trailing = true;
        }
        if (e.action == input::PointerAction::down) {
            field_pressed_ = inside && !on_trailing;
        }
        if (!on_trailing) {
            field_.handle(e);
        }
        if (e.action == input::PointerAction::up && field_pressed_ && inside && !on_trailing && field_.enabled()) {
            field_pressed_ = false;
            expand();
            r.expanded = true;
        }
        if (e.action == input::PointerAction::up) {
            field_pressed_ = false;
        }
        r.consumed = inside;
        return changed(before, r);
    }
    if (e.action == input::PointerAction::down) {
        pressed_ = part_at(e);
    }
    const PointerPart part = pressed_ == PointerPart::none ? part_at(e) : pressed_;
    if (e.action == input::PointerAction::up || e.action == input::PointerAction::leave) {
        pressed_ = PointerPart::none;
    }
    if (part == PointerPart::outside && !full_screen()) {
        if (e.action == input::PointerAction::down) {
            deliver(PointerPart::none, {input::PointerAction::leave, 0.0, 0.0, 0, e.kind}, r);
            close_from_pointer();
            r.collapsed = true;
        }
        return r;
    }
    deliver(part == PointerPart::outside ? PointerPart::field : part, e, r);
    if (e.action == input::PointerAction::down && part == PointerPart::results) {
        field_.set_focused(false);
    }
    r.consumed = part != PointerPart::outside && part != PointerPart::none;
    return changed(before, r);
}

SearchBar::PointerPart SearchBar::part_at(const input::PointerEvent& e) const {
    if (e.action == input::PointerAction::leave) {
        return PointerPart::none;
    }
    const render::Rect view = full_screen() ? area_ : surface();
    if (!contains(view, e.x_dp, e.y_dp)) {
        return PointerPart::outside;
    }
    if (back_->hits(e)) {
        return PointerPart::back;
    }
    if (trailing_ && trailing_->hits(e)) {
        return PointerPart::trailing;
    }
    if (contains(results_rect(), e.x_dp, e.y_dp)) {
        return PointerPart::results;
    }
    return PointerPart::field;
}

void SearchBar::deliver(PointerPart part, const input::PointerEvent& e, SearchBarResponse& r) {
    const input::PointerEvent leave{input::PointerAction::leave, 0.0, 0.0, 0, e.kind};
    const auto to = [&](PointerPart p) -> const input::PointerEvent& { return p == part ? e : leave; };
    if (back_->handle(to(PointerPart::back))) {
        close_from_pointer();
        r.collapsed = true;
    }
    if (trailing_ && trailing_->handle(to(PointerPart::trailing))) {
        r.trailing = true;
    }
    field_.handle(to(PointerPart::field));
    const ListViewResponse lr = results_.handle(to(PointerPart::results));
    if (lr.clicked) {
        r.chosen = lr.clicked;
    }
}

void SearchBar::close_from_pointer() {
    collapse();
    set_focused(false);
}

SearchBarResponse SearchBar::handle(const input::KeyEvent& e) {
    SearchBarResponse r;
    const std::string before = field_.text();
    if (!showing_view()) {
        if (field_.focused() && e.key == input::Key::down && !e.ctrl && field_.enabled()) {
            expand();
            r.expanded = true;
            r.consumed = true;
            return r;
        }
        if (trailing_ && trailing_->handle(e)) {
            r.trailing = true;
            r.consumed = true;
            return r;
        }
        if (field_.focused()) {
            r.consumed = field_.handle(e);
        }
        return changed(before, r);
    }
    if (!focused()) {
        return r;
    }
    r.consumed = true;
    if (e.key == input::Key::escape) {
        collapse();
        r.collapsed = true;
        return r;
    }
    if (e.key == input::Key::tab && !e.ctrl) {
        tab(e.shift);
        return r;
    }
    if (back_->handle(e)) {
        collapse();
        r.collapsed = true;
        return r;
    }
    if (trailing_ && trailing_->handle(e)) {
        r.trailing = true;
        return r;
    }
    if (results_.focused()) {
        const std::optional<std::size_t> row = results_.focused_row();
        const std::optional<std::size_t> first =
            first_enabled_item(results_.size(), [this](std::size_t i) { return results_.item(i).enabled; });
        if (e.key == input::Key::up && !e.ctrl && row && row == first) {
            results_.set_focused(false);
            field_.set_focused(true);
            return r;
        }
        const ListViewResponse lr = results_.handle(e);
        if (lr.clicked) {
            r.chosen = lr.clicked;
        }
        return r;
    }
    if (field_.focused()) {
        if (e.key == input::Key::down && !e.ctrl) {
            focus_results();
            return r;
        }
        if (e.key == input::Key::enter && !e.ctrl) {
            r.searched = field_.text();
            return r;
        }
        field_.handle(e);
    }
    return changed(before, r);
}

SearchBarResponse SearchBar::handle(const input::TextEvent& e) {
    SearchBarResponse r;
    if (!field_.focused()) {
        return r;
    }
    const std::string before = field_.text();
    field_.handle(e);
    r.consumed = true;
    return changed(before, r);
}

bool SearchBar::handle(const input::WheelEvent& e) {
    if (!showing_view()) {
        return false;
    }
    if (contains(results_rect(), e.x_dp, e.y_dp)) {
        results_.handle(e);
    }
    return contains(full_screen() ? area_ : surface(), e.x_dp, e.y_dp);
}

void SearchBar::cancel_interaction() {
    field_.cancel_interaction();
    back_->cancel_interaction();
    if (trailing_) {
        trailing_->cancel_interaction();
    }
    results_.cancel_interaction();
    field_pressed_ = false;
    pressed_ = PointerPart::none;
}

void SearchBar::step(double dt) {
    if (claim_focus_) {
        claim_focus_ = false;
        if (expanded() && !focused()) {
            field_.set_focused(true);
        }
    }
    progress_.step(dt);
    place();
    field_.step(dt);
    back_->step(dt);
    if (trailing_) {
        trailing_->step(dt);
    }
    results_.step(dt);
}

bool SearchBar::animating() const {
    return !progress_.settled() || field_.animating() || back_->animating() || (trailing_ && trailing_->animating()) || results_.animating();
}

void SearchBar::paint_field(render::Canvas& canvas, const color::ColorScheme& scheme, const render::Rect& f, bool expanded_view) const {
    if (expanded_view) {
        back_->paint(canvas, scheme);
    } else {
        const color::Argb icon = field_.enabled() ? scheme[t().leading_icon] : resolve(scheme, t().disabled_icon);
        icons::draw_centered(canvas, options_.search_icon, leading_centre(f), t().icon_size, icon);
    }
    field_.paint(canvas, scheme);
    if (trailing_) {
        trailing_->paint(canvas, scheme);
    }
}

void SearchBar::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (showing_view()) {
        return;
    }
    const double r = corner_radius();
    paint_surface(canvas, collapsed_, render::uniform_radii(r), scheme[t().container], t().shadow_elevation, scheme[color::Role::shadow]);
    canvas.save();
    canvas.push_clip(render::rounded_rect_path(collapsed_, render::uniform_radii(r)));
    paint_field(canvas, scheme, collapsed_, false);
    canvas.pop_clip();
    canvas.restore();
}

void SearchBar::paint_expanded(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!showing_view()) {
        return;
    }
    const render::Rect s = surface();
    const render::CornerRadii radii = render::uniform_radii(std::min(corner_radius(), std::min(s.width, s.height) / 2.0));
    canvas.save();
    canvas.push_clip(area_);
    paint_surface(canvas, s, radii, scheme[t().container], t().shadow_elevation, scheme[color::Role::shadow]);
    canvas.push_clip(render::rounded_rect_path(s, radii));
    paint_field(canvas, scheme, field_rect(), true);
    const double alpha = full_screen() ? progress() : 1.0;
    const color::ColorScheme faded = color::faded(scheme, alpha);
    const render::Rect list = results_rect();
    canvas.fill_rect({s.x, list.y - current_style().divider.thickness, s.width, current_style().divider.thickness}, faded[t().divider]);
    results_.paint(canvas, faded);
    canvas.pop_clip();
    canvas.pop_clip();
    canvas.restore();
}

std::optional<AccessibleNode> SearchBar::accessible() const {
    if (!showing_view()) {
        AccessibleNode node{AccessibleRole::group, {}, collapsed_};
        node.children.push_back(field_.accessible());
        if (trailing_) {
            node.children.push_back(trailing_->accessible());
        }
        return node;
    }
    AccessibleNode node{AccessibleRole::dialog, options_.accessible_name, surface(), {.modal = full_screen()}};
    node.children.push_back(back_->accessible());
    AccessibleNode field = field_.accessible();
    field.description = options_.strings->suggestions_below;
    node.children.push_back(std::move(field));
    if (trailing_) {
        node.children.push_back(trailing_->accessible());
    }
    node.children.push_back(results_.accessible());
    return node;
}

}
