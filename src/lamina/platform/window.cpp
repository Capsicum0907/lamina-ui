#include "lamina/platform/window.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#include "lamina/render/thorvg_canvas.hpp"

namespace lamina::platform {

namespace {

constexpr double nanoseconds_per_second = 1e9;
constexpr double max_frame_seconds = 0.1;

[[noreturn]] void fail(const char* what) {
    throw std::runtime_error(std::string(what) + ": " + SDL_GetError());
}

}

struct Window::Impl {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    int texture_w = 0;
    int texture_h = 0;
    render::ThorVGCanvas canvas{1, 1};
    Uint64 last_ns = 0;

    double pixel_density() const {
        int ww = 0;
        int wh = 0;
        int pw = 0;
        int ph = 0;
        SDL_GetWindowSize(window, &ww, &wh);
        SDL_GetWindowSizeInPixels(window, &pw, &ph);
        return ww > 0 ? static_cast<double>(pw) / ww : 1.0;
    }

    double scale() const {
        const float s = SDL_GetWindowDisplayScale(window);
        return s > 0.0f ? s : 1.0;
    }

    double to_dp(float window_coord) const {
        return window_coord * pixel_density() / scale();
    }

    void ensure_texture(int w, int h) {
        if (texture && w == texture_w && h == texture_h) {
            return;
        }
        if (texture) {
            SDL_DestroyTexture(texture);
        }
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!texture) {
            fail("SDL_CreateTexture");
        }
        texture_w = w;
        texture_h = h;
    }
};

Window::Window(const WindowOptions& options) : impl_(std::make_unique<Impl>()) {
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fail("SDL_Init");
    }
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
    if (!SDL_CreateWindowAndRenderer(options.title.c_str(), options.width_dp, options.height_dp, flags,
                                     &impl_->window, &impl_->renderer)) {
        fail("SDL_CreateWindowAndRenderer");
    }
    const double factor = impl_->scale() / impl_->pixel_density();
    SDL_SetWindowSize(impl_->window, static_cast<int>(std::lround(options.width_dp * factor)),
                      static_cast<int>(std::lround(options.height_dp * factor)));
    SDL_SetWindowPosition(impl_->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    if (!SDL_SetRenderVSync(impl_->renderer, 1)) {
        std::fprintf(stderr, "lamina: vsync is not available (%s); frames are not paced\n", SDL_GetError());
    }
    SDL_ShowWindow(impl_->window);
    impl_->last_ns = SDL_GetTicksNS();
}

Window::~Window() {
    if (impl_->texture) {
        SDL_DestroyTexture(impl_->texture);
    }
    if (impl_->renderer) {
        SDL_DestroyRenderer(impl_->renderer);
    }
    if (impl_->window) {
        SDL_DestroyWindow(impl_->window);
    }
    SDL_Quit();
}

bool Window::next_frame(Frame& frame, bool animating) {
    frame.pointer.clear();
    if (!animating) {
        SDL_WaitEvent(nullptr);
        impl_->last_ns = SDL_GetTicksNS();
    }
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            return false;
        case SDL_EVENT_MOUSE_MOTION:
            frame.pointer.push_back({PointerAction::move, impl_->to_dp(e.motion.x), impl_->to_dp(e.motion.y)});
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (e.button.button == SDL_BUTTON_LEFT) {
                frame.pointer.push_back({PointerAction::down, impl_->to_dp(e.button.x), impl_->to_dp(e.button.y)});
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.button.button == SDL_BUTTON_LEFT) {
                frame.pointer.push_back({PointerAction::up, impl_->to_dp(e.button.x), impl_->to_dp(e.button.y)});
            }
            break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            frame.pointer.push_back({PointerAction::leave, 0.0, 0.0});
            break;
        default:
            break;
        }
    }

    const Uint64 now = SDL_GetTicksNS();
    frame.dt_seconds = std::min(max_frame_seconds, static_cast<double>(now - impl_->last_ns) / nanoseconds_per_second);
    impl_->last_ns = now;

    int pw = 0;
    int ph = 0;
    SDL_GetWindowSizeInPixels(impl_->window, &pw, &ph);
    pw = std::max(pw, 1);
    ph = std::max(ph, 1);
    impl_->canvas.resize(pw, ph);
    frame.scale = impl_->scale();
    frame.width_dp = pw / frame.scale;
    frame.height_dp = ph / frame.scale;
    return true;
}

render::Canvas& Window::canvas() {
    return impl_->canvas;
}

void Window::present() {
    const render::PixelView view = impl_->canvas.finish();
    impl_->ensure_texture(view.width, view.height);
    SDL_UpdateTexture(impl_->texture, nullptr, view.data, static_cast<int>(view.stride));
    SDL_RenderClear(impl_->renderer);
    SDL_RenderTexture(impl_->renderer, impl_->texture, nullptr, nullptr);
    SDL_RenderPresent(impl_->renderer);
}

}
