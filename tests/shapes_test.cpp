#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "lamina/shapes/material_shapes.hpp"
#include "lamina/shapes/morph.hpp"
#include "lamina/shapes/rounded_polygon.hpp"
#include "lamina/shapes/shapes.hpp"
#include "lamina/shapes/to_path.hpp"

namespace sh = lamina::shapes;

namespace {

constexpr int samples_per_cubic = 8;
constexpr int outline_samples_per_cubic = 32;
constexpr float bounds_epsilon = 1e-4f;

std::vector<sh::Point> sample(const std::vector<sh::Cubic>& cubics, int per_cubic = samples_per_cubic) {
    std::vector<sh::Point> points;
    for (const sh::Cubic& c : cubics) {
        for (int i = 0; i < per_cubic; ++i) {
            points.push_back(c.point_on_curve(static_cast<float>(i) / static_cast<float>(per_cubic)));
        }
    }
    return points;
}

float distance_to_segment(sh::Point p, sh::Point a, sh::Point b) {
    const sh::Point ab = b - a;
    const float len2 = sh::get_distance_squared(ab);
    const float t = len2 > 0.0f ? std::clamp(sh::dot_product(p - a, ab) / len2, 0.0f, 1.0f) : 0.0f;
    return sh::get_distance(p - (a + ab * t));
}

float max_distance_to_outline(const std::vector<sh::Point>& from, const std::vector<sh::Point>& outline) {
    float worst = 0.0f;
    for (const sh::Point& p : from) {
        float best = std::numeric_limits<float>::max();
        for (std::size_t i = 0; i < outline.size(); ++i) {
            best = std::min(best, distance_to_segment(p, outline[i], outline[(i + 1) % outline.size()]));
        }
        worst = std::max(worst, best);
    }
    return worst;
}

float outline_difference(const std::vector<sh::Cubic>& a, const std::vector<sh::Cubic>& b) {
    const auto pa = sample(a);
    const auto pb = sample(b);
    const auto outline_a = sample(a, outline_samples_per_cubic);
    const auto outline_b = sample(b, outline_samples_per_cubic);
    return std::max(max_distance_to_outline(pa, outline_b), max_distance_to_outline(pb, outline_a));
}

int corner_count(const sh::RoundedPolygon& p) {
    return static_cast<int>(std::count_if(p.features().begin(), p.features().end(),
                                          [](const sh::Feature& f) { return f.kind() == sh::FeatureKind::corner; }));
}

bool is_closed(const std::vector<sh::Cubic>& cubics) {
    const sh::Point first = cubics.front().anchor0();
    const sh::Point last = cubics.back().anchor1();
    return first == last;
}

bool all_finite(const std::vector<sh::Cubic>& cubics) {
    for (const sh::Cubic& c : cubics) {
        for (const float v : c.points()) {
            if (!std::isfinite(v)) {
                return false;
            }
        }
    }
    return true;
}

}

TEST_CASE("unrounded regular polygon has one corner and one cubic per vertex") {
    for (int n = 3; n <= 12; ++n) {
        const auto polygon = sh::RoundedPolygon::regular(n);
        CHECK(corner_count(polygon) == n);
        CHECK(polygon.cubics().size() == static_cast<std::size_t>(n));
        CHECK(is_closed(polygon.cubics()));
    }
}

TEST_CASE("regular polygon vertices lie on the radius around the center") {
    const sh::Point center{3.0f, -2.0f};
    const auto polygon = sh::RoundedPolygon::regular(6, 2.0f, center);
    CHECK(polygon.center_x() == doctest::Approx(3.0f));
    CHECK(polygon.center_y() == doctest::Approx(-2.0f));
    for (const sh::Cubic& c : polygon.cubics()) {
        CHECK(sh::get_distance(c.anchor0() - center) == doctest::Approx(2.0f).epsilon(1e-5));
    }
}

TEST_CASE("rounded rectangle polygon keeps its exact bounds") {
    const auto polygon = sh::rectangle(4.0f, 2.0f, sh::CornerRounding{0.5f});
    const sh::Bounds b = polygon.calculate_bounds(false);
    CHECK(b.left == doctest::Approx(-2.0f).epsilon(1e-4));
    CHECK(b.right == doctest::Approx(2.0f).epsilon(1e-4));
    CHECK(b.top == doctest::Approx(-1.0f).epsilon(1e-4));
    CHECK(b.bottom == doctest::Approx(1.0f).epsilon(1e-4));
    CHECK(corner_count(polygon) == 4);
    CHECK(polygon.cubics().size() == 4 + 4 + 1);
}

