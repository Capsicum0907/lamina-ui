#include <doctest/doctest.h>

#include <cmath>
#include <string>

#include "lamina/text/edit_buffer.hpp"
#include "lamina/text/line_map.hpp"

using namespace lamina::text;

namespace {

class MonospaceGeometry final : public CaretGeometry {
public:
    explicit MonospaceGeometry(const LineMap& lines) : lines_(lines) {}

    double x_at(std::size_t line, std::size_t index) const override {
        return static_cast<double>(index - lines_.first(line));
    }

    std::size_t index_at(std::size_t line, double x) const override {
        const std::size_t first = lines_.first(line);
        const auto column = static_cast<std::size_t>(std::lround(std::max(0.0, x)));
        return std::min(first + column, lines_.limit(line));
    }

private:
    const LineMap& lines_;
};

LineMap hard_lines(std::u32string_view text) {
    std::vector<LineSpan> spans;
    std::size_t first = 0;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == U'\n') {
            spans.push_back({first, i});
            first = i + 1;
        }
    }
    return LineMap(text, std::move(spans));
}

}

TEST_CASE("insert replaces the selection and moves the cursor after it") {
    EditBuffer b;
    b.insert(U"Hello");
    CHECK(b.text() == U"Hello");
    CHECK(b.cursor() == 5);
    b.select(1, 4);
    CHECK(b.selected() == U"ell");
    b.insert(U"ipp");
    CHECK(b.text() == U"Hippo");
    CHECK(b.cursor() == 4);
    CHECK_FALSE(b.has_selection());
}

TEST_CASE("left and right collapse a selection before they move") {
    EditBuffer b;
    b.set_text(U"Hello");
    b.move_left(true, false);
    b.move_left(true, false);
    CHECK(b.selected() == U"lo");
    b.move_left(false, false);
    CHECK(b.cursor() == 3);
    CHECK_FALSE(b.has_selection());
    b.select(1, 3);
    b.move_right(false, false);
    CHECK(b.cursor() == 3);
    b.move_right(false, false);
    b.move_right(false, false);
    b.move_right(false, false);
    CHECK(b.cursor() == 5);
    b.move_left(false, true);
    CHECK(b.cursor() == 0);
    b.move_left(false, false);
    CHECK(b.cursor() == 0);
}

TEST_CASE("backspace and delete remove a character, a word or the selection") {
    EditBuffer b;
    b.set_text(U"foo bar baz");
    b.backspace(false);
    CHECK(b.text() == U"foo bar ba");
    b.backspace(true);
    CHECK(b.text() == U"foo bar ");
    b.move_to(0, false);
    b.delete_forward(true);
    CHECK(b.text() == U"bar ");
    b.delete_forward(false);
    CHECK(b.text() == U"ar ");
    b.select_all();
    b.backspace(false);
    CHECK(b.text().empty());
    b.backspace(false);
    b.delete_forward(false);
    CHECK(b.text().empty());
}

TEST_CASE("normalize folds line breaks and drops control characters") {
    const EditBuffer single;
    CHECK(single.normalize(U"a\r\nb\rc\nd\te\x01\x7f") == U"a b c d e");
    const EditBuffer multi(true);
    CHECK(multi.normalize(U"a\r\nb\rc\nd") == U"a\nb\nc\nd");
}

TEST_CASE("a composition sits at the cursor until it is committed") {
    EditBuffer b;
    b.set_text(U"abc");
    b.select(2, 3);
    b.compose(U"かな", 1);
    CHECK(b.text() == U"ab");
    CHECK(b.composing());
    CHECK(b.shown() == U"abかな");
    CHECK(b.composition_first() == 2);
    CHECK(b.caret() == 3);
    CHECK(b.shown_index(1) == 1);
    CHECK(b.shown_index(2) == 4);
    b.commit(U"仮名");
    CHECK_FALSE(b.composing());
    CHECK(b.text() == U"ab仮名");
    CHECK(b.shown() == b.text());
    CHECK(b.caret() == 4);
}

TEST_CASE("an empty composition keeps the selection and a cursor past it is clamped") {
    EditBuffer b;
    b.set_text(U"abc");
    b.select(0, 3);
    b.compose(U"", 0);
    CHECK(b.selected() == U"abc");
    b.compose(U"か", 9);
    CHECK(b.caret() == 1);
    b.clear_composition();
    CHECK(b.shown().empty());
}

TEST_CASE("select_word_at selects a run of one class") {
    EditBuffer b;
    b.set_text(U"hello world");
    b.select_word_at(8);
    CHECK(b.selected() == U"world");
    b.set_text(U"");
    b.select_word_at(3);
    CHECK(b.cursor() == 0);
}

TEST_CASE("the line map finds lines, their ends and soft wraps") {
    const std::u32string text = U"abc\ndefghi";
    const LineMap lines(text, {{0, 3}, {4, 7}, {7, 10}});
    CHECK(lines.size() == 3);
    CHECK(lines.limit(0) == 3);
    CHECK(lines.limit(1) == 7);
    CHECK(lines.limit(2) == 10);
    CHECK_FALSE(lines.wraps_at(0, 4));
    CHECK(lines.wraps_at(1, 7));
    CHECK(lines.line_of(7, false) == 2);
    CHECK(lines.line_of(7, true) == 1);
    CHECK(lines.line_of(3, true) == 0);
}

TEST_CASE("home, end, up and down move by line through the line map") {
    EditBuffer b(true);
    b.set_text(U"abc\ndefgh\nij");
    const LineMap lines = hard_lines(b.shown());
    const MonospaceGeometry geometry(lines);
    b.move_to(6, false);
    b.move_home(lines, false, false);
    CHECK(b.cursor() == 4);
    b.move_end(lines, true, false);
    CHECK(b.selected() == U"defgh");
    b.move_to(8, false);
    b.move_vertical(lines, geometry, -1, false);
    CHECK(b.cursor() == 3);
    b.move_vertical(lines, geometry, 1, false);
    CHECK(b.cursor() == 8);
    b.move_vertical(lines, geometry, 1, false);
    CHECK(b.cursor() == 12);
    b.move_vertical(lines, geometry, 1, true);
    CHECK(b.cursor() == 12);
    b.move_home(lines, false, true);
    CHECK(b.cursor() == 0);
    b.move_vertical(lines, geometry, -1, false);
    CHECK(b.cursor() == 0);
    b.move_end(lines, false, true);
    CHECK(b.cursor() == 12);
}
