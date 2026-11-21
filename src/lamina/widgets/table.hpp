#pragma once

#include <cmath>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/tokens/table.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/scroll_area.hpp"

namespace lamina::widgets {

enum class ColumnAlign { start, end };

struct TableColumn {
    std::string title;
    double width = tokens::table_tokens.default_column_width;
    double min_width = tokens::table_tokens.default_min_column_width;
    ColumnAlign align = ColumnAlign::start;
    bool sortable = true;
    bool resizable = true;
    std::function<bool(std::size_t, std::size_t)> less = {};
};

enum class SortDirection { ascending, descending };

struct TableSort {
    std::size_t column;
    SortDirection direction;
    bool operator==(const TableSort&) const = default;
};

enum class TableSelection { none, single, multiple };

struct TableOptions {
    std::vector<TableColumn> columns;
    std::size_t row_count = 0;
    std::function<std::string(std::size_t row, std::size_t column)> cell;
    std::string accessible_name;
    TableSelection selection = TableSelection::multiple;
    tokens::Density density = {};
    std::optional<icons::IconFont> icon_font = std::nullopt;
};

struct TableResponse {
    bool consumed = false;
    bool selection_changed = false;
    std::optional<std::size_t> activated = std::nullopt;
    std::optional<TableSort> sorted = std::nullopt;
    std::optional<std::size_t> resized = std::nullopt;
};

struct TableStats {
    std::size_t painted_rows = 0;
    std::size_t cached_rows = 0;
};

class Table {
public:
    Table(TableOptions options, const text::FontStack& fonts);

    layout::Size measure(const layout::Constraints& c) const {
        return c.constrain({std::isfinite(c.max_width) ? c.max_width : c.min_width, std::isfinite(c.max_height) ? c.max_height : c.min_height});
    }
    void arrange(const render::Rect& r);
    render::Rect bounds() const { return bounds_; }
    render::Rect header_rect() const;
    const render::Rect& viewport() const { return scroll_.viewport(); }
    ScrollArea& scroll_area() { return scroll_; }
    const ScrollArea& scroll_area() const { return scroll_; }

    std::size_t column_count() const { return columns_.size(); }
    std::size_t row_count() const { return order_.size(); }
    void set_row_count(std::size_t count);
    void refresh();
    double row_height() const;
    double header_height() const;
    double column_width(std::size_t column) const { return columns_[column].width; }
    void set_column_width(std::size_t column, double width);
    double content_width() const;
    render::Rect column_header_rect(std::size_t column) const;
    render::Rect row_rect(std::size_t view_row) const;
    render::Rect cell_rect(std::size_t view_row, std::size_t column) const;
    std::optional<std::size_t> row_at(double x, double y) const;
    std::optional<std::size_t> column_at(double x) const;
    std::optional<std::size_t> divider_at(double x, double y) const;
    std::pair<std::size_t, std::size_t> visible_rows() const;
    const TableStats& stats() const { return stats_; }

    std::size_t model_row(std::size_t view_row) const { return order_[view_row]; }
    std::size_t view_row(std::size_t model_row) const { return inverse_[model_row]; }
    std::optional<TableSort> sort() const { return sort_; }
    void sort_by(std::size_t column, SortDirection direction);
    void clear_sort();

    bool is_selected(std::size_t model_row) const { return selected_[model_row]; }
    void set_selected(std::size_t model_row, bool selected);
    std::vector<std::size_t> selected_rows() const;
    std::size_t selected_count() const { return selected_count_; }
    void select_all();
    void clear_selection();
    std::optional<std::size_t> focused_row() const;
    void focus_row(std::size_t view_row);

    bool focused() const { return focused_; }
    void set_focused(bool focused);
    bool can_focus() const { return enabled() && !order_.empty(); }
    bool claims_key(const input::KeyEvent& event) const;
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    tokens::Density density() const { return density_; }
    void set_density(tokens::Density density);
    std::optional<input::Cursor> cursor() const;
    AccessibleNode accessible() const;

    TableResponse handle(const input::PointerEvent& event);
    TableResponse handle(const input::KeyEvent& event);
    bool handle(const input::WheelEvent& event);
    bool handle(const input::ContextClick& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    double next_wake_seconds() const { return scroll_.next_wake_seconds(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Column {
        std::string title;
        text::TextLine label;
        double width;
        double min_width;
        ColumnAlign align;
        bool sortable;
        bool resizable;
        std::function<bool(std::size_t, std::size_t)> less;
        motion::Animated hover;
    };

    struct RowState {
        motion::Animated selected;
        motion::Animated state_layer;
    };

    double column_left(std::size_t column) const;
    void update_content();
    void rebuild_inverse();
    void select_only(std::size_t model_row);
    void select_range(std::size_t from_view, std::size_t to_view, bool keep);
    void move_focus(std::size_t view_row, const input::KeyEvent& e, TableResponse& response);
    void reveal(std::size_t view_row);
    RowState& row_state(std::size_t model_row);
    void retarget_rows();
    std::size_t page_rows() const;
    const std::vector<text::TextLine>& cells(std::size_t model_row) const;

    text::FontStack fonts_;
    std::vector<Column> columns_;
    std::function<std::string(std::size_t, std::size_t)> cell_;
    std::string accessible_name_;
    TableSelection selection_;
    std::optional<icons::IconFont> icon_font_;
    tokens::Density density_{};
    std::vector<std::size_t> order_;
    std::vector<std::size_t> inverse_;
    std::vector<bool> selected_;
    std::size_t selected_count_ = 0;
    std::optional<std::size_t> sort_column_;
    std::optional<TableSort> sort_;
    std::size_t focus_model_ = 0;
    std::size_t anchor_model_ = 0;
    bool focused_ = false;
    bool keyboard_ = false;
    FocusIndicator focus_;
    Enablement enabled_;
    ScrollArea scroll_;
    render::Rect bounds_{0.0, 0.0, 0.0, 0.0};
    std::map<std::size_t, RowState> rows_;
    std::optional<std::size_t> hovered_model_;
    std::optional<std::size_t> pressed_header_;
    std::optional<std::size_t> resizing_;
    std::optional<std::size_t> divider_hover_;
    double resize_start_x_ = 0.0;
    double resize_start_width_ = 0.0;
    mutable std::map<std::size_t, std::vector<text::TextLine>> cache_;
    mutable TableStats stats_;
};

}
