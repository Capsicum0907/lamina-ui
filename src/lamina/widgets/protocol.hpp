#pragma once

#include <concepts>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/render/geometry.hpp"

namespace lamina::widgets {

template <typename W>
concept Paintable = requires(const W& w, render::Canvas& canvas, const color::ColorScheme& scheme) { w.paint(canvas, scheme); };

template <typename W>
concept Steppable = requires(W& w, const W& cw, double dt_seconds) {
    w.step(dt_seconds);
    { cw.animating() } -> std::convertible_to<bool>;
};

template <typename W>
concept HandlesPointer = requires(W& w, const input::PointerEvent& e) { w.handle(e); };

template <typename W>
concept HandlesKeys = requires(W& w, const input::KeyEvent& e) { w.handle(e); };

template <typename W>
concept HandlesText = requires(W& w, const input::TextEvent& e) { w.handle(e); };

template <typename W>
concept Cancellable = requires(W& w) { w.cancel_interaction(); };

template <typename W>
concept Focusable = requires(W& w, const W& cw, bool focused) {
    { cw.focused() } -> std::convertible_to<bool>;
    w.set_focused(focused);
};

template <typename W>
concept FocusGated = Focusable<W> && requires(const W& w) {
    { w.can_focus() } -> std::convertible_to<bool>;
};

template <typename W>
concept Sized = requires(W& w, const W& cw, render::Point origin) {
    { cw.width() } -> std::convertible_to<double>;
    { cw.height() } -> std::convertible_to<double>;
    w.set_origin(origin);
};

template <typename W>
concept WidthSettable = Sized<W> && requires(W& w, double width) { w.set_width(width); };

template <typename W>
concept SizeSettable = Sized<W> && requires(W& w, double width, double height) { w.set_size(width, height); };

template <typename W>
concept Measurable = requires(W& w, const layout::Constraints& c, const render::Rect& r) {
    { w.measure(c) } -> std::convertible_to<layout::Size>;
    w.arrange(r);
};

template <typename W>
concept Placeable = Measurable<W> || Sized<W>;

}
