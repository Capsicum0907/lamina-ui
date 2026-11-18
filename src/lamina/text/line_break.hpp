#pragma once

#include <string_view>

namespace lamina::text {

inline constexpr std::u32string_view no_line_start =
    U",.!?:;)]}、。，．・：；？！ー」』）］｝〉》】"
    U"ぁぃぅぇぉっゃゅょゎァィゥェォッャュョ"
    U"ヮヵヶ々ゝゞヽヾ";

inline constexpr std::u32string_view no_line_end = U"([{「『（［｛〈《【";

constexpr bool is_space(char32_t c) {
    return c == U' ' || c == U'\t' || c == U'　';
}

constexpr bool is_cjk(char32_t c) {
    return (c >= 0x3000 && c <= 0x30ff) || (c >= 0x3400 && c <= 0x4dbf) || (c >= 0x4e00 && c <= 0x9fff) ||
           (c >= 0xf900 && c <= 0xfaff) || (c >= 0xff00 && c <= 0xffef) || (c >= 0x20000 && c <= 0x3134f);
}

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
