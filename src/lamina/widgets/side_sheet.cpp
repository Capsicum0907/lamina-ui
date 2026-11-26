#include "lamina/style/style.hpp"
#include "lamina/widgets/side_sheet.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/divider.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/scrim.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

const tokens::SideSheetTokens& t() {
    return current_style().side_sheet;
}

constexpr input::PointerEvent left{input::PointerAction::leave, 0.0, 0.0};

motion::DragThresholds side_thresholds() {
    return {[](double distance) { return distance * t().hide_fraction; }, t().significant_velocity};
}

std::unique_ptr<IconButton> header_button(const std::optional<SideSheetIcon>& spec) {
    if (!spec) {
        return nullptr;
    }
    return std::make_unique<IconButton>(IconButtonOptions{.icon = spec->icon,
                                                          .accessible_name = spec->accessible_name,
                                                          .style = tokens::IconButtonStyle::standard,
                                                          .size = t().header_button_size,
                                                          .content = t().header_icon});
}

}

SideSheet::SideSheet(SideSheetOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      headline_(text::layout_line(fonts, options_.headline, current_style().type.text(t().headline_type))),
      close_(header_button(options_.close)),
      back_(header_button(options_.back)),
      content_(options_.content),
      drag_(options_.initial, side_thresholds(), current_style().gesture.drag) {
    if (options_.accessible_name.empty()) {
        options_.accessible_name = options_.headline.empty() ? options_.strings->side_sheet_pane_title : options_.headline;
    }
    for (ButtonOptions o : options_.actions) {
        o.size = t().action_size;
        actions_.push_back(std::make_unique<Button>(std::move(o), fonts));
    }
    if (back_) {
        header_focus_.add(*back_);
    }
    if (close_) {
        header_focus_.add(*close_);
    }
    for (auto& a : actions_) {
        action_focus_.add(*a);
    }
    update_anchors();
}

template <typename F>
void SideSheet::each_button(F&& f) {
    if (back_) {
        f(static_cast<IconButton&>(*back_));
    }
    if (close_) {
        f(static_cast<IconButton&>(*close_));
    }
    for (auto& a : actions_) {
        f(*a);
    }
}

void SideSheet::add_header_stops(FocusManager& focus) {
    if (back_) {
        focus.add(*back_);
    }
    if (close_) {
        focus.add(*close_);
    }
}

void SideSheet::add_action_stops(FocusManager& focus) {
    for (auto& a : actions_) {
        focus.add(*a);
    }
}

double SideSheet::travel() const {
    return t().width + margin();
}

double SideSheet::progress() const {
    return std::clamp(1.0 - std::abs(drag_.offset()) / travel(), 0.0, 1.0);
}

double SideSheet::occupied_width() const {
    return (t().width + 2.0 * margin()) * progress();
}

void SideSheet::set_area(const render::Rect& area) {
    area_ = area;
    update_anchors();
    place();
}

layout::Size SideSheet::measure(const layout::Constraints& c) const {
    return c.constrain({occupied_width(), std::isfinite(c.max_height) ? c.max_height : c.min_height});
}

void SideSheet::arrange(const render::Rect& rect) {
    set_area(rect);
}

void SideSheet::update_anchors() {
    drag_.set_anchors({{SideSheetValue::expanded, 0.0}, {SideSheetValue::hidden, sign() * travel()}}, drag_.target_value());
}

render::Rect SideSheet::open_rect() const {
    const double m = margin();
    const double x = options_.edge == SideSheetEdge::end ? area_.x + area_.width - m - t().width : area_.x + m;
    return {x, area_.y + m, t().width, std::max(0.0, area_.height - 2.0 * m)};
}

render::Rect SideSheet::bounds() const {
    render::Rect r = open_rect();
    r.x += drag_.offset();
    return r;
}

double SideSheet::actions_height() const {
    if (actions_.empty()) {
        return 0.0;
    }
    double h = 0.0;
    for (const auto& a : actions_) {
        h = std::max(h, a->height());
    }
    return current_style().divider.thickness + 2.0 * t().actions_padding + h;
}

void SideSheet::place() {
    const render::Rect s = bounds();
    double end = s.x + s.width - t().header_button_padding;
    if (close_) {
        close_->set_origin({end - close_->width(), s.y + (t().header_height - close_->height()) / 2.0});
    }
    if (back_) {
        back_->set_origin({s.x + t().header_button_padding, s.y + (t().header_height - back_->height()) / 2.0});
    }
    const double actions_top = s.y + s.height - actions_height();
    double x = s.x + t().padding;
    for (auto& a : actions_) {
        a->set_origin({x, actions_top + current_style().divider.thickness + t().actions_padding});
        x += a->width() + t().action_space;
    }
    content_.place({s.x + t().padding, s.y + t().header_height, std::max(0.0, s.width - 2.0 * t().padding),
                    std::max(0.0, actions_top - (s.y + t().header_height))});
}

void SideSheet::open(input::InputSource source) {
    if (current_value() == SideSheetValue::hidden && drag_.idle()) {
        content_.reset_scroll();
    }
    drag_.animate_to(SideSheetValue::expanded, current_style().side_sheet_motion, 0.0);
    if (source == input::InputSource::keyboard && modal()) {
        set_focused(false);
        tab(false);
    }
}

void SideSheet::close() {
    drag_.animate_to(SideSheetValue::hidden, current_style().side_sheet_motion, 0.0);
    cancel_interaction();
}

void SideSheet::snap_to(SideSheetValue value) {
    drag_.snap_to(value);
    place();
}

bool SideSheet::visible() const {
    return drag_.positioned() && (drag_.target_value() == SideSheetValue::expanded || !drag_.idle() || progress() > 0.0);
}

bool SideSheet::focused() const {
    return header_focus_.has_focus() || content_.focused() || action_focus_.has_focus();
}

void SideSheet::set_focused(bool focused) {
    if (!focused) {
        header_focus_.clear_focus();
        content_.clear_focus();
        action_focus_.clear_focus();
        return;
    }
    if (!this->focused()) {
        tab(false);
    }
}

void SideSheet::set_enabled(bool enabled) {
    enabled_ = enabled;
    each_button([enabled](auto& b) { b.set_enabled(enabled); });
}

void SideSheet::tab(bool backward) {
    std::vector<FocusManager*> parts{&header_focus_};
    if (content_.focus()) {
        parts.push_back(content_.focus());
    }
    parts.push_back(&action_focus_);
    chain_focus(parts, backward ? FocusDirection::previous : FocusDirection::next, true);
    content_.reveal_focus();
}

SideSheetValue SideSheet::release_target(double velocity) const {
    const bool outwards = velocity * sign() < 0.0;
    if (velocity != 0.0 && outwards) {
        return SideSheetValue::expanded;
    }
    const bool significant = std::abs(velocity) > t().significant_velocity;
    const bool past_half = std::abs(drag_.offset()) > travel() * t().hide_fraction;
    return significant || past_half ? SideSheetValue::hidden : SideSheetValue::expanded;
}

SideSheetResponse SideSheet::dismiss() {
    close();
    return {.dismissed = true, .consumed = true};
}

SideSheetResponse SideSheet::handle(const input::PointerEvent& e) {
    SideSheetResponse r;
    if (!visible()) {
        return r;
    }
    const bool in_area = e.action != input::PointerAction::leave && contains(area_, e.x_dp, e.y_dp);
    const bool in_sheet = in_area && contains(bounds(), e.x_dp, e.y_dp);
    const bool blocks = modal() ? in_area : in_sheet;
    auto buttons = [&](const input::PointerEvent& ev) {
        if (back_ && back_->handle(ev)) {
            r.back = true;
        }
        if (close_ && close_->handle(ev)) {
            r = dismiss();
        }
        for (std::size_t i = 0; i < actions_.size(); ++i) {
            if (actions_[i]->handle(ev)) {
                r.action = i;
            }
        }
    };
    switch (e.action) {
    case input::PointerAction::down:
        scrim_pressed_ = modal() && in_area && !in_sheet;
        tracking_ = false;
        content_claimed_ = false;
        if (in_sheet && options_.gestures) {
            drag_.press(e.x_dp, e.time_seconds);
            tracking_ = true;
        }
        buttons(e);
        content_claimed_ = content_.handle(e);
        r.consumed = blocks;
        return r;
    case input::PointerAction::move:
        if (tracking_ && !content_claimed_) {
            const bool was = drag_.dragging();
            if (drag_.move(e.x_dp, e.time_seconds)) {
                if (!was) {
                    each_button([](auto& b) { b.cancel_interaction(); });
                    content_.handle(left);
                    content_.cancel_interaction();
                }
                place();
                r.consumed = true;
                return r;
            }
        }
        buttons(e);
        content_claimed_ = content_.handle(e) && tracking_;
        r.consumed = blocks;
        return r;
    case input::PointerAction::up: {
        tracking_ = false;
        if (drag_.dragging()) {
            const std::optional<double> v = drag_.release(e.time_seconds);
            const SideSheetValue target = release_target(v.value_or(0.0));
            drag_.animate_to(target, current_style().side_sheet_motion);
            r.dismissed = target == SideSheetValue::hidden;
            r.consumed = true;
            return r;
        }
        drag_.cancel_press();
        if (scrim_pressed_ && in_area && !in_sheet) {
            scrim_pressed_ = false;
            if (options_.dismiss_on_scrim) {
                return dismiss();
            }
            r.consumed = true;
            return r;
        }
        scrim_pressed_ = false;
        buttons(e);
        content_.handle(e);
        r.consumed = r.consumed || blocks;
        return r;
    }
    case input::PointerAction::leave:
        tracking_ = false;
        if (drag_.dragging()) {
            drag_.release(e.time_seconds);
            drag_.animate_to(release_target(0.0), current_style().side_sheet_motion);
        }
        drag_.cancel_press();
        buttons(e);
        content_.handle(e);
        return r;
    }
    return r;
}

SideSheetResponse SideSheet::handle(const input::KeyEvent& e) {
    SideSheetResponse r;
    if (!visible()) {
        return r;
    }
    if (modal()) {
        if (e.key == input::Key::escape) {
            return dismiss();
        }
        if (e.key == input::Key::tab && !e.ctrl) {
            tab(e.shift);
            r.consumed = true;
            return r;
        }
    }
    if (back_ && back_->handle(e)) {
        r.back = true;
        r.consumed = true;
        return r;
    }
    if (close_ && close_->handle(e)) {
        return dismiss();
    }
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        if (actions_[i]->handle(e)) {
            r.action = i;
            r.consumed = true;
            return r;
        }
    }
    r.consumed = content_.handle(e) || modal();
    return r;
}

