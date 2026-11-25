#include "lamina/style/style.hpp"
#include "lamina/widgets/list_view.hpp"

#include <algorithm>

#include "lamina/widgets/item_focus.hpp"

namespace lamina::widgets {

namespace {

const tokens::ListTokens& t() {
    return current_style().list;
}

}

ListView::ListView(ListViewOptions options, const text::FontStack& fonts)
    : fonts_(fonts),
      single_selection_(options.single_selection),
      accessible_name_(std::move(options.accessible_name)),
      top_inset_(options.top_inset),
      density_(options.density),
      dividers_(options.dividers) {
    set_items(std::move(options.items));
}

void ListView::set_density(tokens::Density density) {
    if (density == density_) {
        return;
    }
    density_ = density;
    for (std::size_t i = 0; i < specs_.size(); ++i) {
        specs_[i].density = density;
        heights_[i] = list_item_estimate(specs_[i]);
        measured_width_[i] = -1.0;
    }
    for (auto& [index, row] : live_) {
        row->set_density(density);
    }
    tops_dirty_ = true;
    scroll_.set_content_height(content_height());
    update_rows();
}

void ListView::set_items(std::vector<ListItemSpec> items) {
    specs_ = std::move(items);
    for (ListItemSpec& spec : specs_) {
        spec.density = density_;
    }
    live_.clear();
    pressed_.reset();
    heights_.assign(specs_.size(), 0.0);
    measured_width_.assign(specs_.size(), -1.0);
    for (std::size_t i = 0; i < specs_.size(); ++i) {
        heights_[i] = list_item_estimate(specs_[i]);
    }
    tops_dirty_ = true;
    focus_index_ = std::min(focus_index_, specs_.empty() ? 0 : specs_.size() - 1);
    scroll_.set_content_height(content_height());
    update_rows();
}

void ListView::set_top_inset(double inset) {
    top_inset_ = inset;
    tops_dirty_ = true;
    scroll_.set_content_height(content_height());
    update_rows();
}

void ListView::rebuild_tops() const {
    if (!tops_dirty_) {
        return;
    }
    tops_.assign(heights_.size() + 1, top_inset_);
    for (std::size_t i = 0; i < heights_.size(); ++i) {
        tops_[i + 1] = tops_[i] + heights_[i];
    }
    tops_dirty_ = false;
}

double ListView::content_height() const {
    rebuild_tops();
    return tops_.back();
}

std::size_t ListView::row_for_offset(double y) const {
    rebuild_tops();
    if (specs_.empty()) {
        return 0;
    }
    const auto it = std::upper_bound(tops_.begin(), tops_.end(), y);
    const std::size_t i = it == tops_.begin() ? 0 : static_cast<std::size_t>(it - tops_.begin()) - 1;
    return std::min(i, specs_.size() - 1);
}

std::pair<std::size_t, std::size_t> ListView::visible_rows() const {
    if (specs_.empty()) {
        return {0, 0};
    }
    const double top = scroll_.offset();
    const std::size_t first = row_for_offset(top);
    const std::size_t last = row_for_offset(top + std::max(0.0, scroll_.viewport().height - 1e-6)) + 1;
    const std::size_t from = first > t().overscan_rows ? first - t().overscan_rows : 0;
    const std::size_t to = std::min(specs_.size(), last + t().overscan_rows);
    return {from, to};
}

render::Rect ListView::row_rect(std::size_t index) const {
    rebuild_tops();
    return scroll_.to_window({0.0, tops_[index], scroll_.viewport().width, heights_[index]});
}

std::optional<std::size_t> ListView::row_at(double x, double y) const {
    if (!contains(scroll_.viewport(), x, y) || specs_.empty()) {
        return std::nullopt;
    }
    return row_for_offset(y - scroll_.viewport().y + scroll_.offset());
}

bool ListView::row_enabled(std::size_t index) const {
    return enabled_ && specs_[index].enabled && specs_[index].action != ListItemAction::none;
}

ListItem& ListView::live(std::size_t index) {
    auto it = live_.find(index);
    if (it == live_.end()) {
        ListItemSpec spec = specs_[index];
        spec.enabled = spec.enabled && enabled_;
        it = live_.emplace(index, std::make_unique<ListItem>(std::move(spec), fonts_)).first;
        if (focus_.focused() && index == focus_index_) {
            it->second->set_focused(true);
        }
    }
    return *it->second;
}

void ListView::update_rows() {
    stats_.measured_rows = 0;
    const double width = scroll_.viewport().width;
    for (int pass = 0; pass < 2; ++pass) {
        const auto [from, to] = visible_rows();
        bool changed = false;
        for (std::size_t i = from; i < to; ++i) {
            ListItem& row = live(i);
            if (measured_width_[i] != width) {
                measured_width_[i] = width;
                ++stats_.measured_rows;
                const double h = row.height_for(width);
                if (h != heights_[i]) {
                    heights_[i] = h;
                    tops_dirty_ = true;
                    changed = true;
                }
            }
        }
        if (!changed) {
            break;
        }
        scroll_.set_content_height(content_height());
    }
    rebuild_tops();
    visible_ = visible_rows();
    for (std::size_t i = visible_.first; i < visible_.second; ++i) {
        live(i).arrange({0.0, tops_[i], width, heights_[i]});
    }
    for (auto it = live_.begin(); it != live_.end();) {
        const std::size_t i = it->first;
        const ListItem& row = *it->second;
        const bool keep = (i >= visible_.first && i < visible_.second) || (focus_.focused() && i == focus_index_) || pressed_ == i ||
                          row.hovered() || row.animating();
        it = keep ? std::next(it) : live_.erase(it);
    }
    stats_.live_rows = live_.size();
}

void ListView::set_viewport(const render::Rect& viewport) {
    scroll_.set_viewport(viewport);
    scroll_.set_content_height(content_height());
    update_rows();
}

void ListView::set_selected(std::size_t index, bool selected) {
    if (index >= specs_.size()) {
        return;
    }
    specs_[index].selected = selected && specs_[index].action != ListItemAction::none;
    if (const auto it = live_.find(index); it != live_.end()) {
        it->second->set_selected(selected);
    }
}

void ListView::set_item_enabled(std::size_t index, bool enabled) {
    if (index >= specs_.size()) {
        return;
    }
    specs_[index].enabled = enabled;
    if (const auto it = live_.find(index); it != live_.end()) {
        it->second->set_enabled(enabled && enabled_);
    }
    if (focus_.focused() && index == focus_index_ && !enabled) {
        set_focused(false);
    }
}

bool ListView::can_focus() const {
    for (std::size_t i = 0; i < specs_.size(); ++i) {
        if (row_enabled(i)) {
            return true;
        }
    }
    return false;
}

void ListView::set_focused(bool focused) {
    focused = focused && can_focus();
    if (focused && !focus_.focused()) {
        if (focus_index_ >= specs_.size() || !row_enabled(focus_index_)) {
            focus_index_ = first_enabled_item(specs_.size(), [this](std::size_t i) { return row_enabled(i); }).value_or(0);
        }
    }
    focus_.set_focused(focused);
    if (const auto it = live_.find(focus_index_); it != live_.end()) {
        it->second->set_focused(focused);
    }
    if (focused) {
        focus_row(focus_index_);
    }
}

std::optional<std::size_t> ListView::focused_row() const {
    return focus_.focused() ? std::optional<std::size_t>(focus_index_) : std::nullopt;
}

void ListView::set_enabled(bool enabled) {
    enabled_ = enabled;
    for (auto& [i, row] : live_) {
        row->set_enabled(enabled && specs_[i].enabled);
    }
    if (!can_focus()) {
        focus_.set_focused(false);
    }
}

void ListView::focus_row(std::size_t index) {
    if (index >= specs_.size()) {
        return;
    }
    if (index != focus_index_) {
        if (const auto it = live_.find(focus_index_); it != live_.end()) {
            it->second->set_focused(false);
        }
    }
    focus_index_ = index;
    rebuild_tops();
    scroll_.ensure_visible(tops_[index], tops_[index] + heights_[index]);
    live(index).set_focused(focus_.focused());
    update_rows();
}

AccessibleNode ListView::accessible() const {
    AccessibleNode node{AccessibleRole::list, accessible_name_, scroll_.viewport(),
                        {.enabled = enabled_, .focusable = can_focus(), .focused = focus_.focused()}};
    for (std::size_t i = visible_.first; i < visible_.second; ++i) {
        const auto it = live_.find(i);
        if (it == live_.end()) {
            continue;
        }
        AccessibleNode row = it->second->accessible();
        row.bounds = scroll_.to_window(row.bounds);
        node.children.push_back(std::move(row));
    }
    return node;
}

void ListView::after_click(std::size_t index, ListViewResponse& response) {
    response.clicked = index;
    response.consumed = true;
    specs_[index].selected = live(index).selected();
    if (single_selection_ && specs_[index].selected) {
        for (std::size_t i = 0; i < specs_.size(); ++i) {
            if (i != index && specs_[i].selected && specs_[i].action == ListItemAction::select) {
                set_selected(i, false);
            }
        }
    }
}

ListViewResponse ListView::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    if (e.action == input::PointerAction::down) {
        if (const auto it = live_.find(focus_index_); it != live_.end()) {
            it->second->set_focused(false);
        }
    }
    ListViewResponse response;
    std::vector<std::size_t> clicked;
    const bool claimed = scroll_.route(e, [&](const input::PointerEvent& pe) {
        const input::PointerEvent content = scroll_.to_content(pe);
        for (auto& [i, row] : live_) {
            if (row->handle(content).clicked) {
                clicked.push_back(i);
            }
            if (row->pressed()) {
                pressed_ = i;
            }
        }
        if (pe.action == input::PointerAction::up || pe.action == input::PointerAction::leave) {
            pressed_.reset();
        }
    });
    if (claimed) {
        response.consumed = true;
        pressed_.reset();
        return response;
    }
    for (std::size_t i : clicked) {
        after_click(i, response);
    }
    return response;
}

