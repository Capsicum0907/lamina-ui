#include "lamina/shapes/morph.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>

#include "lamina/shapes/feature_mapping.hpp"
#include "lamina/shapes/polygon_measure.hpp"

namespace lamina::shapes {

namespace {

Bounds union_bounds(const Bounds& a, const Bounds& b) {
    return {std::min(a.left, b.left), std::min(a.top, b.top), std::max(a.right, b.right),
            std::max(a.bottom, b.bottom)};
}

std::optional<detail::MeasuredCubic> get_or_null(const detail::MeasuredPolygon& p, std::size_t index) {
    if (index < p.size()) {
        return p[index];
    }
    return std::nullopt;
}

}

Morph::Morph(RoundedPolygon start, RoundedPolygon end)
    : start_(std::move(start)), end_(std::move(end)), morph_match_(match(start_, end_)) {}

Bounds Morph::calculate_bounds(bool approximate) const {
    return union_bounds(start_.calculate_bounds(approximate), end_.calculate_bounds(approximate));
}

Bounds Morph::calculate_max_bounds() const {
    return union_bounds(start_.calculate_max_bounds(), end_.calculate_max_bounds());
}

std::vector<Cubic> Morph::as_cubics(float progress) const {
    std::vector<Cubic> result;
    std::optional<Cubic> first_cubic;
    std::optional<Cubic> last_cubic;
    for (const auto& [first, second] : morph_match_) {
        const Cubic cubic = Cubic::interpolate(first, second, progress);
        if (!first_cubic) {
            first_cubic = cubic;
        }
        if (last_cubic) {
            result.push_back(*last_cubic);
        }
        last_cubic = cubic;
    }
    if (last_cubic && first_cubic) {
        result.emplace_back(last_cubic->anchor0_x(), last_cubic->anchor0_y(), last_cubic->control0_x(),
                            last_cubic->control0_y(), last_cubic->control1_x(), last_cubic->control1_y(),
                            first_cubic->anchor0_x(), first_cubic->anchor0_y());
    }
    return result;
}

std::vector<std::pair<Cubic, Cubic>> Morph::match(const RoundedPolygon& p1, const RoundedPolygon& p2) {
    const detail::MeasuredPolygon measured_polygon1 = detail::MeasuredPolygon::measure_polygon(p1);
    const detail::MeasuredPolygon measured_polygon2 = detail::MeasuredPolygon::measure_polygon(p2);

    const detail::DoubleMapper double_mapper =
        detail::feature_mapper(measured_polygon1.features(), measured_polygon2.features());

    const float polygon2_cut_point = double_mapper.map(0.0f);

    const detail::MeasuredPolygon& bs1 = measured_polygon1;
    const detail::MeasuredPolygon bs2 = measured_polygon2.cut_and_shift(polygon2_cut_point);

    std::vector<std::pair<Cubic, Cubic>> ret;
    std::size_t i1 = 0;
    std::size_t i2 = 0;
    std::optional<detail::MeasuredCubic> b1 = get_or_null(bs1, i1++);
    std::optional<detail::MeasuredCubic> b2 = get_or_null(bs2, i2++);
    while (b1 && b2) {
        const float b1a = i1 == bs1.size() ? 1.0f : b1->end_outline_progress();
        const float b2a = i2 == bs2.size()
                              ? 1.0f
                              : double_mapper.map_back(
                                    positive_modulo(b2->end_outline_progress() + polygon2_cut_point, 1.0f));
        const float minb = std::min(b1a, b2a);

        std::optional<detail::MeasuredCubic> seg1;
        std::optional<detail::MeasuredCubic> new_b1;
        if (b1a > minb + angle_epsilon) {
            auto cut = b1->cut_at_progress(minb);
            seg1 = cut.first;
            new_b1 = cut.second;
        } else {
            seg1 = *b1;
            new_b1 = get_or_null(bs1, i1++);
        }

        std::optional<detail::MeasuredCubic> seg2;
        std::optional<detail::MeasuredCubic> new_b2;
        if (b2a > minb + angle_epsilon) {
            auto cut = b2->cut_at_progress(positive_modulo(double_mapper.map(minb) - polygon2_cut_point, 1.0f));
            seg2 = cut.first;
            new_b2 = cut.second;
        } else {
            seg2 = *b2;
            new_b2 = get_or_null(bs2, i2++);
        }

        ret.emplace_back(seg1->cubic(), seg2->cubic());
        b1 = std::move(new_b1);
        b2 = std::move(new_b2);
    }
    if (b1 || b2) {
        throw std::logic_error("Expected both Polygon's Cubic to be fully matched");
    }
    return ret;
}

}