bool SideSheet::handle(const input::TextEvent& e) {
    return visible() && content_.handle(e);
}

bool SideSheet::handle(const input::WheelEvent& e) {
    if (!visible()) {
        return false;
    }
    if (contains(bounds(), e.x_dp, e.y_dp)) {
        content_.handle(e);
        return true;
    }
    return modal() && contains(area_, e.x_dp, e.y_dp);
}

void SideSheet::cancel_interaction() {
    each_button([](auto& b) { b.cancel_interaction(); });
    content_.cancel_interaction();
    if (drag_.dragging()) {
        drag_.cancel_press();
        drag_.animate_to(release_target(0.0), current_style().side_sheet_motion);
    }
    drag_.cancel_press();
    tracking_ = false;
    scrim_pressed_ = false;
}

void SideSheet::restyle() {
    style_stamp_.refresh();
    headline_ = text::layout_line(fonts_, options_.headline, current_style().type.text(t().headline_type));
    each_button([](auto& b) { b.restyle(); });
    content_.restyle();
    drag_.set_gesture(current_style().gesture.drag);
    update_anchors();
    place();
}

void SideSheet::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    drag_.step(dt);
    place();
    each_button([dt](auto& b) { b.step(dt); });
    content_.step(dt);
}

bool SideSheet::animating() const {
    bool buttons = (close_ && close_->animating()) || (back_ && back_->animating());
    for (const auto& a : actions_) {
        buttons = buttons || a->animating();
    }
    return !drag_.idle() || drag_.pressed() || buttons || content_.animating();
}

