#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/tokens/menu_bar.hpp"
#include "lamina/widgets/access_key.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/shortcuts.hpp"

namespace lamina::widgets {

struct MenuBarMenu {
    std::string label;
    std::vector<MenuItem> items;
    std::optional<input::Key> access_key = std::nullopt;
};

struct MenuBarOptions {
    std::vector<MenuBarMenu> menus;
    std::string accessible_name;
    tokens::Density density = {};
    std::optional<icons::IconFont> icon_font = std::nullopt;
    ShortcutRegistry* commands = nullptr;
    const tokens::ShortcutFormat* shortcut_format = nullptr;
};

struct MenuBarResponse {
    bool consumed = false;
    std::optional<std::size_t> menu = std::nullopt;
    std::vector<std::size_t> path = {};
    std::string command = {};
};

void apply_shortcuts(std::vector<MenuItem>& items, const ShortcutRegistry& commands);
void sync_commands(Menu& menu, const ShortcutRegistry& commands);

class MenuBar {
public:
    MenuBar(MenuBarOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const;
    void set_width(double width) {
        width_ = width;
        sized_ = true;
    }
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const { return {origin_.x, origin_.y, width_, height()}; }
    void set_window(layout::Size window) { window_ = window; }

    std::size_t count() const { return items_.size(); }
    render::Rect item_rect(std::size_t index) const;
    Menu& menu(std::size_t index) { return *menus_[index]; }
    const Menu& menu(std::size_t index) const { return *menus_[index]; }
    std::optional<std::size_t> open_menu() const { return open_; }
    bool active() const { return active_; }
    std::optional<std::size_t> highlighted() const { return highlight_; }
    bool access_keys_shown() const { return access_keys_fade_.target() > 0.0; }
    bool engaged() const { return active_ || open_.has_value(); }
    bool holds_pointer() const { return open_.has_value(); }

    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    tokens::Density density() const { return density_; }
    void set_density(tokens::Density density);
    void refresh_commands();
    AccessibleNode accessible() const;

    MenuBarResponse handle(const input::PointerEvent& event);
    MenuBarResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void deactivate();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_menus(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Item {
        text::TextLine label;
        std::optional<AccessKeyLabel> access;
        double x;
        double width;
        PressState press;
        motion::Animated state_layer;
        motion::Animated open;
    };

    void open(std::size_t index, input::InputSource source);
    void close_menu();
    void retarget();
    MenuBarResponse finish(std::size_t index, MenuResponse r);
    std::optional<std::size_t> item_at(double x, double y) const;
    std::optional<std::size_t> access_match(const input::KeyEvent& e) const;
    void reveal_access_keys(bool shown);
    void layout_items();

    text::FontStack fonts_;
    std::string accessible_name_;
    std::vector<std::string> names_;
    std::vector<Item> items_;
    std::vector<std::unique_ptr<Menu>> menus_;
    std::vector<std::optional<input::Key>> access_keys_;
    ShortcutRegistry* commands_;
    std::optional<std::size_t> open_;
    std::optional<std::size_t> highlight_;
    std::size_t ring_ = 0;
    bool active_ = false;
    bool drag_open_ = false;
    motion::Animated access_keys_fade_{0.0, current_style().motion.fast_effects()};
    FocusIndicator focus_;
    Enablement enabled_;
    tokens::Density density_{};
    double width_ = 0.0;
    bool sized_ = false;
    render::Point origin_{0.0, 0.0};
    layout::Size window_{0.0, 0.0};
    StyleStamp style_stamp_;
};

}
