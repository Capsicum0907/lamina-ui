#pragma once

#include <cstdint>

namespace lamina::color {

using Argb = std::uint32_t;

constexpr std::uint32_t alpha(Argb c) {
    return c >> 24;
}

}
