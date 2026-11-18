#include "lamina/shapes/to_path.hpp"

namespace lamina::shapes {

namespace {

render::Point to_render(Point p) {
    return {static_cast<double>(p.x), static_cast<double>(p.y)};
}

}

render::Path to_path(std::span<const Cubic> cubics) {
    render::Path path;
    if (cubics.empty()) {
        return path;
    }
    path.move_to(to_render(cubics.front().anchor0()));
    for (const Cubic& c : cubics) {
        path.cubic_to(to_render(c.control0()), to_render(c.control1()), to_render(c.anchor1()));
    }
    path.close();
    return path;
}

render::Path to_path(const RoundedPolygon& polygon) {
    return to_path(polygon.cubics());
}

render::Path to_path(const Morph& morph, float progress) {
    return to_path(morph.as_cubics(progress));
}

}
