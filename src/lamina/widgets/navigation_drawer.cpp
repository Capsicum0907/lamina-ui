#include "lamina/style/style.hpp"
#include "lamina/widgets/navigation_drawer.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/divider.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/widgets/item_focus.hpp"
#include "lamina/widgets/navigation_item.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/paint.hpp"
#include "lamina/widgets/scrim.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

const tokens::NavigationDrawerTokens& t() {
    return current_style().navigation_drawer;
}
const tokens::DrawerMotion& motion_spec() {
    return current_style().navigation_drawer_motion;
}

text::TextStyle style_of(tokens::TypeRole role) {
    return current_style().type.text(role);
}

constexpr input::PointerEvent left{input::PointerAction::leave, 0.0, 0.0};

motion::DragThresholds drawer_thresholds() {
    return {[](double distance) { return distance * t().positional_fraction; }, t().velocity_threshold};
}

double entry_height(DrawerEntryKind kind, tokens::Density density) {
    switch (kind) {
    case DrawerEntryKind::item:
        return current_style().density.drawer_item.at(density);
    case DrawerEntryKind::headline:
        return t().headline_height;
    case DrawerEntryKind::divider:
        return t().divider_top_space + current_style().divider.thickness + t().divider_bottom_space;
    }
    return 0.0;
}

}

DrawerSheet::DrawerSheet(DrawerSheetOptions options, const text::FontStack& fonts) : options_(std::move(options)), fonts_(fonts) {
    if (options_.accessible_name.empty()) {
        options_.accessible_name = options_.strings->navigation_menu;
    }
    for (std::size_t i = 0; i < options_.entries.size(); ++i) {
        const DrawerEntry& e = options_.entries[i];
        const tokens::TypeRole role = e.kind == DrawerEntryKind::headline ? t().headline_type : t().label_type;
        const bool selected = options_.selected == i && e.kind == DrawerEntryKind::item;
        Row row{text::layout_line(fonts_, e.label, style_of(role)),
                e.badge && !e.badge->text.empty() ? std::optional<text::TextLine>(text::layout_line(fonts_, e.badge->text, style_of(t().badge_type)))
                                                  : std::nullopt,
                0.0,
                entry_height(e.kind, options_.density),
                {},
                motion::Animated(0.0, current_style().motion.fast_effects()),
                motion::Animated(selected ? 1.0 : 0.0, motion_spec().colors),
                Enablement(e.enabled),
                e.enabled};
        rows_.push_back(std::move(row));
    }
    layout_rows();
    focus_index_ = focus_start();
}

void DrawerSheet::set_density(tokens::Density density) {
    options_.density = density;
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        rows_[i].height = entry_height(options_.entries[i].kind, density);
    }
    layout_rows();
}

void DrawerSheet::layout_rows() {
    double y = t().top_space;
    for (Row& r : rows_) {
        r.top = y;
        y += r.height;
    }
}

double DrawerSheet::content_height() const {
    return rows_.empty() ? t().top_space : rows_.back().top + rows_.back().height;
}

double DrawerSheet::width_for(double available) const {
    return std::min(t().max_width, std::max(std::min(t().min_width, available), available));
}

layout::Size DrawerSheet::measure(const layout::Constraints& c) const {
    const double w = std::isfinite(c.max_width) ? width_for(c.max_width) : t().max_width;
    const double h = std::isfinite(c.max_height) ? c.max_height : content_height();
    return c.constrain({w, h});
}

void DrawerSheet::arrange(const render::Rect& rect) {
    rect_ = rect;
    scroll_.set_viewport(rect);
    scroll_.set_content_height(content_height());
}

render::Rect DrawerSheet::entry_rect(std::size_t i) const {
    const Row& r = rows_[i];
    return scroll_.to_window({0.0, r.top, rect_.width, r.height});
}

render::Rect DrawerSheet::item_rect(std::size_t i) const {
    const render::Rect e = entry_rect(i);
    return {e.x + t().item_inset, e.y, e.width - 2.0 * t().item_inset, e.height};
}

