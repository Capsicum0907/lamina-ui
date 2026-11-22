#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/dialog.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/popup_transition.hpp"

namespace lamina::widgets {

class DialogPane {
public:
    virtual ~DialogPane() = default;
    virtual double width() const = 0;
    virtual double height() const = 0;
    virtual void set_origin(render::Point origin) = 0;
    virtual void add_focus_stops(FocusManager& focus) = 0;
    virtual bool handle(const input::PointerEvent& event) = 0;
    virtual bool handle(const input::KeyEvent& event) = 0;
    virtual void handle(const input::TextEvent& event) = 0;
    virtual bool handle(const input::WheelEvent& event) = 0;
    virtual void cancel_interaction() = 0;
    virtual void clear_focus() = 0;
    virtual void step(double dt_seconds) = 0;
    virtual bool animating() const = 0;
    virtual void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const = 0;
    virtual AccessibleNode accessible() const = 0;
    virtual std::optional<render::Rect> caret_area() const = 0;
    virtual std::optional<double> next_wake_seconds() const = 0;
};

template <typename W>
class PaneOf final : public DialogPane {
public:
    explicit PaneOf(W& widget) : w_(widget) {}
    double width() const override { return w_.width(); }
    double height() const override { return w_.height(); }
    void set_origin(render::Point origin) override { w_.set_origin(origin); }
    void add_focus_stops(FocusManager& focus) override { w_.add_focus_stops(focus); }
    bool handle(const input::PointerEvent& e) override { return w_.handle(e).consumed; }
    bool handle(const input::KeyEvent& e) override { return w_.handle(e).consumed; }
    void handle(const input::TextEvent& e) override { w_.handle(e); }
    bool handle(const input::WheelEvent& e) override { return w_.handle(e); }
    void cancel_interaction() override { w_.cancel_interaction(); }
    void clear_focus() override { w_.clear_focus(); }
    void step(double dt) override { w_.step(dt); }
    bool animating() const override { return w_.animating(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override { w_.paint(canvas, scheme); }
    AccessibleNode accessible() const override { return w_.accessible(); }
    std::optional<render::Rect> caret_area() const override { return w_.caret_area(); }
    std::optional<double> next_wake_seconds() const override { return w_.next_wake_seconds(); }

private:
    W& w_;
};

enum class PickerDialogLayout { date, time };

struct PickerDialogLeading {
    icons::Icon icon;
    std::string accessible_name;
    std::function<void(IconButton&)> on_click;
};

struct PickerDialogOptions {
    std::string confirm_label;
    std::string dismiss_label;
    PickerDialogLayout layout = PickerDialogLayout::date;
    tokens::CornerSize corner = tokens::CornerSize::extra_large;
    std::string title = {};
    std::optional<PickerDialogLeading> leading = std::nullopt;
    std::string accessible_name = {};
};

class PickerDialog {
public:
    PickerDialog(PickerDialogOptions options, std::unique_ptr<DialogPane> pane, const text::FontStack& fonts);
    PickerDialog(const PickerDialog&) = delete;
    PickerDialog& operator=(const PickerDialog&) = delete;

    void open_in(const render::Rect& area, input::InputSource source = input::InputSource::pointer);
    void close();
    void set_area(const render::Rect& area);
    bool is_open() const { return popup_.is_open(); }
    bool visible() const { return popup_.visible(); }
    bool holds_pointer() const { return is_open(); }
    render::Rect bounds() const { return surface_; }
    render::Rect confirm_bounds() const { return confirm_.bounds(); }
    render::Rect dismiss_bounds() const { return dismiss_.bounds(); }
    IconButton* leading() { return leading_ ? leading_.get() : nullptr; }
    void set_title(std::string title);
    void set_labels(std::string confirm, std::string dismiss);

    DialogResult handle(const input::PointerEvent& event);
    DialogResult handle(const input::KeyEvent& event);
    bool handle(const input::TextEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    std::optional<render::Rect> caret_area() const;
    std::optional<double> next_wake_seconds() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const;

private:
    void place();
    double padding() const;
    double title_height() const;
    double actions_height() const;
    DialogResult finish(DialogResult result);

    PickerDialogOptions options_;
    std::unique_ptr<DialogPane> pane_;
    text::FontStack fonts_;
    Button confirm_;
    Button dismiss_;
    std::unique_ptr<IconButton> leading_;
    FocusManager focus_;
    PopupTransition popup_;
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    render::Rect surface_{0.0, 0.0, 0.0, 0.0};
};

}
