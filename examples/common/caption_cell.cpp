#include "common/caption_cell.hpp"

namespace lamina::examples {

layout::NodePtr caption_cell(widgets::Label& caption, double width, double height, double line_height) {
    caption.measure({});
    return layout::fixed({width, height}, [&caption, line_height](const render::Rect& r) {
        caption.set_origin({r.x, r.y + (r.height - line_height) / 2.0});
    });
}

}
