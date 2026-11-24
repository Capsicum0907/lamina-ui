#include "lamina/style/style.hpp"
#include "lamina/widgets/table.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "lamina/color/mix.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

namespace {

const tokens::TableTokens& t() {
    return current_style().table;
}

bool overlaps(double a0, double a1, double b0, double b1) {
    return a0 < b1 && b0 < a1;
}

}

Table::Table(TableOptions options, const text::FontStack& fonts)
    : fonts_(fonts),
      cell_(std::move(options.cell)),
      accessible_name_(required_name(std::move(options.accessible_name), "a table")),
      selection_(options.selection),
      icon_font_(std::move(options.icon_font)),
      density_(options.density) {
    if (!cell_) {
        throw std::invalid_argument("a table needs a cell function");
    }
    const auto style = tokens::text_style(tokens::type_style(t().header_label));
    for (TableColumn& c : options.columns) {
        if (c.sortable && !icon_font_) {
            throw std::invalid_argument("a table with a sortable column needs an icon font for the sort arrow");
        }
        columns_.push_back({c.title, text::layout_line(fonts_, c.title, style), std::max(c.width, c.min_width), c.min_width, c.align, c.sortable,
                            c.resizable, std::move(c.less), motion::Animated(0.0, t().state_layer_spring)});
    }
    set_row_count(options.row_count);
}

double Table::row_height() const {
    return tokens::density_tokens.table_row.at(density_);
}

double Table::header_height() const {
    return tokens::density_tokens.table_header.at(density_);
}

render::Rect Table::header_rect() const {
    return {bounds_.x, bounds_.y, bounds_.width, std::min(header_height(), bounds_.height)};
}

void Table::arrange(const render::Rect& r) {
    bounds_ = r;
    const double h = std::min(header_height(), r.height);
    scroll_.set_viewport({r.x, r.y + h, r.width, std::max(0.0, r.height - h)});
    update_content();
}

void Table::update_content() {
    scroll_.set_content_height(static_cast<double>(order_.size()) * row_height());
    scroll_.set_content_width(content_width());
}

void Table::set_row_count(std::size_t count) {
    order_.resize(count);
    std::iota(order_.begin(), order_.end(), std::size_t{0});
    rebuild_inverse();
    selected_.assign(count, false);
    selected_count_ = 0;
    sort_.reset();
    focus_model_ = 0;
    anchor_model_ = 0;
    rows_.clear();
    cache_.clear();
    hovered_model_.reset();
    update_content();
}

void Table::refresh() {
    cache_.clear();
    if (sort_) {
        sort_by(sort_->column, sort_->direction);
    }
}

void Table::rebuild_inverse() {
    inverse_.assign(order_.size(), 0);
    for (std::size_t v = 0; v < order_.size(); ++v) {
        inverse_[order_[v]] = v;
    }
}

double Table::column_left(std::size_t column) const {
    double x = 0.0;
    for (std::size_t c = 0; c < column; ++c) {
        x += columns_[c].width;
    }
    return x;
}

double Table::content_width() const {
    return column_left(columns_.size());
}

void Table::set_column_width(std::size_t column, double width) {
    columns_[column].width = std::max(columns_[column].min_width, width);
    update_content();
}

render::Rect Table::column_header_rect(std::size_t column) const {
    const render::Rect& v = scroll_.viewport();
    return {v.x + column_left(column) - scroll_.offset_x(), bounds_.y, columns_[column].width, header_rect().height};
}

render::Rect Table::row_rect(std::size_t view_row) const {
    const render::Rect& v = scroll_.viewport();
    return {v.x - scroll_.offset_x(), v.y + static_cast<double>(view_row) * row_height() - scroll_.offset(), std::max(content_width(), v.width),
            row_height()};
}

render::Rect Table::cell_rect(std::size_t view_row, std::size_t column) const {
    const render::Rect r = row_rect(view_row);
    return {scroll_.viewport().x + column_left(column) - scroll_.offset_x(), r.y, columns_[column].width, r.height};
}

std::optional<std::size_t> Table::row_at(double x, double y) const {
    const render::Rect& v = scroll_.viewport();
    if (!contains(v, x, y)) {
        return std::nullopt;
    }
    const double row = std::floor((y - v.y + scroll_.offset()) / row_height());
    if (row < 0.0 || row >= static_cast<double>(order_.size())) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(row);
}

