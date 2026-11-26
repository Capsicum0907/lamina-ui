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
