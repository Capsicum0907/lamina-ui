#pragma once

#include "lamina/builtin_resources.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/symbol.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct DatePickerTokens {
    double container_width;
    double container_height;
    double header_height;
    double range_header_height;
    double title_top;
    double header_start;
    double range_header_start;
    double header_end;
    double headline_bottom;
    double toggle_end;
    double toggle_bottom;
    double divider_thickness;
    double navigation_height;
    double horizontal_padding;
    double cell_size;
    double day_size;
    int rows;
    double today_outline;
    double year_width;
    double year_height;
    int year_columns;
    double year_row_spacing;
    double year_expand_from_alpha;
    double month_button_height;
    double month_button_padding;
    double month_button_icon_size;
    double month_button_icon_spacing;
    double subhead_start;
    double subhead_top;
    double subhead_bottom;
    double range_band_height;
    double input_start;
    double input_end;
    double input_top;
    double input_bottom;
    double range_input_spacing;
    double switch_parallax;
    double input_focus_delay_seconds;
    TypeRole title_type;
    TypeRole headline_type;
    TypeRole range_headline_type;
    TypeRole weekday_type;
    TypeRole day_type;
    TypeRole year_type;
    TypeRole month_button_type;
    TypeRole subhead_type;
    double range_headline_spacing;
    CornerSize dialog_corner;
    CornerSize docked_corner;
    double docked_elevation;
    double docked_gap;
    double dialog_button_bottom;
    double dialog_button_end;
    double dialog_button_spacing;
    icons::Symbol input_mode_symbol;
    icons::Symbol picker_mode_symbol;
    icons::Symbol menu_symbol;
    icons::Symbol menu_open_symbol;
    icons::Symbol previous_symbol;
    icons::Symbol next_symbol;
    icons::Symbol docked_symbol;
};

inline constexpr DatePickerTokens date_picker_tokens{
    .container_width = 360.0,
    .container_height = 568.0,
    .header_height = 120.0,
    .range_header_height = 68.0,
    .title_top = 16.0,
    .header_start = 24.0,
    .range_header_start = 64.0,
    .header_end = 12.0,
    .headline_bottom = 12.0,
    .toggle_end = 12.0,
    .toggle_bottom = 12.0,
    .divider_thickness = 1.0,
    .navigation_height = 56.0,
    .horizontal_padding = 12.0,
    .cell_size = 48.0,
    .day_size = 40.0,
    .rows = 6,
    .today_outline = 1.0,
    .year_width = 72.0,
    .year_height = 36.0,
    .year_columns = 3,
    .year_row_spacing = 16.0,
    .year_expand_from_alpha = 0.6,
    .month_button_height = 40.0,
    .month_button_padding = 12.0,
    .month_button_icon_size = 18.0,
    .month_button_icon_spacing = 8.0,
    .subhead_start = 24.0,
    .subhead_top = 20.0,
    .subhead_bottom = 8.0,
    .range_band_height = 40.0,
    .input_start = 24.0,
    .input_end = 24.0,
    .input_top = 10.0,
    .input_bottom = 16.0,
    .range_input_spacing = 8.0,
    .switch_parallax = 48.0,
    .input_focus_delay_seconds = duration_ms::medium[1] / 1000.0,
    .title_type = TypeRole::label_large,
    .headline_type = TypeRole::headline_large,
    .range_headline_type = TypeRole::title_large,
    .weekday_type = TypeRole::body_large,
    .day_type = TypeRole::body_large,
    .year_type = TypeRole::body_large,
    .month_button_type = TypeRole::label_large,
    .subhead_type = TypeRole::title_small,
    .range_headline_spacing = 4.0,
    .dialog_corner = CornerSize::extra_large,
    .docked_corner = CornerSize::large,
    .docked_elevation = 6.0,
    .docked_gap = 4.0,
    .dialog_button_bottom = 8.0,
    .dialog_button_end = 6.0,
    .dialog_button_spacing = 8.0,
    .input_mode_symbol = builtin::icons::edit,
    .picker_mode_symbol = builtin::icons::date_range,
    .menu_symbol = builtin::icons::arrow_drop_down,
    .menu_open_symbol = builtin::icons::arrow_drop_up,
    .previous_symbol = builtin::icons::keyboard_arrow_left,
    .next_symbol = builtin::icons::keyboard_arrow_right,
    .docked_symbol = builtin::icons::calendar_today,
};

struct DatePickerColors {
    color::Role container;
    color::Role title;
    color::Role headline;
    color::Role weekday;
    color::Role subhead;
    color::Role navigation;
    color::Role year;
    color::Role current_year;
    color::Role selected_year;
    color::Role selected_year_container;
    color::Role day;
    color::Role selected_day;
    color::Role selected_day_container;
    color::Role today;
    color::Role today_outline;
    color::Role in_range;
    color::Role in_range_container;
    color::Role divider;
    double disabled_opacity;
};

inline constexpr DatePickerColors date_picker_colors{
    .container = color::Role::surface_container_high,
    .title = color::Role::on_surface_variant,
    .headline = color::Role::on_surface_variant,
    .weekday = color::Role::on_surface,
    .subhead = color::Role::on_surface_variant,
    .navigation = color::Role::on_surface_variant,
    .year = color::Role::on_surface_variant,
    .current_year = color::Role::primary,
    .selected_year = color::Role::on_primary,
    .selected_year_container = color::Role::primary,
    .day = color::Role::on_surface,
    .selected_day = color::Role::on_primary,
    .selected_day_container = color::Role::primary,
    .today = color::Role::primary,
    .today_outline = color::Role::primary,
    .in_range = color::Role::on_secondary_container,
    .in_range_container = color::Role::secondary_container,
    .divider = color::Role::outline_variant,
    .disabled_opacity = disabled_alpha,
};

struct DatePickerMotion {
    motion::SpringSpec month_scroll;
    motion::SpringSpec year_expand;
    motion::SpringSpec year_fade_in;
    motion::SpringSpec year_fade_out;
    motion::SpringSpec mode_slide;
    motion::SpringSpec mode_fade_in;
    motion::SpringSpec mode_fade_out;
    motion::SpringSpec range_crossfade;
    motion::SpringSpec selection;
    motion::SpringSpec state_layer;
    motion::SpringSpec popup_scale;
    motion::SpringSpec popup_fade;
};

inline constexpr DatePickerMotion date_picker_motion{
    .month_scroll = motion_scheme.default_effects(),
    .year_expand = motion_scheme.default_effects(),
    .year_fade_in = motion_scheme.default_effects(),
    .year_fade_out = motion_scheme.fast_effects(),
    .mode_slide = motion_scheme.default_spatial(),
    .mode_fade_in = motion_scheme.default_effects(),
    .mode_fade_out = motion_scheme.fast_effects(),
    .range_crossfade = motion_scheme.fast_effects(),
    .selection = motion_scheme.default_effects(),
    .state_layer = motion_scheme.fast_effects(),
    .popup_scale = motion_scheme.fast_spatial(),
    .popup_fade = motion_scheme.fast_effects(),
};

}
