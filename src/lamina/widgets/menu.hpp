#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/platform/input.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/menu.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct MenuItem {
    std::string label;
    std::optional<icons::Icon> leading_icon = std::nullopt;
};

struct MenuOptions {
    std::vector<MenuItem> items;
};

struct MenuResponse {
    bool consumed = false;
    std::optional<std::size_t> chosen = std::nullopt;
};

render::Point menu_position(const render::Rect& anchor, layout::Size window, layout::Size menu);
render::Point menu_position(const render::Rect& anchor, layout::Size window, layout::Size menu, double horizontal_margin,
                            double vertical_margin);
render::Point menu_transform_origin(const render::Rect& anchor, const render::Rect& menu);

class Menu {
public:
    Menu(MenuOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const;
    render::Rect bounds() const;
    render::Rect item_rect(std::size_t index) const;

    void open(const render::Rect& anchor, layout::Size window);
    void close();
    bool is_open() const { return open_; }
    bool visible() const;
    double scale() const { return scale_.value(); }
    double opacity() const;
    render::Point transform_origin() const { return pivot_; }

    MenuResponse handle(const platform::PointerEvent& event);
    bool handle(const platform::KeyEvent& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Item {
        std::optional<text::TextLine> label;
        std::optional<icons::Icon> icon;
        PressState press;
        motion::Animated state_layer;
    };

    const text::FontStack& fonts_;
    std::vector<Item> items_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
    render::Point pivot_{0.0, 0.0};
    bool open_ = false;
    motion::Animated scale_;
    motion::Animated alpha_;
};

}
