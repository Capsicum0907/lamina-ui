#pragma once

#include "lamina/builtin_resources.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/symbol.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct TitleBarTokens {
    double height;
    double leading_space;
    double title_content_space;
    double content_buttons_space;
    double trailing_space;
    double button_width;
    double indicator_width;
    double indicator_height;
    double icon_size;
    double glyph_box;
    double glyph_stroke;
    double glyph_corner;
    double restore_offset;
    CornerSize indicator_corner;
    CornerSize pressed_corner;
    TypeRole title;
    color::Role container;
    color::Role inactive_container;
    color::Role title_color;
    color::Role icon;
    color::Role state_layer;
    color::Role close_container;
    color::Role close_icon;
    icons::Symbol close_symbol;
};

inline constexpr TitleBarTokens title_bar_tokens{
    .height = 40.0,
    .leading_space = 16.0,
    .title_content_space = 16.0,
    .content_buttons_space = 8.0,
    .trailing_space = 4.0,
    .button_width = 44.0,
    .indicator_width = 40.0,
    .indicator_height = 32.0,
    .icon_size = 20.0,
    .glyph_box = 10.0,
    .glyph_stroke = 1.5,
    .glyph_corner = 1.5,
    .restore_offset = 2.0,
    .indicator_corner = CornerSize::full,
    .pressed_corner = CornerSize::small,
    .title = TypeRole::title_small,
    .container = color::Role::surface_container,
    .inactive_container = color::Role::surface,
    .title_color = color::Role::on_surface,
    .icon = color::Role::on_surface_variant,
    .state_layer = color::Role::on_surface,
    .close_container = color::Role::error,
    .close_icon = color::Role::on_error,
    .close_symbol = builtin::icons::close,
};

}
