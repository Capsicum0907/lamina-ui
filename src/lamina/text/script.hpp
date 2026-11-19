#pragma once

namespace lamina::text {

struct CodeRange {
    char32_t first;
    char32_t last;

    constexpr bool contains(char32_t c) const { return c >= first && c <= last; }
};

namespace blocks {

inline constexpr CodeRange general_punctuation{U' ', U'⁯'};
inline constexpr CodeRange cjk_symbols{U'　', U'〿'};
inline constexpr CodeRange hiragana{U'぀', U'ゟ'};
inline constexpr CodeRange katakana{U'゠', U'ヿ'};
inline constexpr CodeRange katakana_extensions{U'ㇰ', U'ㇿ'};
inline constexpr CodeRange cjk_extension_a{U'㐀', U'䶿'};
inline constexpr CodeRange cjk_unified{U'一', U'鿿'};
inline constexpr CodeRange cjk_compatibility{U'豈', U'﫿'};
inline constexpr CodeRange halfwidth_fullwidth{U'＀', U'￯'};
inline constexpr CodeRange cjk_supplementary{U'\U00020000', U'\U0003134f'};

}

inline constexpr CodeRange cjk_blocks[] = {
    blocks::cjk_symbols,       blocks::hiragana,          blocks::katakana,
    blocks::cjk_extension_a,   blocks::cjk_unified,       blocks::cjk_compatibility,
    blocks::halfwidth_fullwidth, blocks::cjk_supplementary,
};

constexpr bool is_space(char32_t c) {
    return c == U' ' || c == U'\t' || c == U'　';
}

constexpr bool is_cjk(char32_t c) {
    for (const CodeRange& r : cjk_blocks) {
        if (r.contains(c)) {
            return true;
        }
    }
    return false;
}

}
