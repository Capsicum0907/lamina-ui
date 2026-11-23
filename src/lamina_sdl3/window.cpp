#include "lamina_sdl3/window.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#include "lamina/input/touch_echo.hpp"
#include "lamina/input/touch_release.hpp"
#include "lamina/layout/frame_size.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina_sdl3/caption_frame.hpp"
#include "lamina_sdl3/report.hpp"
#include "lamina_sdl3/system_ui.hpp"

namespace lamina::sdl3 {

using input::Clipboard;
using input::Key;
using input::PointerAction;
using input::TextEventKind;

namespace {

input::PointerKind pointer_kind(SDL_MouseID which) {
    if (which == SDL_TOUCH_MOUSEID) {
        return input::PointerKind::touch;
    }
    if (which == SDL_PEN_MOUSEID) {
        return input::PointerKind::pen;
    }
    return input::PointerKind::mouse;
}

std::optional<input::FingerAction> finger_action(Uint32 type) {
    switch (type) {
    case SDL_EVENT_FINGER_DOWN: return input::FingerAction::down;
    case SDL_EVENT_FINGER_MOTION: return input::FingerAction::move;
    case SDL_EVENT_FINGER_UP: return input::FingerAction::up;
    case SDL_EVENT_FINGER_CANCELED: return input::FingerAction::cancel;
    default: return std::nullopt;
    }
}

constexpr double nanoseconds_per_second = 1e9;
constexpr double max_frame_seconds = 0.1;
constexpr double milliseconds_per_second = 1000.0;
constexpr std::array<SDL_SystemCursor, 7> system_cursors{SDL_SYSTEM_CURSOR_DEFAULT,   SDL_SYSTEM_CURSOR_TEXT,      SDL_SYSTEM_CURSOR_POINTER,
                                                       SDL_SYSTEM_CURSOR_EW_RESIZE, SDL_SYSTEM_CURSOR_NS_RESIZE, SDL_SYSTEM_CURSOR_MOVE,
                                                       SDL_SYSTEM_CURSOR_NOT_ALLOWED};
constexpr std::size_t cursor_count = system_cursors.size();
constexpr const char* unavailable_title_bar_message = "lamina: the client-drawn title bar is not available; using the system frame";
#ifdef __ANDROID__
constexpr int redraws_after_restore = 2;
#else
constexpr int redraws_after_restore = 0;
#endif

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
    case SDLK_SPACE: return Key::space;
    case SDLK_PAGEUP: return Key::page_up;
    case SDLK_PAGEDOWN: return Key::page_down;
    case SDLK_KP_0: return Key::digit_0;
    case SDLK_KP_1: return Key::digit_1;
    case SDLK_KP_2: return Key::digit_2;
    case SDLK_KP_3: return Key::digit_3;
    case SDLK_KP_4: return Key::digit_4;
    case SDLK_KP_5: return Key::digit_5;
    case SDLK_KP_6: return Key::digit_6;
    case SDLK_KP_7: return Key::digit_7;
    case SDLK_KP_8: return Key::digit_8;
    case SDLK_KP_9: return Key::digit_9;
    case SDLK_KP_PLUS: return Key::plus;
    case SDLK_KP_MINUS: return Key::minus;
    case SDLK_KP_MULTIPLY: return Key::asterisk;
    case SDLK_KP_DIVIDE: return Key::slash;
    case SDLK_KP_PERIOD: return Key::period;
    case SDLK_KP_PERCENT: return Key::percent;
    case SDLK_KP_EQUALS: return Key::equals;
    case SDLK_KP_LEFTPAREN: return Key::paren_left;
    case SDLK_KP_RIGHTPAREN: return Key::paren_right;
    case SDLK_F1: return Key::f1;
    case SDLK_F2: return Key::f2;
    case SDLK_F3: return Key::f3;
    case SDLK_F4: return Key::f4;
    case SDLK_F5: return Key::f5;
    case SDLK_F6: return Key::f6;
    case SDLK_F7: return Key::f7;
    case SDLK_F8: return Key::f8;
    case SDLK_F9: return Key::f9;
    case SDLK_F10: return Key::f10;
    case SDLK_F11: return Key::f11;
    case SDLK_F12: return Key::f12;
    case SDLK_INSERT: return Key::insert;
    case SDLK_APPLICATION: return Key::menu;
#ifdef __ANDROID__
    case SDLK_AC_BACK: return Key::escape;
#endif
    default: break;
    }
    if (k >= SDLK_A && k <= SDLK_Z) {
        return static_cast<Key>(static_cast<SDL_Keycode>(Key::a) + (k - SDLK_A));
    }
    return std::nullopt;
}

