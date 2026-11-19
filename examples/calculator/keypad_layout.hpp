#pragma once

#include <optional>
#include <span>
#include <vector>

namespace lamina::examples::calculator {

struct Box {
    double x;
    double y;
    double width;
    double height;
};

enum class Arrangement { portrait, landscape };

struct Cell {
    int row;
    int column;
};

struct KeyPlacement {
    Cell portrait;
    Cell landscape;
    bool scientific;
};

struct LayoutSettings {
    double margin = 8.0;
    double gap = 8.0;
    double display_min_height = 160.0;
    int portrait_columns = 4;
    int portrait_rows = 5;
    int landscape_rows = 4;
    int landscape_columns = 5;
    int scientific_columns = 3;
    double max_key_aspect = 1.75;
    double landscape_enter_aspect = 1.2;
    double landscape_exit_aspect = 1.0;
    double scientific_enter_aspect = 1.4;
    double scientific_exit_aspect = 1.2;
};

struct KeypadFrame {
    Arrangement arrangement;
    bool scientific;
    Box display;
    Box keypad;
    double key_width;
    double key_height;
    std::vector<Box> keys;
    std::vector<bool> shown;
};

class KeypadLayout {
public:
    explicit KeypadLayout(LayoutSettings settings = {});

    KeypadFrame place(double width, double height, std::span<const KeyPlacement> keys);

    const LayoutSettings& settings() const { return settings_; }

private:
    KeypadFrame portrait(double width, double height, std::span<const KeyPlacement> keys) const;
    KeypadFrame landscape(double width, double height, std::span<const KeyPlacement> keys) const;
    double landscape_key_height(double height) const;
    double landscape_fit_width(double width, int columns) const;

    LayoutSettings settings_;
    std::optional<Arrangement> arrangement_;
    bool scientific_ = false;
};

}
