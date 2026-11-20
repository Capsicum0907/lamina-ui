#pragma once

#include <array>
#include <cstddef>

#include "lamina/text/style.hpp"

namespace lamina::tokens {

enum class TypeRole : std::size_t {
    display_large,
    display_medium,
    display_small,
    headline_large,
    headline_medium,
    headline_small,
    title_large,
    title_medium,
    title_small,
    body_large,
    body_medium,
    body_small,
    label_large,
    label_medium,
    label_small,
    display_large_emphasized,
    display_medium_emphasized,
    display_small_emphasized,
    headline_large_emphasized,
    headline_medium_emphasized,
    headline_small_emphasized,
    title_large_emphasized,
    title_medium_emphasized,
    title_small_emphasized,
    body_large_emphasized,
    body_medium_emphasized,
    body_small_emphasized,
    label_large_emphasized,
    label_medium_emphasized,
    label_small_emphasized,
    count,
};

inline constexpr std::size_t base_type_role_count = static_cast<std::size_t>(TypeRole::display_large_emphasized);
inline constexpr std::size_t type_role_count = static_cast<std::size_t>(TypeRole::count);

constexpr bool is_emphasized(TypeRole role) {
    return static_cast<std::size_t>(role) >= base_type_role_count;
}

constexpr TypeRole base_role(TypeRole role) {
    return is_emphasized(role) ? static_cast<TypeRole>(static_cast<std::size_t>(role) - base_type_role_count) : role;
}

constexpr TypeRole emphasized(TypeRole role) {
    return static_cast<TypeRole>(static_cast<std::size_t>(base_role(role)) + base_type_role_count);
}

struct TypeStyle {
    double size_sp;
    double line_height_sp;
    int weight;
    double tracking_sp;
};

namespace font_weight {
inline constexpr int regular = 400;
inline constexpr int medium = 500;
inline constexpr int bold = 700;
}

inline constexpr std::array<TypeStyle, base_type_role_count> type_scale{{
    {57, 64, font_weight::regular, -0.25},
    {45, 52, font_weight::regular, 0.0},
    {36, 44, font_weight::regular, 0.0},
    {32, 40, font_weight::regular, 0.0},
    {28, 36, font_weight::regular, 0.0},
    {24, 32, font_weight::regular, 0.0},
    {22, 28, font_weight::regular, 0.0},
    {16, 24, font_weight::medium, 0.15},
    {14, 20, font_weight::medium, 0.1},
    {16, 24, font_weight::regular, 0.5},
    {14, 20, font_weight::regular, 0.25},
    {12, 16, font_weight::regular, 0.4},
    {14, 20, font_weight::medium, 0.1},
    {12, 16, font_weight::medium, 0.5},
    {11, 16, font_weight::medium, 0.5},
}};

inline constexpr std::array<int, base_type_role_count> emphasized_weight{
    font_weight::medium, font_weight::medium, font_weight::medium,
    font_weight::medium, font_weight::medium, font_weight::medium,
    font_weight::medium, font_weight::bold, font_weight::bold,
    font_weight::medium, font_weight::medium, font_weight::medium,
    font_weight::bold, font_weight::bold, font_weight::bold,
};

constexpr TypeStyle type_style(TypeRole role, bool emphasized = false) {
    const auto i = static_cast<std::size_t>(base_role(role));
    TypeStyle style = type_scale[i];
    if (emphasized || is_emphasized(role)) {
        style.weight = emphasized_weight[i];
    }
    return style;
}

inline constexpr double tab_width_spaces = 4.0;

constexpr text::TextStyle text_style(const TypeStyle& t) {
    return {t.size_sp, t.line_height_sp, t.weight, t.tracking_sp, tab_width_spaces};
}

}