void DrawerSheet::select(std::size_t index) {
    if (index >= rows_.size() || !is_item(index)) {
        return;
    }
    options_.selected = index;
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        rows_[i].selection.animate_to(i == index ? 1.0 : 0.0);
    }
}

bool DrawerSheet::item_enabled(std::size_t index) const {
    return index < rows_.size() && is_item(index) && rows_[index].enabled.enabled();
}

void DrawerSheet::set_item_enabled(std::size_t index, bool enabled) {
    if (index >= rows_.size() || !is_item(index)) {
        return;
    }
    rows_[index].allowed = enabled;
    options_.entries[index].enabled = enabled;
    refresh_item(rows_, index, enabled_);
    if (focus_index_ == index && !enabled) {
        focus_index_ = focus_start();
    }
    retarget();
}

void DrawerSheet::set_item_badge(std::size_t index, std::optional<BadgeSpec> badge) {
    if (index >= rows_.size()) {
        return;
    }
    options_.entries[index].badge = badge;
    rows_[index].badge = badge && !badge->text.empty()
                             ? std::optional<text::TextLine>(text::layout_line(fonts_, badge->text, style_of(t().badge_type)))
                             : std::nullopt;
}

void DrawerSheet::set_enabled(bool enabled) {
    enabled_ = enabled;
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (is_item(i)) {
            refresh_item(rows_, i, enabled);
        }
    }
    if (!can_focus()) {
        focus_.set_focused(false);
    }
    retarget();
}

bool DrawerSheet::can_focus() const {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (row_enabled(i)) {
            return true;
        }
    }
    return false;
}

std::size_t DrawerSheet::focus_start() const {
    if (options_.selected && *options_.selected < rows_.size() && row_enabled(*options_.selected)) {
        return *options_.selected;
    }
    return first_enabled_item(rows_.size(), [this](std::size_t i) { return row_enabled(i); }).value_or(0);
}

void DrawerSheet::set_focused(bool focused) {
    const bool on = focused && can_focus();
    if (on && !focus_.focused()) {
        focus_index_ = focus_start();
        reveal(focus_index_);
    }
    focus_.set_focused(on);
    retarget();
}

void DrawerSheet::retarget() {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        Row& r = rows_[i];
        const double pointer = row_enabled(i) ? r.press.state_layer_opacity() : 0.0;
        r.state_layer.animate_to(focused() && focus_index_ == i ? std::max(pointer, current_style().state_layer.focus) : pointer);
    }
}

void DrawerSheet::reveal(std::size_t index) {
    const Row& r = rows_[index];
    scroll_.ensure_visible(r.top, r.top + r.height);
}

std::optional<std::size_t> DrawerSheet::row_at(double x, double y) const {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (is_item(i) && contains(item_rect(i), x, y) && contains(rect_, x, y)) {
            return i;
        }
    }
    return std::nullopt;
}

AccessibleNode DrawerSheet::accessible() const {
    AccessibleNode node{AccessibleRole::navigation, options_.accessible_name, rect_,
                        {.enabled = enabled_ && can_focus(), .focusable = can_focus(), .focused = focused()}};
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const DrawerEntry& e = options_.entries[i];
        switch (e.kind) {
        case DrawerEntryKind::item:
            node.children.push_back({AccessibleRole::navigation_item,
                                     e.label,
                                     item_rect(i),
                                     {.enabled = row_enabled(i), .focused = focused() && focus_index_ == i, .selected = options_.selected == i},
                                     {},
                                     badge_description(e.badge)});
            break;
        case DrawerEntryKind::headline:
            node.children.push_back({AccessibleRole::heading, e.label, entry_rect(i)});
            break;
        case DrawerEntryKind::divider:
            node.children.push_back({AccessibleRole::separator, {}, entry_rect(i)});
            break;
        }
    }
    return node;
}

