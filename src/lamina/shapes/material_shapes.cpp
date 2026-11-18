#include "lamina/shapes/material_shapes.hpp"

#include <cmath>
#include <numbers>
#include <span>
#include <utility>
#include <vector>

#include "lamina/shapes/shapes.hpp"

namespace lamina::shapes {

namespace {

struct PointNRound {
    Point o;
    CornerRounding r = unrounded;
};

constexpr CornerRounding corner_round15{0.15f};
constexpr CornerRounding corner_round20{0.2f};
constexpr CornerRounding corner_round30{0.3f};
constexpr CornerRounding corner_round50{0.5f};
constexpr CornerRounding corner_round100{1.0f};

constexpr float rotate_neg45 = -45.0f;
constexpr float rotate_neg90 = -90.0f;
constexpr float rotate_neg135 = -135.0f;

constexpr Point custom_polygon_center{0.5f, 0.5f};
constexpr float full_turn_degrees = 360.0f;
constexpr float half_turn_degrees = 180.0f;
constexpr double degrees_to_radians = std::numbers::pi / 180.0;

constexpr int material_circle_vertices = 10;
constexpr float square_side = 1.0f;
constexpr int arch_vertices = 4;
constexpr float semi_circle_width = 1.6f;
constexpr float semi_circle_height = 1.0f;
constexpr float oval_scale_y = 0.64f;
constexpr int triangle_vertices = 3;
constexpr int sunny_vertices_per_radius = 8;
constexpr float sunny_inner_radius = 0.8f;
constexpr int cookie7_vertices_per_radius = 7;
constexpr float cookie7_inner_radius = 0.75f;
constexpr int cookie9_vertices_per_radius = 9;
constexpr float cookie9_inner_radius = 0.8f;
constexpr int cookie12_vertices_per_radius = 12;
constexpr float cookie12_inner_radius = 0.8f;
constexpr float puffy_scale_y = 0.742f;

struct ComposeMatrix {
    float v00 = 1.0f;
    float v01 = 0.0f;
    float v10 = 0.0f;
    float v11 = 1.0f;

