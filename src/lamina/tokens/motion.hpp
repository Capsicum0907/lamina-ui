#pragma once

#include <array>
#include <cstddef>

#include "lamina/motion/spring.hpp"

namespace lamina::tokens {

enum class SpringSpeed : std::size_t { fast, standard, slow };

enum class SpringKind { spatial, effects };

struct MotionScheme {
    std::array<motion::SpringSpec, 3> spatial;
    std::array<motion::SpringSpec, 3> effects;

    constexpr motion::SpringSpec spring(SpringKind kind, SpringSpeed speed) const {
        const auto& table = kind == SpringKind::spatial ? spatial : effects;
        return table[static_cast<std::size_t>(speed)];
    }

    constexpr motion::SpringSpec fast_spatial() const { return spring(SpringKind::spatial, SpringSpeed::fast); }
    constexpr motion::SpringSpec default_spatial() const { return spring(SpringKind::spatial, SpringSpeed::standard); }
    constexpr motion::SpringSpec slow_spatial() const { return spring(SpringKind::spatial, SpringSpeed::slow); }
    constexpr motion::SpringSpec fast_effects() const { return spring(SpringKind::effects, SpringSpeed::fast); }
    constexpr motion::SpringSpec default_effects() const { return spring(SpringKind::effects, SpringSpeed::standard); }
    constexpr motion::SpringSpec slow_effects() const { return spring(SpringKind::effects, SpringSpeed::slow); }
};

inline constexpr MotionScheme standard_motion{
    .spatial = {{{0.9, 1400.0}, {0.9, 700.0}, {0.9, 300.0}}},
    .effects = {{{1.0, 3800.0}, {1.0, 1600.0}, {1.0, 800.0}}},
};

inline constexpr MotionScheme expressive_motion{
    .spatial = {{{0.6, 800.0}, {0.8, 380.0}, {0.8, 200.0}}},
    .effects = {{{1.0, 3800.0}, {1.0, 1600.0}, {1.0, 800.0}}},
};

inline constexpr const MotionScheme& motion_scheme = expressive_motion;

struct CubicBezier {
    double x1;
    double y1;
    double x2;
    double y2;
};

namespace easing {
inline constexpr CubicBezier emphasized{0.2, 0.0, 0.0, 1.0};
inline constexpr CubicBezier emphasized_accelerate{0.3, 0.0, 0.8, 0.15};
inline constexpr CubicBezier emphasized_decelerate{0.05, 0.7, 0.1, 1.0};
inline constexpr CubicBezier standard{0.2, 0.0, 0.0, 1.0};
inline constexpr CubicBezier standard_accelerate{0.3, 0.0, 1.0, 1.0};
inline constexpr CubicBezier standard_decelerate{0.0, 0.0, 0.0, 1.0};
inline constexpr CubicBezier legacy{0.4, 0.0, 0.2, 1.0};
inline constexpr CubicBezier legacy_accelerate{0.4, 0.0, 1.0, 1.0};
inline constexpr CubicBezier legacy_decelerate{0.0, 0.0, 0.2, 1.0};
inline constexpr CubicBezier linear{0.0, 0.0, 1.0, 1.0};
}

namespace duration_ms {
inline constexpr std::array<int, 4> short_{50, 100, 150, 200};
inline constexpr std::array<int, 4> medium{250, 300, 350, 400};
inline constexpr std::array<int, 4> long_{450, 500, 550, 600};
inline constexpr std::array<int, 4> extra_long{700, 800, 900, 1000};
}

}