DrawerResponse DrawerSheet::handle(const input::PointerEvent& e) {
    DrawerResponse r;
    const bool claimed = scroll_.route(e, [&](const input::PointerEvent& pe) {
        if (pe.action == input::PointerAction::down && contains(rect_, pe.x_dp, pe.y_dp)) {
            focus_.set_focused(false);
        }
        const std::optional<std::size_t> under = pe.action == input::PointerAction::leave ? std::nullopt : row_at(pe.x_dp, pe.y_dp);
        for (std::size_t i = 0; i < rows_.size(); ++i) {
            if (!row_enabled(i)) {
                rows_[i].press = {};
                continue;
            }
            if (rows_[i].press.update(pe, under == i ? item_rect(i) : render::Rect{0.0, 0.0, 0.0, 0.0})) {
                select(i);
                r.selected = i;
            }
        }
        retarget();
    });
    if (claimed) {
        cancel_presses(rows_);
        retarget();
        r.consumed = true;
        return r;
    }
    r.consumed = contains(rect_, e.x_dp, e.y_dp) && e.action != input::PointerAction::leave;
    return r;
}

DrawerResponse DrawerSheet::handle(const input::KeyEvent& e) {
    DrawerResponse r;
    if (!focused()) {
        return r;
    }
    auto enabled = [this](std::size_t i) { return row_enabled(i); };
    if (const std::optional<std::size_t> next = item_key_target(e, focus_index_, rows_.size(), ItemAxis::vertical, enabled)) {
        focus_index_ = *next;
        reveal(focus_index_);
        retarget();
        r.consumed = true;
        return r;
    }
    if (activates(e, KeyActivation::space_or_enter) && row_enabled(focus_index_)) {
        select(focus_index_);
        r.selected = focus_index_;
        r.consumed = true;
    }
    return r;
}

bool DrawerSheet::handle(const input::WheelEvent& e) {
    return contains(rect_, e.x_dp, e.y_dp) && scroll_.handle(e);
}

void DrawerSheet::cancel_interaction() {
    cancel_presses(rows_);
    scroll_.cancel_interaction();
    retarget();
}

void DrawerSheet::restyle() {
    style_stamp_.refresh();
    focus_.restyle();
    scroll_.restyle();
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const DrawerEntry& e = options_.entries[i];
        Row& r = rows_[i];
        r.label = text::layout_line(fonts_, e.label, style_of(e.kind == DrawerEntryKind::headline ? t().headline_type : t().label_type));
        if (r.badge) {
            r.badge = text::layout_line(fonts_, e.badge->text, style_of(t().badge_type));
        }
        r.height = entry_height(e.kind, options_.density);
        r.state_layer.set_spec(current_style().motion.fast_effects());
        r.selection.set_spec(motion_spec().colors);
        r.enabled.restyle();
    }
    layout_rows();
    scroll_.set_content_height(content_height());
}

void DrawerSheet::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    for (Row& r : rows_) {
        r.state_layer.step(dt);
        r.selection.step(dt);
        r.enabled.step(dt);
    }
    focus_.step(dt);
    scroll_.step(dt);
}

bool DrawerSheet::animating() const {
    return !focus_.settled() || scroll_.animating() ||
           std::any_of(rows_.begin(), rows_.end(),
                       [](const Row& r) { return !r.state_layer.settled() || !r.selection.settled() || !r.enabled.settled(); });
}

void DrawerSheet::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    paint_container(canvas, scheme, rect_);
    paint_entries(canvas, scheme);
}

void DrawerSheet::paint_container(render::Canvas& canvas, const color::ColorScheme& scheme, const render::Rect& c) const {
    const bool modal = options_.style == DrawerSheetStyle::modal;
    const render::CornerRadii radii = modal ? current_style().shape.radii(t().modal_shape, c.width, c.height) : render::uniform_radii(0.0);
    widgets::paint_container(canvas, c, radii, scheme, modal ? t().modal_container : t().standard_container, modal ? t().modal_elevation : t().standard_elevation,
                    modal ? SurfaceLayer::overlay : SurfaceLayer::base);
}

