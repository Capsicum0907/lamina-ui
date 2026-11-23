#pragma once

#include <string_view>

namespace lamina::sdl3 {

void report_error(std::string_view message);
void report_info(std::string_view tag, std::string_view message);

}