ListViewResponse ListView::handle(const input::KeyEvent& e) {
    ListViewResponse response;
    if (!focus_.focused() || specs_.empty()) {
        return response;
    }
    auto enabled = [this](std::size_t i) { return row_enabled(i); };
    if (e.key == input::Key::page_down || e.key == input::Key::page_up) {
        rebuild_tops();
        const double step = scroll_.viewport().height - current_style().scroll_area.page_overlap;
        const double y = tops_[focus_index_] + (e.key == input::Key::page_down ? step : -step);
        const std::size_t near = row_for_offset(std::clamp(y, 0.0, content_height()));
        const bool forward = e.key == input::Key::page_down;
        std::optional<std::size_t> target;
        for (std::size_t k = 0; k < specs_.size(); ++k) {
            const std::size_t i = forward ? std::min(specs_.size() - 1, near + k) : (near >= k ? near - k : 0);
            if (enabled(i)) {
                target = i;
                break;
            }
        }
        if (target) {
            focus_row(*target);
        }
        response.consumed = true;
        return response;
    }
    if (const std::optional<std::size_t> target = item_key_target(e, focus_index_, specs_.size(), ItemAxis::vertical, enabled)) {
        focus_row(*target);
        response.consumed = true;
        return response;
    }
    ListItem& row = live(focus_index_);
    if (row.handle(e).clicked) {
        after_click(focus_index_, response);
    }
    return response;
}