void DrawerSheet::paint_entries(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    canvas.save();
    canvas.push_clip(rect_);
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const DrawerEntry& e = options_.entries[i];
        const Row& r = rows_[i];
        const render::Rect er = entry_rect(i);
        if (er.y > rect_.y + rect_.height || er.y + er.height < rect_.y) {
            continue;
        }
        if (e.kind == DrawerEntryKind::divider) {
            canvas.fill_rect({er.x + t().divider_inset, er.y + t().divider_top_space, er.width - 2.0 * t().divider_inset, current_style().divider.thickness},
                             scheme[current_style().divider.color]);
            continue;
        }
        if (e.kind == DrawerEntryKind::headline) {
            text::draw_line(canvas, fonts_, r.label, {er.x + t().headline_padding, er.y + (er.height - r.label.box_height()) / 2.0},
                            scheme[t().headline]);
            continue;
        }
        const render::Rect ir = item_rect(i);
        const double radius = current_style().shape.radius(t().item_shape, ir.width, ir.height);
        const render::CornerRadii radii = render::uniform_radii(radius);
        const double sel = std::clamp(r.selection.value(), 0.0, 1.0);
        if (sel > 0.0) {
            canvas.fill_path(render::rounded_rect_path(ir, radii), color::scale_alpha(scheme[t().selected_container], sel));
        }
        const double layer = std::clamp(r.state_layer.value(), 0.0, 1.0);
        if (layer > 0.0) {
            const color::Argb c = color::mix(scheme[t().unselected_state_layer], scheme[t().selected_state_layer], sel);
            canvas.fill_path(render::rounded_rect_path(ir, radii), color::with_alpha(c, layer));
        }
        const color::Argb disabled = resolve(scheme, t().disabled_content);
        auto content = [&](color::Role off, color::Role on) { return r.enabled.mix(disabled, color::mix(scheme[off], scheme[on], sel)); };
        double x = ir.x + t().item_padding_start;
        if (e.icon) {
            icons::draw_centered(canvas, *e.icon, render::Rect{x, ir.y + (ir.height - t().icon_size) / 2.0, t().icon_size, t().icon_size}, t().icon_size,
                                 content(t().unselected_icon, t().selected_icon));
            x += t().icon_size + t().icon_label_space;
        }
        double end = ir.x + ir.width - t().item_padding_end;
        if (r.badge) {
            text::draw_line(canvas, fonts_, *r.badge, {end - r.badge->width, ir.y + (ir.height - r.badge->box_height()) / 2.0},
                            content(t().unselected_badge, t().selected_badge));
            end -= r.badge->width + t().badge_space;
        }
        canvas.push_clip({x, ir.y, std::max(0.0, end - x), ir.height});
        text::draw_line(canvas, fonts_, r.label, {x, ir.y + (ir.height - r.label.box_height()) / 2.0}, content(t().unselected_label, t().selected_label));
        canvas.pop_clip();
        if (focused() && focus_index_ == i) {
            focus_.paint(canvas, scheme, ir, radius);
        }
    }
    canvas.pop_clip();
    canvas.restore();
    scroll_.paint(canvas, scheme);
}

DrawerSlide::DrawerSlide(DrawerValue initial) : drag_(initial, drawer_thresholds(), current_style().gesture.drag) {}

void DrawerSlide::restyle() {
    drag_.set_gesture(current_style().gesture.drag);
}

void DrawerSlide::set_width(double width) {
    width_ = width;
    drag_.set_anchors({{DrawerValue::closed, -width}, {DrawerValue::open, 0.0}}, drag_.target_value());
}

double DrawerSlide::fraction() const {
    if (width_ <= 0.0) {
        return 0.0;
    }
    return std::clamp((drag_.offset() + width_) / width_, 0.0, 1.0);
}

bool DrawerSlide::shown() const {
    return drag_.positioned() && (drag_.target_value() == DrawerValue::open || !drag_.idle() || fraction() > 0.0);
}

void DrawerSlide::open() {
    drag_.animate_to(DrawerValue::open, motion_spec().open);
}

void DrawerSlide::close() {
    drag_.animate_to(DrawerValue::closed, motion_spec().close);
}