TEST_CASE("circle shape polygon stays on its radius") {
    const auto polygon = sh::circle(8, 1.0f);
    for (const sh::Point& p : sample(polygon.cubics(), 32)) {
        CHECK(sh::get_distance(p) == doctest::Approx(1.0f).epsilon(1e-3));
    }
}

TEST_CASE("star polygon has two corners per radius vertex") {
    const auto polygon = sh::star(5, 1.0f, 0.4f, sh::CornerRounding{0.1f}, sh::CornerRounding{0.05f});
    CHECK(corner_count(polygon) == 10);
    const sh::Bounds b = polygon.calculate_bounds(false);
    CHECK(b.right <= 1.0f + bounds_epsilon);
    CHECK(b.left >= -1.0f - bounds_epsilon);
}

TEST_CASE("pill and pill star shapes construct") {
    const auto pill = sh::pill(3.0f, 1.0f, 0.5f);
    const sh::Bounds b = pill.calculate_bounds(false);
    CHECK(b.right - b.left == doctest::Approx(3.0f).epsilon(1e-3));
    CHECK(b.bottom - b.top == doctest::Approx(1.0f).epsilon(1e-3));
    const auto pill_star = sh::pill_star({.rounding = sh::CornerRounding{0.1f}});
    CHECK(corner_count(pill_star) == 16);
    CHECK(all_finite(pill_star.cubics()));
}

TEST_CASE("all 35 material shapes construct and fit the unit square around its center") {
    CHECK(sh::material_shape_count == 35);
    for (const sh::MaterialShape shape : sh::all_material_shapes()) {
        CAPTURE(static_cast<int>(shape));
        const sh::RoundedPolygon& polygon = sh::material_shape(shape);
        REQUIRE(!polygon.cubics().empty());
        CHECK(all_finite(polygon.cubics()));
        CHECK(is_closed(polygon.cubics()));

        const sh::Bounds approx = polygon.calculate_bounds(true);
        CHECK(approx.left >= -bounds_epsilon);
        CHECK(approx.top >= -bounds_epsilon);
        CHECK(approx.right <= 1.0f + bounds_epsilon);
        CHECK(approx.bottom <= 1.0f + bounds_epsilon);
        CHECK((approx.left + approx.right) / 2.0f == doctest::Approx(0.5f).epsilon(1e-4));
        CHECK((approx.top + approx.bottom) / 2.0f == doctest::Approx(0.5f).epsilon(1e-4));
        CHECK(std::max(approx.right - approx.left, approx.bottom - approx.top) == doctest::Approx(1.0f).epsilon(1e-4));

        const sh::Bounds exact = polygon.calculate_bounds(false);
        CHECK(exact.left >= -bounds_epsilon);
        CHECK(exact.top >= -bounds_epsilon);
        CHECK(exact.right <= 1.0f + bounds_epsilon);
        CHECK(exact.bottom <= 1.0f + bounds_epsilon);
    }
}

TEST_CASE("material shape corner counts follow their definitions") {
    CHECK(corner_count(sh::material_shape(sh::MaterialShape::circle)) == 10);
    CHECK(corner_count(sh::material_shape(sh::MaterialShape::square)) == 4);
    CHECK(corner_count(sh::material_shape(sh::MaterialShape::triangle)) == 3);
    CHECK(corner_count(sh::material_shape(sh::MaterialShape::sunny)) == 16);
    CHECK(corner_count(sh::material_shape(sh::MaterialShape::cookie_9_sided)) == 18);
    CHECK(corner_count(sh::material_shape(sh::MaterialShape::burst)) == 24);
    CHECK(corner_count(sh::material_shape(sh::MaterialShape::pill)) == 10);
    CHECK(corner_count(sh::material_shape(sh::MaterialShape::heart)) == 7);
    CHECK(corner_count(sh::material_shape(sh::MaterialShape::pixel_triangle)) == 25);
}

