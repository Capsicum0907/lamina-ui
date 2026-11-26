#include "restyle_support.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>

namespace lamina::test {

namespace {

std::string environment(const char* name) {
#ifdef _MSC_VER
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || !value) {
        return {};
    }
    std::string out(value);
    std::free(value);
    return out;
#else
    const char* value = std::getenv(name);
    return value ? value : "";
#endif
}

}

const Style& restyle_test_style() {
    static const Style style = [] {
        Style s = m3_expressive_style();
        for (auto& t : s.type.scale) {
            t.size_sp *= restyle_type_scale;
            t.line_height_sp *= restyle_type_scale;
        }
        for (auto& r : s.shape.radius_dp) {
            r *= restyle_corner_scale;
        }
        for (auto& l : s.density.button) {
            l.base += restyle_length_add;
            l.minimum += restyle_length_add;
        }
        for (auto& l : s.density.icon_button) {
            l.base += restyle_length_add;
            l.minimum += restyle_length_add;
        }
        s.motion.spatial.fill(restyle_spring);
        s.motion.effects.fill(restyle_spring);
        s.button.toggle_motion.colors = restyle_spring;
        s.button.toggle_motion.border_width = restyle_spring;
        s.button.toggle_motion.shape = restyle_spring;
        for (motion::SpringSpec* spring : {&s.chip.motion.slot_expand, &s.chip.motion.slot_shrink, &s.chip.motion.slot_fade_in, &s.chip.motion.slot_fade_out,
                                           &s.chip.motion.colors, &s.chip.motion.state_layer, &s.chip.motion.group_move, &s.chip.motion.group_presence,
                                           &s.card.common.state_layer_spring, &s.side_navigation.expansion, &s.side_navigation.selection,
                                           &s.side_navigation.colors, &s.side_navigation.state_layer_spring, &s.navigation_suite.spring,
                                           &s.top_app_bar.common.container_spring, &s.top_app_bar.common.snap_spring, &s.navigation_drawer_motion.colors,
                                           &s.modal_navigation_rail.scrim, &s.pager.settle_spring, &s.pull_to_refresh.settle_spring,
                                           &s.fab.extended_motion.width, &s.fab.extended_motion.label_alpha, &s.segmented_button_motion.label_shift,
                                           &s.segmented_button_motion.check_scale, &s.segmented_button_motion.check_fade,
                                           &s.segmented_button_motion.icon_crossfade, &s.segmented_button_motion.colors,
                                           &s.segmented_button_motion.state_layer}) {
            *spring = restyle_spring;
        }
        for (motion::SpringSpec* spring : {&s.split_button.common.shape_spring, &s.split_button.common.chevron_spring,
                                           &s.date_picker_motion.month_scroll, &s.date_picker_motion.year_expand, &s.date_picker_motion.year_fade_in,
                                           &s.date_picker_motion.year_fade_out, &s.date_picker_motion.mode_slide, &s.date_picker_motion.mode_fade_in,
                                           &s.date_picker_motion.mode_fade_out, &s.date_picker_motion.range_crossfade, &s.date_picker_motion.selection,
                                           &s.date_picker_motion.state_layer, &s.date_picker_motion.popup_scale, &s.date_picker_motion.popup_fade,
                                           &s.time_picker_motion.selector, &s.time_picker_motion.numbers, &s.time_picker_motion.period_shape,
                                           &s.time_picker_motion.colors, &s.time_picker_motion.state_layer, &s.time_picker_motion.mode,
                                           &s.toggle_fab.progress, &s.fab_menu.stagger, &s.fab_menu.item_width, &s.fab_menu.item_alpha,
                                           &s.floating_toolbar.expand, &s.flexible_bottom_app_bar.snap, &s.search_motion.expand, &s.search_motion.collapse,
                                           &s.bottom_sheet_motion.show, &s.bottom_sheet_motion.hide, &s.bottom_sheet_motion.settle,
                                           &s.bottom_sheet_motion.scrim, &s.table.color_spring, &s.table.state_layer_spring, &s.numeric_field.color_spring,
                                           &s.pane_scaffold.motion, &s.pane_scaffold.anchoring, &s.split_pane.handle_fade,
                                           &s.loading_indicator.common.morph_spring, &s.loading_indicator.common.handoff_spring,
                                           &s.drag_handle.size_spring, &s.drag_handle.color_spring, &s.zoom.settle_spring,
                                           &s.navigation_drawer_motion.open, &s.navigation_drawer_motion.close, &s.modal_navigation_rail.slide}) {
            *spring = restyle_spring;
        }
        s.loading_indicator.common.active_size -= restyle_length_add;
        s.wavy_progress.linear_width += restyle_length_add;
        s.split_pane.divider_thickness += restyle_length_add;
        s.floating_toolbar.item_slot += restyle_length_add;
        s.flexible_bottom_app_bar.height += restyle_length_add;
        s.wavy_progress.circular_size += restyle_length_add;
        s.wavy_progress.star_inner_radius *= static_cast<float>(restyle_corner_scale);
        for (motion::TweenSpec* tween : {&s.side_sheet_motion, &s.navigation_drawer_motion.settle, &s.wavy_progress_motion.progress,
                                         &s.wavy_progress_motion.amplitude_up, &s.wavy_progress_motion.amplitude_down,
                                         &s.wavy_progress_motion.circular_handoff_min, &s.wavy_progress_motion.circular_handoff_max}) {
            tween->duration_seconds *= restyle_duration_scale;
        }
        return s;
    }();
    return style;
}

void save_shot(thorvg::Canvas& canvas, const std::string& name) {
    const std::string folder = environment(shots_variable);
    if (folder.empty()) {
        return;
    }
    std::ofstream out(folder + "/" + name + "_" + std::to_string(canvas.width()) + "x" + std::to_string(canvas.height()) + ".bgra",
                      std::ios::binary);
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            const std::uint32_t argb = canvas.pixel(x, y);
            out.write(reinterpret_cast<const char*>(&argb), sizeof(argb));
        }
    }
}

}
