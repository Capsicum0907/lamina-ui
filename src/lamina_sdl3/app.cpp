#include "lamina_sdl3/app.hpp"

#include <utility>

namespace lamina::sdl3 {

void run_app(Window& window, const UpdateFrame& update, const PaintFrame& paint) {
    Frame frame;
    bool animating = true;
    while (window.next_frame(frame, animating)) {
        const color::Argb background = update(frame);
        render::Canvas& canvas = window.canvas();
        canvas.clear(background);
        canvas.save();
        canvas.scale(frame.scale);
        animating = paint(canvas, frame);
        canvas.restore();
        window.present();
    }
}

void run_app(const WindowOptions& options, std::unique_ptr<render::Canvas> canvas, const UpdateFrame& update,
             const PaintFrame& paint) {
    Window window(options, std::move(canvas));
    run_app(window, update, paint);
}

}
