#include "restyle_support.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>

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

void save_shot(thorvg::Canvas& canvas, const std::string& name) {
    char* dir = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&dir, &length, shots_variable) != 0 || !dir) {
        return;
    }
    const std::string folder(dir);
    std::free(dir);
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
