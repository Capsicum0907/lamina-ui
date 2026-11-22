#include "lamina/widgets/segmented_button.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/widgets/item_focus.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::SegmentedButtonTokens& t = tokens::segmented_button_tokens;
constexpr const tokens::SegmentedButtonMotion& m = tokens::segmented_button_motion;

double icon_slot() {
    return t.icon_size + t.icon_spacing;
}

double hidden_offset() {
    return -icon_slot() / 2.0;
}

}

SegmentedButtonRow::SegmentedButtonRow(SegmentedButtonRowOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font)
    : options_(std::move(options)), fonts_(fonts), checkmark_{icon_font, t.checkmark}, enabled_(options_.enabled) {
    if (options_.segments.empty()) {
        throw std::invalid_argument("A segmented button row needs at least one segment");
    }
    const text::TextStyle style = tokens::text_style(tokens::type_style(t.label));
    bool seen_selected = false;
    segments_.reserve(options_.segments.size());
    for (Segment& spec : options_.segments) {
        if (spec.label.empty()) {
            if (!spec.icon) {
                throw std::invalid_argument("A segment needs a label or an icon");
            }
            spec.accessible_name = required_name(std::move(spec.accessible_name), "A segment with only an icon");
        }
        if (options_.selection == SegmentSelection::single && spec.checked) {
            spec.checked = !seen_selected;
            seen_selected = true;
        }
        const bool on = spec.checked;
        const bool shows_icon = on || (spec.icon && !spec.label.empty());
        text::TextLine line = text::layout_line(fonts_, spec.label, style);
        const bool allowed = spec.enabled;
        text::TextLine shown = line;
        segments_.push_back(Item{std::move(spec), std::move(line), std::move(shown), false, {}, motion::Animated(0.0, m.state_layer),
                                 motion::Animated(on ? 1.0 : 0.0, m.colors), motion::Animated(on ? 1.0 : 0.0, m.check_scale),
                                 motion::Animated(on ? 1.0 : 0.0, m.check_fade), motion::Animated(on ? 1.0 : 0.0, m.icon_crossfade),
                                 motion::Animated(shows_icon ? 0.0 : hidden_offset(), m.label_shift), Enablement(allowed && enabled_),
                                 allowed});
    }
    const double mark = text::layout_line(fonts_, tokens::ellipsis, style).width;
    double widest = t.min_width;
    double narrowest = t.min_width;
    for (const Item& item : segments_) {
        const double label = item.spec.label.empty() ? t.icon_size : item.label.width;
        const double least = item.spec.label.empty() ? t.icon_size : std::min(item.label.width, mark);
        widest = std::max(widest, 2.0 * t.horizontal_padding + icon_slot() + label);
        narrowest = std::max(narrowest, 2.0 * t.horizontal_padding + icon_slot() + least);
    }
    natural_segment_ = widest;
    min_segment_ = narrowest;
    segment_width_ = widest;
    fit_labels();
    double tallest = 0.0;
    for (const Item& item : segments_) {
        tallest = std::max(tallest, item.spec.label.empty() ? t.icon_size : item.label.box_height());
    }
    tallest_ = tallest;
    update_height();
}

void SegmentedButtonRow::update_height() {
    const tokens::DenseLength& l = tokens::density_tokens.segmented_button;
    const double removed = l.base - l.at(options_.density);
    height_ = std::max(l.at(options_.density), tallest_ + 2.0 * std::max(0.0, t.vertical_padding - removed / 2.0));
}

void SegmentedButtonRow::set_density(tokens::Density density) {
    options_.density = density;
    update_height();
}

double SegmentedButtonRow::row_width(double segment) const {
    return count() * segment - (count() - 1.0) * t.outline_width;
}

double SegmentedButtonRow::width() const {
    return row_width(segment_width_);
}

double SegmentedButtonRow::natural_width() const {
    return row_width(natural_segment_);
}

double SegmentedButtonRow::min_width() const {
    return row_width(min_segment_);
}

void SegmentedButtonRow::set_width(double width) {
    const double segment = (width + (count() - 1.0) * t.outline_width) / count();
    segment_width_ = std::max(width >= natural_width() ? natural_segment_ : min_segment_, segment);
    fit_labels();
}

