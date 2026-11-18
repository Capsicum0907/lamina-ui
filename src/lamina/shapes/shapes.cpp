#include "lamina/shapes/shapes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace lamina::shapes {

namespace {

constexpr int min_circle_vertices = 3;
constexpr std::size_t pill_star_section_count = 11;
constexpr float quarter_turn = float_pi / 2.0f;
constexpr float three_quarter_turn_factor = 1.5f;

std::vector<CornerRounding> alternate_rounding(int num_vertices_per_radius, CornerRounding rounding,
                                               CornerRounding inner_rounding) {
    std::vector<CornerRounding> result;
    result.reserve(static_cast<std::size_t>(num_vertices_per_radius) * 2);
    for (int i = 0; i < num_vertices_per_radius; ++i) {
        result.push_back(rounding);
        result.push_back(inner_rounding);
    }
    return result;
}

std::vector<Point> star_vertices_from_num_verts(int num_vertices_per_radius, float radius, float inner_radius,
                                                Point center) {
    std::vector<Point> result;
    result.reserve(static_cast<std::size_t>(num_vertices_per_radius) * 2);
    const auto n = static_cast<float>(num_vertices_per_radius);
    for (int i = 0; i < num_vertices_per_radius; ++i) {
        Point vertex = radial_to_cartesian(radius, float_pi / n * 2.0f * static_cast<float>(i));
        result.push_back({vertex.x + center.x, vertex.y + center.y});
        vertex = radial_to_cartesian(inner_radius, float_pi / n * static_cast<float>(2 * i + 1));
        result.push_back({vertex.x + center.x, vertex.y + center.y});
    }
    return result;
}

std::vector<Point> pill_star_vertices_from_num_verts(int num_vertices_per_radius, float width, float height,
                                                     float inner_radius, float vertex_spacing, float start_location,
                                                     Point center) {
    const float endcap_radius = std::min(width, height);
    const float v_seg_len = std::max(height - width, 0.0f);
    const float h_seg_len = std::max(width - height, 0.0f);
    const float v_seg_half = v_seg_len / 2.0f;
    const float h_seg_half = h_seg_len / 2.0f;
    const float circle_perimeter = two_pi * endcap_radius * interpolate(inner_radius, 1.0f, vertex_spacing);
    const float perimeter = 2.0f * h_seg_len + 2.0f * v_seg_len + circle_perimeter;

    std::array<float, pill_star_section_count> sections{};
    sections[0] = 0.0f;
    sections[1] = v_seg_len / 2.0f;
    sections[2] = sections[1] + circle_perimeter / 4.0f;
    sections[3] = sections[2] + h_seg_len;
    sections[4] = sections[3] + circle_perimeter / 4.0f;
    sections[5] = sections[4] + v_seg_len;
    sections[6] = sections[5] + circle_perimeter / 4.0f;
    sections[7] = sections[6] + h_seg_len;
    sections[8] = sections[7] + circle_perimeter / 4.0f;
    sections[9] = sections[8] + v_seg_len / 2.0f;
    sections[10] = perimeter;

    const float t_per_vertex = perimeter / static_cast<float>(2 * num_vertices_per_radius);
    bool inner = false;
    std::size_t curr_sec_index = 0;
    float sec_start = 0.0f;
    float sec_end = sections[1];
    float t = start_location * perimeter;
    std::vector<Point> result;
    result.reserve(static_cast<std::size_t>(num_vertices_per_radius) * 2);
    const Point rect_br{h_seg_half, v_seg_half};
    const Point rect_bl{-h_seg_half, v_seg_half};
    const Point rect_tl{-h_seg_half, -v_seg_half};
    const Point rect_tr{h_seg_half, -v_seg_half};
    for (int i = 0; i < num_vertices_per_radius * 2; ++i) {
        const float bounded_t = std::fmod(t, perimeter);
        if (bounded_t < sec_start) {
            curr_sec_index = 0;
        }
        while (bounded_t >= sections[(curr_sec_index + 1) % sections.size()]) {
            curr_sec_index = (curr_sec_index + 1) % sections.size();
            sec_start = sections[curr_sec_index];
            sec_end = sections[(curr_sec_index + 1) % sections.size()];
        }
        const float t_in_section = bounded_t - sec_start;
        const float t_proportion = t_in_section / (sec_end - sec_start);
        const float curr_radius = inner ? (endcap_radius * inner_radius) : endcap_radius;
        Point vertex;
        switch (curr_sec_index) {
        case 0:
            vertex = {curr_radius, t_proportion * v_seg_half};
            break;
        case 1:
            vertex = radial_to_cartesian(curr_radius, t_proportion * float_pi / 2.0f) + rect_br;
            break;
        case 2:
            vertex = {h_seg_half - t_proportion * h_seg_len, curr_radius};
            break;
        case 3:
            vertex = radial_to_cartesian(curr_radius, quarter_turn + (t_proportion * float_pi / 2.0f)) + rect_bl;
            break;
        case 4:
            vertex = {-curr_radius, v_seg_half - t_proportion * v_seg_len};
            break;
        case 5:
            vertex = radial_to_cartesian(curr_radius, float_pi + (t_proportion * float_pi / 2.0f)) + rect_tl;
            break;
        case 6:
            vertex = {-h_seg_half + t_proportion * h_seg_len, -curr_radius};
            break;
        case 7:
            vertex = radial_to_cartesian(curr_radius,
                                         float_pi * three_quarter_turn_factor + (t_proportion * float_pi / 2.0f)) +
                     rect_tr;
            break;
        default:
            vertex = {curr_radius, -v_seg_half + t_proportion * v_seg_half};
            break;
        }
        result.push_back({vertex.x + center.x, vertex.y + center.y});
        t += t_per_vertex;
        inner = !inner;
    }
    return result;
}

}

