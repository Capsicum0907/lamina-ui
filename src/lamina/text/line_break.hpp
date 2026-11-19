#pragma once

#include <string_view>

#include "lamina/text/script.hpp"

namespace lamina::text {

inline constexpr std::u32string_view no_line_start =
    U",.!?:;)]}、。，．・：；？！ー」』）］｝〉》】"
    U"ぁぃぅぇぉっゃゅょゎァィゥェォッャュョ"
    U"ヮヵヶ々ゝゞヽヾ";

inline constexpr std::u32string_view no_line_end = U"([{「『（［｛〈《【";

constexpr bool can_break_before(std::u32string_view text, std::size_t i) {
    const char32_t prev = text[i - 1];
    const char32_t cur = text[i];
    if (no_line_start.find(cur) != std::u32string_view::npos || no_line_end.find(prev) != std::u32string_view::npos) {
        return false;
    }
    if (is_space(prev) && !is_space(cur)) {
        return true;
    }
    return is_cjk(prev) || is_cjk(cur);
}

}
