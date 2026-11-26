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
#include "lamina/text/font.hpp"
#include "lamina/tokens/split_button.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/menu.hpp"

namespace lamina::widgets {

struct SplitButtonOptions {
    std::string label;
    std::optional<icons::Icon> icon = std::nullopt;
    tokens::ButtonStyle style = tokens::ButtonStyle::filled;
    tokens::ButtonSize size = tokens::ButtonSize::small;
    std::string trailing_name = {};
    std::vector<MenuItem> menu = {};
    bool enabled = true;
    std::string accessible_name = {};
    std::string expanded_state = "Expanded";
    std::string collapsed_state = "Collapsed";
};

struct SplitButtonResponse {
    bool leading_clicked = false;
    bool trailing_toggled = false;
    std::optional<std::size_t> chosen = std::nullopt;
    bool consumed = false;
};

class SplitButton {
public:
    SplitButton(SplitButtonOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font);

    double width() const;
    double height() const;
    double natural_width() const;
    double min_width() const;
    void set_width(double width);
    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& r);
    void set_origin(render::Point origin);
    render::Rect bounds() const { return {origin_.x, origin_.y, width(), height()}; }
    void set_window(layout::Size window) { window_ = window; }

    Button& leading() { return *leading_; }
    const Button& leading() const { return *leading_; }
    Button& trailing() { return *trailing_; }
    const Button& trailing() const { return *trailing_; }
    bool checked() const { return trailing_->checked(); }
    void set_checked(bool checked);
    double chevron_rotation() const { return rotation_.value(); }
    const Menu* menu() const { return menu_.get(); }
    bool menu_open() const { return menu_ && menu_->is_open(); }

    bool enabled() const { return leading_->enabled(); }
    void set_enabled(bool enabled);
    AccessibleNode accessible() const;

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    SplitButtonResponse handle(const input::PointerEvent& event);
    SplitButtonResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_overlay(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void sync_menu(input::InputSource source);
    ButtonShapes leading_shapes() const;
    ButtonShapes trailing_shapes() const;

    SplitButtonOptions options_;
    std::unique_ptr<Button> leading_;
    std::unique_ptr<Button> trailing_;
    TargetArbiter arbiter_;
    std::unique_ptr<Menu> menu_;
    motion::Animated rotation_;
    icons::IconFont icon_font_;
    render::Point origin_{0.0, 0.0};
    layout::Size window_{0.0, 0.0};
    std::optional<double> given_width_;
    std::optional<render::Rect> arranged_;
    StyleStamp style_stamp_;
};

}
