#pragma once

#include "lamina/color/scheme.hpp"

namespace lamina::tokens {

enum class ContainerFill { filled, outlined };
enum class Layering { lift, recede };
enum class SelectionMark { surface, foreground };

struct ContainerTokens {
    ContainerFill fill;
    color::Role outlined_fill;
    color::Role outline;
    double outline_width;
};

struct LayeringTokens {
    Layering mode;
    color::Role receded_container;
    double drawer_push;
};

struct SelectionTokens {
    SelectionMark mark;
    color::Role foreground;
    bool emphasized_label;
    bool underline;
    double underline_thickness;
};

inline constexpr ContainerTokens container_tokens{
    .fill = ContainerFill::filled,
    .outlined_fill = color::Role::surface,
    .outline = color::Role::outline_variant,
    .outline_width = 1.0,
};

inline constexpr LayeringTokens layering_tokens{
    .mode = Layering::lift,
    .receded_container = color::Role::surface,
    .drawer_push = 1.0,
};

inline constexpr SelectionTokens selection_tokens{
    .mark = SelectionMark::surface,
    .foreground = color::Role::primary,
    .emphasized_label = true,
    .underline = true,
    .underline_thickness = 2.0,
};

}
