#include "lamina/shapes/rounded_polygon.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace lamina::shapes {

namespace {

constexpr double min_sin_angle_for_rounding = 1e-3;
constexpr float flanking_anchor_weight = 2.0f;
constexpr float flanking_anchor_divisor = 3.0f;
constexpr float max_bounds_sample_t = 0.5f;
constexpr float first_feature_split_t = 0.5f;

std::optional<Point> line_intersection(Point p0, Point d0, Point p1, Point d1) {
    const Point rotated_d1 = rotate90(d1);
    const float den = dot_product(d0, rotated_d1);
    if (std::abs(den) < distance_epsilon) {
        return std::nullopt;
    }
    const float num = dot_product(p1 - p0, rotated_d1);
    if (std::abs(den) < distance_epsilon * std::abs(num)) {
        return std::nullopt;
    }
    const float k = num / den;
    return p0 + d0 * k;
}

class RoundedCorner {
public:
    RoundedCorner(Point p0, Point p1, Point p2, CornerRounding rounding) : p0_(p0), p1_(p1), p2_(p2) {
        const Point v01 = p0_ - p1_;
        const Point v21 = p2_ - p1_;
        const float d01 = get_distance(v01);
        const float d21 = get_distance(v21);
        if (d01 > 0.0f && d21 > 0.0f) {
            d1_ = v01 / d01;
            d2_ = v21 / d21;
            corner_radius_ = rounding.radius;
            smoothing_ = rounding.smoothing;
            cos_angle_ = dot_product(d1_, d2_);
            sin_angle_ = std::sqrt(1.0f - square(cos_angle_));
            expected_round_cut_ =
                sin_angle_ > min_sin_angle_for_rounding ? corner_radius_ * (cos_angle_ + 1.0f) / sin_angle_ : 0.0f;
        }
    }

    float expected_round_cut() const { return expected_round_cut_; }
    float expected_cut() const { return (1.0f + smoothing_) * expected_round_cut_; }

    std::vector<Cubic> get_cubics(float allowed_cut0, float allowed_cut1) const {
        const float allowed_cut = std::min(allowed_cut0, allowed_cut1);
        if (expected_round_cut_ < distance_epsilon || allowed_cut < distance_epsilon ||
            corner_radius_ < distance_epsilon) {
            return {Cubic::straight_line(p1_.x, p1_.y, p1_.x, p1_.y)};
        }
        const float actual_round_cut = std::min(allowed_cut, expected_round_cut_);
        const float actual_smoothing0 = calculate_actual_smoothing_value(allowed_cut0);
        const float actual_smoothing1 = calculate_actual_smoothing_value(allowed_cut1);
        const float actual_r = corner_radius_ * actual_round_cut / expected_round_cut_;
        const float center_distance = std::sqrt(square(actual_r) + square(actual_round_cut));
        const Point arc_center = p1_ + get_direction((d1_ + d2_) / 2.0f) * center_distance;
        const Point circle_intersection0 = p1_ + d1_ * actual_round_cut;
        const Point circle_intersection2 = p1_ + d2_ * actual_round_cut;
        const Cubic flanking0 = compute_flanking_curve(actual_round_cut, actual_smoothing0, p1_, p0_,
                                                       circle_intersection0, circle_intersection2, arc_center, actual_r);
        const Cubic flanking2 = compute_flanking_curve(actual_round_cut, actual_smoothing1, p1_, p2_,
                                                       circle_intersection2, circle_intersection0, arc_center, actual_r)
                                    .reverse();
        return {
            flanking0,
            Cubic::circular_arc(arc_center.x, arc_center.y, flanking0.anchor1_x(), flanking0.anchor1_y(),
                                flanking2.anchor0_x(), flanking2.anchor0_y()),
            flanking2,
        };
    }

private:
    float calculate_actual_smoothing_value(float allowed_cut) const {
        if (allowed_cut > expected_cut()) {
            return smoothing_;
        }
        if (allowed_cut > expected_round_cut_) {
            return smoothing_ * (allowed_cut - expected_round_cut_) / (expected_cut() - expected_round_cut_);
        }
        return 0.0f;
    }

