#pragma once

#include "lamina/motion/spring.hpp"
#include "lamina/tokens/navigation_drawer.hpp"

namespace lamina::tokens {

struct NavigationSuiteTokens {
    motion::SpringSpec spring;
    double drawer_width;
};

inline constexpr NavigationSuiteTokens navigation_suite_tokens{
    .spring = {0.9, 700.0},
    .drawer_width = navigation_drawer_tokens.max_width,
};

}