layout::Size SegmentedButtonRow::measure(const layout::Constraints& c) const {
    const double room = std::isfinite(c.max_width) ? c.max_width : natural_width();
    const double w = options_.fill_width ? room : std::min(natural_width(), room);
    return {std::max(min_width(), w), height_};
}

void SegmentedButtonRow::arrange(const render::Rect& r) {
    set_width(options_.fill_width ? r.width : std::min(natural_width(), r.width));
    set_origin({r.x, r.y + (r.height - height_) / 2.0});
}

void SegmentedButtonRow::fit_labels() {
    const double room = segment_width_ - 2.0 * t.horizontal_padding - icon_slot();
    if (room == label_room_) {
        return;
    }
    label_room_ = room;
    const text::TextStyle style = tokens::text_style(tokens::type_style(t.label));
    for (Item& item : segments_) {
        const bool fits = item.spec.label.empty() || 2.0 * t.horizontal_padding + icon_slot() + item.label.width <= segment_width_;
        if (fits) {
            if (item.ellipsized) {
                item.shown = item.label;
                item.ellipsized = false;
            }
            continue;
        }
        item.shown = text::ellipsize_line(fonts_, item.spec.label, style, room, tokens::ellipsis);
        item.ellipsized = true;
    }
}

double SegmentedButtonRow::height() const {
    return height_;
}

render::Rect SegmentedButtonRow::segment_bounds(std::size_t index) const {
    return {origin_.x + static_cast<double>(index) * (segment_width_ - t.outline_width), origin_.y, segment_width_, height_};
}

const std::string& SegmentedButtonRow::segment_name(std::size_t index) const {
    const Segment& s = segments_[index].spec;
    return s.accessible_name.empty() ? s.label : s.accessible_name;
}

void SegmentedButtonRow::apply_checked(std::size_t index, bool checked) {
    Item& item = segments_[index];
    if (item.spec.checked == checked) {
        return;
    }
    item.spec.checked = checked;
    item.checked_amount.animate_to(checked ? 1.0 : 0.0);
    const bool crossfades = item.spec.icon && !item.spec.label.empty();
    if (crossfades) {
        item.icon_cross.animate_to(checked ? 1.0 : 0.0);
        return;
    }
    if (checked) {
        item.check_scale.snap_to(0.0);
        item.check_alpha.snap_to(0.0);
        item.check_scale.animate_to(1.0);
        item.check_alpha.animate_to(1.0);
        item.label_offset.animate_to(0.0);
    } else {
        item.check_scale.snap_to(0.0);
        item.check_alpha.snap_to(0.0);
        item.label_offset.animate_to(hidden_offset());
    }
}

void SegmentedButtonRow::set_checked(std::size_t index, bool checked) {
    if (options_.selection == SegmentSelection::single) {
        if (!checked) {
            apply_checked(index, false);
            return;
        }
        for (std::size_t i = 0; i < segments_.size(); ++i) {
            apply_checked(i, i == index);
        }
        return;
    }
    apply_checked(index, checked);
}

std::optional<std::size_t> SegmentedButtonRow::selected() const {
    for (std::size_t i = 0; i < segments_.size(); ++i) {
        if (segments_[i].spec.checked) {
            return i;
        }
    }
    return std::nullopt;
}

std::vector<std::size_t> SegmentedButtonRow::checked_segments() const {
    std::vector<std::size_t> out;
    for (std::size_t i = 0; i < segments_.size(); ++i) {
        if (segments_[i].spec.checked) {
            out.push_back(i);
        }
    }
    return out;
}

bool SegmentedButtonRow::can_focus() const {
    return enabled_ && any_item_enabled(segments_);
}

std::optional<std::size_t> SegmentedButtonRow::focused_segment() const {
    if (!focus_.focused()) {
        return std::nullopt;
    }
    return focus_index_;
}

void SegmentedButtonRow::set_focused(bool focused) {
    focused = focused && can_focus();
    if (focused && !focus_.focused()) {
        const auto enabled = [this](std::size_t i) { return segments_[i].enabled.enabled(); };
        const std::optional<std::size_t> sel = selected();
        if (options_.selection == SegmentSelection::single && sel && enabled(*sel)) {
            focus_index_ = *sel;
        } else if (!enabled(focus_index_)) {
            focus_index_ = first_enabled_item(segments_.size(), enabled).value_or(0);
        }
    }
    focus_.set_focused(focused);
    retarget();
}