std::optional<Key> key_of(const SDL_KeyboardEvent& e) {
    if (const auto key = to_key(e.key)) {
        return key;
    }
    return input::key_for_character(static_cast<char32_t>(SDL_GetKeyFromScancode(e.scancode, e.mod & SDL_KMOD_SHIFT, false)));
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
    std::unique_ptr<render::Canvas> canvas;
    Uint64 last_ns = 0;
    double wake_seconds = -1.0;
    double frame_limit_seconds = max_frame_seconds;
    bool text_input = false;
    SdlClipboard clipboard;
    std::function<void(const Frame&)> live_redraw;
    bool in_live_redraw = false;
    std::exception_ptr live_error;
    Uint32 caption_pointer_event = 0;
    std::unique_ptr<detail::CaptionFrame> caption;
    bool close_requested = false;
    std::array<SDL_Cursor*, cursor_count> cursors{};
    input::Cursor cursor = input::Cursor::arrow;
    bool alt_alone = false;
    int pending_redraws = 0;
    layout::PixelSize last_pixels;
    input::TouchEchoFilter touch_echo;
    input::TouchReleaseGate touch_release;
    int origin_x = 0;
    int origin_y = 0;
    diagnostics::FrameProbe probe;

    void read_display() {
        if (!probe.active()) {
            return;
        }
        diagnostics::DisplayInfo d;
        const char* name = SDL_GetRendererName(renderer);
        d.renderer = name ? name : "";
        int vsync = 0;
        if (SDL_GetRenderVSync(renderer, &vsync)) {
            d.vsync = vsync;
        }
        if (const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window))) {
            d.refresh_hz = mode->refresh_rate;
        }
        d.window_width = last_pixels.width;
        d.window_height = last_pixels.height;
        d.canvas_width = canvas->width();
        d.canvas_height = canvas->height();
        probe.set_display(std::move(d));
    }

    void log_frames(bool going_idle) {
        if (const auto summary = probe.take_log(going_idle)) {
            const std::string_view title = going_idle ? "gesture" : "running";
            report_info(probe.tokens().log_tag, diagnostics::report_line(diagnostics::report_rows(*summary, probe.display(), title, probe.tokens())));
        }
    }

    void measure(Frame& frame) {
        const Uint64 now = SDL_GetTicksNS();
        frame.dt_seconds = std::min(frame_limit_seconds, static_cast<double>(now - last_ns) / nanoseconds_per_second);
        frame_limit_seconds = max_frame_seconds;
        last_ns = now;
        layout::PixelSize reported;
        SDL_GetWindowSizeInPixels(window, &reported.width, &reported.height);
        const SDL_WindowFlags flags = SDL_GetWindowFlags(window);
        last_pixels = layout::frame_pixel_size(reported, last_pixels, (flags & SDL_WINDOW_MINIMIZED) != 0);
        canvas->resize(last_pixels.width, last_pixels.height);
        frame.scale = scale();
        frame.width_dp = last_pixels.width / frame.scale;
        frame.height_dp = last_pixels.height / frame.scale;
        frame.size_class = layout::compute_window_size_class(frame.width_dp, frame.height_dp);
        if (const auto system = detail::system_insets_pixels()) {
            frame.safe_area = layout::insets_in_dp(system->content, 1.0 / frame.scale);
            frame.gesture_insets = layout::insets_in_dp(system->gestures, 1.0 / frame.scale);
        } else {
            frame.safe_area = safe_area();
            frame.gesture_insets = {};
        }
        frame.maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
        frame.active = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    }

    static bool SDLCALL watch(void* userdata, SDL_Event* e) {
        auto* self = static_cast<Impl*>(userdata);
        if (e->type != SDL_EVENT_WINDOW_EXPOSED || e->window.data1 != 1 || e->window.windowID != SDL_GetWindowID(self->window) ||
            !self->live_redraw || self->in_live_redraw || self->live_error) {
            return true;
        }
        self->in_live_redraw = true;
        try {
            Frame frame{};
            self->probe.begin_frame(std::numeric_limits<double>::infinity());
            self->measure(frame);
            self->live_redraw(frame);
        } catch (...) {
            self->live_error = std::current_exception();
        }
        self->in_live_redraw = false;
        return true;
    }

    double pixel_density() const {
        int ww = 0;
        int wh = 0;
        int pw = 0;
        int ph = 0;
        SDL_GetWindowSize(window, &ww, &wh);
        SDL_GetWindowSizeInPixels(window, &pw, &ph);
        return ww > 0 ? static_cast<double>(pw) / ww : 1.0;
    }

    layout::Insets safe_area() const {
        int ww = 0;
        int wh = 0;
        SDL_GetWindowSize(window, &ww, &wh);
        SDL_Rect safe{};
        if (!SDL_GetWindowSafeArea(window, &safe)) {
            return {};
        }
        return layout::safe_area_insets({0.0, 0.0, static_cast<double>(ww), static_cast<double>(wh)},
                                        {static_cast<double>(safe.x), static_cast<double>(safe.y), static_cast<double>(safe.w),
                                         static_cast<double>(safe.h)},
                                        pixel_density() / scale());
    }

    double scale() const {
        const float s = SDL_GetWindowDisplayScale(window);
        return s > 0.0f ? s : 1.0;
    }

    int to_window(double dp) const {
        return std::max(1, static_cast<int>(std::lround(dp * scale() / pixel_density())));
    }

    double to_dp(float window_coord) const {
        return window_coord * pixel_density() / scale();
    }

    void admit_pointer(Frame& frame, const input::PointerEvent& event) {
        const input::ScreenOrigin origin{to_dp(static_cast<float>(origin_x)), to_dp(static_cast<float>(origin_y))};
        if (touch_echo.admit(event, origin, tokens::touch_echo_tolerance_pixels / scale())) {
            frame.pointer.push_back(event);
        }
    }

    void admit_all(Frame& frame, const std::vector<input::PointerEvent>& events) {
        for (const input::PointerEvent& event : events) {
            admit_pointer(frame, event);
        }
    }

    void push_pointer(Frame& frame, const input::PointerEvent& event) {
        admit_all(frame, touch_release.pointer(event));
    }

    void push_finger(Frame& frame, const SDL_TouchFingerEvent& e, input::FingerAction action) {
        if (e.touchID == SDL_MOUSE_TOUCHID || e.touchID == SDL_PEN_TOUCHID) {
            return;
        }
        int ww = 0;
        int wh = 0;
        SDL_GetWindowSize(window, &ww, &wh);
        const input::FingerEvent finger{action, static_cast<std::uint64_t>(e.fingerID), to_dp(e.x * static_cast<float>(ww)),
                                        to_dp(e.y * static_cast<float>(wh)), static_cast<double>(e.timestamp) / nanoseconds_per_second};
        frame.fingers.push_back(finger);
        admit_all(frame, touch_release.finger(finger));
    }

    void drop_texture() {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
        texture = nullptr;
        texture_w = 0;
        texture_h = 0;
    }

    void ensure_texture(int w, int h) {
        if (texture && w == texture_w && h == texture_h) {
            return;
        }
        drop_texture();
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!texture) {
            fail("SDL_CreateTexture");
        }
        texture_w = w;
        texture_h = h;
    }
};

