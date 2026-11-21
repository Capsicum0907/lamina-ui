#pragma once

#include <limits>
#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/numeric_field.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/text_field.hpp"

namespace lamina::widgets {

enum class NumericLabelPosition { start, inside };

struct NumericFieldOptions {
    std::string label = {};
    double value = 0.0;
    double min = -std::numeric_limits<double>::infinity();
    double max = std::numeric_limits<double>::infinity();
    double step = 1.0;
    double large_step = 10.0;
    int decimals = 0;
    std::string unit = {};
    NumericLabelPosition label_position = NumericLabelPosition::start;
    std::optional<double> width = std::nullopt;
    std::optional<double> label_width = std::nullopt;
    bool enabled = true;
    std::string accessible_name = {};
    input::Clipboard* clipboard = nullptr;
    tokens::Density density = {};
};

struct NumericFieldResponse {
    bool changed = false;
    bool committed = false;
    bool consumed = false;
};

class NumericField {
public:
    NumericField(NumericFieldOptions options, const text::FontStack& fonts);

    double width() const;
    double height() const;
    layout::Size measure(const layout::Constraints&) const { return {width(), height()}; }
    void arrange(const render::Rect& r);
    void set_origin(render::Point origin);
    render::Rect bounds() const { return {origin_.x, origin_.y, width(), height()}; }
    render::Rect field_rect() const;
    render::Rect label_rect() const;

    double value() const { return value_; }
    void set_value(double value);
    std::string formatted() const;
    std::string text() const { return editor_.text(); }
    bool editing() const { return editor_.focused(); }
    bool scrubbing() const { return scrubbing_; }
    bool error() const { return error_; }
    std::optional<input::Cursor> cursor() const;
    tokens::Density density() const { return options_.density; }
    void set_density(tokens::Density density);

    bool focused() const { return editor_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    render::Rect caret_area() const { return editor_.caret_area(); }
    double next_blink_seconds() const { return editor_.next_blink_seconds(); }
    AccessibleNode accessible() const;

    NumericFieldResponse handle(const input::PointerEvent& event);
    NumericFieldResponse handle(const input::KeyEvent& event);
    void handle(const input::TextEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double clamp(double v) const;
    double rounded(double v) const;
    std::string format(double v) const;
    std::optional<double> parse(const std::string& s) const;
    bool commit();
    void revert();
    void show_value();
    void place_editor();
    double label_space() const;
    double unit_space() const;
    bool change_to(double v);

    NumericFieldOptions options_;
    text::FontStack fonts_;
    text::TextLine label_;
    text::TextLine unit_;
    TextField editor_;
    double value_ = 0.0;
    render::Point origin_{0.0, 0.0};
    bool error_ = false;
    bool hovered_ = false;
    bool label_hovered_ = false;
    bool pressed_ = false;
    bool scrubbing_ = false;
    double press_x_ = 0.0;
    double press_value_ = 0.0;
    Enablement enabled_;
    motion::Animated hover_;
    motion::Animated focus_;
    motion::Animated error_amount_;
};

}