std::optional<std::size_t> Table::column_at(double x) const {
    for (std::size_t c = 0; c < columns_.size(); ++c) {
        const render::Rect r = column_header_rect(c);
        if (x >= r.x && x < r.x + r.width) {
            return c;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> Table::divider_at(double x, double y) const {
    if (!contains(header_rect(), x, y)) {
        return std::nullopt;
    }
    for (std::size_t c = columns_.size(); c-- > 0;) {
        if (!columns_[c].resizable) {
            continue;
        }
        const render::Rect r = column_header_rect(c);
        if (std::abs(x - (r.x + r.width)) <= t().resize_hit_width / 2.0) {
            return c;
        }
    }
    return std::nullopt;
}

std::pair<std::size_t, std::size_t> Table::visible_rows() const {
    const std::size_t n = order_.size();
    if (n == 0) {
        return {0, 0};
    }
    const double h = row_height();
    const double top = scroll_.offset();
    const auto first = static_cast<std::size_t>(std::max(0.0, std::floor(top / h)));
    const auto last = static_cast<std::size_t>(std::max(0.0, std::ceil((top + scroll_.viewport().height) / h)));
    const std::size_t from = first > t().overscan_rows ? first - t().overscan_rows : 0;
    return {std::min(from, n), std::min(n, last + t().overscan_rows)};
}

void Table::sort_by(std::size_t column, SortDirection direction) {
    const Column& c = columns_[column];
    const auto less = [&](std::size_t a, std::size_t b) {
        if (c.less) {
            return c.less(a, b);
        }
        return cell_(a, column) < cell_(b, column);
    };
    std::iota(order_.begin(), order_.end(), std::size_t{0});
    if (direction == SortDirection::ascending) {
        std::stable_sort(order_.begin(), order_.end(), less);
    } else {
        std::stable_sort(order_.begin(), order_.end(), [&](std::size_t a, std::size_t b) { return less(b, a); });
    }
    rebuild_inverse();
    sort_ = TableSort{column, direction};
}

void Table::clear_sort() {
    std::iota(order_.begin(), order_.end(), std::size_t{0});
    rebuild_inverse();
    sort_.reset();
}

void Table::set_selected(std::size_t model_row, bool selected) {
    if (selected_[model_row] == selected) {
        return;
    }
    selected_[model_row] = selected;
    selected_count_ += selected ? 1 : 0;
    selected_count_ -= selected ? 0 : 1;
    retarget_rows();
}

std::vector<std::size_t> Table::selected_rows() const {
    std::vector<std::size_t> out;
    out.reserve(selected_count_);
    for (std::size_t v = 0; v < order_.size() && out.size() < selected_count_; ++v) {
        if (selected_[order_[v]]) {
            out.push_back(order_[v]);
        }
    }
    return out;
}

void Table::select_all() {
    if (selection_ != TableSelection::multiple) {
        return;
    }
    selected_.assign(order_.size(), true);
    selected_count_ = order_.size();
    retarget_rows();
}

void Table::clear_selection() {
    selected_.assign(order_.size(), false);
    selected_count_ = 0;
    retarget_rows();
}

void Table::select_only(std::size_t model_row) {
    if (selection_ == TableSelection::none) {
        return;
    }
    selected_.assign(order_.size(), false);
    selected_[model_row] = true;
    selected_count_ = 1;
    retarget_rows();
}

void Table::select_range(std::size_t from_view, std::size_t to_view, bool keep) {
    if (!keep) {
        selected_.assign(order_.size(), false);
        selected_count_ = 0;
    }
    for (std::size_t v = std::min(from_view, to_view); v <= std::max(from_view, to_view); ++v) {
        if (!selected_[order_[v]]) {
            selected_[order_[v]] = true;
            ++selected_count_;
        }
    }
    retarget_rows();
}

std::optional<std::size_t> Table::focused_row() const {
    if (order_.empty()) {
        return std::nullopt;
    }
    return view_row(focus_model_);
}

void Table::focus_row(std::size_t view_row) {
    focus_model_ = order_[view_row];
    reveal(view_row);
}

void Table::reveal(std::size_t view_row) {
    const double top = static_cast<double>(view_row) * row_height();
    scroll_.ensure_visible(top, top + row_height());
}

void Table::set_focused(bool focused) {
    focused_ = focused && can_focus();
    if (!focused_) {
        keyboard_ = false;
    } else if (!order_.empty()) {
        keyboard_ = true;
    }
    focus_.set_focused(focused_ && keyboard_);
}

void Table::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        cancel_interaction();
        set_focused(false);
    }
}

void Table::set_density(tokens::Density density) {
    density_ = density;
    arrange(bounds_);
}

bool Table::claims_key(const input::KeyEvent& e) const {
    return focused_ && enabled() && e.ctrl && !e.shift && !e.alt && e.key == input::Key::a && selection_ == TableSelection::multiple;
}

std::optional<input::Cursor> Table::cursor() const {
    if (resizing_ || divider_hover_) {
        return input::Cursor::resize_horizontal;
    }
    return std::nullopt;
}

Table::RowState& Table::row_state(std::size_t model_row) {
    auto it = rows_.find(model_row);
    if (it == rows_.end()) {
        const double on = selected_[model_row] ? 1.0 : 0.0;
        it = rows_.emplace(model_row, RowState{motion::Animated(on, t().color_spring), motion::Animated(0.0, t().state_layer_spring)}).first;
    }
    return it->second;
}

void Table::retarget_rows() {
    for (auto& [model, state] : rows_) {
        state.selected.animate_to(selected_[model] ? 1.0 : 0.0);
        state.state_layer.animate_to(hovered_model_ == model ? tokens::state_layer_opacity::hover : 0.0);
    }
}

std::size_t Table::page_rows() const {
    return std::max<std::size_t>(1, static_cast<std::size_t>(std::floor(scroll_.viewport().height / row_height())) - 1);
}

const std::vector<text::TextLine>& Table::cells(std::size_t model_row) const {
    auto it = cache_.find(model_row);
    if (it == cache_.end()) {
        const auto style = tokens::text_style(tokens::type_style(t().cell_label));
        std::vector<text::TextLine> lines;
        for (std::size_t c = 0; c < columns_.size(); ++c) {
            lines.push_back(text::layout_line(fonts_, cell_(model_row, c), style));
        }
        it = cache_.emplace(model_row, std::move(lines)).first;
    }
    return it->second;
}

void Table::move_focus(std::size_t view_row, const input::KeyEvent& e, TableResponse& response) {
    const std::size_t anchor_view = this->view_row(anchor_model_);
    focus_model_ = order_[view_row];
    if (selection_ == TableSelection::multiple && e.shift) {
        select_range(anchor_view, view_row, e.ctrl);
        response.selection_changed = true;
    } else if (selection_ == TableSelection::multiple && e.ctrl) {
    } else if (selection_ != TableSelection::none) {
        select_only(focus_model_);
        anchor_model_ = focus_model_;
        response.selection_changed = true;
    }
    reveal(view_row);
}

TableResponse Table::handle(const input::KeyEvent& e) {
    TableResponse response;
    if (!enabled() || !focused_ || order_.empty() || e.alt) {
        return response;
    }
    keyboard_ = true;
    focus_.set_focused(true);
    const std::size_t n = order_.size();
    const std::size_t v = view_row(focus_model_);
    response.consumed = true;
    switch (e.key) {
    case input::Key::up: move_focus(v == 0 ? 0 : v - 1, e, response); break;
    case input::Key::down: move_focus(std::min(n - 1, v + 1), e, response); break;
    case input::Key::home: move_focus(0, e, response); break;
    case input::Key::end: move_focus(n - 1, e, response); break;
    case input::Key::page_up: move_focus(v > page_rows() ? v - page_rows() : 0, e, response); break;
    case input::Key::page_down: move_focus(std::min(n - 1, v + page_rows()), e, response); break;
    case input::Key::space:
        if (selection_ == TableSelection::multiple && e.ctrl) {
            set_selected(focus_model_, !selected_[focus_model_]);
            anchor_model_ = focus_model_;
        } else {
            select_only(focus_model_);
            anchor_model_ = focus_model_;
        }
        response.selection_changed = selection_ != TableSelection::none;
        break;
    case input::Key::enter: response.activated = focus_model_; break;
    case input::Key::a:
        if (claims_key(e)) {
            select_all();
            response.selection_changed = true;
        } else {
            response.consumed = false;
        }
        break;
    default: response.consumed = false; break;
    }
    return response;
}

TableResponse Table::handle(const input::PointerEvent& e) {
    TableResponse response;
    if (!enabled()) {
        return response;
    }
    if (resizing_) {
        if (e.action == input::PointerAction::move) {
            set_column_width(*resizing_, resize_start_width_ + e.x_dp - resize_start_x_);
            response.resized = resizing_;
        } else if (e.action == input::PointerAction::up || e.action == input::PointerAction::leave) {
            response.resized = resizing_;
            resizing_.reset();
            divider_hover_ = e.action == input::PointerAction::leave ? std::nullopt : divider_at(e.x_dp, e.y_dp);
        }
        response.consumed = true;
        return response;
    }
    const bool claimed = scroll_.route(e, [&](const input::PointerEvent& pe) {
        content_pointer(pe, response);
        return resizing_.has_value();
    });
    if (claimed) {
        response.consumed = true;
    }
    return response;
}

void Table::content_pointer(const input::PointerEvent& e, TableResponse& response) {
    if (e.action == input::PointerAction::leave) {
        hovered_model_.reset();
        divider_hover_.reset();
        pressed_header_.reset();
        for (Column& c : columns_) {
            c.hover.animate_to(0.0);
        }
        retarget_rows();
        return;
    }
    const bool in_header = contains(header_rect(), e.x_dp, e.y_dp);
    const std::optional<std::size_t> column = in_header ? column_at(e.x_dp) : std::nullopt;
    divider_hover_ = divider_at(e.x_dp, e.y_dp);
    for (std::size_t c = 0; c < columns_.size(); ++c) {
        columns_[c].hover.animate_to(column == c && !divider_hover_ && columns_[c].sortable ? 1.0 : 0.0);
    }
    const std::optional<std::size_t> row = row_at(e.x_dp, e.y_dp);
    hovered_model_ = row ? std::optional<std::size_t>(order_[*row]) : std::nullopt;
    retarget_rows();
    if (e.action == input::PointerAction::down) {
        if (!contains(bounds_, e.x_dp, e.y_dp)) {
            set_focused(false);
            return;
        }
        keyboard_ = false;
        focused_ = can_focus();
        focus_.set_focused(false);
        response.consumed = true;
        if (divider_hover_) {
            resizing_ = divider_hover_;
            resize_start_x_ = e.x_dp;
            resize_start_width_ = columns_[*divider_hover_].width;
            return;
        }
        if (in_header) {
            pressed_header_ = column;
            return;
        }
        if (!row) {
            if (!e.ctrl && !e.shift && selected_count_ > 0) {
                clear_selection();
                response.selection_changed = true;
            }
            return;
        }
        const std::size_t m = order_[*row];
        const std::size_t anchor_view = view_row(anchor_model_);
        focus_model_ = m;
        if (selection_ == TableSelection::multiple && e.shift) {
            select_range(anchor_view, *row, e.ctrl);
        } else if (selection_ == TableSelection::multiple && e.ctrl) {
            set_selected(m, !selected_[m]);
            anchor_model_ = m;
        } else if (selection_ != TableSelection::none) {
            select_only(m);
            anchor_model_ = m;
        }
        response.selection_changed = selection_ != TableSelection::none;
        if (e.clicks >= 2) {
            response.activated = m;
        }
        return;
    }
    if (e.action == input::PointerAction::up && pressed_header_) {
        const std::size_t c = *pressed_header_;
        pressed_header_.reset();
        if (column == c && columns_[c].sortable) {
            const SortDirection d =
                sort_ && sort_->column == c && sort_->direction == SortDirection::ascending ? SortDirection::descending : SortDirection::ascending;
            sort_by(c, d);
            response.sorted = sort_;
        }
        response.consumed = true;
    }
}

bool Table::handle(const input::WheelEvent& e) {
    if (!enabled()) {
        return false;
    }
    if (contains(header_rect(), e.x_dp, e.y_dp)) {
        input::WheelEvent inside = e;
        inside.y_dp = scroll_.viewport().y;
        return scroll_.handle(inside);
    }
    return scroll_.handle(e);
}

bool Table::handle(const input::ContextClick& e) {
    if (!enabled() || !contains(bounds_, e.x_dp, e.y_dp)) {
        return false;
    }
    focused_ = can_focus();
    keyboard_ = false;
    focus_.set_focused(false);
    if (const std::optional<std::size_t> row = row_at(e.x_dp, e.y_dp)) {
        const std::size_t m = order_[*row];
        focus_model_ = m;
        if (!selected_[m]) {
            select_only(m);
            anchor_model_ = m;
        }
    }
    return true;
}

void Table::cancel_interaction() {
    scroll_.cancel_interaction();
    resizing_.reset();
    divider_hover_.reset();
    pressed_header_.reset();
    hovered_model_.reset();
    for (Column& c : columns_) {
        c.hover.animate_to(0.0);
    }
    retarget_rows();
}

void Table::step(double dt) {
    scroll_.step(dt);
    focus_.step(dt);
    enabled_.step(dt);
    for (Column& c : columns_) {
        c.hover.step(dt);
    }
    const auto [from, to] = visible_rows();
    for (std::size_t v = from; v < to; ++v) {
        row_state(order_[v]);
    }
    for (auto it = rows_.begin(); it != rows_.end();) {
        it->second.selected.step(dt);
        it->second.state_layer.step(dt);
        const std::size_t v = view_row(it->first);
        const bool shown = v >= from && v < to;
        if (!shown && it->second.selected.settled() && it->second.state_layer.settled()) {
            it = rows_.erase(it);
        } else {
            ++it;
        }
    }
}

bool Table::animating() const {
    if (scroll_.animating() || !focus_.settled() || !enabled_.settled()) {
        return true;
    }
    for (const Column& c : columns_) {
        if (!c.hover.settled()) {
            return true;
        }
    }
    return std::any_of(rows_.begin(), rows_.end(), [](const auto& r) { return !r.second.selected.settled() || !r.second.state_layer.settled(); });
}

AccessibleNode Table::accessible() const {
    AccessibleNode table{AccessibleRole::table, accessible_name_, bounds_, {.enabled = enabled(), .focusable = can_focus(), .focused = focused_}};
    table.value = std::to_string(order_.size()) + " rows, " + std::to_string(selected_count_) + " selected";
    AccessibleNode header{AccessibleRole::row, "Header", header_rect()};
    for (std::size_t c = 0; c < columns_.size(); ++c) {
        AccessibleNode h{AccessibleRole::column_header, columns_[c].title, column_header_rect(c), {.enabled = enabled()}};
        if (sort_ && sort_->column == c) {
            h.value = sort_->direction == SortDirection::ascending ? "Ascending" : "Descending";
        }
        header.children.push_back(std::move(h));
    }
    table.children.push_back(std::move(header));
    const render::Rect& v = scroll_.viewport();
    const auto [from, to] = visible_rows();
    for (std::size_t r = from; r < to; ++r) {
        const render::Rect rr = row_rect(r);
        if (!overlaps(rr.y, rr.y + rr.height, v.y, v.y + v.height)) {
            continue;
        }
        const std::size_t m = order_[r];
        AccessibleNode row{AccessibleRole::row, cell_(m, 0), rr,
                           {.enabled = enabled(), .focused = focused_ && focus_model_ == m, .selected = selected_[m]}};
        for (std::size_t c = 0; c < columns_.size(); ++c) {
            row.children.push_back({AccessibleRole::cell, cell_(m, c), cell_rect(r, c)});
        }
        table.children.push_back(std::move(row));
    }
    return table;
}

void Table::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect& v = scroll_.viewport();
    const double rh = row_height();
    canvas.save();
    canvas.push_clip(bounds_);
    canvas.fill_rect(bounds_, scheme[t().container]);
    const auto [from, to] = visible_rows();
    stats_.painted_rows = to - from;
    for (auto it = cache_.begin(); it != cache_.end();) {
        const std::size_t r = view_row(it->first);
        it = r >= from && r < to ? std::next(it) : cache_.erase(it);
    }
    const color::Argb disabled = resolve(scheme, t().disabled_label);
    canvas.save();
    canvas.push_clip(v);
    for (std::size_t r = from; r < to; ++r) {
        const std::size_t m = order_[r];
        const render::Rect rr = row_rect(r);
        const auto state = rows_.find(m);
        const double sel = state != rows_.end() ? std::clamp(state->second.selected.value(), 0.0, 1.0) : (selected_[m] ? 1.0 : 0.0);
        const double layer = state != rows_.end() ? state->second.state_layer.value() : 0.0;
        if (sel > 0.0) {
            canvas.fill_rect(rr, color::with_alpha(scheme[t().selected_container], sel));
        }
        if (layer > 0.0) {
            canvas.fill_rect(rr, color::with_alpha(scheme[t().state_layer], layer));
        }
        if (r + 1 < order_.size()) {
            canvas.fill_rect({rr.x, rr.y + rh - t().divider_thickness, rr.width, t().divider_thickness}, scheme[t().divider]);
        }
        const color::Argb ink = enabled_.mix(disabled, color::mix(scheme[t().cell_label_color], scheme[t().selected_label], sel));
        const std::vector<text::TextLine>& lines = cells(m);
        for (std::size_t c = 0; c < columns_.size(); ++c) {
            const render::Rect cr = cell_rect(r, c);
            if (!overlaps(cr.x, cr.x + cr.width, v.x, v.x + v.width)) {
                continue;
            }
            const text::TextLine& line = lines[c];
            const double inner = cr.width - t().cell_leading_padding - t().cell_trailing_padding;
            const double x = columns_[c].align == ColumnAlign::start ? cr.x + t().cell_leading_padding
                                                                     : cr.x + cr.width - t().cell_trailing_padding - line.width;
            canvas.save();
            canvas.push_clip({cr.x + t().cell_leading_padding, cr.y, std::max(0.0, inner), cr.height});
            text::draw_line(canvas, fonts_, line, {x, cr.y + (cr.height - line.box_height()) / 2.0}, ink);
            canvas.pop_clip();
            canvas.restore();
        }
    }
    if (focused_ && !order_.empty()) {
        const std::size_t r = view_row(focus_model_);
        if (r >= from && r < to) {
            const render::Rect rr = row_rect(r);
            focus_.paint_inward(canvas, scheme, {v.x, rr.y, v.width, rr.height}, t().focus_ring_corner);
        }
    }
    canvas.pop_clip();
    canvas.restore();
    const render::Rect h = header_rect();
    canvas.fill_rect(h, scheme[t().header_container]);
    canvas.save();
    canvas.push_clip(h);
    const color::Argb header_ink = enabled_.mix(disabled, scheme[t().header_label_color]);
    for (std::size_t c = 0; c < columns_.size(); ++c) {
        const Column& col = columns_[c];
        const render::Rect hr = column_header_rect(c);
        if (!overlaps(hr.x, hr.x + hr.width, h.x, h.x + h.width)) {
            continue;
        }
        const bool sorted = sort_ && sort_->column == c;
        const double icon_room = col.sortable ? t().sort_icon_gap + t().sort_icon_size : 0.0;
        const double inner = hr.width - t().cell_leading_padding - t().cell_trailing_padding;
        const double label_x = col.align == ColumnAlign::start ? hr.x + t().cell_leading_padding
                                                               : hr.x + hr.width - t().cell_trailing_padding - col.label.width;
        canvas.save();
        canvas.push_clip({hr.x + t().cell_leading_padding, hr.y, std::max(0.0, inner), hr.height});
        text::draw_line(canvas, fonts_, col.label, {label_x, hr.y + (hr.height - col.label.box_height()) / 2.0}, header_ink);
        const double shown = sorted ? 1.0 : std::clamp(col.hover.value(), 0.0, 1.0);
        if (col.sortable && shown > 0.0) {
            const double ix = col.align == ColumnAlign::start ? label_x + col.label.width + t().sort_icon_gap : label_x - icon_room;
            const icons::Symbol symbol = sorted && sort_->direction == SortDirection::descending ? t().descending_symbol : t().ascending_symbol;
            const color::Argb ink = enabled_.mix(disabled, scheme[sorted ? t().active_sort_icon : t().sort_icon]);
            icon_font_->draw(canvas, symbol, {ix, hr.y + (hr.height - t().sort_icon_size) / 2.0}, t().sort_icon_size, color::scale_alpha(ink, shown));
        }
        canvas.pop_clip();
        canvas.restore();
        const double inset = std::min(t().header_divider_inset, hr.height / 2.0);
        canvas.fill_rect({hr.x + hr.width - t().divider_thickness, hr.y + inset, t().divider_thickness, hr.height - 2.0 * inset}, scheme[t().divider]);
    }
    canvas.pop_clip();
    canvas.restore();
    canvas.fill_rect({h.x, h.y + h.height - t().divider_thickness, h.width, t().divider_thickness}, scheme[t().divider]);
    scroll_.paint(canvas, scheme);
    stats_.cached_rows = cache_.size();
    canvas.pop_clip();
    canvas.restore();
}

}
