#include "lamina_sdl3/caption_frame.hpp"

#include <cstdint>
#include <cstdio>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>

namespace lamina::sdl3::detail {

namespace {

constexpr UINT_PTR subclass_id = 1;
constexpr DWM_WINDOW_CORNER_PREFERENCE corner_preference = DWMWCP_ROUND;

class Win32CaptionFrame final : public CaptionFrame {
public:
    Win32CaptionFrame(SDL_Window* window, HWND hwnd, Uint32 pointer_event)
        : window_(window), hwnd_(hwnd), pointer_event_(pointer_event) {
        SetWindowSubclass(hwnd_, &Win32CaptionFrame::proc, subclass_id, reinterpret_cast<DWORD_PTR>(this));
        const DWM_WINDOW_CORNER_PREFERENCE corners = corner_preference;
        DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof corners);
    }

    ~Win32CaptionFrame() override { RemoveWindowSubclass(hwnd_, &Win32CaptionFrame::proc, subclass_id); }

    Win32CaptionFrame(const Win32CaptionFrame&) = delete;
    Win32CaptionFrame& operator=(const Win32CaptionFrame&) = delete;

    void set_regions(input::CaptionRegions regions) override { regions_ = std::move(regions); }

private:
    static LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) {
        auto* self = reinterpret_cast<Win32CaptionFrame*>(data);
        switch (msg) {
        case WM_NCHITTEST:
            return self->hit(lp);
        case WM_NCMOUSEMOVE:
            if (wp == HTMAXBUTTON) {
                self->track_leave();
                self->push_at(input::PointerAction::move, lp);
            } else if (self->over_maximize_) {
                self->over_maximize_ = false;
                self->push(input::PointerAction::leave, 0, 0);
            }
            break;
        case WM_NCMOUSELEAVE:
            if (self->over_maximize_) {
                self->over_maximize_ = false;
                self->push(input::PointerAction::leave, 0, 0);
            }
            break;
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONDBLCLK:
            if (wp == HTMAXBUTTON) {
                self->push_at(input::PointerAction::down, lp);
                return 0;
            }
            break;
        case WM_NCLBUTTONUP:
            if (wp == HTMAXBUTTON) {
                self->push_at(input::PointerAction::up, lp);
                return 0;
            }
            break;
        case WM_NCRBUTTONUP:
            if (wp == HTCAPTION) {
                self->show_system_menu(lp);
                return 0;
            }
            break;
        case WM_SYSCOMMAND:
            if ((wp & 0xFFF0) == SC_KEYMENU && lp == VK_SPACE) {
                return DefWindowProcW(hwnd, msg, wp, lp);
            }
            break;
        default:
            break;
        }
        return DefSubclassProc(hwnd, msg, wp, lp);
    }

    LRESULT hit(LPARAM lp) const {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd_, &pt);
        const SDL_WindowFlags flags = SDL_GetWindowFlags(window_);
        if ((flags & SDL_WINDOW_FULLSCREEN) != 0) {
            return HTCLIENT;
        }
        if ((flags & SDL_WINDOW_MAXIMIZED) == 0) {
            if (const LRESULT edge = resize_edge(pt); edge != HTNOWHERE) {
                return edge;
            }
        }
        const float scale = SDL_GetWindowDisplayScale(window_);
        const double k = scale > 0.0f ? scale : 1.0;
        switch (input::hit_test(regions_, {pt.x / k, pt.y / k})) {
        case input::CaptionHit::drag:
            return HTCAPTION;
        case input::CaptionHit::maximize:
            return HTMAXBUTTON;
        case input::CaptionHit::client:
        case input::CaptionHit::minimize:
        case input::CaptionHit::close:
            break;
        }
        return HTCLIENT;
    }

    LRESULT resize_edge(POINT pt) const {
        RECT rc;
        GetClientRect(hwnd_, &rc);
        const UINT dpi = GetDpiForWindow(hwnd_);
        const int padded = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        const int bx = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + padded;
        const int by = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + padded;
        const bool left = pt.x < bx;
        const bool right = pt.x >= rc.right - bx;
        const bool top = pt.y < by;
        const bool bottom = pt.y >= rc.bottom - by;
        if (top && left) {
            return HTTOPLEFT;
        }
        if (top && right) {
            return HTTOPRIGHT;
        }
        if (bottom && left) {
            return HTBOTTOMLEFT;
        }
        if (bottom && right) {
            return HTBOTTOMRIGHT;
        }
        if (left) {
            return HTLEFT;
        }
        if (right) {
            return HTRIGHT;
        }
        if (top) {
            return HTTOP;
        }
        if (bottom) {
            return HTBOTTOM;
        }
        return HTNOWHERE;
    }

    void track_leave() {
        if (over_maximize_) {
            return;
        }
        over_maximize_ = true;
        TRACKMOUSEEVENT tme{sizeof(TRACKMOUSEEVENT), TME_LEAVE | TME_NONCLIENT, hwnd_, 0};
        TrackMouseEvent(&tme);
    }

    void show_system_menu(LPARAM lp) const {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd_, &pt);
        const float density = SDL_GetWindowPixelDensity(window_);
        const float k = density > 0.0f ? density : 1.0f;
        SDL_ShowWindowSystemMenu(window_, static_cast<int>(static_cast<float>(pt.x) / k), static_cast<int>(static_cast<float>(pt.y) / k));
    }

    void push_at(input::PointerAction action, LPARAM lp) {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd_, &pt);
        push(action, pt.x, pt.y);
    }

    void push(input::PointerAction action, LONG x, LONG y) {
        SDL_Event e{};
        e.type = pointer_event_;
        e.user.windowID = SDL_GetWindowID(window_);
        e.user.code = static_cast<Sint32>(action);
        e.user.data1 = reinterpret_cast<void*>(static_cast<std::intptr_t>(x));
        e.user.data2 = reinterpret_cast<void*>(static_cast<std::intptr_t>(y));
        if (!SDL_PushEvent(&e)) {
            std::fprintf(stderr, "lamina: dropped a caption pointer event (%s)\n", SDL_GetError());
        }
    }

    SDL_Window* window_;
    HWND hwnd_;
    Uint32 pointer_event_;
    input::CaptionRegions regions_;
    bool over_maximize_ = false;
};

}

bool caption_frame_supported() {
    return true;
}

std::unique_ptr<CaptionFrame> attach_caption_frame(SDL_Window* window, Uint32 pointer_event) {
    auto* hwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    if (!hwnd) {
        std::fprintf(stderr, "lamina: no HWND for the window; the client-drawn title bar has no native behaviour\n");
        return nullptr;
    }
    return std::make_unique<Win32CaptionFrame>(window, hwnd, pointer_event);
}

}