TEST_CASE("morph at progress 0 and 1 reproduces the start and end outlines") {
    const sh::MaterialShape pairs[][2] = {
        {sh::MaterialShape::circle, sh::MaterialShape::square},
        {sh::MaterialShape::square, sh::MaterialShape::heart},
        {sh::MaterialShape::cookie_9_sided, sh::MaterialShape::pixel_triangle},
        {sh::MaterialShape::soft_boom, sh::MaterialShape::puffy},
        {sh::MaterialShape::arrow, sh::MaterialShape::bun},
    };
    for (const auto& pair : pairs) {
        CAPTURE(static_cast<int>(pair[0]));
        CAPTURE(static_cast<int>(pair[1]));
        const sh::RoundedPolygon& start = sh::material_shape(pair[0]);
        const sh::RoundedPolygon& end = sh::material_shape(pair[1]);
        const sh::Morph morph(start, end);
        CHECK(outline_difference(morph.as_cubics(0.0f), start.cubics()) < 1e-3f);
        CHECK(outline_difference(morph.as_cubics(1.0f), end.cubics()) < 1e-3f);
        CHECK(outline_difference(morph.as_cubics(0.5f), start.cubics()) > 1e-3f);
    }
}

TEST_CASE("morph between every adjacent pair of material shapes yields finite closed outlines") {
    const auto& all = sh::all_material_shapes();
    for (std::size_t i = 0; i + 1 < all.size(); ++i) {
        CAPTURE(i);
        const sh::Morph morph(sh::material_shape(all[i]), sh::material_shape(all[i + 1]));
        for (const float progress : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
            const auto cubics = morph.as_cubics(progress);
            REQUIRE(!cubics.empty());
            CHECK(all_finite(cubics));
            CHECK(is_closed(cubics));
        }
    }
}

TEST_CASE("morph between differently sized polygons is matched both ways") {
    const auto triangle = sh::RoundedPolygon::regular(3, 1.0f, {}, sh::CornerRounding{0.2f});
    const auto star = sh::star(7, 1.0f, 0.6f, sh::CornerRounding{0.1f});
    const sh::Morph forward(triangle, star);
    const sh::Morph backward(star, triangle);
    CHECK(outline_difference(forward.as_cubics(1.0f), star.cubics()) < 1e-3f);
    CHECK(outline_difference(backward.as_cubics(1.0f), triangle.cubics()) < 1e-3f);
}

TEST_CASE("smoothing changes the polygon outline") {
    const auto plain = sh::rectangle(2.0f, 2.0f, sh::CornerRounding{0.5f, 0.0f});
    const auto smooth = sh::rectangle(2.0f, 2.0f, sh::CornerRounding{0.5f, 1.0f});
    CHECK(outline_difference(plain.cubics(), smooth.cubics()) > 1e-2f);
    const sh::Bounds b = smooth.calculate_bounds(false);
    CHECK(b.right == doctest::Approx(1.0f).epsilon(1e-4));

    const sh::CornerRounding per_vertex[] = {{0.5f, 1.0f}, {0.5f, 0.0f}, {0.5f, 0.0f}, {0.5f, 0.0f}};
    const auto one_smooth = sh::rectangle(2.0f, 2.0f, sh::unrounded, per_vertex);
    CHECK(outline_difference(plain.cubics(), one_smooth.cubics()) > 1e-2f);
    CHECK(outline_difference(smooth.cubics(), one_smooth.cubics()) > 1e-2f);
}

TEST_CASE("polygon converts to a closed render path") {
    const auto& polygon = sh::material_shape(sh::MaterialShape::square);
    const auto path = sh::to_path(polygon);
    REQUIRE(path.verbs().size() == polygon.cubics().size() + 2);
    CHECK(path.verbs().front() == lamina::render::PathVerb::move);
    CHECK(path.verbs().back() == lamina::render::PathVerb::close);
    CHECK(path.points().size() == 1 + 3 * polygon.cubics().size());
    CHECK(path.points().front().x == doctest::Approx(polygon.cubics().front().anchor0_x()));

    const sh::Morph morph(polygon, sh::material_shape(sh::MaterialShape::circle));
    const auto morph_path = sh::to_path(morph, 0.5f);
    CHECK(morph_path.verbs().back() == lamina::render::PathVerb::close);
}