DrawerValue DrawerSlide::release(double time_seconds) {
    const std::optional<double> v = drag_.release(time_seconds);
    return drag_.fling(v.value_or(0.0), motion_spec().settle);
}

void DrawerSlide::cancel() {
    if (drag_.dragging()) {
        drag_.cancel_press();
        drag_.fling(0.0, motion_spec().settle);
    }
    drag_.cancel_press();
}

namespace {

void paint_slid_sheet(render::Canvas& canvas, const color::ColorScheme& scheme, const DrawerSheet& sheet, const render::Rect& area) {
    const render::Rect b = sheet.bounds();
    const double stretch = std::max(0.0, b.x - area.x);
    sheet.paint_container(canvas, scheme, {b.x - stretch, b.y, b.width + stretch, b.height});
    sheet.paint_entries(canvas, scheme);
}

}

ModalNavigationDrawer::ModalNavigationDrawer(NavigationDrawerOptions options, const text::FontStack& fonts)
    : options_(std::move(options)), sheet_(options_.sheet, fonts), slide_(options_.initial) {}

void ModalNavigationDrawer::set_area(const render::Rect& area) {
    area_ = area;
    slide_.set_width(sheet_.width_for(area.width));
    place();
}

void ModalNavigationDrawer::place() {
    sheet_.arrange({area_.x + slide_.offset(), area_.y, slide_.width(), area_.height});
}

void ModalNavigationDrawer::open(input::InputSource source) {
    slide_.open();
    if (source == input::InputSource::keyboard) {
        sheet_.set_focused(true);
    }
}

void ModalNavigationDrawer::close() {
    slide_.close();
    sheet_.set_focused(false);
    sheet_.cancel_interaction();
}

void ModalNavigationDrawer::snap_to(DrawerValue value) {
    slide_.snap_to(value);
    place();
}

DrawerResponse ModalNavigationDrawer::handle(const input::PointerEvent& e) {
    DrawerResponse r;
    const bool in_area = e.action != input::PointerAction::leave && contains(area_, e.x_dp, e.y_dp);
    const bool shown = slide_.shown();
    const bool in_sheet = shown && in_area && contains(sheet_.bounds(), e.x_dp, e.y_dp);
    switch (e.action) {
    case input::PointerAction::down:
        scrim_pressed_ = shown && in_area && !in_sheet;
        tracking_ = false;
        if (in_area && options_.gestures) {
            slide_.press(e.x_dp, e.time_seconds);
            tracking_ = true;
        }
        if (shown) {
            r = sheet_.handle(e);
        }
        r.consumed = shown && in_area;
        return r;
    case input::PointerAction::move: {
        if (tracking_) {
            const bool was = slide_.dragging();
            if (slide_.move(e.x_dp, e.time_seconds)) {
                if (!was) {
                    sheet_.cancel_interaction();
                    scrim_pressed_ = false;
                    r.took_over = true;
                }
                place();
                r.consumed = true;
                return r;
            }
        }
        if (shown) {
            r = sheet_.handle(e);
        }
        r.consumed = shown && in_area;
        return r;
    }
    case input::PointerAction::up:
        tracking_ = false;
        if (slide_.dragging()) {
            r.closed = slide_.release(e.time_seconds) == DrawerValue::closed;
            if (r.closed) {
                sheet_.set_focused(false);
            }
            r.consumed = true;
            return r;
        }
        slide_.cancel();
        if (scrim_pressed_ && in_area && !in_sheet && slide_.current_value() == DrawerValue::open) {
            scrim_pressed_ = false;
            close();
            r.closed = true;
            r.consumed = true;
            return r;
        }
        scrim_pressed_ = false;
        if (shown) {
            r = sheet_.handle(e);
        }
        r.consumed = r.consumed || (shown && in_area);
        return r;
    case input::PointerAction::leave:
        tracking_ = false;
        slide_.cancel();
        sheet_.handle(left);
        return r;
    }
    return r;
}

