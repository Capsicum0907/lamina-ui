#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/menu.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/popup_transition.hpp"

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

    void open(const render::Rect& anchor, layout::Size window, input::InputSource source = input::InputSource::pointer);
    void close();
    bool is_open() const { return popup_.is_open(); }
    bool visible() const { return popup_.visible(); }
    double scale() const { return popup_.scale(); }
    double opacity() const { return popup_.opacity(); }
    render::Point transform_origin() const { return pivot_; }
    std::optional<std::size_t> focused_item() const { return active_; }
    bool focus_shown() const { return focus_.focused(); }

    MenuResponse handle(const input::PointerEvent& event);
    MenuResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
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

    void set_active(std::optional<std::size_t> index);

    text::FontStack fonts_;
    std::vector<Item> items_;
    std::optional<std::size_t> active_;
    FocusIndicator focus_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
    render::Point pivot_{0.0, 0.0};
    PopupTransition popup_;
};

}