bool ListView::handle(const input::WheelEvent& e) {
    return scroll_.handle(e);
}

void ListView::cancel_interaction() {
    pressed_.reset();
    scroll_.cancel_interaction();
    for (auto& [i, row] : live_) {
        row->cancel_interaction();
    }
}

void ListView::step(double dt) {
    const double before = scroll_.offset();
    scroll_.step(dt);
    for (auto& [i, row] : live_) {
        row->step(dt);
    }
    update_rows();
    if (scroll_.offset() != before) {
        if (const std::optional<input::PointerEvent> hover = scroll_.rehover()) {
            for (auto& [i, row] : live_) {
                row->handle(*hover);
            }
        }
    }
}

bool ListView::animating() const {
    return scroll_.animating() || !focus_.settled() ||
           std::any_of(live_.begin(), live_.end(), [](const auto& entry) { return entry.second->animating(); });
}

void ListView::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    stats_.painted_rows = 0;
    const bool dividers = dividers_.value_or(current_style().list.row_dividers);
    scroll_.begin_content(canvas);
    for (std::size_t i = visible_.first; i < visible_.second; ++i) {
        const auto it = live_.find(i);
        if (it == live_.end()) {
            continue;
        }
        it->second->paint(canvas, scheme);
        ++stats_.painted_rows;
        if (dividers && i + 1 < specs_.size()) {
            const render::Rect r = it->second->bounds();
            const tokens::DividerTokens& d = current_style().divider;
            canvas.fill_rect({r.x, r.y + r.height - d.thickness, r.width, d.thickness}, scheme[d.color]);
        }
    }
    scroll_.end_content(canvas);
    scroll_.paint(canvas, scheme);
}

}
