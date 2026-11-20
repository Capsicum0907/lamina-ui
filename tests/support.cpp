#include "support.hpp"

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

}
