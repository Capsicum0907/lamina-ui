#include <doctest/doctest.h>

#include <string>

#include "lamina/text/edit_buffer.hpp"

using namespace lamina::text;

namespace {

constexpr EditHistoryLimits limits{.depth = 8, .pause_seconds = 1.0};

EditBuffer buffer(EditBufferOptions options = {}) {
    options.history = limits;
    return EditBuffer(options);
}

void type(EditBuffer& b, std::u32string_view text) {
    for (const char32_t c : text) {
        b.type(std::u32string_view(&c, 1));
    }
}

std::size_t undo_all(EditBuffer& b) {
    std::size_t steps = 0;
    while (b.undo()) {
        ++steps;
    }
    return steps;
}

}

TEST_CASE("a buffer without history limits keeps no history") {
    EditBuffer b;
    b.type(U"a");
    CHECK_FALSE(b.can_undo());
    CHECK_FALSE(b.undo());
    CHECK(b.text() == U"a");
}

TEST_CASE("consecutive typing is one undo step") {
    EditBuffer b = buffer();
    type(b, U"hello");
    CHECK(b.can_undo());
    CHECK(b.undo());
    CHECK(b.text().empty());
    CHECK(b.cursor() == 0);
    CHECK_FALSE(b.can_undo());
}

TEST_CASE("typing whitespace after a word starts a new step") {
    EditBuffer b = buffer();
    type(b, U"hello world");
    CHECK(b.undo());
    CHECK(b.text() == U"hello");
    CHECK(b.undo());
    CHECK(b.text().empty());
    b.set_text(U"");
    type(b, U"a  b");
    CHECK(b.undo());
    CHECK(b.text() == U"a");
}

TEST_CASE("a typed newline breaks a step like a space") {
    EditBuffer b = buffer({.multiline = true});
    type(b, U"one\ntwo");
    CHECK(b.undo());
    CHECK(b.text() == U"one");
}

TEST_CASE("a caret move without editing breaks a step") {
    EditBuffer b = buffer();
    type(b, U"abc");
    b.move_left(false, false);
    b.move_right(false, false);
    type(b, U"d");
    CHECK(b.undo());
    CHECK(b.text() == U"abc");
    CHECK(b.undo());
    CHECK(b.text().empty());
}

TEST_CASE("a selection change without editing breaks a step") {
    EditBuffer b = buffer();
    type(b, U"abc");
    b.select(3, 3);
    type(b, U"d");
    CHECK(b.undo());
    CHECK(b.text() == U"abc");
}

TEST_CASE("switching between inserting and deleting breaks a step") {
    EditBuffer b = buffer();
    type(b, U"abcd");
    b.backspace(false);
    b.backspace(false);
    type(b, U"xy");
    CHECK(b.text() == U"abxy");
    CHECK(b.undo());
    CHECK(b.text() == U"ab");
    CHECK(b.undo());
    CHECK(b.text() == U"abcd");
    CHECK(b.undo());
    CHECK(b.text().empty());
}

TEST_CASE("consecutive deletes are one step") {
    EditBuffer b = buffer();
    b.set_text(U"abcdef");
    b.move_to(3, false);
    b.backspace(false);
    b.backspace(false);
    b.delete_forward(false);
    CHECK(b.text() == U"aef");
    CHECK(b.undo());
    CHECK(b.text() == U"abcdef");
    CHECK(b.cursor() == 3);
    CHECK_FALSE(b.can_undo());
}

TEST_CASE("a pause longer than the limit breaks a step") {
    EditBuffer b = buffer();
    type(b, U"ab");
    b.advance(limits.pause_seconds);
    type(b, U"c");
    b.advance(limits.pause_seconds * 2);
    type(b, U"d");
    CHECK(b.undo());
    CHECK(b.text() == U"abc");
    CHECK(b.undo());
    CHECK(b.text().empty());
}

TEST_CASE("redo replays undone steps until a new edit clears them") {
    EditBuffer b = buffer();
    type(b, U"one two");
    CHECK(b.undo());
    CHECK(b.undo());
    CHECK(b.text().empty());
    CHECK(b.can_redo());
    CHECK(b.redo());
    CHECK(b.text() == U"one");
    CHECK(b.redo());
    CHECK(b.text() == U"one two");
    CHECK_FALSE(b.redo());
    CHECK(b.undo());
    CHECK(b.can_redo());
    type(b, U"!");
    CHECK_FALSE(b.can_redo());
    CHECK(b.text() == U"one!");
}