DrawerResponse ModalNavigationDrawer::handle(const input::KeyEvent& e) {
    DrawerResponse r;
    if (!slide_.shown()) {
        return r;
    }
    r.consumed = true;
    if (e.key == input::Key::escape && slide_.current_value() == DrawerValue::open) {
        close();
        r.closed = true;
        return r;
    }
    if (e.key == input::Key::tab && !e.ctrl) {
        if (!sheet_.focused()) {
            sheet_.set_focused(true);
        }
        return r;
    }
    const DrawerResponse s = sheet_.handle(e);
    r.selected = s.selected;
    return r;
}

bool ModalNavigationDrawer::handle(const input::WheelEvent& e) {
    if (!slide_.shown()) {
        return false;
    }
    sheet_.handle(e);
    return contains(area_, e.x_dp, e.y_dp);
}

void ModalNavigationDrawer::cancel_interaction() {
    sheet_.cancel_interaction();
    slide_.cancel();
    tracking_ = false;
    scrim_pressed_ = false;
}

void ModalNavigationDrawer::restyle() {
    style_stamp_.refresh();
    sheet_.restyle();
    slide_.restyle();
    set_area(area_);
}

void ModalNavigationDrawer::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    slide_.step(dt);
    place();
    sheet_.step(dt);
}

bool ModalNavigationDrawer::animating() const {
    return slide_.animating() || slide_.pressed() || sheet_.animating();
}

double ModalNavigationDrawer::content_shift() const {
    const tokens::LayeringTokens& l = current_style().layering;
    return l.mode == tokens::Layering::recede ? slide_.fraction() * slide_.width() * l.drawer_push : 0.0;
}

void ModalNavigationDrawer::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!slide_.shown()) {
        return;
    }
    canvas.save();
    canvas.push_clip(area_);
    paint_scrim(canvas, area_, scheme, slide_.fraction());
    paint_slid_sheet(canvas, scheme, sheet_, area_);
    canvas.pop_clip();
    canvas.restore();
}

std::optional<AccessibleNode> ModalNavigationDrawer::accessible() const {
    if (!slide_.shown()) {
        return std::nullopt;
    }
    AccessibleNode node = sheet_.accessible();
    node.state.modal = true;
    if (slide_.current_value() == DrawerValue::open) {
        node.children.push_back({AccessibleRole::button, options_.sheet.strings->close_drawer, area_});
    }
    return node;
}

DismissibleNavigationDrawer::DismissibleNavigationDrawer(NavigationDrawerOptions options, layout::NodePtr content, const text::FontStack& fonts)
    : options_(std::move(options)), sheet_([&] {
          DrawerSheetOptions o = options_.sheet;
          o.style = DrawerSheetStyle::standard;
          return o;
      }(), fonts),
      slide_(options_.initial),
      content_(std::move(content)) {}

layout::Size DismissibleNavigationDrawer::measure(const layout::Constraints& c) {
    const double w = std::isfinite(c.max_width) ? c.max_width : c.min_width;
    const double h = std::isfinite(c.max_height) ? c.max_height : c.min_height;
    if (content_) {
        content_->measure({0.0, w, 0.0, h});
    }
    return c.constrain({w, h});
}

void DismissibleNavigationDrawer::arrange(const render::Rect& rect) {
    area_ = rect;
    slide_.set_width(sheet_.width_for(rect.width));
    place();
}

void DismissibleNavigationDrawer::place() {
    const double x = area_.x + slide_.offset();
    sheet_.arrange({x, area_.y, slide_.width(), area_.height});
    const render::Rect next{x + slide_.width(), area_.y, area_.width, area_.height};
    if (content_ && (next.x != content_rect_.x || next.width != content_rect_.width || next.height != content_rect_.height ||
                     next.y != content_rect_.y)) {
        content_->measure({next.width, next.width, next.height, next.height});
        content_->arrange(next);
    }
    content_rect_ = next;
}

void DismissibleNavigationDrawer::open(input::InputSource source) {
    slide_.open();
    if (source == input::InputSource::keyboard) {
        sheet_.set_focused(true);
    }
}

