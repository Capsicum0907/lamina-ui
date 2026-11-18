#include <doctest/doctest.h>

#include <string>

#include "lamina/render/thorvg_canvas.hpp"
#include "lamina/text/layout.hpp"

using namespace lamina;
using namespace lamina::text;

namespace {

std::string fixture(const char* name) {
    return std::string(LAMINA_TEST_FONTS_DIR) + "/" + name;
}

const auto sans = Font::from_file(fixture("lamina-test-sans.ttf"));
const auto condensed = Font::from_file(fixture("lamina-test-condensed.ttf"));

constexpr TextStyle body{16.0, 24.0, 400, 0.0};

}

TEST_CASE("decode_utf8 handles multibyte text and bad bytes") {
    CHECK(decode_utf8("A") == U"A");
    CHECK(decode_utf8("\xe6\x97\xa5\xe6\x9c\xac") == U"\u65e5\u672c");
    CHECK(decode_utf8("\xf0\x9f\x98\x80") == U"\U0001F600");
    CHECK(decode_utf8("a\xffz") == U"a\ufffdz");
    CHECK(decode_utf8("\xe6\x97") == U"\ufffd");
}

TEST_CASE("a line of Latin text has one glyph per letter and advances left to right") {
    const FontStack fonts({sans});
    const TextLine line = layout_line(fonts, "Hello", body);
    REQUIRE(line.glyphs.size() == 5);
    for (std::size_t i = 1; i < line.glyphs.size(); ++i) {
        CHECK(line.glyphs[i].x > line.glyphs[i - 1].x);
    }
    CHECK(line.width > 0.0);
    CHECK(line.ascent > 0.0);
    CHECK(line.descent > 0.0);
}

TEST_CASE("tracking adds space between glyphs only") {
    const FontStack fonts({sans});
    TextStyle tracked = body;
    tracked.tracking = 2.0;
    const double plain = layout_line(fonts, "Hello", body).width;
    const double spaced = layout_line(fonts, "Hello", tracked).width;
    CHECK(spaced == doctest::Approx(plain + 4 * 2.0));
}

TEST_CASE("heavier weights are wider in a variable font") {
    const FontStack fonts({sans});
    TextStyle bold = body;
    bold.weight = 700;
    CHECK(layout_line(fonts, "Hello", bold).width > layout_line(fonts, "Hello", body).width);
}

TEST_CASE("characters fall back to the first font that has them") {
    const FontStack fonts({condensed, sans});
    const TextLine line = layout_line(fonts, "A\xe3\x81\x82" "B", body);
    REQUIRE(line.glyphs.size() == 3);
    CHECK(line.glyphs[0].font == 0);
    CHECK(line.glyphs[1].font == 1);
    CHECK(line.glyphs[2].font == 0);
    CHECK(line.glyphs[1].cluster == 1);
}

TEST_CASE("the baseline centers the text in the line height") {
    const FontStack fonts({sans});
    const TextLine line = layout_line(fonts, "Hello", body);
    CHECK(line.baseline - line.ascent == doctest::Approx(24.0 - line.baseline - line.descent));
}

TEST_CASE("drawn text covers pixels inside the line box only") {
    const FontStack fonts({sans});
    const TextLine line = layout_line(fonts, "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", {32.0, 48.0, 400, 0.0});
    render::ThorVGCanvas canvas(140, 60);
    canvas.clear(0xffffffff);
    draw_line(canvas, fonts, line, {4.0, 4.0}, 0xff000000);
    int inked = 0;
    for (int y = 0; y < 60; ++y) {
        for (int x = 0; x < 140; ++x) {
            if (canvas.pixel(x, y) != 0xffffffff) {
                ++inked;
                CHECK(x >= 4);
                CHECK(x <= 4 + static_cast<int>(line.width) + 1);
                CHECK(y >= 4);
                CHECK(y <= 4 + 48);
            }
        }
    }
    CHECK(inked > 200);
}