    static Cubic compute_flanking_curve(float actual_round_cut, float actual_smoothing_values, Point corner,
                                        Point side_start, Point circle_segment_intersection,
                                        Point other_circle_segment_intersection, Point circle_center, float actual_r) {
        const Point side_direction = get_direction(side_start - corner);
        const Point curve_start = corner + side_direction * actual_round_cut * (1.0f + actual_smoothing_values);
        const Point p = interpolate(circle_segment_intersection,
                                    (circle_segment_intersection + other_circle_segment_intersection) / 2.0f,
                                    actual_smoothing_values);
        const Point curve_end =
            circle_center + direction_vector(p.x - circle_center.x, p.y - circle_center.y) * actual_r;
        const Point circle_tangent = rotate90(curve_end - circle_center);
        const Point anchor_end = line_intersection(side_start, side_direction, curve_end, circle_tangent)
                                     .value_or(circle_segment_intersection);
        const Point anchor_start = (curve_start + anchor_end * flanking_anchor_weight) / flanking_anchor_divisor;
        return Cubic(curve_start, anchor_start, anchor_end, curve_end);
    }

    Point p0_;
    Point p1_;
    Point p2_;
    Point d1_{};
    Point d2_{};
    float corner_radius_ = 0.0f;
    float smoothing_ = 0.0f;
    float cos_angle_ = 0.0f;
    float sin_angle_ = 0.0f;
    float expected_round_cut_ = 0.0f;
};

std::vector<Point> vertices_from_num_verts(int num_vertices, float radius, Point center) {
    std::vector<Point> result;
    result.reserve(static_cast<std::size_t>(num_vertices));
    for (int i = 0; i < num_vertices; ++i) {
        result.push_back(radial_to_cartesian(radius, float_pi / static_cast<float>(num_vertices) * 2.0f *
                                                         static_cast<float>(i)) +
                         center);
    }
    return result;
}

std::vector<Cubic> build_cubics(const std::vector<Feature>& features, Point center) {
    std::vector<Cubic> result;
    std::optional<Cubic> first_cubic;
    std::optional<Cubic> last_cubic;
    std::optional<std::vector<Cubic>> first_feature_split_start;
    std::optional<std::vector<Cubic>> first_feature_split_end;
    if (!features.empty() && features[0].cubics().size() == 3) {
        const auto [start, end] = features[0].cubics()[1].split(first_feature_split_t);
        first_feature_split_start = std::vector<Cubic>{features[0].cubics()[0], start};
        first_feature_split_end = std::vector<Cubic>{end, features[0].cubics()[2]};
    }
    for (std::size_t i = 0; i <= features.size(); ++i) {
        const std::vector<Cubic>* feature_cubics = nullptr;
        if (i == 0 && first_feature_split_end) {
            feature_cubics = &*first_feature_split_end;
        } else if (i == features.size()) {
            if (!first_feature_split_start) {
                break;
            }
            feature_cubics = &*first_feature_split_start;
        } else {
            feature_cubics = &features[i].cubics();
        }
        for (const Cubic& cubic : *feature_cubics) {
            if (!cubic.zero_length()) {
                if (last_cubic) {
                    result.push_back(*last_cubic);
                }
                last_cubic = cubic;
                if (!first_cubic) {
                    first_cubic = cubic;
                }
            } else if (last_cubic) {
                last_cubic->points()[6] = cubic.anchor1_x();
                last_cubic->points()[7] = cubic.anchor1_y();
            }
        }
    }
    if (last_cubic && first_cubic) {
        result.emplace_back(last_cubic->anchor0_x(), last_cubic->anchor0_y(), last_cubic->control0_x(),
                            last_cubic->control0_y(), last_cubic->control1_x(), last_cubic->control1_y(),
                            first_cubic->anchor0_x(), first_cubic->anchor0_y());
    } else {
        result.emplace_back(center.x, center.y, center.x, center.y, center.x, center.y, center.x, center.y);
    }
    return result;
}

}

