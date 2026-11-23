#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina/text/typeface.hpp"

namespace lamina::examples {

inline constexpr std::string_view title_bar_flag = "--title-bar";
inline constexpr std::string_view typeface_option = "--font";
inline constexpr std::string_view frame_stats_option = "--frame-stats";

bool take_flag(int& argc, char** argv, std::string_view flag);
std::optional<std::string> take_option(int& argc, char** argv, std::string_view name);
text::Typeface take_typeface(int& argc, char** argv);
diagnostics::FrameStatsSwitches take_frame_stats(int& argc, char** argv);
int window_height_dp(int content_height_dp, bool title_bar);

}
