#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/widgets/focus_indicator.hpp"

namespace lamina::examples {

inline constexpr std::array<color::Argb, 5> theme_seeds{0xff6750a4, 0xff006a6a, 0xffb3261e, 0xff7d5260, 0xff386a20};

class ThemeFade {
public:
    explicit ThemeFade(const color::ColorScheme& initial);

    color::ColorScheme current() const;
    void fade_to(const color::ColorScheme& target);
    void step(double dt_seconds);
    bool settled() const;

private:
    color::ColorScheme from_;
    color::ColorScheme to_;
    motion::Animated progress_;
};

struct ChipStyle {
    double size = 32.0;
    double gap = 12.0;
    color::Role ring = color::Role::on_surface;
    double ring_width = 2.0;
    color::Role ring_gap = color::Role::surface;
};

class SeedChips {
public:
    explicit SeedChips(ChipStyle style = {});
    SeedChips(const SeedChips&) = delete;
    SeedChips& operator=(const SeedChips&) = delete;

    const ChipStyle& style() const { return style_; }
    std::vector<layout::NodePtr> nodes();
    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused) { focus_.set_focused(focused); }
    void select(color::Argb seed);
    std::optional<color::Argb> handle(const input::PointerEvent& e);
    std::optional<color::Argb> handle(const input::KeyEvent& e);
    void step(double dt_seconds) { focus_.step(dt_seconds); }
    bool animating() const { return !focus_.settled(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    ChipStyle style_;
    std::array<render::Rect, theme_seeds.size()> rects_{};
    std::size_t selected_ = 0;
    widgets::FocusIndicator focus_;
};

}
