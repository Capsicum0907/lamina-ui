#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct PickerDialogTokens {
    double date_actions_bottom;
    double date_actions_end;
    double actions_spacing;
    double time_padding;
    TypeRole time_title_type;
    color::Role time_title_color;
    double time_title_bottom;
    ButtonStyle action_style;
    ButtonSize action_size;
};

inline constexpr PickerDialogTokens picker_dialog_tokens{
    .date_actions_bottom = 8.0,
    .date_actions_end = 6.0,
    .actions_spacing = 8.0,
    .time_padding = 24.0,
    .time_title_type = TypeRole::label_medium,
    .time_title_color = color::Role::on_surface,
    .time_title_bottom = 20.0,
    .action_style = ButtonStyle::text,
    .action_size = ButtonSize::small,
};

}
