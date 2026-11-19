#include <doctest/doctest.h>

#include <algorithm>

#include "lamina/icons/icon.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using icons::Symbol;

namespace {

struct Ink {
    int count = 0;
    int min_x = 1 << 30;
    int min_y = 1 << 30;
    int max_x = -1;
    int max_y = -1;
};

Ink ink(render::Canvas& canvas, color::Argb background) {
    Ink out;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            if (canvas.pixel(x, y) != background) {
                ++out.count;
                out.min_x = std::min(out.min_x, x);
                out.min_y = std::min(out.min_y, y);
                out.max_x = std::max(out.max_x, x);
                out.max_y = std::max(out.max_y, y);
            }
        }
    }
    return out;
}

bool same_pixels(render::Canvas& a, render::Canvas& b) {
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            if (a.pixel(x, y) != b.pixel(x, y)) {
                return false;
            }
        }
    }
    return true;
}

}

TEST_CASE("every symbol of the table is in the subset font") {
    for (Symbol s : icons::all_symbols) {
        CHECK(icon_font().has(s));
        CHECK_FALSE(icon_font().path(s, {0.0, 0.0}, 24.0).empty());
    }
}

TEST_CASE("an icon is drawn inside its size by size box") {
    thorvg::Canvas canvas(64, 64);
    canvas.clear(0xffffffff);
    icon_font().draw(canvas, Symbol::settings, {16.0, 16.0}, 32.0, 0xff000000);
    const Ink i = ink(canvas, 0xffffffff);
    CHECK(i.count > 100);
    CHECK(i.min_x >= 16);
    CHECK(i.min_y >= 16);
    CHECK(i.max_x < 48);
    CHECK(i.max_y < 48);
    CHECK(i.max_y > 40);
    CHECK(i.min_y < 24);
}

TEST_CASE("the FILL axis changes the outline") {
    thorvg::Canvas canvas(48, 48);
    canvas.clear(0xffffffff);
    icon_font().draw(canvas, Symbol::favorite, {0.0, 0.0}, 48.0, 0xff000000);
    const int outlined = ink(canvas, 0xffffffff).count;
    canvas.clear(0xffffffff);
    icons::IconStyle filled;
    filled.fill = 1.0;
    icon_font().draw(canvas, Symbol::favorite, {0.0, 0.0}, 48.0, 0xff000000, filled);
    const int solid = ink(canvas, 0xffffffff).count;
    CHECK(solid > outlined + 100);
}

TEST_CASE("the weight axis thickens the strokes") {
    thorvg::Canvas canvas(48, 48);
    icons::IconStyle thin;
    thin.weight = 100.0;
    icons::IconStyle bold;
    bold.weight = 700.0;
    canvas.clear(0xffffffff);
    icon_font().draw(canvas, Symbol::add, {0.0, 0.0}, 48.0, 0xff000000, thin);
    const int light = ink(canvas, 0xffffffff).count;
    canvas.clear(0xffffffff);
    icon_font().draw(canvas, Symbol::add, {0.0, 0.0}, 48.0, 0xff000000, bold);
    CHECK(ink(canvas, 0xffffffff).count > light);
}

TEST_CASE("a button with a leading icon is wider by the icon and its spacing") {
    const auto& size = tokens::button_size(tokens::ButtonSize::medium);
    widgets::Button plain({"Send", tokens::ButtonStyle::filled, tokens::ButtonSize::medium, tokens::ButtonShape::round}, fonts());
    widgets::Button with_icon({"Send", tokens::ButtonStyle::filled, tokens::ButtonSize::medium, tokens::ButtonShape::round,
                               icons::Icon{icon_font(), Symbol::send}},
                              fonts());
    CHECK(with_icon.width() == doctest::Approx(plain.width() + size.icon_size + size.icon_label_space));
}

TEST_CASE("button icon size and spacing follow ButtonDefaults") {
    CHECK(tokens::button_size(tokens::ButtonSize::extra_small).icon_label_space == 4.0);
    CHECK(tokens::button_size(tokens::ButtonSize::small).icon_size == 20.0);
    CHECK(tokens::button_size(tokens::ButtonSize::extra_large).icon_size == 40.0);
}

TEST_CASE("mixing icon styles interpolates the axes and switches the optical size at the halfway point") {
    const icons::IconStyle a{0.0, 400.0, 0.0, 24.0};
    const icons::IconStyle b{1.0, 700.0, 200.0, 48.0};
    const icons::IconStyle quarter = icons::mix(a, b, 0.25);
    CHECK(quarter.fill == 0.25);
    CHECK(quarter.weight == 475.0);
    CHECK(quarter.grade == 50.0);
    CHECK(quarter.optical_size == 24.0);
    CHECK(icons::mix(a, b, 0.5).optical_size == 48.0);
}

TEST_CASE("draw_centered puts the icon box in the middle of a rect or around a point") {
    const icons::Icon icon{icon_font(), Symbol::settings};
    thorvg::Canvas expected(64, 64);
    expected.clear(0xffffffff);
    icon_font().draw(expected, Symbol::settings, {16.0, 12.0}, 32.0, 0xff000000);

    thorvg::Canvas in_rect(64, 64);
    in_rect.clear(0xffffffff);
    icons::draw_centered(in_rect, icon, render::Rect{8.0, 4.0, 48.0, 48.0}, 32.0, 0xff000000);
    CHECK(same_pixels(expected, in_rect));

    thorvg::Canvas at_point(64, 64);
    at_point.clear(0xffffffff);
    icons::draw_centered(at_point, icon, render::Point{32.0, 28.0}, 32.0, 0xff000000);
    CHECK(same_pixels(expected, at_point));
}
