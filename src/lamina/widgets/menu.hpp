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
#include "lamina/tokens/motion.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/tokens/menu.hpp"
#include "lamina/widgets/access_key.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/popup_transition.hpp"
#include "lamina/widgets/shortcuts.hpp"

namespace lamina::widgets {

struct MenuItem {
    std::string label;
    std::optional<icons::Icon> leading_icon = std::nullopt;
    std::optional<input::Key> access_key = std::nullopt;
    bool enabled = true;
    std::optional<KeyChord> shortcut = std::nullopt;
    std::string command = {};
    std::vector<MenuItem> submenu = {};
    bool divider_before = false;
};

struct MenuOptions {
    std::vector<MenuItem> items;
    std::string accessible_name = {};
    tokens::Density density = {};
    std::optional<icons::IconFont> icon_font = std::nullopt;
    const tokens::ShortcutFormat* shortcut_format = nullptr;
};

struct MenuResponse {
    bool consumed = false;
    std::optional<std::size_t> chosen = std::nullopt;
    std::vector<std::size_t> path = {};
    std::string command = {};
};

render::Point menu_position(const render::Rect& anchor, layout::Size window, layout::Size menu);
render::Point menu_position(const render::Rect& anchor, layout::Size window, layout::Size menu, double horizontal_margin,
                            double vertical_margin);
render::Point menu_transform_origin(const render::Rect& anchor, const render::Rect& menu);
render::Point context_menu_position(render::Point at, layout::Size window, layout::Size menu);
render::Point context_menu_position(render::Point at, layout::Size window, layout::Size menu, double margin);
render::Point submenu_position(const render::Rect& item, layout::Size window, layout::Size menu);

class Menu {
public:
    Menu(MenuOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const;
    double item_height() const;
    render::Rect bounds() const;
    render::Rect item_rect(std::size_t index) const;
    tokens::Density density() const { return density_; }
    void set_density(tokens::Density density);

    void open(const render::Rect& anchor, layout::Size window, input::InputSource source = input::InputSource::pointer);
    void open(const render::Rect& anchor, layout::Size window, input::InputSource source, double horizontal_margin, double vertical_margin);
    void open_at(render::Point at, layout::Size window, input::InputSource source = input::InputSource::pointer);
    void close();
    bool is_open() const { return popup_.is_open(); }
    bool visible() const { return popup_.visible(); }
    double scale() const { return popup_.scale(); }
    double opacity() const { return popup_.opacity(); }
    render::Point transform_origin() const { return pivot_; }
    std::optional<std::size_t> focused_item() const { return active_; }
    bool focus_shown() const { return focus_.focused(); }
    std::optional<input::Key> access_key(std::size_t index) const { return access_keys_[index]; }
    bool access_keys_shown() const { return access_keys_shown_; }
    std::size_t count() const { return items_.size(); }
    bool item_enabled(std::size_t index) const { return items_[index].enabled.enabled(); }
    void set_item_enabled(std::size_t index, bool enabled);
    const std::string& shortcut_label(std::size_t index) const { return shortcut_labels_[index]; }
    const std::string& command(std::size_t index) const { return commands_[index]; }
    bool has_submenu(std::size_t index) const { return children_[index] != nullptr; }
    Menu* submenu(std::size_t index) { return children_[index].get(); }
    const Menu* submenu(std::size_t index) const { return children_[index].get(); }
    std::optional<std::size_t> open_submenu() const { return open_child_; }
    bool contains_point(double x, double y) const;
    std::optional<std::size_t> item_at(double x, double y) const;
    MenuResponse activate_at(double x, double y);
    std::optional<AccessibleNode> accessible() const;

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
        std::optional<AccessKeyLabel> access;
        std::optional<text::TextLine> shortcut;
        PressState press;
        motion::Animated state_layer;
        Enablement enabled;
        bool divider_before = false;
    };

    void set_active(std::optional<std::size_t> index);
    void retarget();
    std::vector<std::optional<input::Key>> enabled_access_keys() const;
    void reveal_access_keys();
    void place(render::Point origin, const render::Rect& anchor, layout::Size window, input::InputSource source);
    double item_top(std::size_t index) const;
    double divider_height() const;
    void open_child(std::size_t index, input::InputSource source);
    void close_child();
    void hover(std::optional<std::size_t> index);
    MenuResponse chose(std::size_t index);
    MenuResponse from_child(std::size_t index, MenuResponse child);

    text::FontStack fonts_;
    std::string accessible_name_;
    std::vector<std::string> names_;
    std::vector<std::string> commands_;
    std::vector<std::string> shortcut_labels_;
    std::vector<Item> items_;
    std::vector<std::unique_ptr<Menu>> children_;
    std::vector<std::optional<input::Key>> access_keys_;
    std::optional<icons::IconFont> icon_font_;
    bool access_keys_shown_ = false;
    motion::Animated access_keys_fade_{0.0, tokens::motion_scheme.fast_effects()};
    std::optional<std::size_t> active_;
    std::optional<std::size_t> open_child_;
    std::optional<std::size_t> hover_target_;
    double hover_timer_ = -1.0;
    FocusIndicator focus_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
    layout::Size window_{0.0, 0.0};
    tokens::Density density_{};
    render::Point pivot_{0.0, 0.0};
    PopupTransition popup_;
};

}
