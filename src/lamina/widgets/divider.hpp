#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/divider.hpp"
#include "lamina/widgets/accessibility.hpp"

namespace lamina::widgets {

enum class DividerOrientation { horizontal, vertical };

enum class DividerInset { none, start, end, both };

struct DividerOptions {
    DividerOrientation orientation = DividerOrientation::horizontal;
    DividerInset inset = DividerInset::none;
    double thickness = tokens::divider_tokens.thickness;
    color::Role color = tokens::divider_tokens.color;
};

class Divider {
public:
    explicit Divider(DividerOptions options = {}) : options_(options) {}

    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& r) { rect_ = r; }
    render::Rect bounds() const { return rect_; }
    render::Rect line() const;
    const DividerOptions& options() const { return options_; }
    void set_inset(DividerInset inset) { options_.inset = inset; }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    AccessibleNode accessible() const { return {AccessibleRole::separator, {}, line()}; }

private:
    DividerOptions options_;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
};

}