RoundedPolygon::RoundedPolygon(std::vector<Feature> features, Point center)
    : features_(std::move(features)), center_(center), cubics_(build_cubics(features_, center_)) {
    const Cubic* prev = &cubics_.back();
    for (const Cubic& cubic : cubics_) {
        if (std::abs(cubic.anchor0_x() - prev->anchor1_x()) > distance_epsilon ||
            std::abs(cubic.anchor0_y() - prev->anchor1_y()) > distance_epsilon) {
            throw std::invalid_argument(
                "RoundedPolygon must be contiguous, with the anchor points of all curves matching the anchor points "
                "of the preceding and succeeding cubics");
        }
        prev = &cubic;
    }
}

RoundedPolygon RoundedPolygon::regular(int num_vertices, float radius, Point center, CornerRounding rounding,
                                       std::span<const CornerRounding> per_vertex_rounding) {
    const std::vector<Point> vertices = vertices_from_num_verts(num_vertices, radius, center);
    return from_vertices(vertices, rounding, per_vertex_rounding, center);
}

RoundedPolygon RoundedPolygon::from_vertices(std::span<const Point> vertices, CornerRounding rounding,
                                             std::span<const CornerRounding> per_vertex_rounding,
                                             std::optional<Point> center) {
    if (vertices.size() < 3) {
        throw std::invalid_argument("Polygons must have at least 3 vertices");
    }
    if (!per_vertex_rounding.empty() && per_vertex_rounding.size() != vertices.size()) {
        throw std::invalid_argument(
            "perVertexRounding list should be either null or the same size as the number of vertices");
    }
    const std::size_t n = vertices.size();
    std::vector<RoundedCorner> rounded_corners;
    rounded_corners.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const CornerRounding vtx_rounding = per_vertex_rounding.empty() ? rounding : per_vertex_rounding[i];
        rounded_corners.emplace_back(vertices[(i + n - 1) % n], vertices[i], vertices[(i + 1) % n], vtx_rounding);
    }

    std::vector<std::pair<float, float>> cut_adjusts;
    cut_adjusts.reserve(n);
    for (std::size_t ix = 0; ix < n; ++ix) {
        const float expected_round_cut =
            rounded_corners[ix].expected_round_cut() + rounded_corners[(ix + 1) % n].expected_round_cut();
        const float expected_cut = rounded_corners[ix].expected_cut() + rounded_corners[(ix + 1) % n].expected_cut();
        const Point vtx = vertices[ix];
        const Point next_vtx = vertices[(ix + 1) % n];
        const float side_size = distance(vtx.x - next_vtx.x, vtx.y - next_vtx.y);
        if (expected_round_cut > side_size) {
            cut_adjusts.emplace_back(side_size / expected_round_cut, 0.0f);
        } else if (expected_cut > side_size) {
            cut_adjusts.emplace_back(1.0f, (side_size - expected_round_cut) / (expected_cut - expected_round_cut));
        } else {
            cut_adjusts.emplace_back(1.0f, 1.0f);
        }
    }

    std::vector<std::vector<Cubic>> corners;
    corners.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        float allowed_cuts[2]{};
        for (std::size_t delta = 0; delta <= 1; ++delta) {
            const auto [round_cut_ratio, cut_ratio] = cut_adjusts[(i + n - 1 + delta) % n];
            allowed_cuts[delta] =
                rounded_corners[i].expected_round_cut() * round_cut_ratio +
                (rounded_corners[i].expected_cut() - rounded_corners[i].expected_round_cut()) * cut_ratio;
        }
        corners.push_back(rounded_corners[i].get_cubics(allowed_cuts[0], allowed_cuts[1]));
    }

    std::vector<Feature> temp_features;
    temp_features.reserve(n * 2);
    for (std::size_t i = 0; i < n; ++i) {
        const Point prev_vertex = vertices[(i + n - 1) % n];
        const Point curr_vertex = vertices[i];
        const Point next_vertex = vertices[(i + 1) % n];
        const bool is_convex = convex(prev_vertex, curr_vertex, next_vertex);
        temp_features.push_back(Feature::corner(corners[i], is_convex));
        const Cubic& last = corners[i].back();
        const Cubic& next_first = corners[(i + 1) % n].front();
        temp_features.push_back(Feature::edge(
            {Cubic::straight_line(last.anchor1_x(), last.anchor1_y(), next_first.anchor0_x(), next_first.anchor0_y())}));
    }

    const Point c = center ? *center : calculate_center(vertices);
    return RoundedPolygon(std::move(temp_features), c);
}

