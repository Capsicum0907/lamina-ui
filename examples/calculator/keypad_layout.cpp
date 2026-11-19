#include "calculator/keypad_layout.hpp"

#include <algorithm>

namespace lamina::examples::calculator {

KeypadLayout::KeypadLayout(LayoutSettings settings) : settings_(settings) {}

double KeypadLayout::landscape_key_height(double height) const {
    const LayoutSettings& s = settings_;
    const double rows = s.landscape_rows;
    const double available = height - 2.0 * s.margin - s.display_min_height - s.gap;
    return std::max(0.0, (available - (rows - 1.0) * s.gap) / rows);
}

double KeypadLayout::landscape_fit_width(double width, int columns) const {
    const LayoutSettings& s = settings_;
    const double n = columns;
    return std::max(0.0, (width - 2.0 * s.margin - (n - 1.0) * s.gap) / n);
}

KeypadFrame KeypadLayout::place(double width, double height, std::span<const KeyPlacement> keys) {
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
        scientific_ = false;
        return portrait(width, height, keys);
    }
    const double key_height = landscape_key_height(height);
    const double wide = landscape_fit_width(width, s.landscape_columns + s.scientific_columns);
    if (!scientific_ && wide >= s.scientific_enter_aspect * key_height) {
        scientific_ = true;
    } else if (scientific_ && wide < s.scientific_exit_aspect * key_height) {
        scientific_ = false;
    }
    return landscape(width, height, keys);
}

KeypadFrame KeypadLayout::portrait(double width, double height, std::span<const KeyPlacement> keys) const {
    const LayoutSettings& s = settings_;
    const double columns = s.portrait_columns;
    const double rows = s.portrait_rows;
    const double available_width = width - 2.0 * s.margin;
    const double available_height = height - 2.0 * s.margin - s.display_min_height - s.gap;
    const double d = std::max(0.0, std::min((available_width - (columns - 1.0) * s.gap) / columns,
                                            (available_height - (rows - 1.0) * s.gap) / rows));
    const double pitch = d + s.gap;
    const Box keypad{s.margin + (available_width - (columns * d + (columns - 1.0) * s.gap)) / 2.0,
                     height - s.margin - (rows * d + (rows - 1.0) * s.gap), columns * d + (columns - 1.0) * s.gap,
                     rows * d + (rows - 1.0) * s.gap};

    KeypadFrame frame{Arrangement::portrait, false, {keypad.x, s.margin, keypad.width, std::max(0.0, keypad.y - s.gap - s.margin)},
                      keypad, d, d, {}, {}};
    for (const KeyPlacement& k : keys) {
        if (k.scientific) {
            const double x = keypad.x - (s.scientific_columns - k.landscape.column) * pitch;
            frame.keys.push_back({x, keypad.y + k.landscape.row * pitch, d, d});
            frame.shown.push_back(false);
        } else {
            frame.keys.push_back({keypad.x + k.portrait.column * pitch, keypad.y + k.portrait.row * pitch, d, d});
            frame.shown.push_back(true);
        }
    }
    return frame;
}

KeypadFrame KeypadLayout::landscape(double width, double height, std::span<const KeyPlacement> keys) const {
    const LayoutSettings& s = settings_;
    const int columns = s.landscape_columns + (scientific_ ? s.scientific_columns : 0);
    const int first = scientific_ ? 0 : s.scientific_columns;
    const double rows = s.landscape_rows;
    const double fit = landscape_fit_width(width, columns);
    const double key_height = std::min(landscape_key_height(height), fit);
    const double key_width = std::min(fit, s.max_key_aspect * key_height);
    const double n = columns;
    const double keypad_width = n * key_width + (n - 1.0) * s.gap;
    const double keypad_height = rows * key_height + (rows - 1.0) * s.gap;
    const Box keypad{s.margin + (width - 2.0 * s.margin - keypad_width) / 2.0, height - s.margin - keypad_height, keypad_width,
                     keypad_height};

    KeypadFrame frame{Arrangement::landscape, scientific_, {keypad.x, s.margin, keypad.width, std::max(0.0, keypad.y - s.gap - s.margin)},
                      keypad, key_width, key_height, {}, {}};
    for (const KeyPlacement& k : keys) {
        const int column = (k.scientific ? k.landscape.column : k.landscape.column + s.scientific_columns) - first;
        frame.keys.push_back({keypad.x + column * (key_width + s.gap), keypad.y + k.landscape.row * (key_height + s.gap), key_width,
                              key_height});
        frame.shown.push_back(!k.scientific || scientific_);
    }
    return frame;
}

}
