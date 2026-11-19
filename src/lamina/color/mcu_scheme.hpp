#pragma once

#include <memory>

#include "lamina/color/scheme.hpp"

namespace material_color_utilities {
struct DynamicScheme;
}

namespace lamina::color::detail {

std::unique_ptr<material_color_utilities::DynamicScheme> make_mcu_scheme(const SchemeOptions& options);

}
