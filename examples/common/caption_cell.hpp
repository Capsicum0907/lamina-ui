#pragma once

#include "lamina/layout/layout.hpp"
#include "lamina/widgets/label.hpp"

namespace lamina::examples {

layout::NodePtr caption_cell(widgets::Label& caption, double width, double height, double line_height);

}
