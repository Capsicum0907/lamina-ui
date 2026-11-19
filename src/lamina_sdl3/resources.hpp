#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "lamina/icons/icon.hpp"

namespace lamina::sdl3 {

inline constexpr std::string_view assets_directory = "assets";
inline constexpr std::string_view icons_directory = "icons";

std::filesystem::path executable_directory();
std::filesystem::path find_asset(const std::filesystem::path& relative);
icons::IconFont bundled_icon_font();
std::filesystem::path preference_directory(const std::string& organization, const std::string& application);

}
