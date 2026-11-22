#pragma once

#include <algorithm>
#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/focus.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/scroll_area.hpp"

namespace lamina::examples::components {

template <typename W>
class SideScroll {
public:
    SideScroll(W& content, std::string name) : content_(content), name_(std::move(name)) {}

    double width() const { return area_.viewport().width; }
    double height() const { return natural_.height; }
    void set_origin(render::Point origin) { arrange({origin.x, origin.y, width(), height()}); }
    render::Rect bounds() const { return area_.viewport(); }
    const widgets::ScrollArea& scroll() const { return area_; }
    widgets::ScrollArea& scroll() { return area_; }

    layout::Size measure(const layout::Constraints& c) {
        natural_ = content_.measure({0.0, layout::unbounded, 0.0, c.max_height});
        return {std::min(natural_.width, c.max_width), natural_.height};
    }

    void arrange(const render::Rect& r) {
        area_.set_viewport({r.x, r.y, std::min(r.width, natural_.width), natural_.height});
        area_.set_content_width(natural_.width);
        area_.set_content_height(natural_.height);
        content_.arrange({r.x - shift(), r.y, natural_.width, natural_.height});
    }

    auto handle(const input::PointerEvent& e) {
        using Response = decltype(content_.handle(e));
        if (area_.handle(e)) {
            Response r = content_.handle(input::PointerEvent{input::PointerAction::leave, 0.0, 0.0});
            r.consumed = true;
            return r;
        }
        const bool inside = e.action != input::PointerAction::leave && widgets::contains(area_.viewport(), e.x_dp, e.y_dp);
        if (e.action == input::PointerAction::down) {
            held_ = inside;
        }
        if (!inside && !held_) {
            return content_.handle(input::PointerEvent{input::PointerAction::leave, 0.0, 0.0});
        }
        Response r = content_.handle(e);
        if (e.action == input::PointerAction::up || e.action == input::PointerAction::leave) {
            held_ = false;
        }
        return r;
    }
    auto handle(const input::KeyEvent& e) { return content_.handle(e); }
    void handle(const input::TextEvent& e) { content_.handle(e); }
    bool handle(const input::WheelEvent& e) {
        if (!e.shift && e.dx == 0.0) {
            return false;
        }
        return area_.handle(e);
    }

    void cancel_interaction() {
        held_ = false;
        area_.cancel_interaction();
        content_.cancel_interaction();
    }
    void set_enabled(bool enabled) { content_.set_enabled(enabled); }

    void step(double dt) {
        content_.step(dt);
        if (const std::optional<render::Rect> f = focused_part()) {
            const double left = f->x - area_.viewport().x + shift();
            if (left != revealed_) {
                revealed_ = left;
                area_.ensure_visible_x(left, left + f->width);
            }
        } else {
            revealed_.reset();
        }
        area_.step(dt);
    }
    bool animating() const { return content_.animating() || area_.animating(); }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
        const double ring = tokens::focus_indicator.outer_offset + tokens::focus_indicator.thickness;
        const render::Rect v = area_.viewport();
        const double left = shift() > 0.0 ? 0.0 : ring;
        const double right = shift() < area_.max_offset_x() ? 0.0 : ring;
        canvas.push_clip(render::Rect{v.x - left, v.y - ring, v.width + left + right, v.height + 2.0 * ring});
        content_.paint(canvas, scheme);
        canvas.pop_clip();
        area_.paint(canvas, scheme);
    }

    widgets::AccessibleNode accessible() const {
        widgets::AccessibleNode node{widgets::AccessibleRole::group, name_, area_.viewport()};
        node.children.push_back(content_.accessible());
        return node;
    }

private:
    double shift() const { return std::min(area_.offset_x(), area_.max_offset_x()); }

    std::optional<render::Rect> focused_part() const {
        if (!content_.focused()) {
            return std::nullopt;
        }
        std::optional<render::Rect> found;
        auto visit = [&found](const widgets::AccessibleNode& n, const auto& self) -> void {
            if (n.state.focused) {
                found = n.bounds;
            }
            for (const widgets::AccessibleNode& c : n.children) {
                self(c, self);
            }
        };
        visit(content_.accessible(), visit);
        return found;
    }

    W& content_;
    std::string name_;
    widgets::ScrollArea area_;
    layout::Size natural_{0.0, 0.0};
    bool held_ = false;
    std::optional<double> revealed_;
};

}
