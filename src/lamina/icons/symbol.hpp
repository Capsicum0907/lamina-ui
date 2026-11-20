#pragma once

namespace lamina::icons {

struct Symbol {
    char32_t code;

    friend constexpr bool operator==(Symbol, Symbol) = default;
};

}
