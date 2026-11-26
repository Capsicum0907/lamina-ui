#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/style/style.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

namespace lamina::test {

inline constexpr int restyle_canvas_width = 420;
inline constexpr int restyle_canvas_height = 160;
inline constexpr double restyle_type_scale = 1.25;
inline constexpr double restyle_length_add = 8.0;
inline constexpr double restyle_corner_scale = 0.5;
inline constexpr double restyle_velocity_tolerance = 0.01;
inline constexpr motion::SpringSpec restyle_spring{1.0, 50.0};
inline constexpr color::Argb restyle_background = 0xffffffff;

const Style& restyle_test_style();

inline constexpr const char* shots_variable = "LAMINA_STYLE_SHOTS";
void save_shot(thorvg::Canvas& canvas, const std::string& name);

struct RestyleShot {
    render::Rect bounds;
    std::vector<color::Argb> pixels;
};

template <typename W>
void settle_widget(W& w) {
    for (int i = 0; i < settle_frames; ++i) {
        w.step(frame_seconds);
    }
}

template <typename W>
RestyleShot shoot(const W& w) {
    thorvg::Canvas canvas(restyle_canvas_width, restyle_canvas_height);
    canvas.clear(restyle_background);
    w.paint(canvas, scheme());
    RestyleShot s{w.bounds(), {}};
    for (int y = 0; y < restyle_canvas_height; ++y) {
        for (int x = 0; x < restyle_canvas_width; ++x) {
            s.pixels.push_back(canvas.pixel(x, y));
        }
    }
    return s;
}

struct RestyleResult {
    RestyleShot built_under_default;
    RestyleShot built_under_test;
    RestyleShot switched;
};

template <typename Make>
RestyleResult restyle_round(Make make) {
    RestyleResult r;
    {
        auto w = make();
        settle_widget(*w);
        r.built_under_default = shoot(*w);
        const StyleScope scope(restyle_test_style());
        settle_widget(*w);
        r.switched = shoot(*w);
    }
    {
        const StyleScope scope(restyle_test_style());
        auto w = make();
        settle_widget(*w);
        r.built_under_test = shoot(*w);
    }
    return r;
}

inline constexpr int restyle_motion_frames = 6;

template <typename Make, typename Act>
RestyleResult restyle_motion_round(Make make, Act act) {
    RestyleResult r;
    auto run = [&](auto& w) {
        w->step(frame_seconds);
        act(*w);
        for (int i = 0; i < restyle_motion_frames; ++i) {
            w->step(frame_seconds);
        }
        return shoot(*w);
    };
    {
        auto w = make();
        settle_widget(*w);
        r.built_under_default = run(w);
    }
    {
        auto w = make();
        settle_widget(*w);
        const StyleScope scope(restyle_test_style());
        r.switched = run(w);
    }
    {
        const StyleScope scope(restyle_test_style());
        auto w = make();
        settle_widget(*w);
        r.built_under_test = run(w);
    }
    return r;
}

}
