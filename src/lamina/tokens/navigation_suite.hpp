#pragma once

#include "lamina/motion/spring.hpp"

namespace lamina::tokens {

struct NavigationSuiteTokens {
    motion::SpringSpec spring;
};

inline constexpr NavigationSuiteTokens navigation_suite_tokens{
    .spring = {0.9, 700.0},
};

}
