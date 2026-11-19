#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace lamina::examples::todo {

std::optional<std::string> read_file(const std::filesystem::path& path);
void write_file_replacing(const std::filesystem::path& path, std::string_view contents);

}
