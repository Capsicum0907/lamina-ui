#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::tokens {

struct DragHandleTokens {
    double width;
    double height;
    double pressed_width;
    double pressed_height;
    double dragged_width;
    double dragged_height;
    CornerSize shape;
    CornerSize pressed_shape;
    CornerSize dragged_shape;
    color::Role color;
    color::Role pressed_color;
    color::Role dragged_color;
    double container_width;
    motion::SpringSpec size_spring;
    motion::SpringSpec color_spring;
};

inline constexpr DragHandleTokens drag_handle_tokens{
    .width = 4.0,
    .height = 48.0,
    .pressed_width = 12.0,
    .pressed_height = 52.0,
    .dragged_width = 12.0,
    .dragged_height = 52.0,
    .shape = CornerSize::full,
    .pressed_shape = CornerSize::medium,
    .dragged_shape = CornerSize::medium,
    .color = color::Role::outline,
    .pressed_color = color::Role::on_surface,
    .dragged_color = color::Role::on_surface,
    .container_width = 24.0,
    .size_spring = motion_scheme.fast_spatial(),
    .color_spring = motion_scheme.fast_effects(),
};

}
