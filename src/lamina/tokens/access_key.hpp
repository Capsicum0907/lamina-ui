#pragma once

namespace lamina::tokens {

struct AccessKeyTokens {
    double min_underline_thickness;
    char32_t suffix_open;
    char32_t suffix_close;
};

inline constexpr AccessKeyTokens access_key_tokens{
    .min_underline_thickness = 1.0,
    .suffix_open = U'(',
    .suffix_close = U')',
};

}