void SegmentedButtonRow::set_enabled(bool enabled) {
    enabled_ = enabled;
    for (std::size_t i = 0; i < segments_.size(); ++i) {
        refresh_item(segments_, i, enabled_);
    }
    if (!can_focus()) {
        focus_.set_focused(false);
    } else if (!segments_[focus_index_].enabled.enabled()) {
        focus_index_ = first_enabled_item(segments_.size(), [this](std::size_t i) { return segments_[i].enabled.enabled(); }).value_or(0);
    }
    retarget();
}

void SegmentedButtonRow::set_segment_enabled(std::size_t index, bool enabled) {
    segments_[index].allowed = enabled;
    set_enabled(enabled_);
}

void SegmentedButtonRow::retarget() {
    retarget_state_layers(segments_, focused_segment());
}

AccessibleNode SegmentedButtonRow::accessible() const {
    AccessibleNode node{AccessibleRole::group, options_.accessible_name, bounds(),
                        {.enabled = enabled_, .focusable = can_focus(), .focused = focused()}};
    const bool single = options_.selection == SegmentSelection::single;
    for (std::size_t i = 0; i < segments_.size(); ++i) {
        const Item& item = segments_[i];
        AccessibleState state{.enabled = item.enabled.enabled(), .focused = focused_segment() == i};
        if (single) {
            state.selected = item.spec.checked;
        } else {
            state.toggle = item.spec.checked ? AccessibleToggle::on : AccessibleToggle::off;
        }
        node.children.push_back({single ? AccessibleRole::radio_button : AccessibleRole::toggle_button, segment_name(i), segment_bounds(i), state});
    }
    return node;
}

void SegmentedButtonRow::activate(std::size_t index, SegmentedButtonResponse& response) {
    response.clicked = index;
    response.consumed = true;
    if (options_.selection == SegmentSelection::single) {
        set_checked(index, true);
    } else {
        set_checked(index, !segments_[index].spec.checked);
    }
}

SegmentedButtonResponse SegmentedButtonRow::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    SegmentedButtonResponse response;
    for (std::size_t i = 0; i < segments_.size(); ++i) {
        Item& item = segments_[i];
        if (!item.enabled.enabled()) {
            continue;
        }
        render::Rect hit = segment_bounds(i);
        if (i + 1 < segments_.size()) {
            hit.width -= t.outline_width;
        }
        const render::Rect target = touch_target(hit, tokens::minimum_target(options_.density));
        if (item.press.update(e, {hit.x, target.y, hit.width, target.height})) {
            activate(i, response);
        }
        if (item.press.hovered) {
            response.consumed = true;
        }
    }
    retarget();
    return response;
}

SegmentedButtonResponse SegmentedButtonRow::handle(const input::KeyEvent& e) {
    SegmentedButtonResponse response;
    if (!focus_.focused() || !can_focus()) {
        return response;
    }
    const auto enabled = [this](std::size_t i) { return segments_[i].enabled.enabled(); };
    if (const std::optional<std::size_t> next = item_key_target(e, focus_index_, segments_.size(), ItemAxis::horizontal, enabled)) {
        focus_index_ = *next;
        response.consumed = true;
        if (options_.selection == SegmentSelection::single && !segments_[*next].spec.checked) {
            activate(*next, response);
        }
        retarget();
        return response;
    }
    if (focus_.activates(e, KeyActivation::space_or_enter)) {
        activate(focus_index_, response);
    }
    retarget();
    return response;
}

void SegmentedButtonRow::cancel_interaction() {
    cancel_presses(segments_);
    retarget();
}

void SegmentedButtonRow::step(double dt) {
    focus_.step(dt);
    for (Item& item : segments_) {
        item.state_layer.step(dt);
        item.checked_amount.step(dt);
        item.check_scale.step(dt);
        item.check_alpha.step(dt);
        item.icon_cross.step(dt);
        item.label_offset.step(dt);
        item.enabled.step(dt);
    }
}

bool SegmentedButtonRow::animating() const {
    if (!focus_.settled()) {
        return true;
    }
    return std::any_of(segments_.begin(), segments_.end(), [](const Item& i) {
        return !i.state_layer.settled() || !i.checked_amount.settled() || !i.check_scale.settled() || !i.check_alpha.settled() ||
               !i.icon_cross.settled() || !i.label_offset.settled() || !i.enabled.settled();
    });
}

