#include <cstdio>
#include <exception>

#include "common/canvas.hpp"
#include "common/options.hpp"
#include "components/gallery.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/system_fonts.hpp"
#include "res.hpp"

using namespace lamina;

namespace {

class WindowHost final : public examples::components::GalleryHost {
public:
    explicit WindowHost(sdl3::Window& window) : window_(window) {}
    input::Clipboard* clipboard() override { return &window_.clipboard(); }
    bool client_title_bar() const override { return window_.client_title_bar(); }
    void set_caption_regions(input::CaptionRegions regions) override { window_.set_caption_regions(std::move(regions)); }
    void minimize() override { window_.minimize(); }
    void toggle_maximize() override { window_.toggle_maximize(); }
    void close() override { window_.close(); }

private:
    sdl3::Window& window_;
};

}

int main(int argc, char** argv) {
    try {
        const bool want_title_bar = examples::take_flag(argc, argv, examples::title_bar_flag);
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const icons::IconFont icon_font = icons::IconFont::from_resource(examples::res::fonts::symbols);
        sdl3::Window window({"lamina-ui components", examples::components::window_width_dp,
                             examples::window_height_dp(examples::components::window_height_dp, want_title_bar), want_title_bar},
                            examples::window_canvas());
        WindowHost host(window);
        examples::components::Gallery gallery(fonts, icon_font, host);
        auto update = [&](const sdl3::Frame& frame) {
            const color::Argb background = gallery.update(frame);
            window.set_text_input(gallery.caret_area());
            return background;
        };
        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            const bool animating = gallery.paint(canvas, frame);
            if (const auto wake = gallery.next_wake_seconds()) {
                window.wake_after(*wake);
            }
            return animating;
        };
        sdl3::run_app(window, update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
