#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "test_res.hpp"

namespace lamina::test {

inline constexpr int settle_frames = 240;
inline constexpr double frame_seconds = 1.0 / 60.0;

const text::FontStack& fonts();
const icons::IconFont& icon_font();
const color::ColorScheme& scheme();
int count_color(render::Canvas& canvas, color::Argb c);

template <typename W>
void settle(W& w) {
    for (int i = 0; i < settle_frames; ++i) {
        w.step(frame_seconds);
    }
}

template <typename W>
bool click(W& w, double x, double y) {
    w.handle({input::PointerAction::down, x, y});
    const bool clicked = w.handle({input::PointerAction::up, x, y});
    w.handle({input::PointerAction::leave, 0.0, 0.0});
    return clicked;
}

}
