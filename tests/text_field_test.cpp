#include <doctest/doctest.h>

#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/text_field.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using input::TextEventKind;

namespace {

widgets::TextField field(widgets::TextFieldOptions options = {}) {
    options.label = "Label";
    widgets::TextField f(std::move(options), fonts());
    f.set_width(300.0);
    return f;
}

widgets::TextField focused_field(widgets::TextFieldOptions options = {}) {
    widgets::TextField f = field(std::move(options));
    f.set_origin({0.0, 0.0});
    f.handle({PointerAction::down, 10.0, 20.0});
    return f;
}

class FakeClipboard final : public input::Clipboard {
public:
    std::string text() override { return contents; }
    void set_text(std::string_view utf8) override { contents = utf8; }

    std::string contents;
};

input::KeyEvent ctrl(Key key, bool shift = false) {
    return {key, shift, true};
}

}

TEST_CASE("clicking inside focuses and clicking outside blurs") {
    widgets::TextField f = field();
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

TEST_CASE("letters that are not shortcuts leave the field alone") {
    auto f = focused_field();
    f.handle({TextEventKind::commit, "abc", 0});
    f.handle({Key::left, true});
    f.step(0.3);
    const double blink = f.next_blink_seconds();
    CHECK_FALSE(f.handle(ctrl(Key::b)));
    CHECK_FALSE(f.handle({Key::q, false}));
    CHECK_FALSE(f.handle({Key::v, false}));
    CHECK(f.text() == "abc");
    CHECK(f.selected_text() == "c");
    CHECK(f.next_blink_seconds() == blink);
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
    widgets::TextField f = field();
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
    auto f = focused_field({.clipboard = &clipboard});
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
    auto f = focused_field({.multiline = true, .clipboard = &clipboard});
    f.handle(ctrl(Key::v));
    CHECK(f.text() == "one\ntwo\nthree");
    CHECK(f.line_count() == 3);
}

TEST_CASE("enter inserts a newline in a multi-line field and ctrl+enter submits") {
    auto f = focused_field({.multiline = true});
    f.handle({TextEventKind::commit, "a", 0});
    CHECK_FALSE(f.handle({Key::enter, false}));
    f.handle({TextEventKind::commit, "b", 0});
    CHECK(f.text() == "a\nb");
    CHECK(f.handle(ctrl(Key::enter)));
}

TEST_CASE("up and down move between lines keeping the x position") {
    auto f = focused_field({.multiline = true});
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
    auto f = focused_field({.multiline = true});
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
    thorvg::Canvas canvas(600, 80);
    canvas.clear(background);
    f.paint(canvas, scheme);
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
    auto f = focused_field({.multiline = true});
    f.set_text("abc\ndef");
    const double y = f.caret_area().y;
    f.handle({TextEventKind::compose, "\xe3\x81\x8b", 1});
    CHECK(f.text() == "abc\ndef");
    CHECK(f.caret_area().y == doctest::Approx(y));
    CHECK(f.line_count() == 2);
}

TEST_CASE("end on a soft-wrapped line without spaces stays on that line") {
    auto f = focused_field({.multiline = true});
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
    auto f = focused_field({.multiline = true});
    f.handle({TextEventKind::commit, "abc", 0});
    const double x = f.caret_area().x;
    f.handle({TextEventKind::commit, " ", 0});
    CHECK(f.caret_area().x > x);
}

TEST_CASE("handle reports true only for a press inside the field") {
    widgets::TextField f = field();
    CHECK_FALSE(f.handle({PointerAction::move, 10.0, 20.0}));
    CHECK(f.handle({PointerAction::down, 10.0, 20.0}));
    CHECK_FALSE(f.handle({PointerAction::up, 10.0, 20.0}));
    CHECK_FALSE(f.handle({PointerAction::leave, 0.0, 0.0}));
    CHECK_FALSE(f.handle({PointerAction::down, 900.0, 900.0}));
    CHECK_FALSE(f.focused());
}

TEST_CASE("the width is at least the minimum and the bounds follow width and height") {
    widgets::TextField f({.label = "Label"}, fonts());
    CHECK(f.width() == doctest::Approx(tokens::outlined_text_field.min_width));
    f.set_width(10.0);
    CHECK(f.width() == doctest::Approx(tokens::outlined_text_field.min_width));
    f.set_width(400.0);
    f.set_origin({5.0, 7.0});
    CHECK(f.width() == doctest::Approx(400.0));
    CHECK(f.height() == doctest::Approx(tokens::outlined_text_field.height));
    CHECK(f.bounds().x == doctest::Approx(5.0));
    CHECK(f.bounds().y == doctest::Approx(7.0));
    CHECK(f.bounds().width == doctest::Approx(f.width()));
    CHECK(f.bounds().height == doctest::Approx(f.height()));
}

TEST_CASE("a narrower multi-line field wraps into more lines") {
    widgets::TextField f({.label = "Label", .multiline = true}, fonts());
    f.set_width(600.0);
    f.set_text(std::string(80, 'w'));
    const std::size_t wide = f.line_count();
    f.set_width(tokens::outlined_text_field.min_width);
    CHECK(f.line_count() > wide);
}

TEST_CASE("the floating label leaves a gap in the outline without painting over it") {
    widgets::TextField f = field();
    f.set_origin({10.0, 20.0});
    f.set_text("filled");
    settle(f);
    const color::Argb clear = 0xff102030;
    thorvg::Canvas canvas(400, 100);
    canvas.clear(clear);
    f.paint(canvas, scheme());
    const auto& t = tokens::outlined_text_field;
    const int row = 20;
    const int gap_left = static_cast<int>(10.0 + t.padding - t.label_gap_padding);
    CHECK(canvas.pixel(gap_left + 1, row) == clear);
    CHECK(canvas.pixel(gap_left - 4, row) != clear);
    CHECK(canvas.pixel(300, row) != clear);
    CHECK(canvas.pixel(300, row + 2) == clear);
}

TEST_CASE("a field without a label keeps its outline whole") {
    widgets::TextField f({}, fonts());
    f.set_width(300.0);
    f.set_origin({10.0, 20.0});
    f.handle({PointerAction::down, 50.0, 40.0});
    f.set_text("filled");
    settle(f);
    const color::Argb clear = 0xff102030;
    thorvg::Canvas canvas(400, 100);
    canvas.clear(clear);
    f.paint(canvas, scheme());
    const auto& t = tokens::outlined_text_field;
    const int gap_left = static_cast<int>(10.0 + t.padding - t.label_gap_padding);
    CHECK(canvas.pixel(gap_left + 1, 20) == scheme()[t.focus_outline]);
}

namespace {

double tab_stop() {
    const text::TextStyle style = tokens::text_style(tokens::type_style(tokens::outlined_text_field.input_type));
    return (text::layout_line(fonts(), " ", style).width + style.tracking) * style.tab_width;
}

double input_width(std::string_view utf8) {
    const text::TextStyle style = tokens::text_style(tokens::type_style(tokens::outlined_text_field.input_type));
    return text::layout_line(fonts(), utf8, style).width;
}

}

TEST_CASE("a field that accepts tabs inserts one at the caret and replaces the selection") {
    auto f = focused_field({.multiline = true, .accepts_tab = true});
    CHECK(f.accepts_tab());
    f.handle({TextEventKind::commit, "ab", 0});
    f.handle({Key::left, false});
    CHECK_FALSE(f.handle({Key::tab, false}));
    CHECK(f.text() == "a\tb");
    CHECK(f.cursor() == 2);
    f.handle(ctrl(Key::a));
    f.handle({Key::tab, false});
    CHECK(f.text() == "\t");
    CHECK(f.cursor() == 1);
}

TEST_CASE("an inserted tab is an ordinary edit that one backspace removes") {
    auto f = focused_field({.accepts_tab = true});
    f.handle({TextEventKind::commit, "ab", 0});
    f.handle({Key::tab, false});
    CHECK(f.text() == "ab\t");
    f.handle({Key::backspace, false});
    CHECK(f.text() == "ab");
    CHECK(f.cursor() == 2);
}

TEST_CASE("shift+tab and ctrl+tab leave a tab-accepting field's text alone") {
    auto f = focused_field({.accepts_tab = true});
    f.handle({TextEventKind::commit, "ab", 0});
    CHECK_FALSE(f.handle({Key::tab, true}));
    CHECK_FALSE(f.handle(ctrl(Key::tab)));
    CHECK_FALSE(f.handle(ctrl(Key::tab, true)));
    CHECK(f.text() == "ab");
}

TEST_CASE("a field that does not accept tabs ignores tab and turns pasted tabs into spaces") {
    FakeClipboard clipboard;
    clipboard.contents = "a\tb";
    auto plain = focused_field({.clipboard = &clipboard});
    plain.handle({Key::tab, false});
    CHECK(plain.text().empty());
    plain.handle(ctrl(Key::v));
    CHECK(plain.text() == "a b");
    auto tabs = focused_field({.accepts_tab = true, .clipboard = &clipboard});
    tabs.handle(ctrl(Key::v));
    CHECK(tabs.text() == "a\tb");
    tabs.set_text("x\ty");
    CHECK(tabs.text() == "x\ty");
}

TEST_CASE("the caret after a tab stands at the next tab stop") {
    auto f = focused_field({.multiline = true, .accepts_tab = true});
    const double left = f.caret_area().x;
    f.handle({Key::tab, false});
    CHECK(f.caret_area().x - left == doctest::Approx(tab_stop()));
    f.handle({TextEventKind::commit, "a", 0});
    f.handle({Key::tab, false});
    CHECK(f.caret_area().x - left == doctest::Approx(2.0 * tab_stop()));
}

TEST_CASE("a click inside a tab's span puts the caret at the nearer edge of the tab") {
    auto f = focused_field({.multiline = true, .accepts_tab = true});
    f.set_text("a\tb");
    const double left = tokens::outlined_text_field.padding;
    const double a = input_width("a");
    const double y = f.height() / 2.0;
    f.handle({PointerAction::down, left + a + (tab_stop() - a) * 0.25, y, 1});
    f.handle({PointerAction::up, left + a + (tab_stop() - a) * 0.25, y, 1});
    CHECK(f.cursor() == 1);
    f.handle({PointerAction::down, left + a + (tab_stop() - a) * 0.75, y, 1});
    f.handle({PointerAction::up, left + a + (tab_stop() - a) * 0.75, y, 1});
    CHECK(f.cursor() == 2);
}

TEST_CASE("word moves step over a tab as over a space") {
    auto f = focused_field({.accepts_tab = true});
    f.set_text("one\ttwo");
    f.handle({Key::home, false});
    f.handle(ctrl(Key::right));
    CHECK(f.cursor() == 4);
}

namespace {

void type_keys(widgets::TextField& f, std::string_view text) {
    for (const char c : text) {
        f.handle({TextEventKind::commit, std::string(1, c), 0});
    }
}

}

TEST_CASE("ctrl+z undoes typing as one step and both redo keys replay it") {
    auto f = focused_field();
    type_keys(f, "abc");
    CHECK(f.handles_history_key(ctrl(Key::z)));
    CHECK_FALSE(f.handle(ctrl(Key::z)));
    CHECK(f.text().empty());
    CHECK(f.handles_history_key(ctrl(Key::z, true)));
    f.handle(ctrl(Key::z, true));
    CHECK(f.text() == "abc");
    f.handle(ctrl(Key::z));
    CHECK(f.handles_history_key(ctrl(Key::y)));
    f.handle(ctrl(Key::y));
    CHECK(f.text() == "abc");
    CHECK(f.cursor() == 3);
}

TEST_CASE("ctrl+shift+y is not a redo key") {
    auto f = focused_field();
    type_keys(f, "abc");
    f.handle(ctrl(Key::z));
    CHECK_FALSE(f.handles_history_key(ctrl(Key::y, true)));
    f.handle(ctrl(Key::y, true));
    CHECK(f.text().empty());
}

TEST_CASE("a new edit after undo clears the redo history") {
    auto f = focused_field();
    type_keys(f, "ab");
    f.handle(ctrl(Key::z));
    type_keys(f, "c");
    CHECK_FALSE(f.can_redo());
    CHECK_FALSE(f.handles_history_key(ctrl(Key::y)));
    f.handle(ctrl(Key::y));
    CHECK(f.text() == "c");
}

TEST_CASE("a pause in typing longer than the token starts a new undo step") {
    auto f = focused_field();
    type_keys(f, "ab");
    f.step(tokens::outlined_text_field.undo_pause_seconds / 2.0);
    type_keys(f, "c");
    f.step(tokens::outlined_text_field.undo_pause_seconds * 2.0);
    type_keys(f, "d");
    f.handle(ctrl(Key::z));
    CHECK(f.text() == "abc");
    f.handle(ctrl(Key::z));
    CHECK(f.text().empty());
}

TEST_CASE("a caret move and whitespace break the typing into steps") {
    auto f = focused_field();
    type_keys(f, "one two");
    f.handle({Key::left, false});
    f.handle({Key::right, false});
    type_keys(f, "s");
    f.handle(ctrl(Key::z));
    CHECK(f.text() == "one two");
    f.handle(ctrl(Key::z));
    CHECK(f.text() == "one");
    f.handle(ctrl(Key::z));
    CHECK(f.text().empty());
}

TEST_CASE("paste, cut and an inserted tab are single undo steps") {
    FakeClipboard clipboard;
    auto f = focused_field({.accepts_tab = true, .clipboard = &clipboard});
    type_keys(f, "ab");
    clipboard.contents = "XY";
    f.handle(ctrl(Key::v));
    f.handle({Key::tab, false});
    type_keys(f, "c");
    CHECK(f.text() == "abXY\tc");
    f.handle(ctrl(Key::z));
    CHECK(f.text() == "abXY\t");
    f.handle(ctrl(Key::z));
    CHECK(f.text() == "abXY");
    f.handle(ctrl(Key::z));
    CHECK(f.text() == "ab");
    f.select(0, 2);
    f.handle(ctrl(Key::x));
    CHECK(f.text().empty());
    f.handle(ctrl(Key::z));
    CHECK(f.text() == "ab");
    CHECK(f.selected_text() == "ab");
}

TEST_CASE("undo and redo restore the caret and the selection") {
    auto f = focused_field();
    f.set_text("Hello world");
    f.select(6, 11);
    type_keys(f, "X");
    CHECK(f.text() == "Hello X");
    f.handle(ctrl(Key::z));
    CHECK(f.anchor() == 6);
    CHECK(f.cursor() == 11);
    CHECK(f.selected_text() == "world");
    f.handle(ctrl(Key::y));
    CHECK(f.text() == "Hello X");
    CHECK(f.cursor() == 7);
    CHECK(f.selected_text().empty());
}

TEST_CASE("the undo history keeps the token's depth of steps") {
    FakeClipboard clipboard;
    clipboard.contents = "x";
    auto f = focused_field({.clipboard = &clipboard});
    const std::size_t depth = tokens::outlined_text_field.undo_depth;
    for (std::size_t i = 0; i < depth + 2; ++i) {
        f.handle(ctrl(Key::v));
    }
    std::size_t steps = 0;
    while (f.can_undo()) {
        f.handle(ctrl(Key::z));
        ++steps;
    }
    CHECK(steps == depth);
    CHECK(f.text() == "xx");
}

TEST_CASE("set_text clears the undo history") {
    auto f = focused_field();
    type_keys(f, "abc");
    f.set_text("");
    CHECK_FALSE(f.can_undo());
    CHECK_FALSE(f.handles_history_key(ctrl(Key::z)));
}

TEST_CASE("undo keys pass through when the field has nothing to undo or redo or no focus") {
    auto f = focused_field();
    f.step(0.3);
    const double blink = f.next_blink_seconds();
    CHECK_FALSE(f.handles_history_key(ctrl(Key::z)));
    CHECK_FALSE(f.handles_history_key(ctrl(Key::z, true)));
    CHECK_FALSE(f.handles_history_key(ctrl(Key::y)));
    CHECK_FALSE(f.handle(ctrl(Key::z)));
    CHECK(f.next_blink_seconds() == blink);
    CHECK_FALSE(f.handles_history_key({Key::z, false}));
    type_keys(f, "a");
    CHECK(f.handles_history_key(ctrl(Key::z)));
    CHECK_FALSE(f.handles_history_key(ctrl(Key::y)));
    f.set_focused(false);
    CHECK_FALSE(f.handles_history_key(ctrl(Key::z)));
    f.handle(ctrl(Key::z));
    CHECK(f.text() == "a");
}

TEST_CASE("undo keys are taken but do nothing while an IME composition is open") {
    auto f = focused_field();
    f.handle({TextEventKind::compose, "に", 1});
    CHECK(f.handles_history_key(ctrl(Key::z)));
    CHECK(f.handles_history_key(ctrl(Key::y)));
    f.handle(ctrl(Key::z));
    f.handle({TextEventKind::commit, "日本", 0});
    type_keys(f, "a");
    CHECK(f.text() == "日本a");
    f.handle(ctrl(Key::z));
    CHECK(f.text() == "日本");
    f.handle(ctrl(Key::z));
    CHECK(f.text().empty());
}
