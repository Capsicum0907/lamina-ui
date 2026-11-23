#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina/input/caption.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/layout/safe_area.hpp"
#include "lamina/layout/window_size_class.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/raster.hpp"

namespace lamina::sdl3 {

struct WindowOptions {
    std::string title;
    int width_dp;
    int height_dp;
    bool client_title_bar = false;
    diagnostics::FrameStatsSwitches frame_stats = {};
    bool gpu_images = true;
    std::size_t image_texture_bytes = tokens::raster.image_texture_bytes;
};

struct Frame {
    double dt_seconds;
    double width_dp;
    double height_dp;
    double scale;
    bool maximized;
    bool active;
    std::vector<input::PointerEvent> pointer;
    std::vector<input::WheelEvent> wheel;
    std::vector<input::KeyEvent> keys;
    std::vector<input::TextEvent> text;
    layout::WindowSizeClass size_class = {};
    std::vector<input::ContextClick> context = {};
    layout::Insets safe_area = {};
    layout::Insets gesture_insets = {};
    std::vector<input::FingerEvent> fingers = {};
};

bool client_title_bar_supported();

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
    void set_cursor(input::Cursor cursor);
    input::Clipboard& clipboard();
    render::Canvas& canvas();
    void present();
    diagnostics::FrameProbe& frame_probe();
    void set_frame_stats(diagnostics::FrameStatsSwitches switches);
    void set_gpu_images(bool on);
    bool gpu_images() const;
    void set_image_texture_budget(std::size_t bytes);

    bool client_title_bar() const;
    void set_caption_regions(input::CaptionRegions regions);
    void minimize();
    void toggle_maximize();
    void set_size(double width_dp, double height_dp);
    void set_minimum_size(double width_dp, double height_dp);
    void set_always_on_top(bool on_top);
    void set_dark_system_bars(bool dark);
    void set_system_bars_hidden(bool hidden);
    void system_back();
    bool system_dark_theme() const;
    void* native_handle() const;
    void close();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
