#include <doctest/doctest.h>

#include <cmath>
#include <string>

#include "lamina/text/layout.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina_thorvg/canvas.hpp"

using namespace lamina;
using namespace lamina::text;

namespace {

std::string fixture(const char* name) {
    return std::string(LAMINA_TEST_FONTS_DIR) + "/" + name;
}

const auto sans = Font::from_file(fixture("lamina-test-sans.ttf"));
const auto condensed = Font::from_file(fixture("lamina-test-condensed.ttf"));

constexpr TextStyle body{16.0, 24.0, 400, 0.0, tokens::tab_width_spaces};

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

TEST_CASE("the line box is the line height unless the font extents are taller") {
    const TextLine roomy{{}, body, 0.0, 14.0, 4.0, 0.0, 0, 0};
    CHECK(roomy.box_height() == 24.0);
    const TextLine tall{{}, body, 0.0, 20.0, 8.0, 0.0, 0, 0};
    CHECK(tall.box_height() == 28.0);
}

TEST_CASE("drawn text covers pixels inside the line box only") {
    const FontStack fonts({sans});
    const TextLine line = layout_line(fonts, "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", {32.0, 48.0, 400, 0.0});
    thorvg::Canvas canvas(140, 60);
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

namespace {

std::u32string line_text(std::string_view utf8, const TextLine& line) {
    const std::u32string all = decode_utf8(utf8);
    return all.substr(line.first, line.last - line.first);
}

}

TEST_CASE("Latin paragraphs break at spaces and drop the trailing space") {
    const FontStack fonts({sans});
    const char* text = "Hello world Hello world";
    const double one_word = layout_line(fonts, "Hello world", body).width;
    const TextBlock block = layout_paragraph(fonts, text, body, one_word + 1.0);
    REQUIRE(block.lines.size() == 2);
    CHECK(line_text(text, block.lines[0]) == U"Hello world");
    CHECK(line_text(text, block.lines[1]) == U"Hello world");
    CHECK(block.height == doctest::Approx(2 * body.line_height));
}

TEST_CASE("Japanese paragraphs break between characters but keep punctuation off the line start") {
    const FontStack fonts({sans});
    const char* text = "\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86\xe3\x80\x82\xe3\x81\x88\xe3\x81\x8a";
    const double three = layout_line(fonts, "\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86", body).width;
    const TextBlock block = layout_paragraph(fonts, text, body, three + 1.0);
    REQUIRE(block.lines.size() == 3);
    CHECK(line_text(text, block.lines[0]) == U"\u3042\u3044");
    CHECK(line_text(text, block.lines[1]) == U"\u3046\u3002\u3048");
    CHECK(line_text(text, block.lines[2]) == U"\u304a");
}

TEST_CASE("explicit newlines start new lines") {
    const FontStack fonts({sans});
    const TextBlock block = layout_paragraph(fonts, "Hello\nworld", body, 1000.0);
    REQUIRE(block.lines.size() == 2);
}

TEST_CASE("a word longer than the line is split rather than overflowing forever") {
    const FontStack fonts({sans});
    const TextBlock block = layout_paragraph(fonts, "Helloworldhelloworld", body, 40.0);
    CHECK(block.lines.size() > 1);
    for (const auto& line : block.lines) {
        CHECK(line.last > line.first);
    }
}

namespace {

double tab_interval(const FontStack& fonts, const TextStyle& style) {
    return layout_line(fonts, " ", style).width * style.tab_width;
}

}

TEST_CASE("a tab advances to the next tab stop, measured in spaces from the line start") {
    const FontStack fonts({sans});
    const double stop = tab_interval(fonts, body);
    REQUIRE(stop > 0.0);
    const TextLine leading = layout_line(fonts, "\tx", body);
    REQUIRE(leading.glyphs.size() == 2);
    CHECK(leading.glyphs[0].x == doctest::Approx(0.0));
    CHECK(leading.glyphs[1].x == doctest::Approx(stop));

    const double word = layout_line(fonts, "Hello", body).width;
    const double expected = (std::floor(word / stop) + 1.0) * stop;
    const TextLine after_word = layout_line(fonts, "Hello\tx", body);
    REQUIRE(after_word.glyphs.size() == 7);
    CHECK(after_word.glyphs[6].x == doctest::Approx(expected));
    CHECK(layout_line(fonts, "Hello\t", body).width == doctest::Approx(expected));
}

TEST_CASE("a tab keeps tracking out of its stop and the line's end") {
    const FontStack fonts({sans});
    TextStyle tracked = body;
    tracked.tracking = 2.0;
    const double stop = tab_interval(fonts, tracked) + tracked.tracking * tracked.tab_width;
    const TextLine line = layout_line(fonts, "x\t", tracked);
    CHECK(line.width == doctest::Approx(stop));
}

TEST_CASE("the caret and hit-testing treat a tab as one character spanning to its stop") {
    const FontStack fonts({sans});
    const double stop = tab_interval(fonts, body);
    const TextLine line = layout_line(fonts, "a\tb", body);
    const double a = layout_line(fonts, "a", body).width;
    CHECK(caret_x(line, 1) == doctest::Approx(a));
    CHECK(caret_x(line, 2) == doctest::Approx(stop));
    CHECK(index_at(line, a + (stop - a) * 0.25) == 1);
    CHECK(index_at(line, a + (stop - a) * 0.75) == 2);
}

TEST_CASE("a tab draws nothing") {
    const FontStack fonts({sans});
    CHECK(line_path(fonts, layout_line(fonts, "\t", body), {0.0, 0.0}).empty());
}

TEST_CASE("a tab width of zero gives a tab no width") {
    const FontStack fonts({sans});
    TextStyle none = body;
    none.tab_width = 0.0;
    CHECK(layout_line(fonts, "\t", none).width == doctest::Approx(0.0));
    CHECK(layout_line(fonts, "a\tb", none).width == doctest::Approx(layout_line(fonts, "ab", none).width));
}

TEST_CASE("paragraph tabs are measured from their own line and count toward wrapping") {
    const FontStack fonts({sans});
    const double stop = tab_interval(fonts, body);
    const TextBlock after_newline = layout_paragraph(fonts, "Hello world\n\tx", body, 1000.0);
    REQUIRE(after_newline.lines.size() == 2);
    REQUIRE(after_newline.lines[1].glyphs.size() == 2);
    CHECK(after_newline.lines[1].glyphs[1].x == doctest::Approx(stop));

    const char* text = "a\tbbbb";
    const double bbbb = layout_line(fonts, "bbbb", body).width;
    const TextBlock wrapped = layout_paragraph(fonts, text, body, stop + bbbb - 1.0);
    REQUIRE(wrapped.lines.size() == 2);
    CHECK(line_text(text, wrapped.lines[0]) == U"a");
    CHECK(line_text(text, wrapped.lines[1]) == U"bbbb");
    CHECK(layout_paragraph(fonts, text, body, stop + bbbb + 1.0).lines.size() == 1);
}
