#pragma once

#include "lamina/builtin_resources.hpp"
#include "lamina/icons/symbol.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/icon_button.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::tokens {

enum class ConnectedPosition { leading, middle, trailing };

constexpr Corners connected_corners(ConnectedPosition p, CornerSize inner) {
    switch (p) {
    case ConnectedPosition::leading:
        return {CornerSize::full, inner, inner, CornerSize::full};
    case ConnectedPosition::trailing:
        return {inner, CornerSize::full, CornerSize::full, inner};
    case ConnectedPosition::middle:
        break;
    }
    return all_corners(inner);
}

struct ButtonGroupTokens {
    double between_space;
    double connected_between_space;
    double expanded_ratio;
    double release_threshold;
    CornerSize connected_inner_corner;
    CornerSize connected_pressed_inner_corner;
    CornerSize connected_checked_corner;
    icons::Symbol overflow_symbol;
    IconButtonStyle overflow_style;
    motion::SpringSpec width_spring;

    constexpr Corners rest_corners(ConnectedPosition p) const { return connected_corners(p, connected_inner_corner); }
    constexpr Corners pressed_corners(ConnectedPosition p) const { return connected_corners(p, connected_pressed_inner_corner); }
};

inline constexpr ButtonGroupTokens button_group_tokens{
    .between_space = 12.0,
    .connected_between_space = 2.0,
    .expanded_ratio = 0.15,
    .release_threshold = 0.75,
    .connected_inner_corner = CornerSize::small,
    .connected_pressed_inner_corner = CornerSize::extra_small,
    .connected_checked_corner = CornerSize::full,
    .overflow_symbol = builtin::icons::more_vert,
    .overflow_style = IconButtonStyle::filled,
    .width_spring = motion_scheme.fast_spatial(),
};

constexpr Corners connected_rest_corners(ConnectedPosition p) {
    return button_group_tokens.rest_corners(p);
}

constexpr Corners connected_pressed_corners(ConnectedPosition p) {
    return button_group_tokens.pressed_corners(p);
}

}