RoundedPolygon RoundedPolygon::from_features(std::vector<Feature> features, std::optional<Point> center) {
    if (features.size() < 2) {
        throw std::invalid_argument("Polygons must have at least 2 features");
    }
    if (center) {
        return RoundedPolygon(std::move(features), *center);
    }
    std::vector<Point> vertices;
    for (const Feature& feature : features) {
        for (const Cubic& cubic : feature.cubics()) {
            vertices.push_back(cubic.anchor0());
        }
    }
    const Point c = calculate_center(vertices);
    return RoundedPolygon(std::move(features), c);
}

RoundedPolygon RoundedPolygon::transformed(const PointTransformer& f) const {
    std::vector<Feature> result;
    result.reserve(features_.size());
    for (const Feature& feature : features_) {
        result.push_back(feature.transformed(f));
    }
    return RoundedPolygon(std::move(result), f(center_));
}

RoundedPolygon RoundedPolygon::normalized() const {
    const Bounds bounds = calculate_bounds();
    const float width = bounds.right - bounds.left;
    const float height = bounds.bottom - bounds.top;
    const float side = std::max(width, height);
    const float offset_x = (side - width) / 2.0f - bounds.left;
    const float offset_y = (side - height) / 2.0f - bounds.top;
    return transformed([=](Point p) { return Point{(p.x + offset_x) / side, (p.y + offset_y) / side}; });
}

Bounds RoundedPolygon::calculate_max_bounds() const {
    float max_dist_squared = 0.0f;
    for (const Cubic& cubic : cubics_) {
        const float anchor_distance = distance_squared(cubic.anchor0_x() - center_.x, cubic.anchor0_y() - center_.y);
        const Point middle_point = cubic.point_on_curve(max_bounds_sample_t);
        const float middle_distance = distance_squared(middle_point.x - center_.x, middle_point.y - center_.y);
        max_dist_squared = std::max(max_dist_squared, std::max(anchor_distance, middle_distance));
    }
    const float d = std::sqrt(max_dist_squared);
    return {center_.x - d, center_.y - d, center_.x + d, center_.y + d};
}

Bounds RoundedPolygon::calculate_bounds(bool approximate) const {
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::denorm_min();
    float max_y = std::numeric_limits<float>::denorm_min();
    for (const Cubic& cubic : cubics_) {
        const Bounds b = cubic.calculate_bounds(approximate);
        min_x = std::min(min_x, b.left);
        min_y = std::min(min_y, b.top);
        max_x = std::max(max_x, b.right);
        max_y = std::max(max_y, b.bottom);
    }
    return {min_x, min_y, max_x, max_y};
}

Point calculate_center(std::span<const Point> vertices) {
    float cumulative_x = 0.0f;
    float cumulative_y = 0.0f;
    for (const Point& v : vertices) {
        cumulative_x += v.x;
        cumulative_y += v.y;
    }
    const auto count = static_cast<float>(static_cast<int>(vertices.size()));
    return {cumulative_x / count, cumulative_y / count};
}

}