void DismissibleNavigationDrawer::close() {
    slide_.close();
    sheet_.set_focused(false);
    sheet_.cancel_interaction();
}

void DismissibleNavigationDrawer::snap_to(DrawerValue value) {
    slide_.snap_to(value);
    place();
}

DrawerResponse DismissibleNavigationDrawer::handle(const input::PointerEvent& e) {
    DrawerResponse r;
    const bool in_area = e.action != input::PointerAction::leave && contains(area_, e.x_dp, e.y_dp);
    const bool shown = slide_.shown();
    const bool in_sheet = shown && in_area && contains(sheet_.bounds(), e.x_dp, e.y_dp);
    switch (e.action) {
    case input::PointerAction::down:
        tracking_ = false;
        if (in_area && options_.gestures) {
            slide_.press(e.x_dp, e.time_seconds);
            tracking_ = true;
        }
        if (shown) {
            r = sheet_.handle(e);
        }
        r.consumed = in_sheet;
        return r;
    case input::PointerAction::move:
        if (tracking_) {
            const bool was = slide_.dragging();
            if (slide_.move(e.x_dp, e.time_seconds)) {
                if (!was) {
                    sheet_.cancel_interaction();
                    r.took_over = true;
                }
                place();
                r.consumed = true;
                return r;
            }
        }
        if (shown) {
            r = sheet_.handle(e);
        }
        r.consumed = in_sheet;
        return r;
    case input::PointerAction::up:
        tracking_ = false;
        if (slide_.dragging()) {
            r.closed = slide_.release(e.time_seconds) == DrawerValue::closed;
            if (r.closed) {
                sheet_.set_focused(false);
            }
            r.consumed = true;
            return r;
        }
        slide_.cancel();
        if (shown) {
            r = sheet_.handle(e);
        }
        r.consumed = r.consumed || in_sheet;
        return r;
    case input::PointerAction::leave:
        tracking_ = false;
        slide_.cancel();
        sheet_.handle(left);
        return r;
    }
    return r;
}

DrawerResponse DismissibleNavigationDrawer::handle(const input::KeyEvent& e) {
    DrawerResponse r;
    if (!sheet_.focused()) {
        return r;
    }
    if (e.key == input::Key::escape && slide_.current_value() == DrawerValue::open) {
        close();
        r.closed = true;
        r.consumed = true;
        return r;
    }
    return sheet_.handle(e);
}

bool DismissibleNavigationDrawer::handle(const input::WheelEvent& e) {
    return slide_.shown() && sheet_.handle(e);
}

void DismissibleNavigationDrawer::cancel_interaction() {
    sheet_.cancel_interaction();
    slide_.cancel();
    tracking_ = false;
}

void DismissibleNavigationDrawer::restyle() {
    style_stamp_.refresh();
    sheet_.restyle();
    slide_.restyle();
    arrange(area_);
}

void DismissibleNavigationDrawer::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    slide_.step(dt);
    place();
    sheet_.step(dt);
    if (!slide_.open_or_opening() && sheet_.focused()) {
        sheet_.set_focused(false);
    }
}

bool DismissibleNavigationDrawer::animating() const {
    return slide_.animating() || slide_.pressed() || sheet_.animating();
}

void DismissibleNavigationDrawer::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!slide_.shown()) {
        return;
    }
    canvas.save();
    canvas.push_clip(area_);
    paint_slid_sheet(canvas, scheme, sheet_, area_);
    canvas.pop_clip();
    canvas.restore();
}

std::optional<AccessibleNode> DismissibleNavigationDrawer::accessible() const {
    if (!slide_.shown()) {
        return std::nullopt;
    }
    return sheet_.accessible();
}

layout::NodePtr permanent_drawer(DrawerSheet& sheet, layout::NodePtr content) {
    auto row = layout::row({measured_node(sheet), layout::weighted(std::move(content), 1.0)});
    row->cross_align(layout::CrossAlign::stretch);
    return row;
}

}
