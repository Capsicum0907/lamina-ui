#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/dialog.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/popup_transition.hpp"

namespace lamina::widgets {

struct DialogOptions {
    std::string headline;
    std::string supporting_text;
    std::string confirm_label;
    std::string dismiss_label;
    std::optional<icons::Icon> icon = std::nullopt;
};

enum class DialogResult { none, confirmed, dismissed };

color::ColorScheme faded(const color::ColorScheme& scheme, double opacity);

class Dialog {
public:
    Dialog(DialogOptions options, const text::FontStack& fonts);
    Dialog(const Dialog&) = delete;
    Dialog& operator=(const Dialog&) = delete;

    void open(layout::Size window);
    void close();
    void set_window(layout::Size window);
    bool is_open() const { return popup_.is_open(); }
    bool visible() const { return popup_.visible(); }
    double opacity() const { return popup_.opacity(); }
    double scale() const { return popup_.scale(); }
    render::Rect bounds() const { return surface_; }
    render::Rect confirm_bounds() const { return confirm_.bounds(); }
    std::optional<render::Rect> dismiss_bounds() const;
    std::optional<render::Rect> icon_bounds() const;
    render::Rect headline_bounds() const { return headline_.bounds(); }
    std::size_t button_lines() const { return actions_->line_count(); }

    DialogResult handle(const input::PointerEvent& event);
    DialogResult handle(const input::KeyEvent& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    Label headline_;
    Label supporting_;
    Button confirm_;
    std::optional<Button> dismiss_;
    std::optional<icons::Icon> icon_;
    render::Point icon_origin_{0.0, 0.0};
    std::shared_ptr<layout::Flow> actions_;
    layout::NodePtr root_;
    layout::Size window_{0.0, 0.0};
    render::Rect surface_{0.0, 0.0, 0.0, 0.0};
    PopupTransition popup_;
};

}
