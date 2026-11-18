#include <doctest/doctest.h>

#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/render/thorvg_canvas.hpp"
#include "lamina/tokens/text_field.hpp"
#include "lamina/widgets/text_field.hpp"

using namespace lamina;
using platform::Key;
using platform::PointerAction;
using platform::TextEventKind;

namespace {

const text::FontStack& fonts() {
    static const text::FontStack stack({text::Font::from_file(std::string(LAMINA_TEST_FONTS_DIR) + "/lamina-test-sans.ttf")});
    return stack;
}

widgets::TextField focused_field(widgets::TextFieldOptions options = {}) {
    widgets::TextField f("Label", fonts(), 300.0, options);
    f.set_origin({0.0, 0.0});
    f.handle({PointerAction::down, 10.0, 20.0});
    return f;
}

class FakeClipboard final : public platform::Clipboard {
public:
    std::string text() override { return contents; }
    void set_text(std::string_view utf8) override { contents = utf8; }

    std::string contents;
};

platform::KeyEvent ctrl(Key key, bool shift = false) {
    return {key, shift, true};
}

}

TEST_CASE("clicking inside focuses and clicking outside blurs") {
    widgets::TextField f("Label", fonts(), 300.0);
    f.handle({PointerAction::down, 10.0, 20.0});
    CHECK(f.focused());
    f.handle({PointerAction::down, 900.0, 900.0});
    CHECK_FALSE(f.focused());
}

TEST_CASE("committed text is inserted at the cursor") {
    auto f = focused_field();
    f.handle({TextEventKind::commit, "Hello", 0});
    f.handle({Key::left, false});
    f.handle({Key::left, false});
    f.handle({TextEventKind::commit, "\xe3\x81\x82", 0});
    CHECK(f.text() == "Hel\xe3\x81\x82lo");
}

TEST_CASE("backspace and delete remove one character") {
    auto f = focused_field();
    f.handle({TextEventKind::commit, "abc", 0});
    f.handle({Key::backspace, false});
    CHECK(f.text() == "ab");
    f.handle({Key::home, false});
    f.handle({Key::delete_forward, false});
    CHECK(f.text() == "b");
}

TEST_CASE("composition is shown but not committed until the IME commits it") {
    auto f = focused_field();
    f.handle({TextEventKind::commit, "a", 0});
    f.handle({TextEventKind::compose, "\xe3\x81\x8b", 1});
    CHECK(f.text() == "a");
    f.handle({Key::backspace, false});
    CHECK(f.text() == "a");
    f.handle({TextEventKind::commit, "\xe8\xa6\x8b", 0});
    CHECK(f.text() == "a\xe8\xa6\x8b");
}

TEST_CASE("enter reports a submit and escape blurs") {
    auto f = focused_field();
    CHECK(f.handle({Key::enter, false}));
    f.handle({Key::escape, false});
    CHECK_FALSE(f.focused());
}

TEST_CASE("the caret area moves right as text is typed") {
    auto f = focused_field();
    const double before = f.caret_area().x;
    f.handle({TextEventKind::commit, "Hello", 0});
    CHECK(f.caret_area().x > before);
}

TEST_CASE("shift with arrows extends a selection that typing replaces") {
    auto f = focused_field();
    f.handle({TextEventKind::commit, "Hello", 0});
    f.handle({Key::left, true});
    f.handle({Key::left, true});
    CHECK(f.selected_text() == "lo");
    f.handle({TextEventKind::commit, "p!", 0});
    CHECK(f.text() == "Help!");
    f.handle({Key::home, true});
    CHECK(f.selected_text() == "Help!");
    f.handle({Key::right, false});
    CHECK(f.cursor() == 5);
    CHECK(f.selected_text().empty());
}

TEST_CASE("select all, then backspace or delete removes only the selection") {
    auto f = focused_field();
    f.handle({TextEventKind::commit, "abc def", 0});
    f.handle({Key::left, false});
    f.handle({Key::left, true});
    f.handle({Key::backspace, false});
    CHECK(f.text() == "abc df");
    f.handle({Key::a, false});
    CHECK(f.selected_text().empty());
    f.handle(ctrl(Key::a));
    CHECK(f.selected_text() == "abc df");
    f.handle({Key::delete_forward, false});
    CHECK(f.text().empty());
}