void SideSheet::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    canvas.save();
    canvas.push_clip(area_);
    if (modal()) {
        paint_scrim(canvas, area_, scheme, progress());
    }
    const render::Rect s = bounds();
    const double r = current_style().shape.radius(t().corner, s.width, s.height);
    render::CornerRadii radii = render::uniform_radii(options_.detached ? r : 0.0);
    if (modal() && !options_.detached) {
        radii = options_.edge == SideSheetEdge::end ? render::CornerRadii{r, 0.0, 0.0, r} : render::CornerRadii{0.0, r, r, 0.0};
    }
    paint_container(canvas, s, radii, scheme, modal() ? t().modal_container : t().standard_container, modal() ? t().modal_elevation : t().standard_elevation,
                    modal() ? SurfaceLayer::overlay : SurfaceLayer::base);
    canvas.push_clip(render::rounded_rect_path(s, radii));
    if (!options_.headline.empty()) {
        const double x = back_ ? back_->bounds().x + back_->width() + t().back_headline_space : s.x + t().padding;
        const double limit = (close_ ? close_->bounds().x : s.x + s.width - t().padding) - x;
        canvas.push_clip({x, s.y, std::max(0.0, limit), t().header_height});
        text::draw_line(canvas, fonts_, headline_, {x, s.y + (t().header_height - headline_.box_height()) / 2.0}, scheme[t().headline]);
        canvas.pop_clip();
    }
    if (back_) {
        back_->paint(canvas, scheme);
    }
    if (close_) {
        close_->paint(canvas, scheme);
    }
    content_.paint(canvas, scheme);
    if (!actions_.empty()) {
        const double top = s.y + s.height - actions_height();
        canvas.fill_rect({s.x, top, s.width, current_style().divider.thickness}, scheme[t().divider]);
        for (const auto& a : actions_) {
            a->paint(canvas, scheme);
        }
    }
    canvas.pop_clip();
    canvas.pop_clip();
    canvas.restore();
}

std::optional<AccessibleNode> SideSheet::accessible() const {
    if (!visible()) {
        return std::nullopt;
    }
    AccessibleNode node{modal() ? AccessibleRole::dialog : AccessibleRole::group, options_.accessible_name, bounds(), {.modal = modal()}};
    if (back_) {
        node.children.push_back(back_->accessible());
    }
    if (!options_.headline.empty()) {
        node.children.push_back({AccessibleRole::heading, options_.headline, {bounds().x, bounds().y, bounds().width, t().header_height}});
    }
    if (close_) {
        node.children.push_back(close_->accessible());
    }
    content_.collect(node.children);
    for (const auto& a : actions_) {
        node.children.push_back(a->accessible());
    }
    if (modal() && options_.dismiss_on_scrim) {
        node.children.push_back({AccessibleRole::button, options_.strings->close_sheet, area_});
    }
    return node;
}

}
