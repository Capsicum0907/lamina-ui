#pragma once

#include <filesystem>
#include <string>

namespace lamina::sdl3 {

std::filesystem::path preference_directory(const std::string& organization, const std::string& application);

}