RoundedPolygon circle(int num_vertices, float radius, Point center) {
    if (num_vertices < min_circle_vertices) {
        throw std::invalid_argument("Circle must have at least three vertices");
    }
    const float theta = float_pi / static_cast<float>(num_vertices);
    const float polygon_radius = radius / jvm_cos(theta);
    return RoundedPolygon::regular(num_vertices, polygon_radius, center, CornerRounding{radius});
}

RoundedPolygon rectangle(float width, float height, CornerRounding rounding,
                         std::span<const CornerRounding> per_vertex_rounding, Point center) {
    const float left = center.x - width / 2.0f;
    const float top = center.y - height / 2.0f;
    const float right = center.x + width / 2.0f;
    const float bottom = center.y + height / 2.0f;
    const std::array<Point, 4> vertices{Point{right, bottom}, Point{left, bottom}, Point{left, top}, Point{right, top}};
    return RoundedPolygon::from_vertices(vertices, rounding, per_vertex_rounding, center);
}

RoundedPolygon star(int num_vertices_per_radius, float radius, float inner_radius, CornerRounding rounding,
                    std::optional<CornerRounding> inner_rounding, std::span<const CornerRounding> per_vertex_rounding,
                    Point center) {
    if (radius <= 0.0f || inner_radius <= 0.0f) {
        throw std::invalid_argument("Star radii must both be greater than 0");
    }
    if (inner_radius >= radius) {
        throw std::invalid_argument("innerRadius must be less than radius");
    }
    std::vector<CornerRounding> generated;
    std::span<const CornerRounding> pv_rounding = per_vertex_rounding;
    if (pv_rounding.empty() && inner_rounding) {
        generated = alternate_rounding(num_vertices_per_radius, rounding, *inner_rounding);
        pv_rounding = generated;
    }
    const std::vector<Point> vertices =
        star_vertices_from_num_verts(num_vertices_per_radius, radius, inner_radius, center);
    return RoundedPolygon::from_vertices(vertices, rounding, pv_rounding, center);
}

RoundedPolygon pill(float width, float height, float smoothing, Point center) {
    if (!(width > 0.0f && height > 0.0f)) {
        throw std::invalid_argument("Pill shapes must have positive width and height");
    }
    const float w_half = width / 2.0f;
    const float h_half = height / 2.0f;
    const std::array<Point, 4> vertices{
        Point{w_half + center.x, h_half + center.y},
        Point{-w_half + center.x, h_half + center.y},
        Point{-w_half + center.x, -h_half + center.y},
        Point{w_half + center.x, -h_half + center.y},
    };
    return RoundedPolygon::from_vertices(vertices, CornerRounding{std::min(w_half, h_half), smoothing}, {}, center);
}

RoundedPolygon pill_star(const PillStarOptions& options) {
    if (!(options.width > 0.0f && options.height > 0.0f)) {
        throw std::invalid_argument("Pill shapes must have positive width and height");
    }
    if (!(options.inner_radius_ratio > 0.0f && options.inner_radius_ratio <= 1.0f)) {
        throw std::invalid_argument("innerRadius must be between 0 and 1");
    }
    std::vector<CornerRounding> generated;
    std::span<const CornerRounding> pv_rounding = options.per_vertex_rounding;
    if (pv_rounding.empty() && options.inner_rounding) {
        generated = alternate_rounding(options.num_vertices_per_radius, options.rounding, *options.inner_rounding);
        pv_rounding = generated;
    }
    const std::vector<Point> vertices = pill_star_vertices_from_num_verts(
        options.num_vertices_per_radius, options.width, options.height, options.inner_radius_ratio,
        options.vertex_spacing, options.start_location, options.center);
    return RoundedPolygon::from_vertices(vertices, options.rounding, pv_rounding, options.center);
}

}
