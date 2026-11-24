#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

#include "lamina/style/style.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/scrim.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;

namespace {

constexpr int canvas_width = 320;
constexpr int canvas_height = 80;
constexpr render::Point filled_origin{10.0, 10.0};
constexpr render::Point outlined_origin{170.0, 10.0};
constexpr render::Rect scrim_area{160.0, 0.0, 160.0, 80.0};
constexpr double modified_scrim_opacity = 0.6;
constexpr color::Argb background = 0xffffffff;

struct Frame {
    std::vector<color::Argb> left;
    std::vector<color::Argb> right;
};

bool painted(const std::vector<color::Argb>& pixels) {
    return std::any_of(pixels.begin(), pixels.end(), [](color::Argb p) { return p != background; });
}

Frame capture(thorvg::Canvas& canvas) {
    Frame f;
    for (int y = 0; y < canvas_height; ++y) {
        for (int x = 0; x < canvas_width; ++x) {
            (x < canvas_width / 2 ? f.left : f.right).push_back(canvas.pixel(x, y));
        }
    }
    return f;
}

Frame paint_buttons(const Style& style) {
    const StyleScope scope(style);
    widgets::Button filled({"Send", ButtonStyle::filled, ButtonSize::small, ButtonShape::round}, fonts());
    widgets::Button outlined({"Send", ButtonStyle::outlined, ButtonSize::small, ButtonShape::round}, fonts());
    filled.set_origin(filled_origin);
    outlined.set_origin(outlined_origin);
    thorvg::Canvas canvas(canvas_width, canvas_height);
    canvas.clear(background);
    filled.paint(canvas, scheme());
    outlined.paint(canvas, scheme());
    return capture(canvas);
}

Frame paint_scrimmed(const Style& style) {
    const StyleScope scope(style);
    widgets::Button filled({"Send", ButtonStyle::filled, ButtonSize::small, ButtonShape::round}, fonts());
    filled.set_origin(filled_origin);
    thorvg::Canvas canvas(canvas_width, canvas_height);
    canvas.clear(background);
    filled.paint(canvas, scheme());
    widgets::paint_scrim(canvas, scrim_area, scheme(), 1.0);
    return capture(canvas);
}

}

TEST_CASE("the default style is the M3 Expressive style, built from the token tables") {
    const Style& s = m3_expressive_style();
    CHECK(&current_style() == &s);
    CHECK(s.color == &color::spec_2025());
    CHECK(s.scrim.opacity == tokens::scrim_tokens.opacity);
    CHECK(s.button.size(ButtonSize::medium).height == tokens::button_size(ButtonSize::medium).height);
    CHECK(s.button.style(ButtonStyle::outlined).outline == tokens::button_style(ButtonStyle::outlined).outline);
    CHECK(s.motion.fast_spatial().stiffness == tokens::motion_scheme.fast_spatial().stiffness);
    CHECK(s.shape.radius(tokens::CornerSize::large, 100.0, 40.0) == tokens::corner_radius(tokens::CornerSize::large, 100.0, 40.0));
    CHECK(s.type.style(tokens::TypeRole::title_medium).size_sp == tokens::type_style(tokens::TypeRole::title_medium).size_sp);
    CHECK(s.elevation.levels == tokens::elevation_dp);
    CHECK(s.minimum_target({}) == tokens::minimum_target({}));
}

TEST_CASE("a style scope installs a style and restores the one before it") {
    Style outer = m3_expressive_style();
    Style inner = m3_expressive_style();
    {
        const StyleScope a(outer);
        CHECK(&current_style() == &outer);
        {
            const StyleScope b(inner);
            CHECK(&current_style() == &inner);
        }
        CHECK(&current_style() == &outer);
    }
    CHECK(&current_style() == &m3_expressive_style());
}

TEST_CASE("a copy of the default style paints exactly as the default") {
    const Style copy = m3_expressive_style();
    const Frame a = paint_buttons(m3_expressive_style());
    const Frame b = paint_buttons(copy);
    CHECK(a.left == b.left);
    CHECK(a.right == b.right);
}

TEST_CASE("a style with another scrim opacity changes the scrim and leaves the button beside it alone") {
    Style modified = m3_expressive_style();
    modified.scrim.opacity = modified_scrim_opacity;
    const Frame a = paint_scrimmed(m3_expressive_style());
    const Frame b = paint_scrimmed(modified);
    CHECK(painted(a.left));
    CHECK(a.left == b.left);
    CHECK(a.right != b.right);
}

TEST_CASE("a style with another outline changes outlined buttons and leaves filled buttons alone") {
    Style modified = m3_expressive_style();
    modified.button.styles[static_cast<std::size_t>(ButtonStyle::outlined)].outline = color::Role::primary;
    const Frame a = paint_buttons(m3_expressive_style());
    const Frame b = paint_buttons(modified);
    CHECK(painted(a.left));
    CHECK(painted(a.right));
    CHECK(a.left == b.left);
    CHECK(a.right != b.right);
}