TEST_CASE("a redone step does not merge with the next typing") {
    EditBuffer b = buffer();
    type(b, U"ab");
    CHECK(b.undo());
    CHECK(b.redo());
    type(b, U"c");
    CHECK(b.undo());
    CHECK(b.text() == U"ab");
}

TEST_CASE("the history keeps at most its depth of steps") {
    EditBuffer b = buffer();
    for (std::size_t i = 0; i < limits.depth + 3; ++i) {
        b.insert(U"x");
    }
    CHECK(undo_all(b) == limits.depth);
    CHECK(b.text() == U"xxx");
}

TEST_CASE("set_text clears the history") {
    EditBuffer b = buffer();
    type(b, U"abc");
    CHECK(b.undo());
    b.set_text(U"new");
    CHECK_FALSE(b.can_undo());
    CHECK_FALSE(b.can_redo());
}

TEST_CASE("undo and redo restore the caret and the selection around the edit") {
    EditBuffer b = buffer();
    b.set_text(U"Hello world");
    b.select(6, 11);
    b.insert(U"there");
    CHECK(b.text() == U"Hello there");
    CHECK(b.undo());
    CHECK(b.text() == U"Hello world");
    CHECK(b.anchor() == 6);
    CHECK(b.cursor() == 11);
    CHECK(b.redo());
    CHECK(b.text() == U"Hello there");
    CHECK(b.cursor() == 11);
    CHECK_FALSE(b.has_selection());
}

TEST_CASE("replacing the selection by typing is a step of its own") {
    EditBuffer b = buffer();
    type(b, U"abc");
    b.select(1, 2);
    type(b, U"X");
    type(b, U"Y");
    CHECK(b.text() == U"aXYc");
    CHECK(b.undo());
    CHECK(b.text() == U"aXc");
    CHECK(b.undo());
    CHECK(b.text() == U"abc");
    CHECK(b.anchor() == 1);
    CHECK(b.cursor() == 2);
}

TEST_CASE("insert and erase_selection are single steps") {
    EditBuffer b = buffer();
    type(b, U"ab");
    b.insert(U"cd");
    b.insert(U"ef");
    CHECK(b.undo());
    CHECK(b.text() == U"abcd");
    CHECK(b.undo());
    CHECK(b.text() == U"ab");
    b.select(0, 2);
    b.erase_selection();
    CHECK(b.undo());
    CHECK(b.text() == U"ab");
}

TEST_CASE("an edit that changes nothing is not recorded") {
    EditBuffer b = buffer();
    b.insert(U"");
    b.backspace(false);
    CHECK_FALSE(b.can_undo());
}

TEST_CASE("an IME commit is one step, including the selection it replaced") {
    EditBuffer b = buffer();
    type(b, U"ab");
    b.select(0, 2);
    b.compose(U"に", 1);
    b.compose(U"にほん", 3);
    CHECK(b.text().empty());
    b.commit(U"日本");
    type(b, U"x");
    CHECK(b.text() == U"日本x");
    CHECK(b.undo());
    CHECK(b.text() == U"日本");
    CHECK(b.undo());
    CHECK(b.text() == U"ab");
    CHECK(b.anchor() == 0);
    CHECK(b.cursor() == 2);
}

TEST_CASE("a commit of several characters without a composition is one step") {
    EditBuffer b = buffer();
    type(b, U"a");
    b.commit(U"bc");
    CHECK(b.undo());
    CHECK(b.text() == U"a");
}

TEST_CASE("a single committed character merges with typing") {
    EditBuffer b = buffer();
    b.commit(U"a");
    b.commit(U"b");
    CHECK(b.undo());
    CHECK(b.text().empty());
}

TEST_CASE("a cancelled composition records the selection it erased") {
    EditBuffer b = buffer();
    b.insert(U"abc");
    b.select(0, 3);
    b.compose(U"x", 1);
    b.compose(U"", 0);
    CHECK(b.text().empty());
    CHECK(b.undo());
    CHECK(b.text() == U"abc");
    b.set_text(U"abc");
    b.compose(U"x", 1);
    b.clear_composition();
    CHECK_FALSE(b.can_undo());
}

TEST_CASE("undo and redo do nothing while composing") {
    EditBuffer b = buffer();
    type(b, U"ab");
    b.compose(U"に", 1);
    CHECK_FALSE(b.undo());
    CHECK_FALSE(b.redo());
    CHECK(b.composition() == U"に");
    CHECK(b.text() == U"ab");
}
