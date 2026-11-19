#include "timer/layout.hpp"

#include <algorithm>

namespace lamina::examples::timer {

TimerLayout::TimerLayout(LayoutSettings settings) : settings_(settings) {}

TimerFrame TimerLayout::place(double width, double height) {
    const LayoutSettings& s = settings_;
    const double aspect = height > 0.0 ? width / height : 0.0;
    if (!arrangement_) {
        arrangement_ = aspect >= s.landscape_enter_aspect ? Arrangement::landscape : Arrangement::portrait;
    } else if (*arrangement_ == Arrangement::portrait && aspect >= s.landscape_enter_aspect) {
        arrangement_ = Arrangement::landscape;
    } else if (*arrangement_ == Arrangement::landscape && aspect <= s.landscape_exit_aspect) {
        arrangement_ = Arrangement::portrait;
    }
    if (*arrangement_ == Arrangement::portrait) {
        return {Arrangement::portrait, entry_portrait(width, height), running_portrait(width, height)};
    }
    return {Arrangement::landscape, entry_landscape(width, height), running_landscape(width, height)};
}

void TimerLayout::place_keys(EntryFrame& frame, double x, double y, double d) const {
    const double pitch = d + settings_.gap;
    for (std::size_t i = 0; i < key_count; ++i) {
        const double column = static_cast<double>(i % keypad_columns);
        const double row = static_cast<double>(i / keypad_columns);
        frame.keys[i] = {x + column * pitch, y + row * pitch, d, d};
    }
}

void TimerLayout::place_presets(EntryFrame& frame, double x, double y, double width) const {
    const double n = static_cast<double>(preset_count);
    const double w = std::max(0.0, (width - (n - 1.0) * settings_.gap) / n);
    for (std::size_t i = 0; i < preset_count; ++i) {
        frame.presets[i] = {x + static_cast<double>(i) * (w + settings_.gap), y, w, settings_.preset_height};
    }
}

EntryFrame TimerLayout::entry_portrait(double width, double height) const {
    const LayoutSettings& s = settings_;
    const double columns = static_cast<double>(keypad_columns);
    const double rows = static_cast<double>(keypad_rows);
    const double by_width = (width - 2.0 * s.margin - (columns - 1.0) * s.gap) / columns;
    const double fixed = 2.0 * s.margin + s.readout_min_height + s.preset_height + (rows + 2.0) * s.gap;
    const double by_height = (height - fixed) / (rows + s.start_height_ratio);
    const double d = std::max(0.0, std::min({by_width, by_height, s.max_key}));
    const double pad_width = columns * d + (columns - 1.0) * s.gap;
    const double pad_height = rows * d + (rows - 1.0) * s.gap;
    const double x = (width - pad_width) / 2.0;

    EntryFrame frame{};
    const double start_height = s.start_height_ratio * d;
    const double start_width = s.start_width_ratio * d;
    frame.start = {(width - start_width) / 2.0, height - s.margin - start_height, start_width, start_height};
    const double pad_y = frame.start.y - s.gap - pad_height;
    place_keys(frame, x, pad_y, d);
    const double presets_y = pad_y - s.gap - s.preset_height;
    place_presets(frame, x, presets_y, pad_width);
    frame.readout = {s.margin, s.margin, std::max(0.0, width - 2.0 * s.margin), std::max(0.0, presets_y - s.gap - s.margin)};
    return frame;
}

EntryFrame TimerLayout::entry_landscape(double width, double height) const {
    const LayoutSettings& s = settings_;
    const double columns = static_cast<double>(keypad_columns);
    const double rows = static_cast<double>(keypad_rows);
    const double by_height = (height - 2.0 * s.margin - (rows - 1.0) * s.gap) / rows;
    const double by_width = ((width - 2.0 * s.margin - s.gap) / 2.0 - (columns - 1.0) * s.gap) / columns;
    const double d = std::max(0.0, std::min({by_height, by_width, s.max_key}));
    const double pad_width = columns * d + (columns - 1.0) * s.gap;
    const double pad_height = rows * d + (rows - 1.0) * s.gap;
    const double pad_x = width - s.margin - pad_width;
    const double pad_y = (height - pad_height) / 2.0;

    EntryFrame frame{};
    place_keys(frame, pad_x, pad_y, d);
    const double left_width = std::max(0.0, pad_x - s.gap - s.margin);
    const double start_height = s.start_height_ratio * d;
    const double start_width = s.start_width_ratio * d;
    const double presets_width = std::min(left_width, pad_width);
    const double presets_x = s.margin + (left_width - presets_width) / 2.0;
    frame.start = {s.margin + (left_width - start_width) / 2.0, pad_y + pad_height - start_height, start_width, start_height};
    const double presets_y = frame.start.y - s.gap - s.preset_height;
    place_presets(frame, presets_x, presets_y, presets_width);
    frame.readout = {s.margin, pad_y, left_width, std::max(0.0, presets_y - s.gap - pad_y)};
    return frame;
}

double TimerLayout::control_height(double ring) const {
    return std::min(settings_.control_ratio * ring, settings_.control_max);
}

RunningFrame TimerLayout::inside_ring(const Box& ring, const Box& add, const Box& reset) const {
    const LayoutSettings& s = settings_;
    const double d = ring.width;
    const double cx = ring.x + d / 2.0;
    const double cy = ring.y + d / 2.0;
    const double time_width = s.time_width_ratio * d;
    const double time_height = s.time_height_ratio * d;
    const double toggle_height = s.toggle_height_ratio * d;
    const double toggle_width = s.toggle_width_ratio * toggle_height;
    return {ring,
            {cx - time_width / 2.0, cy - s.time_offset_ratio * d - time_height / 2.0, time_width, time_height},
            {cx - toggle_width / 2.0, cy + s.toggle_offset_ratio * d - toggle_height / 2.0, toggle_width, toggle_height},
            add,
            reset};
}

RunningFrame TimerLayout::running_portrait(double width, double height) const {
    const LayoutSettings& s = settings_;
    const double free_width = width - 2.0 * s.margin;
    const double free_height = height - 2.0 * s.margin - s.ring_gap;
    double d = std::min(free_width, free_height / (1.0 + s.control_ratio));
    const double control = control_height(d);
    d = std::max(0.0, std::min(free_width, free_height - control));
    const double group = d + s.ring_gap + control;
    const Box ring{(width - d) / 2.0, (height - group) / 2.0, d, d};
    const double add_width = s.add_width_ratio * control;
    const double row = add_width + s.gap + control;
    const double y = ring.y + d + s.ring_gap;
    const double x = (width - row) / 2.0;
    return inside_ring(ring, {x, y, add_width, control}, {x + add_width + s.gap, y, control, control});
}

RunningFrame TimerLayout::running_landscape(double width, double height) const {
    const LayoutSettings& s = settings_;
    const double free_width = width - 2.0 * s.margin - s.ring_gap;
    const double free_height = height - 2.0 * s.margin;
    double d = std::min(free_height, free_width / (1.0 + s.control_ratio * s.add_width_ratio));
    const double control = control_height(d);
    const double column = s.add_width_ratio * control;
    d = std::max(0.0, std::min(free_height, free_width - column));
    const double group = d + s.ring_gap + column;
    const Box ring{(width - group) / 2.0, (height - d) / 2.0, d, d};
    const double x = ring.x + d + s.ring_gap;
    const double y = ring.y + d / 2.0 - (2.0 * control + s.gap) / 2.0;
    return inside_ring(ring, {x, y, column, control}, {x + (column - control) / 2.0, y + control + s.gap, control, control});
}

}