    Point map(Point p) const { return {v00 * p.x + v10 * p.y + 0.0f, v01 * p.x + v11 * p.y + 0.0f}; }
};

ComposeMatrix rotate_z(float degrees) {
    const double r = static_cast<double>(degrees) * degrees_to_radians;
    const auto s = static_cast<float>(std::sin(r));
    const auto c = static_cast<float>(std::cos(r));
    return {c, s, -s, c};
}

ComposeMatrix scale(float x, float y) {
    return {x, 0.0f, 0.0f, y};
}

RoundedPolygon transformed(const RoundedPolygon& polygon, const ComposeMatrix& m) {
    return polygon.transformed([m](Point p) { return m.map(p); });
}

float to_radians(float degrees) {
    return degrees / full_turn_degrees * 2.0f * float_pi;
}

float angle_degrees(Point p) {
    return jvm_atan2(p.y, p.x) * half_turn_degrees / float_pi;
}

Point rotate_degrees(Point p, float angle, Point center) {
    const float a = to_radians(angle);
    const Point off = p - center;
    return Point{off.x * jvm_cos(a) - off.y * jvm_sin(a), off.x * jvm_sin(a) + off.y * jvm_cos(a)} + center;
}

std::vector<PointNRound> do_repeat(std::span<const PointNRound> points, int reps, Point center, bool mirroring) {
    std::vector<PointNRound> result;
    if (mirroring) {
        std::vector<float> angles;
        std::vector<float> distances;
        for (const PointNRound& p : points) {
            angles.push_back(angle_degrees(p.o - center));
            distances.push_back(get_distance(p.o - center));
        }
        const int actual_reps = reps * 2;
        const float section_angle = full_turn_degrees / static_cast<float>(actual_reps);
        const auto last_index = static_cast<int>(points.size()) - 1;
        for (int it = 0; it < actual_reps; ++it) {
            for (int index = 0; index <= last_index; ++index) {
                const int i = it % 2 == 0 ? index : last_index - index;
                if (i > 0 || it % 2 == 0) {
                    const auto ui = static_cast<std::size_t>(i);
                    const float a =
                        to_radians(section_angle * static_cast<float>(it) +
                                   (it % 2 == 0 ? angles[ui] : section_angle - angles[ui] + 2.0f * angles[0]));
                    const Point final_point = Point{jvm_cos(a), jvm_sin(a)} * distances[ui] + center;
                    result.push_back({final_point, points[ui].r});
                }
            }
        }
    } else {
        const auto np = static_cast<int>(points.size());
        for (int it = 0; it < np * reps; ++it) {
            const auto& source = points[static_cast<std::size_t>(it % np)];
            const Point point = rotate_degrees(
                source.o, static_cast<float>(it / np) * full_turn_degrees / static_cast<float>(reps), center);
            result.push_back({point, source.r});
        }
    }
    return result;
}

RoundedPolygon custom_polygon(std::span<const PointNRound> pnr, int reps, bool mirroring = false,
                              Point center = custom_polygon_center) {
    const std::vector<PointNRound> actual_points = do_repeat(pnr, reps, center, mirroring);
    std::vector<Point> vertices;
    std::vector<CornerRounding> per_vertex_rounding;
    vertices.reserve(actual_points.size());
    per_vertex_rounding.reserve(actual_points.size());
    for (const PointNRound& p : actual_points) {
        vertices.push_back(p.o);
        per_vertex_rounding.push_back(p.r);
    }
    return RoundedPolygon::from_vertices(vertices, unrounded, per_vertex_rounding, center);
}

constexpr PointNRound slanted_points[] = {
    {{0.926f, 0.970f}, {0.189f, 0.811f}},
    {{-0.021f, 0.967f}, {0.187f, 0.057f}},
};
constexpr int slanted_reps = 2;

constexpr CornerRounding arch_rounding[] = {corner_round100, corner_round100, corner_round20, corner_round20};

constexpr PointNRound fan_points[] = {
    {{1.004f, 1.000f}, {0.148f, 0.417f}},
    {{0.000f, 1.000f}, {0.151f}},
    {{0.000f, -0.003f}, {0.148f}},
    {{0.978f, 0.020f}, {0.803f}},
};
constexpr int fan_reps = 1;

constexpr PointNRound arrow_points[] = {
    {{0.500f, 0.892f}, {0.313f}},
    {{-0.216f, 1.050f}, {0.207f}},
    {{0.499f, -0.160f}, {0.215f, 1.000f}},
    {{1.225f, 1.060f}, {0.211f}},
};
constexpr int arrow_reps = 1;

constexpr CornerRounding semi_circle_rounding[] = {corner_round20, corner_round20, corner_round100,
                                                   corner_round100};

constexpr PointNRound pill_points[] = {
    {{0.961f, 0.039f}, {0.426f}},
    {{1.001f, 0.428f}},
    {{1.000f, 0.609f}, {1.000f}},
};
constexpr int pill_reps = 2;

constexpr PointNRound diamond_points[] = {
    {{0.500f, 1.096f}, {0.151f, 0.524f}},
    {{0.040f, 0.500f}, {0.159f}},
};
constexpr int diamond_reps = 2;

constexpr PointNRound clam_shell_points[] = {
    {{0.171f, 0.841f}, {0.159f}},
    {{-0.020f, 0.500f}, {0.140f}},
    {{0.170f, 0.159f}, {0.159f}},
};
constexpr int clam_shell_reps = 2;

constexpr PointNRound pentagon_points[] = {
    {{0.500f, -0.009f}, {0.172f}},
    {{1.030f, 0.365f}, {0.164f}},
    {{0.828f, 0.970f}, {0.169f}},
};
constexpr int pentagon_reps = 1;

constexpr PointNRound gem_points[] = {
    {{0.499f, 1.023f}, {0.241f, 0.778f}},
    {{-0.005f, 0.792f}, {0.208f}},
    {{0.073f, 0.258f}, {0.228f}},
    {{0.433f, -0.000f}, {0.491f}},
};
constexpr int gem_reps = 1;

constexpr PointNRound very_sunny_points[] = {
    {{0.500f, 1.080f}, {0.085f}},
    {{0.358f, 0.843f}, {0.085f}},
};
constexpr int very_sunny_reps = 8;

constexpr PointNRound cookie4_points[] = {
    {{1.237f, 1.236f}, {0.258f}},
    {{0.500f, 0.918f}, {0.233f}},
};
constexpr int cookie4_reps = 4;

constexpr PointNRound cookie6_points[] = {
    {{0.723f, 0.884f}, {0.394f}},
    {{0.500f, 1.099f}, {0.398f}},
};
constexpr int cookie6_reps = 6;

constexpr PointNRound ghostish_points[] = {
    {{0.500f, 0.0f}, {1.000f}},
    {{1.0f, 0.0f}, {1.000f}},
    {{1.0f, 1.140f}, {0.254f, 0.106f}},
    {{0.575f, 0.906f}, {0.253f}},
};
constexpr int ghostish_reps = 1;

constexpr PointNRound clover4_points[] = {
    {{0.500f, 0.074f}},
    {{0.725f, -0.099f}, {0.476f}},
};
constexpr int clover4_reps = 4;

constexpr PointNRound clover8_points[] = {
    {{0.500f, 0.036f}},
    {{0.758f, -0.101f}, {0.209f}},
};
constexpr int clover8_reps = 8;

constexpr PointNRound burst_points[] = {
    {{0.500f, -0.006f}, {0.006f}},
    {{0.592f, 0.158f}, {0.006f}},
};
constexpr int burst_reps = 12;

constexpr PointNRound soft_burst_points[] = {
    {{0.193f, 0.277f}, {0.053f}},
    {{0.176f, 0.055f}, {0.053f}},
};
constexpr int soft_burst_reps = 10;

constexpr PointNRound boom_points[] = {
    {{0.457f, 0.296f}, {0.007f}},
    {{0.500f, -0.051f}, {0.007f}},
};
constexpr int boom_reps = 15;

constexpr PointNRound soft_boom_points[] = {
    {{0.733f, 0.454f}},
    {{0.839f, 0.437f}, {0.532f}},
    {{0.949f, 0.449f}, {0.439f, 1.000f}},
    {{0.998f, 0.478f}, {0.174f}},
};
constexpr int soft_boom_reps = 16;

constexpr PointNRound flower_points[] = {
    {{0.370f, 0.187f}},
    {{0.416f, 0.049f}, {0.381f}},
    {{0.479f, 0.001f}, {0.095f}},
};
constexpr int flower_reps = 8;

constexpr PointNRound puffy_points[] = {
    {{0.500f, 0.053f}},
    {{0.545f, -0.040f}, {0.405f}},
    {{0.670f, -0.035f}, {0.426f}},
    {{0.717f, 0.066f}, {0.574f}},
    {{0.722f, 0.128f}},
    {{0.777f, 0.002f}, {0.360f}},
    {{0.914f, 0.149f}, {0.660f}},
    {{0.926f, 0.289f}, {0.660f}},
    {{0.881f, 0.346f}},
    {{0.940f, 0.344f}, {0.126f}},
    {{1.003f, 0.437f}, {0.255f}},
};
constexpr int puffy_reps = 2;

constexpr PointNRound puffy_diamond_points[] = {
    {{0.870f, 0.130f}, {0.146f}},
    {{0.818f, 0.357f}},
    {{1.000f, 0.332f}, {0.853f}},
};
constexpr int puffy_diamond_reps = 4;

constexpr PointNRound pixel_circle_points[] = {
    {{0.500f, 0.000f}}, {{0.704f, 0.000f}}, {{0.704f, 0.065f}}, {{0.843f, 0.065f}},
    {{0.843f, 0.148f}}, {{0.926f, 0.148f}}, {{0.926f, 0.296f}}, {{1.000f, 0.296f}},
};
constexpr int pixel_circle_reps = 2;

constexpr PointNRound pixel_triangle_points[] = {
    {{0.110f, 0.500f}}, {{0.113f, 0.000f}}, {{0.287f, 0.000f}}, {{0.287f, 0.087f}}, {{0.421f, 0.087f}},
    {{0.421f, 0.170f}}, {{0.560f, 0.170f}}, {{0.560f, 0.265f}}, {{0.674f, 0.265f}}, {{0.675f, 0.344f}},
    {{0.789f, 0.344f}}, {{0.789f, 0.439f}}, {{0.888f, 0.439f}},
};
constexpr int pixel_triangle_reps = 1;

constexpr PointNRound bun_points[] = {
    {{0.796f, 0.500f}},
    {{0.853f, 0.518f}, {1.0f}},
    {{0.992f, 0.631f}, {1.0f}},
    {{0.968f, 1.000f}, {1.0f}},
};
constexpr int bun_reps = 2;

constexpr PointNRound heart_points[] = {
    {{0.500f, 0.268f}, {0.016f}},
    {{0.792f, -0.066f}, {0.958f}},
    {{1.064f, 0.276f}, {1.000f}},
    {{0.501f, 0.946f}, {0.129f}},
};
constexpr int heart_reps = 1;

std::array<RoundedPolygon, material_shape_count> build_normalized() {
    return []<std::size_t... I>(std::index_sequence<I...>) {
        return std::array<RoundedPolygon, material_shape_count>{
            material_shape_unnormalized(static_cast<MaterialShape>(I)).normalized()...};
    }(std::make_index_sequence<material_shape_count>{});
}

std::array<MaterialShape, material_shape_count> build_all() {
    std::array<MaterialShape, material_shape_count> result{};
    for (std::size_t i = 0; i < material_shape_count; ++i) {
        result[i] = static_cast<MaterialShape>(i);
    }
    return result;
}

}

const std::array<MaterialShape, material_shape_count>& all_material_shapes() {
    static const std::array<MaterialShape, material_shape_count> all = build_all();
    return all;
}

const RoundedPolygon& material_shape(MaterialShape shape) {
    static const std::array<RoundedPolygon, material_shape_count> cache = build_normalized();
    return cache[static_cast<std::size_t>(shape)];
}

RoundedPolygon material_shape_unnormalized(MaterialShape shape) {
    switch (shape) {
    case MaterialShape::circle:
        return circle(material_circle_vertices);
    case MaterialShape::square:
        return rectangle(square_side, square_side, corner_round30);
    case MaterialShape::slanted:
        return custom_polygon(slanted_points, slanted_reps);
    case MaterialShape::arch:
        return transformed(RoundedPolygon::regular(arch_vertices, 1.0f, {}, unrounded, arch_rounding),
                           rotate_z(rotate_neg135));
    case MaterialShape::fan:
        return custom_polygon(fan_points, fan_reps);
    case MaterialShape::arrow:
        return custom_polygon(arrow_points, arrow_reps);
    case MaterialShape::semi_circle:
        return rectangle(semi_circle_width, semi_circle_height, unrounded, semi_circle_rounding);
    case MaterialShape::oval:
        return transformed(transformed(circle(), scale(1.0f, oval_scale_y)), rotate_z(rotate_neg45));
    case MaterialShape::pill:
        return custom_polygon(pill_points, pill_reps, true);
    case MaterialShape::triangle:
        return transformed(RoundedPolygon::regular(triangle_vertices, 1.0f, {}, corner_round20),
                           rotate_z(rotate_neg90));
    case MaterialShape::diamond:
        return custom_polygon(diamond_points, diamond_reps);
    case MaterialShape::clam_shell:
        return custom_polygon(clam_shell_points, clam_shell_reps);
    case MaterialShape::pentagon:
        return custom_polygon(pentagon_points, pentagon_reps, true);
    case MaterialShape::gem:
        return custom_polygon(gem_points, gem_reps, true);
    case MaterialShape::sunny:
        return star(sunny_vertices_per_radius, 1.0f, sunny_inner_radius, corner_round15);
    case MaterialShape::very_sunny:
        return custom_polygon(very_sunny_points, very_sunny_reps);
    case MaterialShape::cookie_4_sided:
        return custom_polygon(cookie4_points, cookie4_reps);
    case MaterialShape::cookie_6_sided:
        return custom_polygon(cookie6_points, cookie6_reps);
    case MaterialShape::cookie_7_sided:
        return transformed(star(cookie7_vertices_per_radius, 1.0f, cookie7_inner_radius, corner_round50),
                           rotate_z(rotate_neg90));
    case MaterialShape::cookie_9_sided:
        return transformed(star(cookie9_vertices_per_radius, 1.0f, cookie9_inner_radius, corner_round50),
                           rotate_z(rotate_neg90));
    case MaterialShape::cookie_12_sided:
        return transformed(star(cookie12_vertices_per_radius, 1.0f, cookie12_inner_radius, corner_round50),
                           rotate_z(rotate_neg90));
    case MaterialShape::ghostish:
        return custom_polygon(ghostish_points, ghostish_reps, true);
    case MaterialShape::clover_4_leaf:
        return custom_polygon(clover4_points, clover4_reps, true);
    case MaterialShape::clover_8_leaf:
        return custom_polygon(clover8_points, clover8_reps);
    case MaterialShape::burst:
        return custom_polygon(burst_points, burst_reps);
    case MaterialShape::soft_burst:
        return custom_polygon(soft_burst_points, soft_burst_reps);
    case MaterialShape::boom:
        return custom_polygon(boom_points, boom_reps);
    case MaterialShape::soft_boom:
        return custom_polygon(soft_boom_points, soft_boom_reps, true);
    case MaterialShape::flower:
        return custom_polygon(flower_points, flower_reps, true);
    case MaterialShape::puffy:
        return transformed(custom_polygon(puffy_points, puffy_reps, true), scale(1.0f, puffy_scale_y));
    case MaterialShape::puffy_diamond:
        return custom_polygon(puffy_diamond_points, puffy_diamond_reps, true);
    case MaterialShape::pixel_circle:
        return custom_polygon(pixel_circle_points, pixel_circle_reps, true);
    case MaterialShape::pixel_triangle:
        return custom_polygon(pixel_triangle_points, pixel_triangle_reps, true);
    case MaterialShape::bun:
        return custom_polygon(bun_points, bun_reps, true);
    case MaterialShape::heart:
        return custom_polygon(heart_points, heart_reps, true);
    }
    return circle(material_circle_vertices);
}

}