TEST_CASE("an IME composition replaces the selection") {
    auto f = focused_field();
    f.handle({TextEventKind::commit, "abc", 0});
    f.handle({Key::left, true});
    f.handle({TextEventKind::compose, "\xe3\x81\x8b", 1});
    CHECK(f.text() == "ab");
    f.handle({TextEventKind::commit, "\xe3\x81\x8b", 0});
    CHECK(f.text() == "ab\xe3\x81\x8b");
}

TEST_CASE("ctrl with arrows, backspace and delete work by word") {
    auto f = focused_field();
    f.handle({TextEventKind::commit, "foo bar baz", 0});
    f.handle(ctrl(Key::left));
    CHECK(f.cursor() == 8);
    f.handle(ctrl(Key::left));
    CHECK(f.cursor() == 4);
    f.handle(ctrl(Key::right, true));
    CHECK(f.selected_text() == "bar ");
    f.handle({Key::end, false});
    f.handle(ctrl(Key::backspace));
    CHECK(f.text() == "foo bar ");
    f.handle({Key::home, false});
    f.handle(ctrl(Key::delete_forward));
    CHECK(f.text() == "bar ");
}

TEST_CASE("word movement treats a run of one script as a word") {
    auto f = focused_field();
    f.set_text("\xe6\xbc\xa2\xe5\xad\x97\xe3\x81\x8b\xe3\x81\xaa\xe3\x82\xab\xe3\x83\x8a");
    f.handle({Key::home, false});
    f.handle(ctrl(Key::right));
    CHECK(f.cursor() == 2);
    f.handle(ctrl(Key::right));
    CHECK(f.cursor() == 4);
    f.handle(ctrl(Key::right));
    CHECK(f.cursor() == 6);
}

TEST_CASE("double click selects a word and dragging selects a range") {
    widgets::TextField f("Label", fonts(), 300.0);
    f.set_text("hello world");
    f.handle({PointerAction::down, 17.0, 28.0, 2});
    CHECK(f.selected_text() == "hello");
    f.handle({PointerAction::up, 17.0, 28.0});
    f.handle({PointerAction::down, 17.0, 28.0, 1});
    f.handle({PointerAction::move, 900.0, 28.0});
    CHECK(f.selected_text() == "hello world");
    f.handle({PointerAction::up, 900.0, 28.0});
    f.handle({PointerAction::move, 17.0, 28.0});
    CHECK(f.selected_text() == "hello world");
    f.handle({PointerAction::down, 17.0, 28.0, 3});
    CHECK(f.selected_text() == "hello world");
}

TEST_CASE("copy, cut and paste go through the clipboard") {
    FakeClipboard clipboard;
    auto f = focused_field({false, &clipboard});
    f.handle({TextEventKind::commit, "copy me", 0});
    f.handle(ctrl(Key::left, true));
    f.handle(ctrl(Key::c));
    CHECK(clipboard.contents == "me");
    CHECK(f.text() == "copy me");
    f.handle(ctrl(Key::x));
    CHECK(f.text() == "copy ");
    f.handle({Key::home, false});
    f.handle(ctrl(Key::v));
    CHECK(f.text() == "mecopy ");
    clipboard.contents = "a\r\nb";
    f.handle(ctrl(Key::a));
    f.handle(ctrl(Key::v));
    CHECK(f.text() == "a b");
}

TEST_CASE("pasting keeps newlines in a multi-line field") {
    FakeClipboard clipboard;
    clipboard.contents = "one\r\ntwo\rthree";
    auto f = focused_field({true, &clipboard});
    f.handle(ctrl(Key::v));
    CHECK(f.text() == "one\ntwo\nthree");
    CHECK(f.line_count() == 3);
}

TEST_CASE("enter inserts a newline in a multi-line field and ctrl+enter submits") {
    auto f = focused_field({true, nullptr});
    f.handle({TextEventKind::commit, "a", 0});
    CHECK_FALSE(f.handle({Key::enter, false}));
    f.handle({TextEventKind::commit, "b", 0});
    CHECK(f.text() == "a\nb");
    CHECK(f.handle(ctrl(Key::enter)));
}

