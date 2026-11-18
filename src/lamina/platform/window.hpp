#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lamina/render/canvas.hpp"

namespace lamina::platform {

struct WindowOptions {
    std::string title;
    int width_dp;
    int height_dp;
};

enum class PointerAction { move, down, up, leave };

struct PointerEvent {
    PointerAction action;
    double x_dp;
    double y_dp;
};

struct Frame {
    double dt_seconds;
    double width_dp;
    double height_dp;
    double scale;
    std::vector<PointerEvent> pointer;
};

class Window {
public:
    explicit Window(const WindowOptions& options);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool next_frame(Frame& frame, bool animating);
    render::Canvas& canvas();
    void present();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
