#include <doctest/doctest.h>

#include "lamina/text/line_break.hpp"
#include "lamina/text/word_boundary.hpp"

using namespace lamina::text;

TEST_CASE("characters are classed by script, with spaces and newlines apart") {
    CHECK(word_class(U'a') == WordClass::word);
    CHECK(word_class(U'_') == WordClass::word);
    CHECK(word_class(U'-') == WordClass::punctuation);
    CHECK(word_class(U' ') == WordClass::space);
    CHECK(word_class(U'　') == WordClass::space);
    CHECK(word_class(U'\n') == WordClass::newline);
    CHECK(word_class(U'あ') == WordClass::hiragana);
    CHECK(word_class(U'カ') == WordClass::katakana);
    CHECK(word_class(U'ｶ') == WordClass::katakana);
    CHECK(word_class(U'漢') == WordClass::han);
    CHECK(word_class(U'々') == WordClass::han);
    CHECK(word_class(U'。') == WordClass::punctuation);
    CHECK(word_class(U'！') == WordClass::punctuation);
    CHECK(word_class(U'Ａ') == WordClass::word);
}

TEST_CASE("next and previous word stop at a change of class and skip spaces") {
    const std::u32string_view s = U"foo bar, baz";
    CHECK(next_word(s, 0) == 4);
    CHECK(next_word(s, 4) == 7);
    CHECK(next_word(s, 7) == 9);
    CHECK(next_word(s, s.size()) == s.size());
    CHECK(previous_word(s, s.size()) == 9);
    CHECK(previous_word(s, 9) == 7);
    CHECK(previous_word(s, 7) == 4);
    CHECK(previous_word(s, 3) == 0);
    CHECK(previous_word(s, 0) == 0);
}

TEST_CASE("a run of one Japanese script is one word") {
    const std::u32string_view s = U"漢字かなカナ";
    CHECK(next_word(s, 0) == 2);
    CHECK(next_word(s, 2) == 4);
    CHECK(next_word(s, 4) == 6);
    CHECK(previous_word(s, 6) == 4);
}

TEST_CASE("word_at returns the run of one class around an index") {
    const std::u32string_view s = U"hello world";
    CHECK(word_at(s, 2).first == 0);
    CHECK(word_at(s, 2).last == 5);
    CHECK(word_at(s, 5).first == 5);
    CHECK(word_at(s, 5).last == 6);
    CHECK(word_at(s, 99).first == 6);
    CHECK(word_at(s, 99).last == 11);
    CHECK(word_at(U"", 0).last == 0);
}

TEST_CASE("the CJK blocks for line breaking hold the Japanese scripts") {
    CHECK(is_cjk(U'あ'));
    CHECK(is_cjk(U'カ'));
    CHECK(is_cjk(U'漢'));
    CHECK(is_cjk(U'。'));
    CHECK(is_cjk(U'Ａ'));
    CHECK_FALSE(is_cjk(U'a'));
    CHECK_FALSE(is_cjk(U'—'));
}
