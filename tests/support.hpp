#pragma once

#include <vector>

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

struct RecordedFill {
    render::Rect bounds;
    color::Argb color;
};

class RecordingCanvas : public render::Canvas {
public:
    RecordingCanvas(int width, int height) : width_(width), height_(height) {}

    int width() const override { return width_; }
    int height() const override { return height_; }
    void resize(int width, int height) override;
    void clear(color::Argb) override { fills.clear(); }
    render::PixelView finish() override { return {nullptr, width_, height_, 0}; }

    std::vector<RecordedFill> fills;

protected:
    void fill_device_path(const render::Path& path, color::Argb color, render::FillRule rule) override;
    void fill_device_shadow(const render::Path&, const render::Shadow&) override {}
    void fill_device_rect(render::Point from, render::Point to, color::Argb color) override;
    void draw_device_image(const render::Image&, const render::Affine&, double) override {}
    void begin_device_clip(const render::Rect&) override {}
    void begin_device_clip(const render::Path&) override {}
    void end_device_clip() override {}

private:
    int width_;
    int height_;
};

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
