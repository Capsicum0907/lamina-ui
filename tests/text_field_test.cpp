#include <doctest/doctest.h>

#include <string>

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

widgets::TextField focused_field() {
    widgets::TextField f("Label", fonts(), 300.0);
    f.set_origin({0.0, 0.0});
    f.handle({PointerAction::down, 10.0, 20.0});
    return f;
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
