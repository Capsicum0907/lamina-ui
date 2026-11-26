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
inline constexpr double restyle_duration_scale = 0.5;
inline constexpr double restyle_velocity_tolerance = 0.01;
inline constexpr motion::SpringSpec restyle_spring{1.0, 50.0};
inline constexpr motion::CubicBezier restyle_easing = tokens::easing::standard;
inline constexpr color::Argb restyle_background = 0xffffffff;

const Style& restyle_test_style();

inline constexpr const char* shots_variable = "LAMINA_STYLE_SHOTS";
void save_shot(thorvg::Canvas& canvas, const std::string& name);

struct RestyleCanvas {
    int width = restyle_canvas_width;
    int height = restyle_canvas_height;
};

struct RestyleShot {
    render::Rect bounds;
    std::vector<color::Argb> pixels;
    RestyleCanvas canvas = {};
};

template <typename W>
void settle_widget(W& w) {
    for (int i = 0; i < settle_frames; ++i) {
        w.step(frame_seconds);
    }
}

template <typename W>
RestyleShot shoot(const W& w, RestyleCanvas size = {}) {
    thorvg::Canvas canvas(size.width, size.height);
    canvas.clear(restyle_background);
    w.paint(canvas, scheme());
    RestyleShot s{w.bounds(), {}, size};
    for (int y = 0; y < size.height; ++y) {
        for (int x = 0; x < size.width; ++x) {
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
RestyleResult restyle_round(Make make, RestyleCanvas size = {}) {
    RestyleResult r;
    {
        auto w = make();
        settle_widget(*w);
        r.built_under_default = shoot(*w, size);
        const StyleScope scope(restyle_test_style());
        settle_widget(*w);
        r.switched = shoot(*w, size);
    }
    {
        const StyleScope scope(restyle_test_style());
        auto w = make();
        settle_widget(*w);
        r.built_under_test = shoot(*w, size);
    }
    return r;
}

inline constexpr int restyle_motion_frames = 6;

template <typename Make, typename Act>
RestyleResult restyle_motion_round(Make make, Act act, RestyleCanvas size = {}) {
    RestyleResult r;
    auto run = [&](auto& w) {
        w->step(frame_seconds);
        act(*w);
        for (int i = 0; i < restyle_motion_frames; ++i) {
            w->step(frame_seconds);
        }
        return shoot(*w, size);
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

template <typename Make, typename Start>
RestyleResult restyle_tween_round(Make make, Start start, RestyleCanvas size = {}, int frames = restyle_motion_frames) {
    RestyleResult r;
    auto begin = [&](auto& w) {
        settle_widget(*w);
        start(*w);
        for (int i = 0; i < frames; ++i) {
            w->step(frame_seconds);
        }
    };
    {
        auto w = make();
        begin(w);
        settle_widget(*w);
        r.built_under_default = shoot(*w, size);
    }
    {
        auto w = make();
        begin(w);
        const StyleScope scope(restyle_test_style());
        settle_widget(*w);
        r.switched = shoot(*w, size);
    }
    {
        const StyleScope scope(restyle_test_style());
        auto w = make();
        begin(w);
        settle_widget(*w);
        r.built_under_test = shoot(*w, size);
    }
    return r;
}

}
