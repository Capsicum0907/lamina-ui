#pragma once

#include <string_view>

namespace lamina::examples {

inline constexpr std::string_view title_bar_flag = "--title-bar";

bool take_flag(int& argc, char** argv, std::string_view flag);
int window_height_dp(int content_height_dp, bool title_bar);

}
