#include "lamina/text/word_boundary.hpp"

#include <algorithm>

#include "lamina/text/script.hpp"

namespace lamina::text {

namespace {

struct ClassRange {
    CodeRange range;
    WordClass cls;
};

constexpr ClassRange class_ranges[] = {
    {{U'々', U'々'}, WordClass::han},
    {blocks::cjk_symbols, WordClass::punctuation},
    {{U'ぁ', blocks::hiragana.last}, WordClass::hiragana},
    {blocks::katakana, WordClass::katakana},
    {blocks::katakana_extensions, WordClass::katakana},
    {blocks::cjk_extension_a, WordClass::han},
    {blocks::cjk_unified, WordClass::han},
    {blocks::cjk_compatibility, WordClass::han},
    {blocks::cjk_supplementary, WordClass::han},
    {blocks::general_punctuation, WordClass::punctuation},
    {{U'！', U'／'}, WordClass::punctuation},
    {{U'：', U'＠'}, WordClass::punctuation},
    {{U'［', U'｀'}, WordClass::punctuation},
    {{U'｛', U'･'}, WordClass::punctuation},
    {{U'ｦ', U'ﾟ'}, WordClass::katakana},
};

constexpr char32_t ascii_end = U'\x80';

constexpr bool is_word_ascii(char32_t c) {
    return (c >= U'0' && c <= U'9') || (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || c == U'_';
}

}

WordClass word_class(char32_t c) {
    if (c == U'\n') {
        return WordClass::newline;
    }
    if (is_space(c)) {
        return WordClass::space;
    }
    for (const ClassRange& r : class_ranges) {
        if (r.range.contains(c)) {
            return r.cls;
        }
    }
    if (c < ascii_end) {
        return is_word_ascii(c) ? WordClass::word : WordClass::punctuation;
    }
    return WordClass::word;
}

std::size_t next_word(std::u32string_view s, std::size_t i) {
    if (i >= s.size()) {
        return s.size();
    }
    const WordClass c = word_class(s[i]);
    if (c != WordClass::space) {
        while (i < s.size() && word_class(s[i]) == c) {
            ++i;
        }
    }
    while (i < s.size() && word_class(s[i]) == WordClass::space) {
        ++i;
    }
    return i;
}

std::size_t previous_word(std::u32string_view s, std::size_t i) {
    i = std::min(i, s.size());
    while (i > 0 && word_class(s[i - 1]) == WordClass::space) {
        --i;
    }
    if (i == 0) {
        return 0;
    }
    const WordClass c = word_class(s[i - 1]);
    while (i > 0 && word_class(s[i - 1]) == c) {
        --i;
    }
    return i;
}

TextRange word_at(std::u32string_view s, std::size_t index) {
    if (s.empty()) {
        return {0, 0};
    }
    const std::size_t at = std::min(index, s.size() - 1);
    const WordClass c = word_class(s[at]);
    std::size_t first = at;
    while (first > 0 && word_class(s[first - 1]) == c) {
        --first;
    }
    std::size_t last = at + 1;
    while (last < s.size() && word_class(s[last]) == c) {
        ++last;
    }
    return {first, last};
}

}
