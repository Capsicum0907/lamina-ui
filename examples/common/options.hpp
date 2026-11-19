#pragma once

#include <string_view>

namespace lamina::examples {

inline constexpr std::string_view title_bar_flag = "--title-bar";

bool take_flag(int& argc, char** argv, std::string_view flag);

}
