#include "lamina_sdl3/app.hpp"

#include <utility>

namespace lamina::sdl3 {

void run_app(Window& window, const UpdateFrame& update, const PaintFrame& paint) {
    Frame frame;
    bool animating = true;
    diagnostics::FrameProbe* probe = &window.frame_probe();
    auto draw = [&](const Frame& f) {
        color::Argb background = 0;
        {
            const diagnostics::StageScope stage(probe, diagnostics::Stage::update);
            background = update(f);
        }
        render::Canvas& canvas = window.canvas();
        {
            const diagnostics::StageScope stage(probe, diagnostics::Stage::paint);
            canvas.clear(background);
            canvas.save();
            canvas.scale(f.scale);
            animating = paint(canvas, f);
            canvas.restore();
        }
        window.present();
    };
    window.on_live_resize(draw);
    try {
        while (window.next_frame(frame, animating)) {
            draw(frame);
        }
    } catch (...) {
        window.on_live_resize({});
        throw;
    }
    window.on_live_resize({});
}

void run_app(const WindowOptions& options, std::unique_ptr<render::Canvas> canvas, const UpdateFrame& update,
             const PaintFrame& paint) {
    Window window(options, std::move(canvas));
    run_app(window, update, paint);
}

}
