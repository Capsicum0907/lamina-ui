#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lamina/platform/input.hpp"
#include "lamina/render/canvas.hpp"

namespace lamina::platform {

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
    std::vector<PointerEvent> pointer;
    std::vector<KeyEvent> keys;
    std::vector<TextEvent> text;
};

class Window {
public:
    explicit Window(const WindowOptions& options);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool next_frame(Frame& frame, bool animating);
    void wake_after(double seconds);
    void set_text_input(const render::Rect* caret_area_dp);
    render::Canvas& canvas();
    void present();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
