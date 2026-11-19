#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/input/clipboard.hpp"
#include "lamina/input/events.hpp"
#include "lamina/render/canvas.hpp"

namespace lamina::sdl3 {

struct WindowOptions {
    std::string title;
    int width_dp;
    int height_dp;
};

struct Frame {
    double dt_seconds;
    double width_dp;
    double height_dp;
    double scale;
    std::vector<input::PointerEvent> pointer;
    std::vector<input::WheelEvent> wheel;
    std::vector<input::KeyEvent> keys;
    std::vector<input::TextEvent> text;
};

class Window {
public:
    Window(const WindowOptions& options, std::unique_ptr<render::Canvas> canvas);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool next_frame(Frame& frame, bool animating);
    void on_live_resize(std::function<void(const Frame&)> redraw);
    void wake_after(double seconds);
    void set_text_input(const std::optional<render::Rect>& caret_area_dp);
    input::Clipboard& clipboard();
    render::Canvas& canvas();
    void present();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
