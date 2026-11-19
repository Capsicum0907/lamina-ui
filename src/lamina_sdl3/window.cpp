#include "lamina_sdl3/window.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <stdexcept>

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#include "lamina_thorvg/canvas.hpp"

namespace lamina::sdl3 {

using input::Clipboard;
using input::Key;
using input::PointerAction;
using input::TextEventKind;

namespace {

constexpr double nanoseconds_per_second = 1e9;
constexpr double max_frame_seconds = 0.1;
constexpr double milliseconds_per_second = 1000.0;

std::optional<Key> to_key(SDL_Keycode k) {
    switch (k) {
    case SDLK_BACKSPACE: return Key::backspace;
    case SDLK_DELETE: return Key::delete_forward;
    case SDLK_LEFT: return Key::left;
    case SDLK_RIGHT: return Key::right;
    case SDLK_UP: return Key::up;
    case SDLK_DOWN: return Key::down;
    case SDLK_HOME: return Key::home;
    case SDLK_END: return Key::end;
    case SDLK_RETURN:
    case SDLK_KP_ENTER: return Key::enter;
    case SDLK_ESCAPE: return Key::escape;
    case SDLK_TAB: return Key::tab;
    default: break;
    }
    if (k >= SDLK_A && k <= SDLK_Z) {
        return static_cast<Key>(static_cast<SDL_Keycode>(Key::a) + (k - SDLK_A));
    }
    return std::nullopt;
}

bool ctrl_held(SDL_Keymod mod) {
    return (mod & SDL_KMOD_CTRL) != 0 && (mod & SDL_KMOD_ALT) == 0;
}

class SdlClipboard final : public Clipboard {
public:
    std::string text() override {
        char* raw = SDL_GetClipboardText();
        std::string out = raw ? raw : "";
        SDL_free(raw);
        return out;
    }

    void set_text(std::string_view utf8) override {
        SDL_SetClipboardText(std::string(utf8).c_str());
    }
};

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
    thorvg::Canvas canvas{1, 1};
    Uint64 last_ns = 0;
    double wake_seconds = -1.0;
    bool text_input = false;
    SdlClipboard clipboard;

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
    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");
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
    frame.keys.clear();
    frame.text.clear();
    if (!animating) {
        if (impl_->wake_seconds >= 0.0) {
            SDL_WaitEventTimeout(nullptr, static_cast<Sint32>(std::ceil(impl_->wake_seconds * milliseconds_per_second)));
        } else {
            SDL_WaitEvent(nullptr);
        }
        impl_->last_ns = SDL_GetTicksNS();
    }
    impl_->wake_seconds = -1.0;
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
                frame.pointer.push_back({PointerAction::down, impl_->to_dp(e.button.x), impl_->to_dp(e.button.y), e.button.clicks});
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.button.button == SDL_BUTTON_LEFT) {
                frame.pointer.push_back({PointerAction::up, impl_->to_dp(e.button.x), impl_->to_dp(e.button.y)});
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            if (const auto key = to_key(e.key.key)) {
                frame.keys.push_back({*key, (e.key.mod & SDL_KMOD_SHIFT) != 0, ctrl_held(e.key.mod)});
            }
            break;
        case SDL_EVENT_TEXT_INPUT:
            frame.text.push_back({TextEventKind::commit, e.text.text, 0});
            break;
        case SDL_EVENT_TEXT_EDITING:
            frame.text.push_back({TextEventKind::compose, e.edit.text ? e.edit.text : "", e.edit.start});
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

void Window::wake_after(double seconds) {
    if (impl_->wake_seconds < 0.0 || seconds < impl_->wake_seconds) {
        impl_->wake_seconds = std::max(0.0, seconds);
    }
}

void Window::set_text_input(const render::Rect* area) {
    if (!area) {
        if (impl_->text_input) {
            SDL_StopTextInput(impl_->window);
            impl_->text_input = false;
        }
        return;
    }
    const double k = impl_->scale() / impl_->pixel_density();
    const SDL_Rect rect{static_cast<int>(std::lround(area->x * k)), static_cast<int>(std::lround(area->y * k)),
                        std::max(1, static_cast<int>(std::lround(area->width * k))),
                        std::max(1, static_cast<int>(std::lround(area->height * k)))};
    SDL_SetTextInputArea(impl_->window, &rect, 0);
    if (!impl_->text_input) {
        SDL_StartTextInput(impl_->window);
        impl_->text_input = true;
    }
}

Clipboard& Window::clipboard() {
    return impl_->clipboard;
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