bool client_title_bar_supported() {
    return detail::caption_frame_supported();
}

Window::Window(const WindowOptions& options, std::unique_ptr<render::Canvas> canvas) : impl_(std::make_unique<Impl>()) {
    if (!canvas) {
        throw std::invalid_argument("lamina::sdl3::Window needs a canvas");
    }
    impl_->canvas = std::move(canvas);
    impl_->canvas->set_probe(&impl_->probe);
    impl_->probe.set_switches(options.frame_stats);
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fail("SDL_Init");
    }
    const bool client_title_bar = options.client_title_bar && client_title_bar_supported();
    if (options.client_title_bar && !client_title_bar) {
        report_error(unavailable_title_bar_message);
    }
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
    if (client_title_bar) {
        flags |= SDL_WINDOW_BORDERLESS;
    }
    if (!SDL_CreateWindowAndRenderer(options.title.c_str(), options.width_dp, options.height_dp, flags,
                                     &impl_->window, &impl_->renderer)) {
        fail("SDL_CreateWindowAndRenderer");
    }
    if (client_title_bar) {
        impl_->caption_pointer_event = SDL_RegisterEvents(1);
        if (impl_->caption_pointer_event != 0) {
            impl_->caption = detail::attach_caption_frame(impl_->window, impl_->caption_pointer_event);
        }
        if (!impl_->caption) {
            report_error(unavailable_title_bar_message);
            SDL_SetWindowBordered(impl_->window, true);
        }
    }
    const double factor = impl_->scale() / impl_->pixel_density();
    SDL_SetWindowSize(impl_->window, static_cast<int>(std::lround(options.width_dp * factor)),
                      static_cast<int>(std::lround(options.height_dp * factor)));
    SDL_SetWindowPosition(impl_->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    if (!SDL_SetRenderVSync(impl_->renderer, 1)) {
        report_error(std::string("lamina: vsync is not available (") + SDL_GetError() + "); frames are not paced");
    }
    SDL_ShowWindow(impl_->window);
    SDL_GetWindowPosition(impl_->window, &impl_->origin_x, &impl_->origin_y);
    impl_->last_ns = SDL_GetTicksNS();
}