int SegmentedButtonRow::z_of(std::size_t index) const {
    const Item& item = segments_[index];
    const int interactions = (item.press.pressed ? 1 : 0) + (focused_segment() == index ? 1 : 0);
    return interactions + (item.spec.checked ? t.checked_z : 0);
}

void SegmentedButtonRow::paint_segment(render::Canvas& canvas, const color::ColorScheme& scheme, std::size_t index) const {
    const Item& item = segments_[index];
    const render::Rect r = segment_bounds(index);
    const double full = tokens::corner_radius(t.corner, r.width, r.height);
    const bool first = index == 0;
    const bool last = index + 1 == segments_.size();
    const render::CornerRadii radii{first ? full : 0.0, last ? full : 0.0, last ? full : 0.0, first ? full : 0.0};
    const double k = std::clamp(item.checked_amount.value(), 0.0, 1.0);

    const color::Argb content =
        item.enabled.mix(resolve(scheme, t.disabled_content), color::mix(scheme[t.inactive_content], scheme[t.active_content], k));
    const color::Argb active_container =
        item.enabled.mix(scheme[t.disabled_active_container], scheme[t.active_container]);
    const color::Argb container = color::scale_alpha(active_container, k);
    const color::Argb filled = color::source_over(color::with_alpha(content, item.state_layer.value()), container);
    const render::Path shape = render::rounded_rect_path(r, radii);
    if (color::alpha(filled) != 0) {
        canvas.fill_path(shape, filled);
    }
    const color::Argb outline = item.enabled.mix(resolve(scheme, t.disabled_outline), scheme[t.outline]);
    render::Path ring = shape;
    const double w = t.outline_width;
    const render::Rect inner{r.x + w, r.y + w, r.width - 2.0 * w, r.height - 2.0 * w};
    auto inset = [w](double v) { return std::max(0.0, v - w); };
    ring.append(render::rounded_rect_path(inner, {inset(radii.top_left), inset(radii.top_right), inset(radii.bottom_right), inset(radii.bottom_left)}));
    canvas.fill_path(ring, outline, render::FillRule::even_odd);

    const double label_w = item.spec.label.empty() ? t.icon_size : item.shown.width;
    const double content_w = icon_slot() + label_w;
    const double cx = r.x + (r.width - content_w) / 2.0;
    const double icon_y = r.y + (r.height - t.icon_size) / 2.0;
    canvas.push_clip(shape);
    if (item.spec.icon && !item.spec.label.empty()) {
        const double c = std::clamp(item.icon_cross.value(), 0.0, 1.0);
        if (c < 1.0) {
            item.spec.icon->font.draw(canvas, item.spec.icon->symbol, {cx, icon_y}, t.icon_size, color::scale_alpha(content, 1.0 - c));
        }
        if (c > 0.0) {
            checkmark_.font.draw(canvas, checkmark_.symbol, {cx, icon_y}, t.icon_size, color::scale_alpha(content, c));
        }
    } else {
        const double s = std::max(0.0, item.check_scale.value());
        const double a = std::clamp(item.check_alpha.value(), 0.0, 1.0);
        if (s > 0.0 && a > 0.0) {
            canvas.save();
            canvas.translate(cx, icon_y + t.icon_size);
            canvas.scale(s);
            checkmark_.font.draw(canvas, checkmark_.symbol, {0.0, -t.icon_size}, t.icon_size, color::scale_alpha(content, a));
            canvas.restore();
        }
    }
    const double label_x = cx + icon_slot() + item.label_offset.value();
    if (item.spec.label.empty()) {
        item.spec.icon->font.draw(canvas, item.spec.icon->symbol, {label_x, icon_y}, t.icon_size, content);
    } else {
        text::draw_line(canvas, fonts_, item.shown, {label_x, r.y + (r.height - item.shown.box_height()) / 2.0}, content);
    }
    canvas.pop_clip();
    if (focused_segment() == index) {
        focus_.paint_inward(canvas, scheme, r, radii);
    }
}

void SegmentedButtonRow::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    std::vector<std::size_t> order(segments_.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [this](std::size_t a, std::size_t b) { return z_of(a) < z_of(b); });
    for (std::size_t i : order) {
        paint_segment(canvas, scheme, i);
    }
}

}
