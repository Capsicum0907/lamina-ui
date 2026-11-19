#pragma once

#include <array>
#include <cstddef>
#include <optional>

namespace lamina::examples::timer {

struct Box {
    double x;
    double y;
    double width;
    double height;
};

enum class Arrangement { portrait, landscape };

inline constexpr std::size_t keypad_columns = 3;
inline constexpr std::size_t keypad_rows = 4;
inline constexpr std::size_t key_count = keypad_columns * keypad_rows;
inline constexpr std::size_t preset_count = 4;

struct LayoutSettings {
    double margin = 16.0;
    double gap = 12.0;
    double readout_min_height = 112.0;
    double preset_height = 48.0;
    double max_key = 112.0;
    double start_height_ratio = 0.8;
    double start_width_ratio = 1.6;
    double ring_gap = 24.0;
    double control_ratio = 0.22;
    double control_max = 96.0;
    double add_width_ratio = 2.0;
    double time_width_ratio = 0.72;
    double time_height_ratio = 0.22;
    double time_offset_ratio = 0.05;
    double toggle_height_ratio = 0.17;
    double toggle_width_ratio = 1.5;
    double toggle_offset_ratio = 0.2;
    double landscape_enter_aspect = 1.2;
    double landscape_exit_aspect = 1.0;
};

struct EntryFrame {
    Box readout;
    std::array<Box, preset_count> presets;
    std::array<Box, key_count> keys;
    Box start;
};

struct RunningFrame {
    Box ring;
    Box time;
    Box toggle;
    Box add;
    Box reset;
};

struct TimerFrame {
    Arrangement arrangement;
    EntryFrame entry;
    RunningFrame running;
};

class TimerLayout {
public:
    explicit TimerLayout(LayoutSettings settings = {});

    TimerFrame place(double width, double height);

    const LayoutSettings& settings() const { return settings_; }

private:
    EntryFrame entry_portrait(double width, double height) const;
    EntryFrame entry_landscape(double width, double height) const;
    RunningFrame running_portrait(double width, double height) const;
    RunningFrame running_landscape(double width, double height) const;
    RunningFrame inside_ring(const Box& ring, const Box& add, const Box& reset) const;
    void place_keys(EntryFrame& frame, double x, double y, double d) const;
    void place_presets(EntryFrame& frame, double x, double y, double width) const;
    double control_height(double ring) const;

    LayoutSettings settings_;
    std::optional<Arrangement> arrangement_;
};

}