TEST_CASE("up and down move between lines keeping the x position") {
    auto f = focused_field({true, nullptr});
    f.set_text("abc\ndef\nghi");
    REQUIRE(f.line_count() == 3);
    const double x = f.caret_area().x;
    const double y = f.caret_area().y;
    f.handle({Key::up, false});
    CHECK(f.cursor() == 7);
    CHECK(f.caret_area().x == doctest::Approx(x).epsilon(0.05));
    CHECK(f.caret_area().y < y);
    f.handle({Key::up, false});
    CHECK(f.cursor() == 3);
    f.handle({Key::up, false});
    CHECK(f.cursor() == 0);
    f.handle({Key::down, false});
    CHECK(f.cursor() == 7);
    f.handle({Key::home, false});
    CHECK(f.cursor() == 4);
    f.handle({Key::end, true});
    CHECK(f.selected_text() == "def");
    f.handle({Key::down, true});
    CHECK(f.selected_text() == "def\nghi");
}

TEST_CASE("a multi-line field grows with its lines and wraps long text") {
    auto f = focused_field({true, nullptr});
    const double one_line = f.bounds().height;
    CHECK(one_line == doctest::Approx(56.0));
    f.set_text("a\nb\nc");
    CHECK(f.bounds().height > one_line);
    f.set_text(std::string(200, 'w'));
    CHECK(f.line_count() > 1);
    const render::Rect caret = f.caret_area();
    CHECK(caret.x <= f.bounds().x + f.bounds().width);
    CHECK(caret.y > f.bounds().y + one_line / 2.0);
}

TEST_CASE("a single-line field scrolls to keep the caret visible and clips its text") {
    auto f = focused_field();
    f.set_text(std::string(200, 'w'));
    const render::Rect r = f.bounds();
    CHECK(f.caret_area().x <= r.x + r.width);
    CHECK(f.caret_area().x > r.x + r.width / 2.0);
    f.handle({Key::home, false});
    CHECK(f.caret_area().x == doctest::Approx(r.x + tokens::outlined_text_field.padding));

    const auto scheme = color::spec_2021().build(color::default_scheme_options);
    const color::Argb background = scheme[color::Role::surface];
    render::ThorVGCanvas canvas(600, 80);
    canvas.clear(background);
    f.paint(canvas, scheme, background);
    const int row = static_cast<int>(f.caret_area().y + f.caret_area().height / 2.0);
    bool inside_drawn = false;
    for (int x = 20; x < static_cast<int>(r.width) - 20; ++x) {
        inside_drawn = inside_drawn || canvas.pixel(x, row) != background;
    }
    CHECK(inside_drawn);
    bool outside_clean = true;
    for (int x = static_cast<int>(r.width) + 2; x < canvas.width(); ++x) {
        outside_clean = outside_clean && canvas.pixel(x, row) == background;
    }
    CHECK(outside_clean);
}

TEST_CASE("an IME composition on a later line keeps the caret on that line") {
    auto f = focused_field({true, nullptr});
    f.set_text("abc\ndef");
    const double y = f.caret_area().y;
    f.handle({TextEventKind::compose, "\xe3\x81\x8b", 1});
    CHECK(f.text() == "abc\ndef");
    CHECK(f.caret_area().y == doctest::Approx(y));
    CHECK(f.line_count() == 2);
}

TEST_CASE("end on a soft-wrapped line without spaces stays on that line") {
    auto f = focused_field({true, nullptr});
    std::string kana;
    for (int i = 0; i < 60; ++i) {
        kana += "\xe3\x81\x82";
    }
    f.set_text(kana);
    REQUIRE(f.line_count() > 1);
    f.handle(ctrl(Key::home));
    const double y = f.caret_area().y;
    f.handle({Key::end, false});
    CHECK(f.cursor() > 0);
    CHECK(f.caret_area().y == doctest::Approx(y));
    f.handle({Key::down, false});
    CHECK(f.caret_area().y > y);
}

TEST_CASE("a trailing space moves the caret in a multi-line field") {
    auto f = focused_field({true, nullptr});
    f.handle({TextEventKind::commit, "abc", 0});
    const double x = f.caret_area().x;
    f.handle({TextEventKind::commit, " ", 0});
    CHECK(f.caret_area().x > x);
}