Window::~Window() {
    impl_->caption.reset();
    for (SDL_Cursor* c : impl_->cursors) {
        if (c) {
            SDL_DestroyCursor(c);
        }
    }
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

void Window::on_live_resize(std::function<void(const Frame&)> redraw) {
    const bool had = static_cast<bool>(impl_->live_redraw);
    impl_->live_redraw = std::move(redraw);
    if (impl_->live_redraw && !had) {
        SDL_AddEventWatch(&Impl::watch, impl_.get());
    } else if (!impl_->live_redraw && had) {
        SDL_RemoveEventWatch(&Impl::watch, impl_.get());
    }
}

bool Window::next_frame(Frame& frame, bool animating) {
    if (impl_->live_error) {
        std::rethrow_exception(std::exchange(impl_->live_error, nullptr));
    }
    if (impl_->close_requested) {
        return false;
    }
    frame.pointer.clear();
    frame.wheel.clear();
    frame.keys.clear();
    frame.text.clear();
    frame.context.clear();
    frame.fingers.clear();
    const bool redraw = impl_->pending_redraws > 0;
    impl_->pending_redraws = std::max(0, impl_->pending_redraws - 1);
    double waited = 0.0;
    if (!animating && !redraw) {
        const Uint64 wait_start = SDL_GetTicksNS();
        if (impl_->probe.switches().log) {
            const double idle = impl_->probe.idle_wait_seconds();
            const double first = impl_->wake_seconds >= 0.0 ? std::min(idle, impl_->wake_seconds) : idle;
            if (!SDL_WaitEventTimeout(nullptr, static_cast<Sint32>(std::ceil(first * milliseconds_per_second)))) {
                impl_->log_frames(true);
            }
        }
        const double slept = static_cast<double>(SDL_GetTicksNS() - wait_start) / nanoseconds_per_second;
        if (impl_->wake_seconds >= 0.0) {
            if (slept < impl_->wake_seconds) {
                SDL_WaitEventTimeout(nullptr, static_cast<Sint32>(std::ceil((impl_->wake_seconds - slept) * milliseconds_per_second)));
            }
            impl_->frame_limit_seconds = std::max(max_frame_seconds, impl_->wake_seconds);
        } else {
            SDL_WaitEvent(nullptr);
            impl_->last_ns = SDL_GetTicksNS();
        }
        waited = static_cast<double>(SDL_GetTicksNS() - wait_start) / nanoseconds_per_second;
    }
    impl_->probe.begin_frame(waited);
    impl_->wake_seconds = -1.0;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (impl_->caption_pointer_event != 0 && e.type == impl_->caption_pointer_event) {
            const double k = impl_->scale();
            frame.pointer.push_back({static_cast<PointerAction>(e.user.code),
                                     static_cast<double>(reinterpret_cast<std::intptr_t>(e.user.data1)) / k,
                                     static_cast<double>(reinterpret_cast<std::intptr_t>(e.user.data2)) / k, 1});
            continue;
        }
        switch (e.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            return false;
        case SDL_EVENT_MOUSE_MOTION:
            impl_->push_pointer(frame, {PointerAction::move, impl_->to_dp(e.motion.x), impl_->to_dp(e.motion.y), 0, pointer_kind(e.motion.which),
                                        static_cast<double>(e.motion.timestamp) / nanoseconds_per_second});
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            impl_->alt_alone = false;
            if (e.button.button == SDL_BUTTON_LEFT) {
                const SDL_Keymod mod = SDL_GetModState();
                impl_->push_pointer(frame, {PointerAction::down, impl_->to_dp(e.button.x), impl_->to_dp(e.button.y), e.button.clicks,
                                            pointer_kind(e.button.which), static_cast<double>(e.button.timestamp) / nanoseconds_per_second,
                                            (mod & SDL_KMOD_SHIFT) != 0, ctrl_held(mod)});
            } else if (e.button.button == SDL_BUTTON_RIGHT) {
                frame.context.push_back({impl_->to_dp(e.button.x), impl_->to_dp(e.button.y)});
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.button.button == SDL_BUTTON_LEFT) {
                const SDL_Keymod mod = SDL_GetModState();
                impl_->push_pointer(frame, {PointerAction::up, impl_->to_dp(e.button.x), impl_->to_dp(e.button.y), 0, pointer_kind(e.button.which),
                                            static_cast<double>(e.button.timestamp) / nanoseconds_per_second, (mod & SDL_KMOD_SHIFT) != 0,
                                            ctrl_held(mod)});
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL: {
            const float flip = e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
            const SDL_Keymod mod = SDL_GetModState();
            frame.wheel.push_back({impl_->to_dp(e.wheel.mouse_x), impl_->to_dp(e.wheel.mouse_y), flip * e.wheel.x, -flip * e.wheel.y,
                                   (mod & SDL_KMOD_SHIFT) != 0, ctrl_held(mod)});
            break;
        }
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED:
            impl_->push_finger(frame, e.tfinger, *finger_action(e.type));
            break;
        case SDL_EVENT_KEY_DOWN:
            if (e.key.key == SDLK_LALT || e.key.key == SDLK_RALT) {
                impl_->alt_alone = e.key.repeat ? impl_->alt_alone : (e.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_GUI)) == 0;
            } else {
                impl_->alt_alone = false;
            }
            if (const auto key = key_of(e.key)) {
                frame.keys.push_back({*key, (e.key.mod & SDL_KMOD_SHIFT) != 0, ctrl_held(e.key.mod), e.key.repeat, (e.key.mod & SDL_KMOD_ALT) != 0,
                                      detail::is_system_back(e.key.key)});
            }
            break;
        case SDL_EVENT_KEY_UP:
            if ((e.key.key == SDLK_LALT || e.key.key == SDLK_RALT) && impl_->alt_alone) {
                frame.keys.push_back({Key::alt, false});
            }
            impl_->alt_alone = false;
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            impl_->alt_alone = false;
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
        case SDL_EVENT_WINDOW_MOVED:
            impl_->origin_x = e.window.data1;
            impl_->origin_y = e.window.data2;
            break;
        case SDL_EVENT_WINDOW_RESTORED:
            impl_->pending_redraws = redraws_after_restore;
            break;
        case SDL_EVENT_RENDER_DEVICE_RESET:
            impl_->drop_texture();
            break;
        default:
            break;
        }
    }

    impl_->admit_all(frame, impl_->touch_release.end_frame());
    if (impl_->touch_release.holding()) {
        impl_->pending_redraws = std::max(impl_->pending_redraws, 1);
    }
    impl_->measure(frame);
    impl_->read_display();
    return true;
}

