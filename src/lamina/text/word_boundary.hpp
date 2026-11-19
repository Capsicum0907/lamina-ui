#pragma once

#include <cstddef>
#include <string_view>

namespace lamina::text {

enum class WordClass { space, newline, punctuation, hiragana, katakana, han, word };

struct TextRange {
    std::size_t first;
    std::size_t last;
};

WordClass word_class(char32_t c);

std::size_t next_word(std::u32string_view text, std::size_t index);
std::size_t previous_word(std::u32string_view text, std::size_t index);
TextRange word_at(std::u32string_view text, std::size_t index);

}
