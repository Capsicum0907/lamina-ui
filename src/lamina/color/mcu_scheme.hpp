#pragma once

#include <memory>

#include "cpp/dynamiccolor/dynamic_scheme.h"
#include "lamina/color/scheme.hpp"

namespace lamina::color::detail {

std::unique_ptr<material_color_utilities::DynamicScheme> make_mcu_scheme(const SchemeOptions& options);

}