bool Window::client_title_bar() const {
    return static_cast<bool>(impl_->caption);
}

void Window::set_caption_regions(input::CaptionRegions regions) {
    if (impl_->caption) {
        impl_->caption->set_regions(std::move(regions));
    }
}

void Window::minimize() {
    SDL_MinimizeWindow(impl_->window);
}

void Window::toggle_maximize() {
    if ((SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_MAXIMIZED) != 0) {
        SDL_RestoreWindow(impl_->window);
    } else {
        SDL_MaximizeWindow(impl_->window);
    }
}

void Window::set_size(double width_dp, double height_dp) {
    if ((SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_MAXIMIZED) != 0) {
        SDL_RestoreWindow(impl_->window);
    }
    SDL_SetWindowSize(impl_->window, impl_->to_window(width_dp), impl_->to_window(height_dp));
}

void Window::set_minimum_size(double width_dp, double height_dp) {
    SDL_SetWindowMinimumSize(impl_->window, impl_->to_window(width_dp), impl_->to_window(height_dp));
}

void Window::set_always_on_top(bool on_top) {
    if (!SDL_SetWindowAlwaysOnTop(impl_->window, on_top)) {
        report_error(std::string("lamina: cannot change always on top (") + SDL_GetError() + ")");
    }
}

void Window::set_dark_system_bars(bool dark) {
    detail::set_dark_system_bars(dark);
}

