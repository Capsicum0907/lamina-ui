#pragma once

namespace lamina::shapes {

struct CornerRounding {
    float radius = 0.0f;
    float smoothing = 0.0f;
};

inline constexpr CornerRounding unrounded{};

}
