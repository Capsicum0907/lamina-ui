#include "todo/layout.hpp"

#include <algorithm>

namespace lamina::examples::todo {

TodoLayout::TodoLayout(LayoutSettings settings) : settings_(settings) {}

Column TodoLayout::column(double width) const {
    const LayoutSettings& s = settings_;
    const double w = std::max(0.0, std::min(width - 2.0 * s.margin, s.max_column_width));
    const double x = (width - w) / 2.0;
    const double text_x = x + s.check_size + s.text_gap;
    const double remove_x = x + w - s.remove_size;
    return {x, w, s.check_size, text_x, std::max(0.0, remove_x - s.remove_gap - text_x), remove_x};
}

TodoFrame TodoLayout::place(double width, double height, const ListInput& rows) const {
    const LayoutSettings& s = settings_;
    TodoFrame f{};
    f.column = column(width);
    const Column& c = f.column;
    f.add = {c.x + c.width - s.add_size, s.margin + (s.field_height - s.add_size) / 2.0, s.add_size, s.add_size};
    f.field = {c.x, s.margin, std::max(0.0, c.width - s.add_size - s.add_gap), s.field_height};
    const double top = s.margin + s.field_height + s.list_gap;
    f.viewport = {0.0, top, width, std::max(0.0, height - top)};

    double y = s.list_top_padding;
    for (const double h : rows.active) {
        f.active.push_back({y, h, true});
        y += h;
    }
    if (!rows.completed.empty()) {
        if (!rows.active.empty()) {
            y += s.section_gap;
        }
        f.header = Box{c.x, y, c.width, s.header_height};
        y += s.header_height;
        for (const double h : rows.completed) {
            if (rows.expanded) {
                f.completed.push_back({y, h, true});
                y += h;
            } else {
                f.completed.push_back({y, 0.0, false});
            }
        }
    }
    f.content_height = y + s.list_end_padding;
    return f;
}

}
