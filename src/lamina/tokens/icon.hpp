#pragma once

namespace lamina::tokens {

struct SymbolAxes {
    double fill;
    double weight;
    double grade;
    double optical_size_min;
    double optical_size_max;
};

inline constexpr SymbolAxes symbol_axes{0.0, 400.0, 0.0, 20.0, 48.0};

}
