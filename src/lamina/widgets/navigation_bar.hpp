#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/navigation_bar.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/navigation_item.hpp"

namespace lamina::widgets {

struct NavigationBarOptions {
    std::vector<NavigationItem> items;
    std::size_t selected = 0;
};

class NavigationBar {
public:
    NavigationBar(NavigationBarOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const;
    void set_width(double width) { width_ = width; }
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    std::size_t count() const { return items_.size(); }
    std::size_t selected() const { return options_.selected; }
    void select(std::size_t index);
    render::Rect item_rect(std::size_t index) const;
    render::Rect indicator_rect(std::size_t index) const;
    double indicator_progress(std::size_t index) const { return items_[index].indicator.value(); }

    bool handle(const input::PointerEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Item {
        std::optional<text::TextLine> label;
        PressState press;
        motion::Animated state_layer;
        motion::Animated indicator;
        motion::Animated color;
    };

    double content_top(std::size_t index) const;
    render::Rect full_indicator_rect(std::size_t index) const;

    NavigationBarOptions options_;
    text::FontStack fonts_;
    std::vector<Item> items_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
};

}
