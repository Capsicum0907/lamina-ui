#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/focus_manager.hpp"

namespace lamina::examples::components {

struct PageContext {
    text::FontStack fonts;
    icons::IconFont icon_font;
    std::function<void(std::string)> report;
    std::function<void(widgets::FocusManager&)> add_header;
};

class Page {
public:
    virtual ~Page() = default;
    virtual layout::NodePtr body() const = 0;
    virtual widgets::FocusManager& focus() = 0;
    virtual bool handle(const input::PointerEvent& event) = 0;
    virtual bool handle(const input::KeyEvent& event) = 0;
    virtual bool handle(const input::WheelEvent& event) = 0;
    virtual void step(double dt_seconds) = 0;
    virtual bool animating() const = 0;
    virtual void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const = 0;
    virtual void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const = 0;
    virtual std::vector<widgets::AccessibleNode> accessible_nodes() const = 0;
    virtual void set_enabled(bool enabled) = 0;
    virtual bool set_enabled(std::string_view name, bool enabled) = 0;
    virtual void cancel_interaction() = 0;
    virtual std::optional<double> next_wake_seconds() const = 0;
    virtual void set_window(layout::Size window) = 0;
};

std::unique_ptr<Page> make_chips_page(PageContext context);
std::unique_ptr<Page> make_lists_page(PageContext context);
std::unique_ptr<Page> make_bars_page(PageContext context);

}
