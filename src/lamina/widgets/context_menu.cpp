#include "lamina/widgets/context_menu.hpp"

#include <stdexcept>
#include <utility>

#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

ContextMenuArea::ContextMenuArea(Menu& menu, ContextMenuAreaOptions options) : menu_(menu), options_(std::move(options)) {
    if (!options_.area) {
        throw std::invalid_argument("a context menu area needs its area");
    }
}

void ContextMenuArea::open(render::Point at, input::InputSource source) {
    if (options_.before_open) {
        options_.before_open(at);
    }
    menu_.open_at(at, window_, source);
}

bool ContextMenuArea::handle(const input::ContextClick& e) {
    if (!contains(options_.area(), e.x_dp, e.y_dp)) {
        return false;
    }
    open({e.x_dp, e.y_dp}, input::InputSource::pointer);
    return true;
}

bool ContextMenuArea::handle(const input::KeyEvent& e) {
    if (menu_.is_open() || !requests_context_menu(e) || !options_.focused || !options_.focused()) {
        return false;
    }
    std::optional<render::Point> at = options_.keyboard_point ? options_.keyboard_point() : std::nullopt;
    if (!at) {
        const render::Rect r = options_.area();
        at = render::Point{r.x, r.y};
    }
    open(*at, input::InputSource::keyboard);
    return true;
}

}