void Window::system_back() {
    detail::system_back();
}

bool Window::system_dark_theme() const {
    return SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK;
}

void* Window::native_handle() const {
    return SDL_GetPointerProperty(SDL_GetWindowProperties(impl_->window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
}

void Window::close() {
    impl_->close_requested = true;
}

void Window::wake_after(double seconds) {
    if (impl_->wake_seconds < 0.0 || seconds < impl_->wake_seconds) {
        impl_->wake_seconds = std::max(0.0, seconds);
    }
}

void Window::set_cursor(input::Cursor cursor) {
    if (cursor == impl_->cursor) {
        return;
    }
    impl_->cursor = cursor;
    const auto i = static_cast<std::size_t>(cursor);
    if (i >= cursor_count) {
        return;
    }
    if (!impl_->cursors[i]) {
        impl_->cursors[i] = SDL_CreateSystemCursor(system_cursors[i]);
    }
    if (impl_->cursors[i]) {
        SDL_SetCursor(impl_->cursors[i]);
    }
}

void Window::set_text_input(const std::optional<render::Rect>& area) {
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
    return *impl_->canvas;
}

void Window::present() {
    const render::PixelView view = impl_->canvas->finish();
    {
        const diagnostics::StageScope stage(&impl_->probe, diagnostics::Stage::upload);
        impl_->ensure_texture(view.width, view.height);
        SDL_UpdateTexture(impl_->texture, nullptr, view.data, static_cast<int>(view.stride));
    }
    {
        const diagnostics::StageScope stage(&impl_->probe, diagnostics::Stage::present);
        SDL_RenderClear(impl_->renderer);
        SDL_RenderTexture(impl_->renderer, impl_->texture, nullptr, nullptr);
        SDL_RenderPresent(impl_->renderer);
    }
    impl_->probe.end_frame();
    impl_->log_frames(false);
}

diagnostics::FrameProbe& Window::frame_probe() {
    return impl_->probe;
}

void Window::set_frame_stats(diagnostics::FrameStatsSwitches switches) {
    impl_->probe.set_switches(switches);
    impl_->read_display();
}

}
