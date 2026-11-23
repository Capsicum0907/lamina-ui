#include <exception>
#include <optional>

#include "common/canvas.hpp"
#include "common/options.hpp"
#include "components/gallery.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/frame_stats.hpp"
#include "lamina_sdl3/report.hpp"
#include "lamina_sdl3/system_fonts.hpp"
#include "res.hpp"

#ifdef __ANDROID__
#include <SDL3/SDL_main.h>
#endif

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
    void set_dark_theme(bool dark) override { window_.set_dark_system_bars(dark); }
    void system_back() override { window_.system_back(); }
    bool frame_stats() const override { return window_.frame_probe().switches().hud; }
    void set_frame_stats(bool on) override { window_.set_frame_stats({on, on}); }
    diagnostics::FrameProbe* frame_probe() override { return &window_.frame_probe(); }

private:
    sdl3::Window& window_;
};

}

int main(int argc, char** argv) {
    try {
        const bool want_title_bar = examples::take_flag(argc, argv, examples::title_bar_flag);
        const text::Typeface typeface = examples::take_typeface(argc, argv);
        const diagnostics::FrameStatsSwitches frame_stats = examples::take_frame_stats(argc, argv);
        const icons::IconFont icon_font = icons::IconFont::from_resource(examples::res::fonts::symbols);
        sdl3::Window window({"lamina-ui components", examples::components::window_width_dp,
                             examples::window_height_dp(examples::components::window_height_dp, want_title_bar), want_title_bar, frame_stats},
                            examples::window_canvas());
        WindowHost host(window);
        auto load = [](text::Typeface t) { return sdl3::typeface_stack(t, examples::res::embedded_fonts); };
        examples::components::GalleryStart start;
#ifdef __ANDROID__
        start.dark = window.system_dark_theme();
#endif
        std::optional<examples::components::Gallery> built;
        if (argc > 1) {
            built.emplace(sdl3::font_stack_from_arguments(argc, argv), icon_font, host, start);
        } else {
            built.emplace(examples::components::GalleryFonts{load, typeface}, icon_font, host, start);
        }
        examples::components::Gallery& gallery = *built;
        auto update = [&](const sdl3::Frame& frame) {
            const color::Argb background = gallery.update(frame);
            window.set_text_input(gallery.caret_area());
            window.set_cursor(gallery.cursor().value_or(input::Cursor::arrow));
            return background;
        };
        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            const bool animating = gallery.paint(canvas, frame);
            sdl3::paint_frame_stats(window, canvas, gallery.fonts(), gallery.scheme(), frame);
            if (const auto wake = gallery.next_wake_seconds()) {
                window.wake_after(*wake);
            }
            return animating;
        };
        sdl3::run_app(window, update, paint);
        return 0;
    } catch (const std::exception& ex) {
        sdl3::report_error(ex.what());
        return 1;
    }
}
