#include "restyle_support.hpp"

namespace lamina::test {

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
        return s;
    }();
    return style;
}

}
