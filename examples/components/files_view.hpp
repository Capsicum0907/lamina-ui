#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "components/pages.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/focus_manager.hpp"

namespace lamina::examples::components {

class FilesView {
public:
    explicit FilesView(const PageContext& context);
    ~FilesView();
    FilesView(const FilesView&) = delete;
    FilesView& operator=(const FilesView&) = delete;

    layout::NodePtr node();
    widgets::FocusManager& focus();
    bool handle(const input::PointerEvent& event);
    bool holds_pointer() const;
    bool handle(const input::KeyEvent& event);
    bool handle(const input::WheelEvent& event);
    bool handle(const input::ContextClick& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::vector<widgets::AccessibleNode> accessible_nodes() const;
    void set_enabled(bool enabled);
    void cancel_interaction();
    void set_window(layout::Size window);
    std::optional<input::Cursor> cursor() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
