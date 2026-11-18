#include <array>
#include <cstdio>
#include <string>
#include <vector>

#include "lamina/shapes/material_shapes.hpp"
#include "lamina/shapes/morph.hpp"
#include "lamina/shapes/shapes.hpp"

using namespace lamina::shapes;

namespace {

void dump(const std::string& tag, const std::vector<Cubic>& cubics) {
    std::printf("%s %zu", tag.c_str(), cubics.size());
    for (const Cubic& c : cubics) {
        const std::array<float, 8> v{c.anchor0_x(),  c.anchor0_y(),  c.control0_x(), c.control0_y(),
                                     c.control1_x(), c.control1_y(), c.anchor1_x(),  c.anchor1_y()};
        for (float x : v) {
            std::printf(" %.9g", static_cast<double>(x));
        }
    }
    std::printf("\n");
}

std::string progress_tag(float p) {
    if (p == 0.0f) {
        return "0.0";
    }
    if (p == 1.0f) {
        return "1.0";
    }
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%g", static_cast<double>(p));
    return buf;
}

}

int main() {
    const auto& all = all_material_shapes();
    for (std::size_t i = 0; i < all.size(); ++i) {
        dump("shape" + std::to_string(i), material_shape(all[i]).cubics());
    }
    for (std::size_t i = 0; i + 1 < all.size(); ++i) {
        const Morph m(material_shape(all[i]), material_shape(all[i + 1]));
        for (float p : {0.0f, 0.3f, 1.0f}) {
            dump("morph" + std::to_string(i) + "@" + progress_tag(p), m.as_cubics(p));
        }
    }
    dump("smooth", rectangle(2.0f, 2.0f, {0.5f, 1.0f}).cubics());
    PillStarOptions o;
    o.rounding = {0.1f, 0.0f};
    dump("pillstar", pill_star(o).cubics());
    dump("pill", pill(3.0f, 1.0f, 0.5f).cubics());
    return 0;
}
