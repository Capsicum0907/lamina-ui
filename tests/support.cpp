#include "support.hpp"

#include <algorithm>
#include <limits>
#include <string>

namespace lamina::test {

const text::FontStack& fonts() {
    static const text::FontStack stack({text::Font::from_file(std::string(LAMINA_TEST_FONTS_DIR) + "/lamina-test-sans.ttf")});
    return stack;
}

const icons::IconFont& icon_font() {
    static const icons::IconFont font = icons::IconFont::from_resource(res::fonts::symbols);
    return font;
}

const color::ColorScheme& scheme() {
    static const color::ColorScheme s = color::spec_2021().build(color::default_scheme_options);
    return s;
}

int count_color(render::Canvas& canvas, color::Argb c) {
    int n = 0;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            n += canvas.pixel(x, y) == c ? 1 : 0;
        }
    }
    return n;
}

void RecordingCanvas::resize(int width, int height) {
    width_ = width;
    height_ = height;
}

void RecordingCanvas::fill_device_path(const render::Path& path, color::Argb color, render::FillRule) {
    if (path.points().empty()) {
        return;
    }
    double x0 = std::numeric_limits<double>::infinity();
    double y0 = x0;
    double x1 = -x0;
    double y1 = -x0;
    for (const render::Point& p : path.points()) {
        x0 = std::min(x0, p.x);
        y0 = std::min(y0, p.y);
        x1 = std::max(x1, p.x);
        y1 = std::max(y1, p.y);
    }
    fills.push_back({{x0, y0, x1 - x0, y1 - y0}, color});
}

void RecordingCanvas::fill_device_rect(render::Point from, render::Point to, color::Argb color) {
    fills.push_back({{from.x, from.y, to.x - from.x, to.y - from.y}, color});
}

}
