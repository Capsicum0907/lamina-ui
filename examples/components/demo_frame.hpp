#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina/widgets/content_slot.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/list_item.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::components {

struct DemoFrameStyle {
    tokens::CornerSize corner = tokens::CornerSize::large;
    color::Role background = color::Role::surface;
    color::Role border = color::Role::outline_variant;
    double border_width = 1.0;
    double padding = 16.0;
    double gap = 12.0;
    tokens::TypeRole title_type = tokens::TypeRole::title_large;
    tokens::TypeRole body_type = tokens::TypeRole::body_medium;
    color::Role title = color::Role::on_surface;
    color::Role body = color::Role::on_surface_variant;
};

inline const DemoFrameStyle demo_frame_style;

class DemoFrame {
public:
    explicit DemoFrame(layout::Size size) : size_(size) {}

    layout::NodePtr node(layout::NodePtr content) {
        content_ = std::move(content);
        return std::make_shared<layout::Leaf>([this](const layout::Constraints&) { return size_; },
                                              [this](const render::Rect& r) {
                                                  rect_ = r;
                                                  if (content_) {
                                                      content_->measure(layout::Constraints::tight({r.width, r.height}));
                                                      content_->arrange(r);
                                                  }
                                                  for (const auto& f : listeners_) {
                                                      f(r);
                                                  }
                                              });
    }

    void on_arrange(std::function<void(const render::Rect&)> f) { listeners_.push_back(std::move(f)); }
    void set_size(layout::Size size) { size_ = size; }

    const render::Rect& rect() const { return rect_; }
    double radius() const { return current_style().shape.radius(demo_frame_style.corner, rect_.width, rect_.height); }

    void paint_background(render::Canvas& canvas, const color::ColorScheme& scheme) const {
        canvas.fill_path(render::rounded_rect_path(rect_, render::uniform_radii(radius())), scheme[demo_frame_style.background]);
    }
    void begin_clip(render::Canvas& canvas) const {
        canvas.save();
        canvas.push_clip(render::rounded_rect_path(rect_, render::uniform_radii(radius())));
    }
    void end_clip(render::Canvas& canvas) const {
        canvas.pop_clip();
        canvas.restore();
    }
    void paint_border(render::Canvas& canvas, const color::ColorScheme& scheme) const {
        canvas.fill_path(render::ring_path(rect_, radius(), demo_frame_style.border_width), scheme[demo_frame_style.border],
                         render::FillRule::even_odd);
    }

private:
    layout::Size size_;
    layout::NodePtr content_;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
    std::vector<std::function<void(const render::Rect&)>> listeners_;
};

struct SheetBody {
    widgets::WidgetSet widgets;
    widgets::FocusManager focus;
    std::vector<widgets::ListItem*> items;
    layout::NodePtr node;

    widgets::ContentSlot slot(bool own_focus = true) { return {node, &widgets, own_focus ? &focus : nullptr}; }
};

inline std::unique_ptr<SheetBody> list_body(std::vector<widgets::ListItemSpec> specs, const text::FontStack& fonts) {
    auto body = std::make_unique<SheetBody>();
    std::vector<layout::NodePtr> nodes;
    for (widgets::ListItemSpec& s : specs) {
        s.transparent = true;
        auto& item = body->widgets.emplace<widgets::ListItem>(std::move(s), fonts);
        body->focus.add(item);
        body->items.push_back(&item);
        nodes.push_back(widgets::node(item));
    }
    auto c = layout::column(std::move(nodes));
    c->fill_cross(true);
    body->node = c;
    return body;
}

}
