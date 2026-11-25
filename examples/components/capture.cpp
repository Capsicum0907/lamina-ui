#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "common/theme.hpp"
#include "common/options.hpp"
#include "components/gallery.hpp"
#include "lamina/calendar/date.hpp"
#include "lamina/calendar/locale.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/layout/window_size_class.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/scroll_area.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina_sdl3/system_fonts.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "res.hpp"
#include "lamina/style/style.hpp"

using namespace lamina;
using input::Key;
using input::PointerAction;

namespace {

struct CaptureStyle {
    double fps = 60.0;
    double settle_seconds = 1.5;
    double still_seconds = 0.9;
    double cursor_size = 18.0;
    double key_badge_seconds = 0.8;
    double key_badge_margin = 16.0;
    double key_badge_padding = 8.0;
    color::Argb cursor_fill = 0xff202020;
    color::Argb cursor_outline = 0xffffffff;
    color::Argb badge_fill = 0xd0202020;
    color::Argb badge_text = 0xffffffff;
    tokens::TypeRole badge_type = tokens::TypeRole::label_large;
    color::Argb stage = 0xff808080;
    double sweep_below_breakpoint = 40.0;
    double sweep_narrowest = 360.0;
    double sweep_step_seconds = 1.6;
    double sweep_back_seconds = 3.0;
    int reveal_notches = 40;
    double reveal_pause_seconds = 0.05;
    double reveal_margin = 16.0;
    double pull_start = 40.0;
    double pull_short = 90.0;
    double pull_long = 300.0;
    double pull_seconds = 0.8;
    double pull_hold = 0.3;
    int scroll_notches = 6;
    std::string_view page_tabs = "Pages";
    double overlap_tolerance = 0.5;
    std::array<std::pair<std::string_view, std::string_view>, 0> known_overflows{};
    std::string_view side_scroll_target = "Horizontal time picker";
    int side_focus_tabs = 8;
    std::string_view redraw_target = "Send";
    std::string_view back_menu = "Menu";
    std::string_view back_field = "Name";
    std::string_view back_dialog = "Dialog";
    std::string_view back_sheet_scene = "sheets_modal";
    int back_press_limit = 4;
    int redraw_frame_limit = 600;
    int redraw_tolerance = 2;
    int fling_redraw_flings = 6;
    double fling_redraw_from = 0.8;
    double fling_redraw_to = 0.3;
    double fling_redraw_seconds = 0.08;
    std::array<std::string_view, 16> page_scenes{"default", "chips",  "lists",    "bars",     "inputs",   "dates",    "times",  "sheets",
                                                 "drawers", "search", "groups",   "progress", "toolbars", "adaptive", "desktop", "viewer"};
    std::string_view viewer_target = "Pattern";
    double finger_tap_seconds = 0.05;
    double double_tap_gap_seconds = 0.12;
    double pinch_from = 0.25;
    double pinch_to = 0.45;
    double pinch_seconds = 0.5;
    double pan_fraction = 0.3;
    double pan_seconds = 0.12;
    double swipe_from = 0.75;
    double swipe_to = 0.3;
    double swipe_height = 0.75;
    double swipe_seconds = 0.2;
    double swipe_hold_seconds = 0.3;
    double page_scroll_from = 0.8;
    double page_scroll_to = 0.2;
    double page_scroll_seconds = 0.6;
    std::string_view photos_target = "Photos";
    double bench_x = 0.9;
    double bench_from = 0.85;
    double bench_to = 0.25;
    double bench_refresh_hz = 120.0;
};

inline constexpr std::string_view window_option = "--window";
inline constexpr std::string_view verify_repaint_flag = "--verify-repaint";

struct WindowSize {
    double width = examples::components::window_width_dp;
    double height = examples::components::window_height_dp;
};

WindowSize take_window(int& argc, char** argv) {
    const std::optional<std::string> value = examples::take_option(argc, argv, window_option);
    if (!value) {
        return {};
    }
    const std::size_t x = value->find('x');
    if (x == std::string::npos) {
        throw std::invalid_argument(std::string(window_option) + " " + *value + ": expected <width>x<height> in dp");
    }
    return {std::stod(value->substr(0, x)), std::stod(value->substr(x + 1))};
}

class MemoryClipboard final : public input::Clipboard {
public:
    std::string text() override { return text_; }
    void set_text(std::string_view text) override { text_ = std::string(text); }

private:
    std::string text_;
};

class HeadlessHost final : public examples::components::GalleryHost {
public:
    input::Clipboard* clipboard() override { return &clipboard_; }
    void set_dark_theme(bool dark) override { dark_theme_ = dark; }
    std::optional<bool> dark_theme() const { return dark_theme_; }
    void system_back() override { ++system_backs_; }
    int system_backs() const { return system_backs_; }

private:
    MemoryClipboard clipboard_;
    std::optional<bool> dark_theme_;
    int system_backs_ = 0;
};

class Driver {
public:
    Driver(examples::components::Gallery& gallery, const text::FontStack& fonts, double scale, std::FILE* out, WindowSize window)
        : gallery_(gallery),
          fonts_(fonts),
          scale_(scale),
          out_(out),
          stage_(window),
          window_(window),
          canvas_(static_cast<int>(window.width * scale), static_cast<int>(window.height * scale)) {}

    double stage_width() const { return stage_.width; }
    double fps() const { return style_.fps; }
    const CaptureStyle& style() const { return style_; }
    double window_width() const { return window_.width; }
    double window_height() const { return window_.height; }
    std::optional<render::Rect> find(std::string_view name) const { return gallery_.find(name); }
    void set_window_width(double width) { window_.width = std::clamp(width, 1.0, stage_.width); }

    void set_recording(bool on) { recording_ = on; }
    void set_probe(diagnostics::FrameProbe* probe) {
        probe_ = probe;
        canvas_.set_probe(probe);
    }
    void set_cursor(bool on) { cursor_ = on; }
    void set_full_repaint(bool on) { canvas_.set_damage_tracking(!on); }
    void set_verify(bool on) { verify_ = on; }
    std::size_t verified_frames() const { return verified_frames_; }
    std::uint64_t outside_pixels() const { return outside_pixels_; }
    render::Point pointer() const { return pointer_; }
    void set_pointer(render::Point p) { pointer_ = p; }
    bool animating() const { return animating_; }

    void frame() {
        sdl3::Frame f{1.0 / style_.fps,
                      window_.width,
                      window_.height,
                      scale_,
                      false,
                      true,
                      std::move(pointer_events_),
                      std::move(wheel_events_),
                      std::move(key_events_),
                      std::move(text_events_)};
        f.size_class = layout::compute_window_size_class(f.width_dp, f.height_dp);
        f.context = std::move(context_events_);
        f.fingers = std::move(finger_events_);
        context_events_.clear();
        finger_events_.clear();
        pointer_events_.clear();
        wheel_events_.clear();
        key_events_.clear();
        text_events_.clear();
        if (probe_) {
            probe_->begin_frame(0.0);
        }
        color::Argb background = 0;
        {
            const diagnostics::StageScope stage(probe_, diagnostics::Stage::update);
            background = gallery_.update(f);
        }
        const render::Rect window_rect{0.0, 0.0, window_.width, window_.height};
        {
            const diagnostics::StageScope stage(probe_, diagnostics::Stage::paint);
            canvas_.clear(style_.stage);
            canvas_.save();
            canvas_.scale(scale_);
            canvas_.fill_rect(window_rect, background);
            canvas_.push_clip(window_rect);
            animating_ = gallery_.paint(canvas_, f);
            paint_overlay(canvas_);
            canvas_.pop_clip();
            canvas_.restore();
        }
        if (probe_) {
            canvas_.finish();
            probe_->end_frame();
        }
        if (verify_) {
            verify();
        }
        badge_left_ -= f.dt_seconds;
        clock_ += f.dt_seconds;
        if (recording_) {
            write();
        }
    }

    void wait(double seconds) {
        const int n = std::max(1, static_cast<int>(std::lround(seconds * style_.fps)));
        for (int i = 0; i < n; ++i) {
            frame();
        }
    }

    render::Point centre_of(std::string_view name) const {
        const std::optional<render::Rect> r = gallery_.find(name);
        if (!r) {
            throw std::runtime_error("no widget named " + std::string(name));
        }
        if (r->width <= 0.0 || r->height <= 0.0) {
            throw std::runtime_error(std::string(name) + " is out of view");
        }
        return {r->x + r->width / 2.0, r->y + r->height / 2.0};
    }

    void move_to(render::Point to, double seconds) {
        const render::Point from = pointer_;
        const int n = std::max(1, static_cast<int>(std::lround(seconds * style_.fps)));
        for (int i = 1; i <= n; ++i) {
            const double t = static_cast<double>(i) / n;
            const double k = t * t * (3.0 - 2.0 * t);
            pointer_ = {from.x + (to.x - from.x) * k, from.y + (to.y - from.y) * k};
            pointer_events_.push_back(at(PointerAction::move));
            frame();
        }
    }
    void move_to(std::string_view name, double seconds) { move_to(centre_of(name), seconds); }

    void click() {
        pointer_events_.push_back(at(PointerAction::down, 1));
        frame();
        wait(0.08);
        pointer_events_.push_back(at(PointerAction::up));
        frame();
    }

    void back() {
        key_events_.push_back({Key::escape, false, false, false, false, true});
        badge("Back");
        frame();
    }

    void touch_tap() {
        input::PointerEvent down = at(PointerAction::down, 1);
        down.kind = input::PointerKind::touch;
        input::PointerEvent up = at(PointerAction::up);
        up.kind = input::PointerKind::touch;
        pointer_events_.push_back(down);
        pointer_events_.push_back(up);
        if (input::leaves_after_up(up.kind)) {
            pointer_events_.push_back(at(PointerAction::leave));
        }
        frame();
    }

    void touch_fling_to(render::Point to, double seconds) {
        pointer_events_.push_back(touch(at(PointerAction::down, 1)));
        frame();
        const render::Point from = pointer_;
        const int n = std::max(1, static_cast<int>(std::lround(seconds * style_.fps)));
        for (int i = 1; i <= n; ++i) {
            const double t = static_cast<double>(i) / n;
            pointer_ = {from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t};
            pointer_events_.push_back(touch(at(PointerAction::move)));
            frame();
        }
        pointer_events_.push_back(touch(at(PointerAction::up)));
        if (input::leaves_after_up(input::PointerKind::touch)) {
            pointer_events_.push_back(at(PointerAction::leave));
        }
        frame();
    }

    void touch_swipe(render::Point to, double seconds, double hold, bool lift) {
        pointer_events_.push_back(touch(at(PointerAction::down, 1)));
        frame();
        const render::Point from = pointer_;
        const int n = std::max(1, static_cast<int>(std::lround(seconds * style_.fps)));
        for (int i = 1; i <= n; ++i) {
            const double t = static_cast<double>(i) / n;
            pointer_ = {from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t};
            pointer_events_.push_back(touch(at(PointerAction::move)));
            frame();
        }
        if (!lift) {
            return;
        }
        wait(hold);
        pointer_events_.push_back(touch(at(PointerAction::up)));
        pointer_events_.push_back(at(PointerAction::leave));
        frame();
    }

    void touch_move_to(render::Point to, double seconds) {
        const render::Point from = pointer_;
        const int n = std::max(1, static_cast<int>(std::lround(seconds * style_.fps)));
        for (int i = 1; i <= n; ++i) {
            const double t = static_cast<double>(i) / n;
            pointer_ = {from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t};
            pointer_events_.push_back(touch(at(PointerAction::move)));
            frame();
        }
    }

    void touch_lift() {
        pointer_events_.push_back(touch(at(PointerAction::up)));
        pointer_events_.push_back(at(PointerAction::leave));
        frame();
    }

    void fingers_to(const std::vector<render::Point>& from, const std::vector<render::Point>& to, double seconds, double hold) {
        const auto finger = [this](input::FingerAction a, std::size_t i, render::Point p) {
            return input::FingerEvent{a, static_cast<std::uint64_t>(i + 1), p.x, p.y, clock_};
        };
        for (std::size_t i = 0; i < from.size(); ++i) {
            finger_events_.push_back(finger(input::FingerAction::down, i, from[i]));
        }
        pointer_ = from.front();
        pointer_events_.push_back(touch(at(PointerAction::down, 1)));
        frame();
        const int n = std::max(1, static_cast<int>(std::lround(seconds * style_.fps)));
        std::vector<render::Point> live = from;
        for (int s = 1; s <= n; ++s) {
            const double t = static_cast<double>(s) / n;
            for (std::size_t i = 0; i < from.size(); ++i) {
                live[i] = {from[i].x + (to[i].x - from[i].x) * t, from[i].y + (to[i].y - from[i].y) * t};
                finger_events_.push_back(finger(input::FingerAction::move, i, live[i]));
            }
            pointer_ = live.front();
            pointer_events_.push_back(touch(at(PointerAction::move)));
            frame();
        }
        wait(hold);
        for (std::size_t i = 0; i < live.size(); ++i) {
            finger_events_.push_back(finger(input::FingerAction::up, i, live[i]));
        }
        pointer_events_.push_back(touch(at(PointerAction::up)));
        pointer_events_.push_back(at(PointerAction::leave));
        frame();
    }

    void finger_tap(render::Point p) {
        fingers_to({p}, {p}, 0.0, style_.finger_tap_seconds);
    }

    std::vector<std::uint8_t> pixels() {
        const render::PixelView px = canvas_.finish();
        std::vector<std::uint8_t> out;
        out.reserve(static_cast<std::size_t>(px.width) * static_cast<std::size_t>(px.height) * 4);
        for (int y = 0; y < px.height; ++y) {
            const auto* row = reinterpret_cast<const std::uint8_t*>(px.data) + px.stride * static_cast<std::size_t>(y);
            out.insert(out.end(), row, row + static_cast<std::size_t>(px.width) * 4);
        }
        return out;
    }

    void right_click() {
        context_events_.push_back({pointer_.x, pointer_.y});
        badge("Right click");
        frame();
    }

    void modified_click(bool shift, bool ctrl) {
        input::PointerEvent down = at(PointerAction::down, 1);
        down.shift = shift;
        down.ctrl = ctrl;
        pointer_events_.push_back(down);
        badge(ctrl && shift ? "Ctrl+Shift+click" : ctrl ? "Ctrl+click" : "Shift+click");
        frame();
        wait(0.08);
        input::PointerEvent up = at(PointerAction::up);
        up.shift = shift;
        up.ctrl = ctrl;
        pointer_events_.push_back(up);
        frame();
    }

    void chord(Key k, bool shift, bool ctrl, bool alt, std::string label) {
        key_events_.push_back({k, shift, ctrl, false, alt});
        badge(std::move(label));
        frame();
    }

    void press() {
        pointer_events_.push_back(at(PointerAction::down, 1));
        frame();
    }

    void release() {
        pointer_events_.push_back(at(PointerAction::up));
        frame();
    }

    void fling_to(render::Point to, double seconds) {
        press();
        const render::Point from = pointer_;
        const int n = std::max(1, static_cast<int>(std::lround(seconds * style_.fps)));
        for (int i = 1; i <= n; ++i) {
            const double t = static_cast<double>(i) / n;
            pointer_ = {from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t};
            pointer_events_.push_back(at(PointerAction::move));
            frame();
        }
        release();
    }

    void drag_to(render::Point to, double seconds) {
        press();
        move_to(to, seconds);
        wait(0.1);
        release();
    }

    void ctrl_key(Key k, std::string label) {
        key_events_.push_back({k, false, true});
        badge(std::move(label));
        frame();
    }

    void key(Key k, bool shift = false, std::string label = {}) {
        key_events_.push_back({k, shift});
        badge(label.empty() ? key_name(k, shift) : label);
        frame();
    }

    void type(std::string_view utf8, double per_char) {
        std::size_t i = 0;
        while (i < utf8.size()) {
            std::size_t n = 1;
            const auto c = static_cast<unsigned char>(utf8[i]);
            n = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
            text_events_.push_back({input::TextEventKind::commit, std::string(utf8.substr(i, n)), 0});
            badge(std::string(utf8.substr(i, n)));
            frame();
            wait(per_char);
            i += n;
        }
    }

    void wheel(double dy) {
        wheel_events_.push_back({pointer_.x, pointer_.y, 0.0, dy});
        frame();
    }

    void wheel_sideways(double dx) {
        wheel_events_.push_back({pointer_.x, pointer_.y, dx, 0.0});
        frame();
    }

    void shift_wheel(double dy) {
        wheel_events_.push_back({pointer_.x, pointer_.y, 0.0, dy, true});
        badge("Shift + wheel");
        frame();
    }

    void settle() { wait(style_.settle_seconds); }
    double still_seconds() const { return style_.still_seconds; }

    void write_raw(const std::string& path) {
        std::ofstream file(path, std::ios::binary);
        const render::PixelView px = canvas_.finish();
        for (int y = 0; y < px.height; ++y) {
            file.write(reinterpret_cast<const char*>(px.data + px.stride * y), static_cast<std::streamsize>(px.width) * 4);
        }
        if (!file) {
            throw std::runtime_error("could not write " + path);
        }
    }

    int width() const { return canvas_.width(); }
    int height() const { return canvas_.height(); }

private:
    static input::PointerEvent touch(input::PointerEvent e) {
        e.kind = input::PointerKind::touch;
        return e;
    }

    input::PointerEvent at(PointerAction action, int clicks = 0) const {
        return {action, pointer_.x, pointer_.y, clicks, input::PointerKind::mouse, clock_};
    }

    static std::string key_name(Key k, bool shift) {
        static const std::map<Key, std::string> names{{Key::tab, "Tab"},      {Key::space, "Space"}, {Key::enter, "Enter"},
                                                      {Key::escape, "Esc"},   {Key::left, "←"},  {Key::right, "→"},
                                                      {Key::up, "↑"},    {Key::down, "↓"},  {Key::backspace, "Backspace"},
                                                      {Key::page_down, "PgDn"},  {Key::page_up, "PgUp"},
                                                      {Key::home, "Home"},       {Key::end, "End"},
                                                      {Key::delete_forward, "Delete"}, {Key::alt, "Alt"},
                                                      {Key::f2, "F2"},           {Key::f5, "F5"},
                                                      {Key::f10, "F10"},         {Key::menu, "Menu"}};
        const auto it = names.find(k);
        const std::string base = it == names.end() ? "?" : it->second;
        return shift ? "Shift+" + base : base;
    }

    void badge(std::string text) {
        badge_text_ = std::move(text);
        badge_left_ = style_.key_badge_seconds;
    }

    void paint_overlay(render::Canvas& c) {
        if (badge_left_ > 0.0 && !badge_text_.empty()) {
            const text::TextLine line = text::layout_line(fonts_, badge_text_, current_style().type.text(style_.badge_type));
            const double h = line.box_height() + 2.0 * style_.key_badge_padding;
            const double w = line.width + 2.0 * style_.key_badge_padding;
            const render::Rect r{style_.key_badge_margin, window_.height - style_.key_badge_margin - h, w, h};
            const double a = std::clamp(badge_left_ / 0.2, 0.0, 1.0);
            c.fill_path(render::rounded_rect_path(r, render::uniform_radii(h / 2.0)), color::scale_alpha(style_.badge_fill, a));
            text::draw_line(c, fonts_, line, {r.x + style_.key_badge_padding, r.y + style_.key_badge_padding},
                            color::scale_alpha(style_.badge_text, a));
        }
        if (!cursor_) {
            return;
        }
        const double s = style_.cursor_size;
        const render::Point p = pointer_;
        auto arrow = [&](double grow) {
            render::Path path;
            path.move_to({p.x - grow * 0.4, p.y - grow});
            path.line_to({p.x - grow * 0.4, p.y + s + grow});
            path.line_to({p.x + s * 0.28, p.y + s * 0.72 + grow * 0.3});
            path.line_to({p.x + s * 0.72 + grow, p.y + s * 0.72 + grow * 0.3});
            path.close();
            return path;
        };
        const input::Cursor shape = gallery_.cursor().value_or(input::Cursor::arrow);
        if (shape == input::Cursor::resize_horizontal || shape == input::Cursor::resize_vertical) {
            const bool across = shape == input::Cursor::resize_horizontal;
            auto resize = [&](double grow) {
                const double half = s * 0.6 + grow;
                const double head = s * 0.32 + grow;
                const double shaft = s * 0.09 + grow * 0.6;
                auto at = [&](double along, double side) {
                    return across ? render::Point{p.x + along, p.y + side} : render::Point{p.x + side, p.y + along};
                };
                render::Path path;
                path.move_to(at(-half, 0.0));
                path.line_to(at(-half + head, -head));
                path.line_to(at(-half + head, -shaft));
                path.line_to(at(half - head, -shaft));
                path.line_to(at(half - head, -head));
                path.line_to(at(half, 0.0));
                path.line_to(at(half - head, head));
                path.line_to(at(half - head, shaft));
                path.line_to(at(-half + head, shaft));
                path.line_to(at(-half + head, head));
                path.close();
                return path;
            };
            c.fill_path(resize(1.5), style_.cursor_outline);
            c.fill_path(resize(0.0), style_.cursor_fill);
            return;
        }
        if (shape == input::Cursor::text) {
            auto beam = [&](double grow) {
                const double h = s * 0.55 + grow;
                const double w = s * 0.08 + grow * 0.6;
                const double cap = s * 0.22 + grow;
                render::Path path;
                path.move_to({p.x - cap, p.y - h});
                path.line_to({p.x + cap, p.y - h});
                path.line_to({p.x + cap, p.y - h + 2.0 * w});
                path.line_to({p.x + w, p.y - h + 2.0 * w});
                path.line_to({p.x + w, p.y + h - 2.0 * w});
                path.line_to({p.x + cap, p.y + h - 2.0 * w});
                path.line_to({p.x + cap, p.y + h});
                path.line_to({p.x - cap, p.y + h});
                path.line_to({p.x - cap, p.y + h - 2.0 * w});
                path.line_to({p.x - w, p.y + h - 2.0 * w});
                path.line_to({p.x - w, p.y - h + 2.0 * w});
                path.line_to({p.x - cap, p.y - h + 2.0 * w});
                path.close();
                return path;
            };
            c.fill_path(beam(1.5), style_.cursor_outline);
            c.fill_path(beam(0.0), style_.cursor_fill);
            return;
        }
        c.fill_path(arrow(1.5), style_.cursor_outline);
        c.fill_path(arrow(0.0), style_.cursor_fill);
    }

    void verify() {
        const render::PixelView got = canvas_.finish();
        const std::vector<render::PixelRect> changed = canvas_.take_changed_pixels();
        const std::size_t area = static_cast<std::size_t>(got.width) * static_cast<std::size_t>(got.height);
        const bool comparable = previous_.size() == area;
        render::Fingerprint print;
        std::uint64_t outside = 0;
        for (int y = 0; y < got.height; ++y) {
            const auto* a = reinterpret_cast<const std::uint32_t*>(got.data + got.stride * y);
            print.add_bytes({got.data + got.stride * y, static_cast<std::size_t>(got.width) * sizeof(std::uint32_t)});
            for (int x = 0; comparable && x < got.width; ++x) {
                const std::size_t i = static_cast<std::size_t>(y) * static_cast<std::size_t>(got.width) + static_cast<std::size_t>(x);
                if (previous_[i] != a[x]) {
                    const bool inside = std::any_of(changed.begin(), changed.end(), [&](const render::PixelRect& r) {
                        return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
                    });
                    outside += inside ? 0 : 1;
                }
            }
        }
        previous_.resize(area);
        for (int y = 0; y < got.height; ++y) {
            std::memcpy(previous_.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(got.width), got.data + got.stride * y,
                        static_cast<std::size_t>(got.width) * sizeof(std::uint32_t));
        }
        std::uint64_t repainted = 0;
        for (const render::PixelRect& r : changed) {
            repainted += static_cast<std::uint64_t>(r.area());
        }
        std::printf("frame %zu %016llx repainted %llu outside %llu\n", verified_frames_, static_cast<unsigned long long>(print.value()),
                    static_cast<unsigned long long>(repainted), static_cast<unsigned long long>(outside));
        std::fflush(stdout);
        verified_frames_ += 1;
        outside_pixels_ += outside;
    }
    void write() {
        if (!out_) {
            return;
        }
        const render::PixelView px = canvas_.finish();
        for (int y = 0; y < px.height; ++y) {
            std::fwrite(px.data + px.stride * y, 4, static_cast<std::size_t>(px.width), out_);
        }
    }

    examples::components::Gallery& gallery_;
    text::FontStack fonts_;
    double scale_;
    std::FILE* out_;
    WindowSize stage_;
    WindowSize window_;
    thorvg::Canvas canvas_;
    CaptureStyle style_;
    bool recording_ = false;
    bool verify_ = false;
    std::size_t verified_frames_ = 0;
    std::uint64_t outside_pixels_ = 0;
    std::vector<std::uint32_t> previous_;
    bool cursor_ = true;
    bool animating_ = false;
    render::Point pointer_{720.0, 820.0};
    std::vector<input::PointerEvent> pointer_events_;
    std::vector<input::WheelEvent> wheel_events_;
    std::vector<input::KeyEvent> key_events_;
    std::vector<input::TextEvent> text_events_;
    std::vector<input::ContextClick> context_events_;
    std::vector<input::FingerEvent> finger_events_;
    std::string badge_text_;
    double badge_left_ = 0.0;
    double clock_ = 0.0;
    diagnostics::FrameProbe* probe_ = nullptr;
};

void tab_to(Driver& d, examples::components::Gallery& g, std::string_view name, int limit = 60) {
    for (int i = 0; i < limit; ++i) {
        const auto focused = widgets::focused_accessible(g.accessible_nodes());
        if (focused && focused->name == name) {
            return;
        }
        d.key(Key::tab);
    }
    throw std::runtime_error("Tab never reached " + std::string(name));
}

bool fully_in_view(const Driver& d, std::string_view name, std::string_view row) {
    const std::optional<render::Rect> r = d.find(name);
    const std::optional<render::Rect> box = d.find(row);
    if (!r || !box) {
        throw std::runtime_error("no widget named " + std::string(r ? row : name));
    }
    return r->width > 0.0 && r->x > box->x && r->x + r->width < box->x + box->width;
}

void reveal_in_row(Driver& d, std::string_view name, std::string_view row) {
    bool scrolled = false;
    for (const double direction : {1.0, -1.0}) {
        for (int i = 0; i < d.style().reveal_notches && !fully_in_view(d, name, row); ++i) {
            d.move_to(row, 0.0);
            d.wheel(direction);
            d.wait(d.style().reveal_pause_seconds);
            scrolled = true;
        }
    }
    if (scrolled) {
        d.settle();
    }
}

void show_page(Driver& d, std::string_view name) {
    d.set_cursor(false);
    reveal_in_row(d, name, d.style().page_tabs);
    d.move_to(name, 0.05);
    d.click();
    d.move_to({720.0, 845.0}, 0.05);
    d.set_cursor(true);
    d.wait(0.6);
}

void wheel_at(Driver& d, std::string_view name, double dy, int notches, double pause) {
    d.move_to(name, 0.4);
    for (int i = 0; i < notches; ++i) {
        d.wheel(dy);
        d.wait(pause);
    }
}

std::optional<std::string> month_name(examples::components::Gallery& g) {
    const auto nodes = g.accessible_nodes();
    const auto node = widgets::find_accessible(nodes, [](const widgets::AccessibleNode& n) {
        return n.role == widgets::AccessibleRole::button && n.description == "Activate to change the year";
    });
    if (!node) {
        throw std::runtime_error("no year menu button");
    }
    return node->name;
}

calendar::Date capture_today() {
    const auto now = std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()}.get_local_time();
    const std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(now)};
    return {static_cast<int>(ymd.year()), static_cast<int>(static_cast<unsigned>(ymd.month())), static_cast<int>(static_cast<unsigned>(ymd.day()))};
}

std::string day_name(const calendar::Date& d) {
    const calendar::CalendarLocale& l = calendar::english_us();
    const std::string date = calendar::format_date(l.long_date_pattern, d, l.names);
    return d == capture_today() ? l.strings.today_description + l.strings.list_separator + date : date;
}

std::string first_matching(examples::components::Gallery& g, const std::function<bool(const widgets::AccessibleNode&)>& match) {
    const auto nodes = g.accessible_nodes();
    const auto node = widgets::find_accessible(nodes, match);
    if (!node) {
        throw std::runtime_error("no matching node");
    }
    return node->name;
}

render::Point centre_by(examples::components::Gallery& g, const std::function<bool(const widgets::AccessibleNode&)>& match) {
    const auto nodes = g.accessible_nodes();
    const auto node = widgets::find_accessible(nodes, match);
    if (!node) {
        throw std::runtime_error("no matching node");
    }
    return {node->bounds.x + node->bounds.width / 2.0, node->bounds.y + node->bounds.height / 2.0};
}

render::Point dial_point(examples::components::Gallery& g, double degrees, double radius) {
    const render::Point c = centre_by(g, [](const widgets::AccessibleNode& n) {
        return n.role == widgets::AccessibleRole::group && (n.name == "Select hour" || n.name == "Select minutes");
    });
    const double r = degrees * 3.14159265358979323846 / 180.0;
    return {c.x + radius * std::sin(r), c.y - radius * std::cos(r)};
}

render::Point centre_in(examples::components::Gallery& g, const std::string& scope, const std::string& name) {
    const auto nodes = g.accessible_nodes();
    const auto root = widgets::find_accessible(nodes, [&](const widgets::AccessibleNode& n) { return n.name == scope && !n.children.empty(); });
    if (!root) {
        throw std::runtime_error("no scope " + scope);
    }
    const auto node = widgets::find_accessible(root->children, name);
    if (!node) {
        throw std::runtime_error("no " + name + " in " + scope);
    }
    return {node->bounds.x + node->bounds.width / 2.0, node->bounds.y + node->bounds.height / 2.0};
}

void day_click(Driver& d, examples::components::Gallery& g, const std::string& scope, const calendar::Date& date, double seconds) {
    d.move_to(centre_in(g, scope, day_name(date)), seconds);
    d.click();
}

using Scene = std::function<void(Driver&, examples::components::Gallery&)>;

const std::map<std::string, Scene>& stills() {
    static const std::map<std::string, Scene> scenes{
        {"default", [](Driver&, examples::components::Gallery&) {}},
        {"scrolled",
         [](Driver& d, examples::components::Gallery&) {
             d.move_to({d.window_width() / 2.0, d.window_height() / 2.0}, 0.2);
             for (int i = 0; i < d.style().scroll_notches; ++i) {
                 d.wheel(1.0);
                 d.wait(d.style().reveal_pause_seconds);
             }
         }},
        {"focus_reveal",
         [](Driver& d, examples::components::Gallery& g) {
             d.set_cursor(false);
             tab_to(d, g, "Name");
         }},
        {"chips", [](Driver& d, examples::components::Gallery&) { show_page(d, "Chips"); }},
        {"chips_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Chips");
             d.set_cursor(false);
             tab_to(d, g, "Filters");
             d.key(Key::right);
             d.wait(0.4);
         }},
        {"chips_hover",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Chips");
             d.move_to("Kyoto", 0.3);
             d.wait(0.4);
         }},
        {"chips_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Chips");
             d.move_to("Enabled", 0.1);
             d.click();
             d.move_to({720.0, 845.0}, 0.1);
         }},
        {"lists", [](Driver& d, examples::components::Gallery&) { show_page(d, "Lists"); }},
        {"lists_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Lists");
             d.set_cursor(false);
             tab_to(d, g, "Messages");
             for (int i = 0; i < 4; ++i) {
                 d.key(Key::down);
             }
             d.wait(0.6);
         }},
        {"lists_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Lists");
             d.move_to("Enabled", 0.1);
             d.click();
             d.move_to({720.0, 845.0}, 0.1);
         }},
        {"bars", [](Driver& d, examples::components::Gallery&) { show_page(d, "App bars"); }},
        {"bars_scrolled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "App bars");
             for (const char* list : {"Inbox list", "Photos list", "Mail list", "Albums list"}) {
                 wheel_at(d, list, 1.0, 3, 0.1);
             }
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(0.8);
         }},
        {"inputs", [](Driver& d, examples::components::Gallery&) { show_page(d, "Inputs"); }},
        {"inputs_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Inputs");
             d.set_cursor(false);
             tab_to(d, g, "Travel mode");
             d.key(Key::right);
             tab_to(d, g, "Range end");
             d.key(Key::left);
             d.wait(0.4);
         }},
        {"dates", [](Driver& d, examples::components::Gallery&) { show_page(d, "Dates"); }},
        {"dates_years",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Dates");
             d.move_to(*month_name(g), 0.3);
             d.click();
             d.wait(0.6);
         }},
        {"dates_input",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Dates");
             d.move_to("Switch to text input mode", 0.3);
             d.click();
             d.wait(0.6);
             d.ctrl_key(Key::a, "Ctrl+A");
             d.type("13212026", 0.05);
             d.move_to({1100.0, 600.0}, 0.2);
         }},
        {"dates_dialog",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Dates");
             d.move_to("Date picker dialog", 0.3);
             d.click();
             d.wait(0.6);
         }},
        {"dates_range_dialog",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Dates");
             d.move_to("Date range dialog", 0.3);
             d.click();
             d.wait(0.6);
         }},
        {"dates_docked",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Dates");
             d.move_to("Open calendar", 0.3);
             d.click();
             d.wait(0.6);
         }},
        {"dates_japanese",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Dates");
             d.move_to("日本語", 0.3);
             d.click();
             d.move_to({1100.0, 600.0}, 0.2);
         }},
        {"dates_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Dates");
             d.set_cursor(false);
             tab_to(d, g, *month_name(g));
             d.key(Key::tab);
             d.key(Key::tab);
             d.key(Key::tab);
             d.key(Key::right);
             d.key(Key::down);
             d.wait(0.4);
         }},
        {"times", [](Driver& d, examples::components::Gallery&) { show_page(d, "Times"); }},
        {"times_side_scrolled",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Times");
             d.move_to(d.style().side_scroll_target, 0.3);
             for (int i = 0; i < d.style().scroll_notches; ++i) {
                 d.shift_wheel(1.0);
                 d.wait(d.style().reveal_pause_seconds);
             }
             const render::Rect r = *g.find(d.style().side_scroll_target);
             d.move_to({r.x + r.width / 2.0, r.y + r.height - current_style().scroll_area.bar_hit_width / 2.0}, 0.3);
             d.wait(0.4);
         }},
        {"sheets", [](Driver& d, examples::components::Gallery&) { show_page(d, "Sheets"); }},
        {"drawers", [](Driver& d, examples::components::Gallery&) { show_page(d, "Drawers"); }},
        {"search", [](Driver& d, examples::components::Gallery&) { show_page(d, "Search"); }},
        {"times_24h",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Times");
             d.move_to("24-hour", 0.3);
             d.click();
             d.move_to({1100.0, 820.0}, 0.2);
         }},
        {"times_dialog",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Times");
             d.move_to("Time picker dialog", 0.3);
             d.click();
             d.wait(0.6);
         }},
        {"times_input_dialog",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Times");
             d.move_to("Time picker dialog", 0.3);
             d.click();
             d.wait(0.6);
             d.move_to("Switch to text input mode", 0.3);
             d.click();
             d.wait(0.4);
             d.move_to({d.pointer().x + 240.0, d.pointer().y + 80.0}, 0.3);
             d.wait(0.4);
         }},
        {"times_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Times");
             d.set_cursor(false);
             tab_to(d, g, "Clock");
             d.key(Key::tab);
             d.key(Key::tab);
             d.key(Key::tab);
             d.key(Key::tab);
             d.key(Key::right);
             d.wait(0.4);
         }},
        {"times_side_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Times");
             d.set_cursor(false);
             tab_to(d, g, "Clock");
             for (int i = 0; i < d.style().side_focus_tabs; ++i) {
                 d.key(Key::tab);
             }
             d.wait(0.6);
         }},
        {"times_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Times");
             d.move_to("Enabled", 0.1);
             d.click();
             d.move_to({720.0, 845.0}, 0.1);
         }},
        {"dates_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Dates");
             d.move_to("Enabled", 0.1);
             d.click();
             d.move_to({720.0, 845.0}, 0.1);
         }},
        {"inputs_tooltip",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Inputs");
             d.move_to("Italic", 0.3);
             d.wait(0.6);
         }},
        {"dates_tooltip",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Dates");
             d.move_to("Switch to text input mode", 0.3);
             d.wait(0.6);
         }},
        {"times_tooltip",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Times");
             d.move_to("Time picker dialog", 0.3);
             d.click();
             d.wait(0.6);
             d.move_to("Switch to text input mode", 0.3);
             d.wait(0.6);
         }},
        {"inputs_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Inputs");
             d.move_to("Enabled", 0.1);
             d.click();
             d.move_to({720.0, 845.0}, 0.1);
         }},
        {"disabled",
         [](Driver& d, examples::components::Gallery&) {
             d.move_to("Enabled", 0.1);
             d.click();
             d.move_to({640.0, 830.0}, 0.1);
         }},
        {"focus",
         [](Driver& d, examples::components::Gallery& g) {
             d.set_cursor(false);
             tab_to(d, g, "Tonal settings");
         }},
        {"tooltip", [](Driver& d, examples::components::Gallery&) { d.move_to("Favorite", 0.2); }},
        {"caret", [](Driver& d, examples::components::Gallery&) { d.move_to("Hover me", 0.2); }},
        {"rich",
         [](Driver& d, examples::components::Gallery&) {
             d.move_to("About photos", 0.2);
             d.wait(0.3);
             d.move_to({d.pointer().x + 160.0, d.pointer().y + 60.0}, 0.2);
         }},
        {"placeholder",
         [](Driver& d, examples::components::Gallery&) {
             d.move_to("Name", 0.2);
             d.click();
         }},
        {"menu",
         [](Driver& d, examples::components::Gallery&) {
             d.move_to("Menu", 0.2);
             d.click();
             d.wait(0.4);
             d.move_to("Copy", 0.2);
         }},
        {"dialog",
         [](Driver& d, examples::components::Gallery&) {
             d.move_to({900.0, 300.0}, 0.2);
             d.move_to("Dialog", 0.2);
             d.click();
             d.wait(0.5);
             d.move_to("Paragraph 1: notifications arrive on every device you are signed in to, and you can change this later.", 0.2);
             d.wheel(2.0);
         }},
    };
    return scenes;
}

const std::map<std::string, Scene>& videos() {
    static const std::map<std::string, Scene> scenes{
        {"tooltip",
         [](Driver& d, examples::components::Gallery&) {
             d.wait(0.4);
             d.move_to("Filled settings", 0.8);
             d.wait(1.2);
             d.move_to("Tonal settings", 0.4);
             d.wait(1.0);
             d.move_to("Favorite", 0.6);
             d.wait(1.0);
             d.move_to({d.pointer().x, d.pointer().y + 90.0}, 0.4);
             d.wait(0.6);
             d.move_to("Hover me", 1.0);
             d.wait(1.2);
             d.move_to("About photos", 0.8);
             d.wait(0.8);
             d.move_to({d.pointer().x + 120.0, d.pointer().y + 70.0}, 0.6);
             d.wait(1.2);
             d.move_to({1180.0, 800.0}, 0.8);
             d.click();
             d.wait(0.8);
         }},
        {"focus",
         [](Driver& d, examples::components::Gallery& g) {
             d.set_cursor(false);
             g.set_enabled("Send", false);
             g.set_enabled("Like", false);
             g.set_enabled("Outlined settings", false);
             g.set_enabled("Bluetooth", false);
             g.set_enabled("Videos", false);
             g.set_enabled("Medium", false);
             g.set_enabled("Brightness", false);
             d.wait(0.6);
             for (int i = 0; i < 30; ++i) {
                 d.key(Key::tab);
                 d.wait(0.35);
             }
             tab_to(d, g, "Travel");
             d.wait(0.4);
             d.key(Key::right);
             d.wait(0.6);
             d.key(Key::right);
             d.wait(0.6);
             tab_to(d, g, "Menu");
             d.wait(0.4);
             d.key(Key::space);
             d.wait(0.6);
             d.key(Key::down);
             d.wait(0.5);
             d.key(Key::down);
             d.wait(0.5);
             d.key(Key::down);
             d.wait(0.5);
             d.key(Key::escape);
             d.wait(0.8);
         }},
        {"placeholder",
         [](Driver& d, examples::components::Gallery&) {
             d.wait(0.4);
             d.move_to("Name", 0.8);
             d.wait(0.3);
             d.click();
             d.wait(1.0);
             d.type("Ada", 0.25);
             d.wait(0.6);
             d.key(Key::backspace);
             d.wait(0.25);
             d.key(Key::backspace);
             d.wait(0.25);
             d.key(Key::backspace);
             d.wait(0.8);
             d.move_to("Search", 0.6);
             d.click();
             d.wait(1.0);
             d.type("部品", 0.3);
             d.wait(0.8);
             d.move_to({1100.0, 800.0}, 0.6);
             d.click();
             d.wait(1.0);
         }},
        {"dialog",
         [](Driver& d, examples::components::Gallery&) {
             d.wait(0.4);
             d.move_to({900.0, 300.0}, 0.6);
             d.move_to("Dialog", 0.4);
             d.click();
             d.wait(1.0);
             d.move_to("Paragraph 1: notifications arrive on every device you are signed in to, and you can change this later.", 0.6);
             for (int i = 0; i < 4; ++i) {
                 d.wheel(1.0);
                 d.wait(0.45);
             }
             d.wait(0.6);
             for (int i = 0; i < 4; ++i) {
                 d.wheel(-1.0);
                 d.wait(0.3);
             }
             d.move_to("Mentions", 0.6);
             d.click();
             d.wait(0.6);
             d.move_to("Email", 0.5);
             d.click();
             d.wait(0.5);
             d.type("name@example.com", 0.06);
             d.wait(0.6);
             d.move_to("Cancel", 0.7);
             d.wait(0.3);
             d.click();
             d.wait(1.0);
         }},
        {"filter_chips",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Chips");
             for (const char* name : {"Unread", "Has files", "Elevated", "Starred"}) {
                 d.move_to(name, 0.6);
                 d.wait(0.3);
                 d.click();
                 d.wait(0.7);
             }
             for (const char* name : {"Music", "Science", "Travel"}) {
                 d.move_to(name, 0.6);
                 d.wait(0.2);
                 d.click();
                 d.wait(0.6);
             }
             d.move_to({720.0, 845.0}, 0.6);
             d.set_cursor(false);
             tab_to(d, g, "Filters");
             d.wait(0.5);
             for (int i = 0; i < 2; ++i) {
                 d.key(Key::right);
                 d.wait(0.5);
                 d.key(Key::space);
                 d.wait(0.7);
             }
             d.wait(0.6);
         }},
        {"input_chip",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Chips");
             d.move_to("Grace", 0.8);
             d.wait(0.4);
             d.move_to("Remove Grace", 0.4);
             d.wait(0.5);
             d.click();
             d.wait(1.2);
             d.move_to("Remove Lisbon", 0.8);
             d.wait(0.4);
             d.click();
             d.wait(1.2);
             d.move_to({720.0, 845.0}, 0.6);
             d.set_cursor(false);
             tab_to(d, g, "Recipients");
             d.wait(0.5);
             d.key(Key::right);
             d.wait(0.6);
             d.key(Key::delete_forward);
             d.wait(1.2);
             d.key(Key::backspace, false, "Backspace");
             d.wait(1.4);
         }},
        {"card",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Chips");
             d.move_to("Kyoto", 0.8);
             d.wait(0.8);
             d.click();
             d.wait(0.6);
             d.move_to("Porto", 0.6);
             d.wait(0.6);
             d.click();
             d.wait(0.6);
             d.move_to("Open", 0.6);
             d.wait(0.5);
             d.click();
             d.wait(0.6);
             d.move_to("Static", 0.6);
             d.wait(0.6);
             d.click();
             d.wait(0.6);
             d.move_to({720.0, 845.0}, 0.6);
             d.set_cursor(false);
             tab_to(d, g, "Kyoto");
             d.wait(0.6);
             d.key(Key::enter);
             d.wait(0.6);
             d.key(Key::tab);
             d.wait(0.6);
             d.key(Key::tab);
             d.wait(0.8);
         }},
        {"list_keys",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Lists");
             d.set_cursor(false);
             tab_to(d, g, "Messages");
             d.wait(0.5);
             for (int i = 0; i < 8; ++i) {
                 d.key(Key::down);
                 d.wait(0.3);
             }
             d.key(Key::page_down);
             d.wait(0.8);
             d.key(Key::page_down);
             d.wait(0.8);
             d.key(Key::end);
             d.wait(1.2);
             d.key(Key::up);
             d.wait(0.5);
             d.key(Key::home);
             d.wait(1.2);
             tab_to(d, g, "Settings");
             d.wait(0.4);
             for (int i = 0; i < 3; ++i) {
                 d.key(Key::down);
                 d.wait(0.35);
             }
             d.key(Key::space);
             d.wait(0.7);
             for (int i = 0; i < 4; ++i) {
                 d.key(Key::down);
                 d.wait(0.35);
             }
             d.key(Key::space);
             d.wait(0.7);
             d.key(Key::down);
             d.wait(0.5);
             d.key(Key::down);
             d.wait(0.8);
         }},
        {"badge",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "App bars");
             d.move_to("Add 1", 0.8);
             for (int i = 0; i < 4; ++i) {
                 d.click();
                 d.wait(0.45);
             }
             d.move_to("Add 100", 0.6);
             for (int i = 0; i < 10; ++i) {
                 d.click();
                 d.wait(0.35);
             }
             d.wait(0.5);
             d.move_to("Clear", 0.6);
             d.click();
             d.wait(0.8);
             d.move_to("Add 1", 0.6);
             d.click();
             d.wait(1.0);
         }},
        {"app_bar",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "App bars");
             wheel_at(d, "Inbox list", 1.0, 3, 0.35);
             d.wait(0.4);
             wheel_at(d, "Inbox list", -1.0, 3, 0.3);
             d.wait(0.6);
             wheel_at(d, "Photos list", 1.0, 3, 0.35);
             d.wait(0.4);
             wheel_at(d, "Photos list", -0.5, 1, 0.8);
             wheel_at(d, "Photos list", -1.0, 4, 0.3);
             d.wait(0.6);
             wheel_at(d, "Mail list", 0.5, 1, 1.0);
             wheel_at(d, "Mail list", 1.0, 4, 0.35);
             d.wait(0.4);
             wheel_at(d, "Mail list", -1.0, 6, 0.3);
             d.wait(0.6);
             wheel_at(d, "Albums list", 0.6, 1, 1.0);
             wheel_at(d, "Albums list", 1.0, 3, 0.35);
             d.wait(0.6);
             wheel_at(d, "Albums list", -1.0, 6, 0.3);
             d.wait(1.0);
         }},
        {"segments",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Inputs");
             d.wait(0.4);
             for (const char* name : {"Month", "Year", "Day"}) {
                 d.move_to(name, 0.6);
                 d.click();
                 d.wait(0.6);
             }
             for (const char* name : {"Transit", "Drive"}) {
                 d.move_to(name, 0.6);
                 d.click();
                 d.wait(0.6);
             }
             for (const char* name : {"XS", "L", "S"}) {
                 d.move_to(name, 0.5);
                 d.click();
                 d.wait(0.6);
             }
             for (const char* name : {"Italic", "Bold"}) {
                 d.move_to(name, 0.5);
                 d.click();
                 d.wait(0.6);
             }
             d.move_to({700.0, 700.0}, 0.6);
             d.set_cursor(false);
             tab_to(d, g, "View");
             d.wait(0.4);
             for (Key k : {Key::right, Key::right, Key::left, Key::end, Key::home}) {
                 d.key(k);
                 d.wait(0.5);
             }
             d.key(Key::tab);
             d.wait(0.4);
             d.key(Key::right);
             d.wait(0.6);
             d.wait(0.6);
         }},
        {"range_slider",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Inputs");
             d.wait(0.3);
             d.move_to("Range start", 0.6);
             d.drag_to({d.pointer().x + 70.0, d.pointer().y}, 0.9);
             d.wait(0.3);
             d.move_to("Range end", 0.6);
             d.drag_to({d.pointer().x - 60.0, d.pointer().y}, 1.0);
             d.wait(0.3);
             d.drag_to({d.pointer().x + 160.0, d.pointer().y}, 1.0);
             d.wait(0.4);
             const render::Point steps = centre_by(g, [](const widgets::AccessibleNode& n) { return n.name == "Steps"; });
             d.move_to({steps.x + 150.0, steps.y}, 0.6);
             d.click();
             d.wait(0.4);
             d.move_to({steps.x - 150.0, steps.y}, 0.7);
             d.click();
             d.wait(0.5);
             d.move_to("Balance", 0.6);
             d.drag_to({d.pointer().x - 140.0, d.pointer().y}, 1.0);
             d.wait(0.4);
             d.move_to("Bottom to top", 0.6);
             d.drag_to({d.pointer().x, d.pointer().y - 70.0}, 0.8);
             d.wait(0.4);
             d.move_to({1100.0, 760.0}, 0.5);
             d.set_cursor(false);
             tab_to(d, g, "Range start");
             for (int i = 0; i < 4; ++i) {
                 d.key(Key::right);
                 d.wait(0.25);
             }
             d.key(Key::tab);
             d.wait(0.3);
             d.key(Key::page_down);
             d.wait(0.4);
             d.key(Key::page_down);
             d.wait(0.8);
         }},
        {"date_picker",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Dates");
             const calendar::Date today = capture_today();
             d.wait(0.3);
             d.move_to("Change to next month", 0.8);
             d.click();
             d.wait(0.6);
             d.click();
             d.wait(0.6);
             d.move_to("Change to previous month", 0.5);
             d.click();
             d.wait(0.6);
             const calendar::YearMonth next = calendar::add_months(calendar::year_month(today), 1);
             day_click(d, g, "Select date", {next.year, next.month, 10}, 0.7);
             d.wait(0.6);
             d.move_to(*month_name(g), 0.7);
             d.click();
             d.wait(0.8);
             d.move_to(first_matching(g, [&](const widgets::AccessibleNode& n) { return n.name == "Navigate to year " + std::to_string(next.year + 1); }), 0.7);
             d.click();
             d.wait(0.8);
             day_click(d, g, "Select date", {next.year + 1, next.month, 15}, 0.6);
             d.wait(0.6);
             d.move_to("Switch to text input mode", 0.8);
             d.click();
             d.wait(0.8);
             d.ctrl_key(Key::a, "Ctrl+A");
             d.type("02302027", 0.12);
             d.wait(0.9);
             d.ctrl_key(Key::a, "Ctrl+A");
             d.type("02152027", 0.12);
             d.wait(0.9);
             d.move_to("Switch to calendar input mode", 0.6);
             d.click();
             d.wait(1.0);
             d.move_to("Date picker dialog", 0.8);
             d.click();
             d.wait(0.8);
             day_click(d, g, "Date picker dialog", {today.year, today.month, 26}, 0.7);
             d.wait(0.5);
             d.move_to("OK", 0.6);
             d.click();
             d.wait(0.8);
             d.move_to("日本語", 0.8);
             d.click();
             d.wait(1.2);
         }},
        {"date_range",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Dates");
             const calendar::Date today = capture_today();
             d.wait(0.3);
             d.move_to("Date range dialog", 0.8);
             d.click();
             d.wait(0.8);
             day_click(d, g, "Date range dialog", {today.year, today.month, 9}, 0.7);
             d.wait(0.5);
             day_click(d, g, "Date range dialog", {today.year, today.month, 18}, 0.7);
             d.wait(0.8);
             day_click(d, g, "Date range dialog", {today.year, today.month, 4}, 0.6);
             d.wait(0.5);
             day_click(d, g, "Date range dialog", {today.year, today.month, 6}, 0.6);
             d.wait(0.6);
             d.move_to("OK", 0.6);
             d.click();
             d.wait(0.6);
             d.move_to("Open calendar", 0.8);
             d.click();
             d.wait(0.7);
             day_click(d, g, "Date", {today.year, today.month, 12}, 0.7);
             d.wait(1.0);
             d.set_cursor(false);
             tab_to(d, g, *month_name(g));
             d.key(Key::tab);
             d.key(Key::tab);
             d.key(Key::tab);
             d.wait(0.5);
             for (Key k : {Key::right, Key::right, Key::down, Key::down, Key::page_down, Key::home, Key::end, Key::left}) {
                 d.key(k);
                 d.wait(0.5);
             }
             d.key(Key::enter);
             d.wait(1.0);
         }},
        {"time_picker",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Times");
             d.wait(0.3);
             d.move_to("3 o'clock", 0.8);
             d.click();
             d.wait(1.0);
             d.move_to(dial_point(g, 20.0, 100.0), 0.6);
             d.press();
             for (int deg = 30; deg <= 200; deg += 10) {
                 d.move_to(dial_point(g, static_cast<double>(deg), 100.0), 0.08);
             }
             d.wait(0.2);
             d.release();
             d.wait(0.8);
             d.move_to("PM", 0.7);
             d.click();
             d.wait(0.6);
             d.move_to("Select hour", 0.7);
             d.click();
             d.wait(0.8);
             d.move_to("11 o'clock", 0.7);
             d.click();
             d.wait(1.2);
             d.move_to("24-hour", 0.8);
             d.click();
             d.wait(0.8);
             d.move_to("Select hour", 0.7);
             d.click();
             d.wait(0.8);
             d.move_to("18 hours", 0.8);
             d.click();
             d.wait(1.2);
             d.move_to("12-hour", 0.8);
             d.click();
             d.wait(0.6);
             d.move_to("Time picker dialog", 0.8);
             d.click();
             d.wait(0.8);
             d.move_to("Switch to text input mode", 0.7);
             d.click();
             d.wait(0.6);
             d.move_to(centre_in(g, "Time picker dialog", "for hour"), 0.6);
             d.click();
             d.type("0730", 0.25);
             d.wait(0.5);
             d.move_to("OK", 0.6);
             d.click();
             d.wait(1.0);
         }},
        {"enable",
         [](Driver& d, examples::components::Gallery&) {
             d.wait(0.4);
             d.move_to("Send", 0.8);
             d.wait(0.6);
             d.move_to("Enabled", 1.0);
             d.wait(0.3);
             d.click();
             d.wait(1.0);
             d.move_to("Send", 0.8);
             d.wait(0.4);
             d.click();
             d.wait(0.5);
             d.move_to("Volume", 0.8);
             d.click();
             d.wait(0.5);
             d.move_to("Enabled", 1.0);
             d.wait(0.3);
             d.click();
             d.wait(1.2);
         }},
    };
    return scenes;
}

render::Point centre_nth(examples::components::Gallery& g, const std::string& name, int nth) {
    int seen = 0;
    std::optional<render::Point> found;
    widgets::visit_accessible(g.accessible_nodes(), [&](const widgets::AccessibleNode& n) {
        if (!found && n.name == name && seen++ == nth) {
            found = render::Point{n.bounds.x + n.bounds.width / 2.0, n.bounds.y + n.bounds.height / 2.0};
        }
    });
    if (!found) {
        throw std::runtime_error("no widget named " + name);
    }
    return *found;
}

render::Point centre_field(examples::components::Gallery& g, int nth) {
    int seen = 0;
    std::optional<render::Point> found;
    widgets::visit_accessible(g.accessible_nodes(), [&](const widgets::AccessibleNode& n) {
        if (!found && n.role == widgets::AccessibleRole::text_field && n.name == "Search" && seen++ == nth) {
            found = render::Point{n.bounds.x + n.bounds.width / 2.0, n.bounds.y + n.bounds.height / 2.0};
        }
    });
    if (!found) {
        throw std::runtime_error("no search field");
    }
    return *found;
}

void tab_to_field(Driver& d, examples::components::Gallery& g, int limit = 60) {
    for (int i = 0; i < limit; ++i) {
        const auto focused = widgets::focused_accessible(g.accessible_nodes());
        if (focused && focused->role == widgets::AccessibleRole::text_field) {
            return;
        }
        d.key(Key::tab);
    }
    throw std::runtime_error("Tab never reached a text field");
}

render::Rect bounds_of(examples::components::Gallery& g, const std::string& name) {
    const auto nodes = g.accessible_nodes();
    const auto node = widgets::find_accessible(nodes, [&](const widgets::AccessibleNode& n) { return n.name == name && !n.children.empty(); });
    if (!node) {
        throw std::runtime_error("no container named " + name);
    }
    return node->bounds;
}

void click_at(Driver& d, render::Point p, double seconds) {
    d.move_to(p, seconds);
    d.click();
}

void click_named(Driver& d, std::string_view name, double seconds) {
    d.move_to(name, seconds);
    d.click();
}

void open_modal_sheet(Driver& d) {
    show_page(d, "Sheets");
    click_named(d, "Show bottom sheet", 0.5);
    d.wait(1.0);
}

void toggle_enabled(Driver& d) {
    d.move_to("Enabled", 0.3);
    d.click();
    d.move_to({720.0, 845.0}, 0.2);
    d.wait(0.6);
}

const std::map<std::string, Scene>& pattern_stills() {
    static const std::map<std::string, Scene> scenes{
        {"sheets_modal", [](Driver& d, examples::components::Gallery&) { open_modal_sheet(d); }},
        {"sheets_expanded",
         [](Driver& d, examples::components::Gallery& g) {
             open_modal_sheet(d);
             click_at(d, centre_in(g, "Bottom Sheet", "Drag handle"), 0.4);
             d.wait(1.2);
             click_at(d, centre_in(g, "Nearby places", "Drag handle"), 0.4);
             d.wait(1.2);
             d.move_to({720.0, 845.0}, 0.2);
         }},
        {"sheets_side_modal",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Sheets");
             click_named(d, "Modal side sheet", 0.4);
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(0.8);
         }},
        {"sheets_side_detached",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Sheets");
             click_named(d, "Detached side sheet", 0.4);
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(0.8);
         }},
        {"sheets_side_standard",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Sheets");
             click_named(d, "Standard side sheet", 0.4);
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(0.8);
         }},
        {"sheets_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Sheets");
             d.set_cursor(false);
             tab_to(d, g, "Show bottom sheet");
             d.key(Key::enter);
             d.wait(1.0);
             d.key(Key::tab);
             d.key(Key::tab);
             d.wait(0.6);
         }},
        {"sheets_tooltip",
         [](Driver& d, examples::components::Gallery& g) {
             open_modal_sheet(d);
             d.move_to(centre_in(g, "Bottom Sheet", "Drag handle"), 0.4);
             d.wait(1.2);
         }},
        {"sheets_disabled",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Sheets");
             click_named(d, "Standard side sheet", 0.4);
             d.wait(0.8);
             click_at(d, centre_in(g, "Nearby places", "Drag handle"), 0.4);
             d.wait(1.0);
             toggle_enabled(d);
         }},
        {"drawers_modal",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Drawers");
             click_named(d, "Open navigation menu", 0.4);
             d.wait(0.8);
             d.move_to("Outbox", 0.4);
             d.wait(0.6);
         }},
        {"drawers_dismissible",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Drawers");
             click_named(d, "Toggle navigation menu", 0.4);
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(0.8);
         }},
        {"drawers_permanent",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Drawers");
             click_named(d, "Permanent", 0.4);
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(0.6);
         }},
        {"drawers_rail",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Drawers");
             click_named(d, "Expand rail", 0.4);
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(1.0);
         }},
        {"drawers_hiding",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Drawers");
             click_named(d, "Hiding rail", 0.4);
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(1.0);
         }},
        {"drawers_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Drawers");
             d.set_cursor(false);
             tab_to(d, g, "Open navigation menu");
             d.key(Key::enter);
             d.wait(0.8);
             d.key(Key::down);
             d.key(Key::down);
             d.wait(0.6);
         }},
        {"drawers_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Drawers");
             click_named(d, "Toggle navigation menu", 0.4);
             d.wait(0.8);
             toggle_enabled(d);
         }},
        {"search_full",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Search");
             click_at(d, centre_field(g, 0), 0.4);
             d.wait(0.6);
             d.type("ka", 0.1);
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(0.8);
         }},
        {"search_docked",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Search");
             click_at(d, centre_field(g, 1), 0.4);
             d.wait(0.6);
             d.type("o", 0.1);
             d.move_to(centre_in(g, "Search", "Kochi"), 0.4);
             d.wait(0.8);
         }},
        {"search_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Search");
             d.set_cursor(false);
             tab_to_field(d, g);
             d.key(Key::down);
             d.wait(0.8);
             d.key(Key::down);
             d.key(Key::down);
             d.wait(0.6);
         }},
        {"search_tooltip",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Search");
             d.move_to(centre_nth(g, "Mic", 1), 0.4);
             d.wait(1.2);
         }},
        {"search_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Search");
             toggle_enabled(d);
         }},
    };
    return scenes;
}

const std::map<std::string, Scene>& pattern_videos() {
    static const std::map<std::string, Scene> scenes{
        {"bottom_sheet",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Sheets");
             d.wait(0.4);
             click_named(d, "Show bottom sheet", 0.8);
             d.wait(1.2);
             const render::Point handle = centre_in(g, "Bottom Sheet", "Drag handle");
             d.move_to(handle, 0.6);
             d.wait(0.3);
             d.fling_to({handle.x, handle.y - 260.0}, 0.15);
             d.wait(1.4);
             const render::Point top = centre_in(g, "Bottom Sheet", "Drag handle");
             d.move_to(top, 0.6);
             d.drag_to({top.x, top.y + 40.0}, 0.6);
             d.wait(1.0);
             d.drag_to({top.x, top.y + 260.0}, 0.9);
             d.wait(1.4);
             const render::Point half = centre_in(g, "Bottom Sheet", "Drag handle");
             d.move_to({half.x, half.y + 120.0}, 0.6);
             d.wait(0.3);
             d.fling_to({half.x, half.y + 260.0}, 0.12);
             d.wait(1.4);
             const render::Point peek = centre_in(g, "Nearby places", "Drag handle");
             d.move_to(peek, 0.8);
             d.wait(0.3);
             d.fling_to({peek.x, peek.y - 200.0}, 0.15);
             d.wait(1.4);
             click_at(d, centre_in(g, "Nearby places", "Drag handle"), 0.8);
             d.wait(1.4);
             click_named(d, "Show bottom sheet", 0.8);
             d.wait(1.2);
             d.key(Key::escape);
             d.wait(1.4);
         }},
        {"side_sheet",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Sheets");
             d.wait(0.4);
             click_named(d, "Modal side sheet", 0.8);
             d.wait(1.0);
             click_at(d, centre_in(g, "Filters", "On sale"), 0.6);
             d.wait(0.5);
             d.move_to(centre_in(g, "Filters", "Close"), 0.6);
             d.wait(1.2);
             d.click();
             d.wait(1.0);
             click_named(d, "Detached side sheet", 0.8);
             d.wait(1.0);
             const render::Rect sheet = bounds_of(g, "Detached filters");
             d.move_to({sheet.x + 60.0, sheet.y + 420.0}, 0.6);
             d.wait(0.3);
             d.fling_to({sheet.x + 220.0, sheet.y + 420.0}, 0.15);
             d.wait(1.0);
             click_named(d, "Standard side sheet", 0.8);
             d.wait(1.2);
             click_at(d, centre_in(g, "Standard filters", "Top rated"), 0.6);
             d.wait(0.6);
             click_named(d, "Standard side sheet", 0.8);
             d.wait(1.2);
         }},
        {"drawer",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Drawers");
             d.wait(0.4);
             click_named(d, "Open navigation menu", 0.8);
             d.wait(1.0);
             d.move_to("Favorites", 0.6);
             d.wait(0.4);
             d.click();
             d.wait(1.0);
             const render::Point menu = centre_nth(g, "Open navigation menu", 0);
             d.move_to({menu.x, menu.y + 200.0}, 0.6);
             d.drag_to({menu.x + 230.0, menu.y + 200.0}, 0.8);
             d.wait(1.0);
             click_at(d, {menu.x + 380.0, menu.y + 400.0}, 0.6);
             d.wait(1.0);
             click_named(d, "Toggle navigation menu", 0.8);
             d.wait(1.0);
             click_at(d, centre_nth(g, "Outbox", 0), 0.6);
             d.wait(0.6);
             click_named(d, "Toggle navigation menu", 0.6);
             d.wait(1.0);
             click_named(d, "Expand rail", 0.8);
             d.wait(1.0);
             click_named(d, "Messages", 0.6);
             d.wait(0.6);
             d.key(Key::escape);
             d.wait(1.2);
             click_named(d, "Hiding rail", 0.8);
             d.wait(1.0);
             const render::Rect rail = bounds_of(g, "Main");
             d.move_to({rail.x + 150.0, rail.y + 560.0}, 0.6);
             d.wait(0.3);
             d.fling_to({rail.x + 20.0, rail.y + 560.0}, 0.12);
             d.wait(1.2);
         }},
        {"search",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Search");
             d.wait(0.4);
             click_at(d, centre_field(g, 0), 0.8);
             d.wait(1.0);
             d.type("ka", 0.25);
             d.wait(0.8);
             d.key(Key::down);
             d.wait(0.4);
             d.key(Key::down);
             d.wait(0.4);
             d.key(Key::down);
             d.wait(0.4);
             d.key(Key::enter);
             d.wait(1.2);
             click_at(d, centre_field(g, 1), 0.8);
             d.wait(1.0);
             d.type("os", 0.25);
             d.wait(0.6);
             d.move_to(centre_in(g, "Search", "Osaka"), 0.6);
             d.wait(0.6);
             d.click();
             d.wait(1.2);
         }},
    };
    return scenes;
}

void hold_on(Driver& d, render::Point p, double move, double hold) {
    d.move_to(p, move);
    d.press();
    d.wait(hold);
    d.release();
}

void reveal_frame(Driver& d, examples::components::Gallery& g, const std::string& list) {
    for (int i = 0; i < d.style().reveal_notches; ++i) {
        const render::Rect r = bounds_of(g, list);
        if (r.y + r.height + d.style().reveal_margin <= d.window_height()) {
            return;
        }
        d.move_to({r.x - d.style().reveal_margin, r.y}, 0.0);
        d.wheel(1.0);
        d.wait(d.style().reveal_pause_seconds);
    }
}

void scroll_frame(Driver& d, examples::components::Gallery& g, const std::string& list, double dy, int notches, double pause) {
    const render::Rect r = bounds_of(g, list);
    d.move_to({r.x + r.width / 2.0, r.y + r.height * 0.4}, 0.5);
    for (int i = 0; i < notches; ++i) {
        d.wheel(dy);
        d.wait(pause);
    }
}

const std::map<std::string, Scene>& expressive_stills() {
    static const std::map<std::string, Scene> scenes{
        {"groups", [](Driver& d, examples::components::Gallery&) { show_page(d, "Groups"); }},
        {"groups_pressed",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Groups");
             d.move_to(centre_in(g, "Connected", "Month"), 0.4);
             d.press();
             d.wait(0.6);
         }},
        {"groups_overflow",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Groups");
             click_at(d, centre_in(g, "Overflow", "More options"), 0.4);
             d.wait(0.6);
             d.move_to(centre_in(g, "Overflow", "Layers"), 0.4);
         }},
        {"groups_split",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Groups");
             click_at(d, centre_in(g, "Medium split button", "More send options"), 0.4);
             d.wait(0.8);
             d.move_to(centre_in(g, "Medium split button", "Save draft"), 0.4);
         }},
        {"groups_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Groups");
             tab_to(d, g, "Coffee");
         }},
        {"groups_tooltip",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Groups");
             d.move_to(centre_in(g, "Small split button", "More send options"), 0.4);
             d.wait(1.2);
         }},
        {"groups_collapsed",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Groups");
             click_named(d, "Collapse", 0.4);
             d.wait(0.8);
         }},
        {"groups_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Groups");
             toggle_enabled(d);
         }},
        {"progress",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Progress");
             d.wait(1.3);
         }},
        {"progress_running",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Progress");
             click_named(d, "Run", 0.4);
             d.wait(1.9);
         }},
        {"progress_handoff",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Progress");
             click_named(d, "Indeterminate", 0.4);
             d.wait(1.0);
         }},
        {"toolbars", [](Driver& d, examples::components::Gallery&) { show_page(d, "Toolbars"); }},
        {"toolbars_scrolled",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Toolbars");
             scroll_frame(d, g, "Photos list", 1.0, 3, 0.15);
             scroll_frame(d, g, "Notes list", 1.0, 3, 0.15);
             scroll_frame(d, g, "Inbox list", 1.0, 3, 0.15);
             d.move_to({720.0, 845.0}, 0.3);
             d.wait(1.0);
         }},
        {"toolbars_menu",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Toolbars");
             click_at(d, centre_in(g, "New item", "New"), 0.4);
             d.wait(0.9);
             d.move_to(centre_in(g, "New item", "Meeting"), 0.4);
         }},
        {"toolbars_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Toolbars");
             tab_to(d, g, "Edit");
             d.key(Key::tab);
             d.wait(0.3);
         }},
        {"toolbars_tooltip",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Toolbars");
             d.move_to(centre_in(g, "Format", "Color"), 0.4);
             d.wait(1.2);
         }},
        {"toolbars_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Toolbars");
             toggle_enabled(d);
         }},
    };
    return scenes;
}

const std::map<std::string, Scene>& expressive_videos() {
    static const std::map<std::string, Scene> scenes{
        {"button_groups",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Groups");
             d.wait(0.4);
             for (const char* name : {"Day", "Month", "Year", "Week"}) {
                 hold_on(d, centre_in(g, "Connected", name), 0.5, 0.45);
                 d.wait(0.5);
             }
             for (const char* name : {"Food", "Coffee", "Work"}) {
                 hold_on(d, centre_in(g, "Connected icons", name), 0.5, 0.4);
                 d.wait(0.4);
             }
             for (const char* name : {"Bluetooth", "Location"}) {
                 hold_on(d, centre_in(g, "Standard", name), 0.5, 0.35);
                 d.wait(0.4);
             }
             click_at(d, centre_in(g, "Actions", "Copy"), 0.5);
             d.wait(0.5);
             click_at(d, centre_in(g, "Overflow", "More options"), 0.6);
             d.wait(0.8);
             click_at(d, centre_in(g, "Overflow", "Filter"), 0.6);
             d.wait(1.0);
         }},
        {"split_button",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Groups");
             d.wait(0.4);
             click_at(d, centre_in(g, "Small split button", "Send"), 0.6);
             d.wait(0.5);
             click_at(d, centre_in(g, "Small split button", "More send options"), 0.5);
             d.wait(1.0);
             click_at(d, centre_in(g, "Small split button", "Schedule send"), 0.6);
             d.wait(0.9);
             hold_on(d, centre_in(g, "Large split button", "Send"), 0.6, 0.5);
             d.wait(0.5);
             click_at(d, centre_in(g, "Large split button", "More send options"), 0.5);
             d.wait(1.0);
             d.move_to(centre_in(g, "Large split button", "Discard"), 0.5);
             d.wait(0.4);
             d.key(Key::escape);
             d.wait(0.9);
             click_at(d, centre_in(g, "Outlined split button", "Outlined options"), 0.6);
             d.wait(0.9);
             click_at(d, centre_in(g, "Outlined split button", "Outlined options"), 0.3);
             d.wait(1.0);
         }},
        {"fab_menu",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Toolbars");
             d.wait(0.4);
             click_at(d, centre_in(g, "New item", "New"), 0.8);
             d.wait(1.2);
             click_at(d, centre_in(g, "New item", "New"), 0.4);
             d.wait(1.0);
             click_at(d, centre_in(g, "New item", "New"), 0.4);
             d.wait(1.0);
             click_at(d, centre_in(g, "New item", "Chat"), 0.6);
             d.wait(1.2);
             d.move_to({720.0, 845.0}, 0.5);
             d.set_recording(false);
             tab_to(d, g, "New");
             d.set_recording(true);
             d.wait(0.4);
             d.key(Key::enter);
             d.wait(1.0);
             d.key(Key::down);
             d.wait(0.4);
             d.key(Key::tab);
             d.wait(0.4);
             d.key(Key::escape);
             d.wait(1.2);
         }},
        {"loading",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Progress");
             d.move_to({720.0, 845.0}, 0.0);
             d.set_cursor(false);
             d.wait(5.0);
             d.set_cursor(true);
             d.move_to(centre_by(g, [](const widgets::AccessibleNode& n) { return n.name == "Indicator value" && n.role == widgets::AccessibleRole::slider; }), 0.8);
             d.drag_to({d.pointer().x - 250.0, d.pointer().y}, 1.6);
             d.drag_to({d.pointer().x + 300.0, d.pointer().y}, 2.4);
             d.wait(0.8);
         }},
        {"wavy_progress",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Progress");
             d.wait(0.6);
             click_named(d, "Run", 0.6);
             d.wait(5.6);
             click_named(d, "Indeterminate", 0.6);
             d.wait(1.5);
             click_named(d, "Indeterminate", 0.4);
             d.wait(1.6);
         }},
        {"pull_refresh",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Progress");
             reveal_frame(d, g, "Refresh list");
             const render::Rect r = bounds_of(g, "Refresh list");
             const render::Point start{r.x + r.width / 2.0, r.y + d.style().pull_start};
             d.move_to(start, 0.4);
             d.touch_swipe({start.x, start.y + d.style().pull_short}, d.style().pull_seconds, d.style().pull_hold, true);
             d.wait(1.0);
             d.move_to(start, 0.3);
             d.touch_swipe({start.x, start.y + d.style().pull_long}, d.style().pull_seconds, d.style().pull_hold, true);
             d.wait(3.0);
             d.move_to(start, 0.3);
             d.touch_swipe({start.x, start.y + d.style().pull_long}, d.style().pull_seconds, d.style().pull_hold, false);
             d.touch_move_to(start, d.style().pull_seconds);
             d.touch_lift();
             d.wait(1.0);
             click_named(d, "Arrow indicator", 0.4);
             d.move_to(start, 0.3);
             d.touch_swipe({start.x, start.y + d.style().pull_short}, d.style().pull_seconds, d.style().pull_hold, true);
             d.wait(1.0);
             d.move_to(start, 0.3);
             d.touch_swipe({start.x, start.y + d.style().pull_long}, d.style().pull_seconds, d.style().pull_hold, true);
             d.wait(3.0);
         }},
        {"toolbars",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Toolbars");
             d.wait(0.4);
             scroll_frame(d, g, "Photos list", 1.0, 4, 0.18);
             d.wait(0.8);
             scroll_frame(d, g, "Photos list", -1.0, 3, 0.18);
             d.wait(0.8);
             click_at(d, centre_in(g, "Photo tools", "Favorite"), 0.5);
             d.wait(0.4);
             scroll_frame(d, g, "Notes list", 1.0, 4, 0.18);
             d.wait(0.8);
             scroll_frame(d, g, "Notes list", -1.0, 3, 0.18);
             d.wait(0.8);
             scroll_frame(d, g, "Inbox list", 1.0, 2, 0.25);
             d.wait(1.0);
             scroll_frame(d, g, "Inbox list", -1.0, 2, 0.25);
             d.wait(1.0);
         }},
    };
    return scenes;
}

render::Point slider_point(examples::components::Gallery& g, double value, double min, double max) {
    const auto nodes = g.accessible_nodes();
    const auto node = widgets::find_accessible(
        nodes, [](const widgets::AccessibleNode& n) { return n.name == "Window width" && n.role == widgets::AccessibleRole::slider; });
    if (!node) {
        throw std::runtime_error("no window width slider");
    }
    const render::Rect r = node->bounds;
    const double inset = r.height / 2.0;
    const double f = (value - min) / (max - min);
    return {r.x + inset + (r.width - 2.0 * inset) * f, r.y + r.height / 2.0};
}

render::Point field_point(examples::components::Gallery& g, const std::string& name) {
    const auto nodes = g.accessible_nodes();
    const auto node = widgets::find_accessible(
        nodes, [&](const widgets::AccessibleNode& n) { return n.name == name && n.role == widgets::AccessibleRole::spin_button; });
    if (!node) {
        throw std::runtime_error("no numeric field " + name);
    }
    return {node->bounds.x + node->bounds.width - 40.0, node->bounds.y + node->bounds.height / 2.0};
}

render::Point handle_point(examples::components::Gallery& g, const std::string& name) {
    const auto nodes = g.accessible_nodes();
    const auto node = widgets::find_accessible(
        nodes, [&](const widgets::AccessibleNode& n) { return n.name == name && n.role == widgets::AccessibleRole::separator; });
    if (!node) {
        throw std::runtime_error("no divider " + name);
    }
    return {node->bounds.x + node->bounds.width / 2.0, node->bounds.y + node->bounds.height / 2.0};
}

void preset(Driver& d, const char* name) {
    d.set_cursor(false);
    d.move_to(name, 0.05);
    d.click();
    d.move_to({720.0, 845.0}, 0.05);
    d.set_cursor(true);
    d.wait(1.2);
}

void editor_view(Driver& d) {
    show_page(d, "Desktop");
    d.set_cursor(false);
    d.move_to("Editor", 0.05);
    d.click();
    d.move_to({720.0, 845.0}, 0.05);
    d.set_cursor(true);
    d.wait(0.6);
}

const std::map<std::string, Scene>& adaptive_stills() {
    static const std::map<std::string, Scene> scenes{
        {"adaptive", [](Driver& d, examples::components::Gallery&) { show_page(d, "Adaptive"); }},
        {"adaptive_compact",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Adaptive");
             preset(d, "Phone");
         }},
        {"adaptive_medium",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Adaptive");
             preset(d, "Tablet");
         }},
        {"adaptive_expanded",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Adaptive");
             preset(d, "Laptop");
         }},
        {"adaptive_large",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Adaptive");
             preset(d, "Monitor");
         }},
        {"adaptive_icons",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Adaptive");
             preset(d, "Monitor");
             preset(d, "Icons");
         }},
        {"adaptive_detail",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Adaptive");
             preset(d, "Phone");
             click_named(d, "Trevor Hansen", 0.5);
             d.move_to({720.0, 845.0}, 0.3);
             d.wait(1.0);
         }},
        {"adaptive_supporting",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Adaptive");
             preset(d, "Laptop");
             click_named(d, "Supporting", 0.4);
             d.move_to({720.0, 845.0}, 0.3);
             d.wait(1.0);
         }},
        {"adaptive_handle",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Adaptive");
             preset(d, "Laptop");
             d.move_to(handle_point(g, "Pane expansion drag handle"), 0.5);
             d.press();
             d.move_to({d.pointer().x + 140.0, d.pointer().y}, 0.6);
             d.wait(0.3);
         }},
        {"adaptive_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Adaptive");
             preset(d, "Laptop");
             tab_to(d, g, "Pane expansion drag handle");
             d.wait(0.6);
         }},
        {"adaptive_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Adaptive");
             preset(d, "Laptop");
             click_named(d, "Enabled", 0.4);
             d.move_to({720.0, 845.0}, 0.3);
         }},
        {"desktop", [](Driver& d, examples::components::Gallery&) { show_page(d, "Desktop"); }},
        {"desktop_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Desktop");
             tab_to(d, g, "Range");
             d.wait(0.6);
         }},
        {"desktop_editor", [](Driver& d, examples::components::Gallery&) { editor_view(d); }},
        {"desktop_editor_scrolled",
         [](Driver& d, examples::components::Gallery& g) {
             editor_view(d);
             const render::Rect r = *g.find("Timeline");
             d.move_to({r.x + r.width * 0.6, r.y + r.height * 0.6}, 0.4);
             for (int i = 0; i < 8; ++i) {
                 d.shift_wheel(1.0);
                 d.wait(0.05);
             }
             d.wait(0.4);
         }},
        {"desktop_divider",
         [](Driver& d, examples::components::Gallery& g) {
             editor_view(d);
             d.move_to(handle_point(g, "Assets and stage"), 0.5);
             d.wait(0.4);
         }},
        {"desktop_editing",
         [](Driver& d, examples::components::Gallery& g) {
             editor_view(d);
             click_at(d, field_point(g, "Width"), 0.5);
             d.wait(0.3);
             d.type("12x", 0.1);
             d.key(Key::enter);
             d.wait(0.3);
             click_at(d, field_point(g, "Rotation"), 0.5);
             d.type("4-5", 0.1);
             d.key(Key::enter);
             d.move_to({d.pointer().x + 30.0, d.pointer().y + 6.0}, 0.3);
             d.wait(0.4);
         }},
        {"desktop_disabled",
         [](Driver& d, examples::components::Gallery&) {
             editor_view(d);
             click_named(d, "Enabled", 0.4);
             d.move_to({720.0, 845.0}, 0.3);
         }},
    };
    return scenes;
}

const std::map<std::string, Scene>& adaptive_videos() {
    static const std::map<std::string, Scene> scenes{
        {"adaptive_sweep",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Adaptive");
             preset(d, "Phone");
             d.move_to(slider_point(g, 412.0, 360.0, 1392.0), 0.8);
             d.wait(0.3);
             d.press();
             d.move_to(slider_point(g, 1392.0, 360.0, 1392.0), 9.0);
             d.wait(1.0);
             d.move_to(slider_point(g, 760.0, 360.0, 1392.0), 4.0);
             d.wait(0.8);
             d.release();
             d.wait(0.6);
         }},
        {"adaptive_panes",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Adaptive");
             preset(d, "Phone");
             click_named(d, "Trevor Hansen", 0.6);
             d.wait(1.2);
             click_named(d, "Back", 0.6);
             d.wait(1.0);
             click_named(d, "Britta Holt", 0.6);
             d.wait(1.0);
             d.key(Key::escape);
             d.wait(1.0);
             d.move_to(slider_point(g, 412.0, 360.0, 1392.0), 0.6);
             d.drag_to(slider_point(g, 1000.0, 360.0, 1392.0), 2.0);
             d.wait(1.0);
             click_named(d, "Sandra Adams", 0.6);
             d.wait(1.2);
         }},
        {"pane_divider",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Adaptive");
             preset(d, "Laptop");
             d.move_to(handle_point(g, "Pane expansion drag handle"), 0.8);
             d.wait(0.4);
             d.press();
             d.move_to({d.pointer().x + 160.0, d.pointer().y}, 1.0);
             d.wait(0.2);
             d.release();
             d.wait(1.2);
             d.move_to(handle_point(g, "Pane expansion drag handle"), 0.5);
             d.fling_to({d.pointer().x + 120.0, d.pointer().y}, 0.12);
             d.wait(1.4);
             d.move_to(handle_point(g, "Pane expansion drag handle"), 0.5);
             d.press();
             d.move_to({d.pointer().x - 380.0, d.pointer().y}, 1.4);
             d.wait(0.2);
             d.release();
             d.wait(1.2);
             d.move_to({720.0, 845.0}, 0.5);
             d.set_recording(false);
             tab_to(d, g, "Pane expansion drag handle");
             d.set_recording(true);
             d.wait(0.6);
             for (int i = 0; i < 3; ++i) {
                 d.key(Key::enter);
                 d.wait(1.0);
             }
             for (int i = 0; i < 4; ++i) {
                 d.key(Key::left);
                 d.wait(0.25);
             }
             d.wait(0.8);
         }},
        {"numeric_scrub",
         [](Driver& d, examples::components::Gallery& g) {
             editor_view(d);
             const render::Point x = field_point(g, "X");
             d.move_to(x, 0.8);
             d.wait(0.3);
             d.press();
             d.move_to({x.x + 120.0, x.y}, 1.6);
             d.move_to({x.x - 60.0, x.y}, 1.4);
             d.wait(0.2);
             d.release();
             d.wait(0.5);
             const render::Point o = field_point(g, "Opacity");
             d.move_to(o, 0.6);
             d.press();
             d.move_to({o.x - 200.0, o.y}, 1.4);
             d.release();
             d.wait(0.5);
             click_at(d, field_point(g, "Duration"), 0.6);
             d.wait(0.3);
             for (int i = 0; i < 4; ++i) {
                 d.key(Key::up);
                 d.wait(0.25);
             }
             d.key(Key::up, true);
             d.wait(0.4);
             d.type("12.25", 0.12);
             d.key(Key::enter);
             d.wait(0.8);
         }},
        {"timeline_scroll",
         [](Driver& d, examples::components::Gallery& g) {
             editor_view(d);
             const render::Rect r = *g.find("Timeline");
             d.move_to({r.x + r.width * 0.6, r.y + r.height * 0.55}, 0.8);
             for (int i = 0; i < 10; ++i) {
                 d.shift_wheel(1.0);
                 d.wait(0.12);
             }
             d.wait(0.6);
             for (int i = 0; i < 16; ++i) {
                 d.wheel_sideways(-0.5);
                 d.wait(0.04);
             }
             d.wait(0.6);
             const render::Rect t = *g.find("Timeline");
             d.move_to({t.x + 160.0, t.y + t.height - 4.0}, 0.6);
             d.wait(0.4);
             d.drag_to({t.x + 560.0, t.y + t.height - 4.0}, 1.4);
             d.wait(0.6);
             d.move_to(handle_point(g, "Stage and timeline"), 0.7);
             d.wait(0.3);
             d.drag_to({d.pointer().x, d.pointer().y + 90.0}, 1.0);
             d.wait(0.4);
             d.move_to(handle_point(g, "Assets and stage"), 0.7);
             d.wait(0.3);
             d.drag_to({d.pointer().x + 100.0, d.pointer().y}, 1.0);
             d.wait(0.8);
         }},
        {"density",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Desktop");
             d.wait(0.6);
             for (const char* s : {"Density 0", "Density -1", "Density -2", "Density -3"}) {
                 d.move_to(centre_in(g, s, "Week"), 0.5);
                 d.click();
                 d.wait(0.3);
             }
             d.wait(0.6);
         }},
    };
    return scenes;
}

void files_view(Driver& d) {
    show_page(d, "Desktop");
    d.set_cursor(false);
    d.move_to("Files", 0.05);
    d.click();
    d.move_to({720.0, 845.0}, 0.05);
    d.set_cursor(true);
    d.wait(0.6);
}

widgets::AccessibleNode table_of(examples::components::Gallery& g) {
    const auto nodes = g.accessible_nodes();
    const auto node = widgets::find_accessible(nodes, [](const widgets::AccessibleNode& n) { return n.role == widgets::AccessibleRole::table; });
    if (!node) {
        throw std::runtime_error("no table");
    }
    return *node;
}

render::Point table_row(examples::components::Gallery& g, std::size_t nth, double x_offset = 160.0) {
    const widgets::AccessibleNode t = table_of(g);
    if (nth + 1 >= t.children.size()) {
        throw std::runtime_error("the table shows fewer rows");
    }
    const render::Rect r = t.children[nth + 1].bounds;
    return {std::max(r.x, t.bounds.x) + x_offset, r.y + r.height / 2.0};
}

render::Rect column_header(examples::components::Gallery& g, const std::string& name) {
    const widgets::AccessibleNode t = table_of(g);
    for (const widgets::AccessibleNode& h : t.children.front().children) {
        if (h.name == name) {
            return h.bounds;
        }
    }
    throw std::runtime_error("no column " + name);
}

render::Point menu_entry(examples::components::Gallery& g, const std::string& name) {
    const auto nodes = g.accessible_nodes();
    std::optional<widgets::AccessibleNode> found;
    widgets::visit_accessible(nodes, [&](const widgets::AccessibleNode& n) {
        if (n.role == widgets::AccessibleRole::menu_item && n.name == name) {
            found = n;
        }
    });
    if (!found) {
        throw std::runtime_error("no menu item " + name);
    }
    return {found->bounds.x + std::min(60.0, found->bounds.width / 2.0), found->bounds.y + found->bounds.height / 2.0};
}

void files_menus_by_pointer(Driver& d, examples::components::Gallery& g) {
    d.move_to(menu_entry(g, "File"), 0.6);
    d.click();
    d.wait(0.6);
    d.move_to(menu_entry(g, "Open recent"), 0.5);
    d.wait(0.9);
    d.move_to(menu_entry(g, "budget.xlsx"), 0.5);
    d.wait(0.6);
    d.move_to(menu_entry(g, "Edit"), 0.5);
    d.wait(0.7);
    d.move_to(menu_entry(g, "View"), 0.4);
    d.wait(0.5);
    d.move_to(menu_entry(g, "Help"), 0.4);
    d.wait(0.5);
    d.move_to(menu_entry(g, "View"), 0.4);
    d.wait(0.4);
    d.move_to(menu_entry(g, "Density"), 0.4);
    d.wait(0.9);
    d.move_to(menu_entry(g, "Dense"), 0.5);
    d.wait(0.3);
    d.click();
    d.wait(1.0);
}

void files_menus_by_keyboard(Driver& d) {
    d.key(Key::alt);
    d.wait(0.6);
    d.key(Key::right);
    d.wait(0.4);
    d.key(Key::down);
    d.wait(0.6);
    for (int i = 0; i < 3; ++i) {
        d.key(Key::down);
        d.wait(0.3);
    }
    d.key(Key::right);
    d.wait(0.6);
    d.key(Key::down);
    d.wait(0.3);
    d.key(Key::right);
    d.wait(0.6);
    d.key(Key::left);
    d.wait(0.5);
    d.key(Key::escape);
    d.wait(0.5);
    d.key(Key::escape);
    d.wait(0.5);
    d.chord(Key::f, false, false, true, "Alt+F");
    d.wait(0.6);
    d.key(Key::r);
    d.wait(0.6);
    d.key(Key::down);
    d.wait(0.3);
    d.key(Key::enter);
    d.wait(0.8);
    d.chord(Key::s, false, true, false, "Ctrl+S");
    d.wait(0.8);
    d.chord(Key::s, true, true, false, "Ctrl+Shift+S");
    d.wait(1.0);
}

render::Rect viewer_rect(const Driver& d) {
    const std::optional<render::Rect> r = d.find(d.style().viewer_target);
    if (!r) {
        throw std::runtime_error("no widget named " + std::string(d.style().viewer_target));
    }
    return *r;
}

void viewer_pinch(Driver& d) {
    const CaptureStyle& s = d.style();
    const render::Rect r = viewer_rect(d);
    const render::Point c{r.x + r.width / 2.0, r.y + r.height / 2.0};
    const double from = r.width * s.pinch_from;
    const double to = r.width * s.pinch_to;
    d.fingers_to({{c.x - from, c.y}, {c.x + from, c.y}}, {{c.x - to, c.y}, {c.x + to, c.y}}, s.pinch_seconds, s.finger_tap_seconds);
}

void viewer_pan(Driver& d) {
    const CaptureStyle& s = d.style();
    const render::Rect r = viewer_rect(d);
    const render::Point c{r.x + r.width / 2.0, r.y + r.height / 2.0};
    d.fingers_to({c}, {{c.x - r.width * s.pan_fraction, c.y}}, s.pan_seconds, 0.0);
}

void viewer_double_tap(Driver& d, double fx, double fy) {
    const render::Rect r = viewer_rect(d);
    const render::Point p{r.x + r.width * fx, r.y + r.height * fy};
    d.finger_tap(p);
    d.wait(d.style().double_tap_gap_seconds);
    d.finger_tap(p);
}

void page_swipe(Driver& d, bool lift) {
    const CaptureStyle& s = d.style();
    const double y = d.window_height() * s.swipe_height;
    d.set_pointer({d.window_width() * s.swipe_from, y});
    d.touch_swipe({d.window_width() * s.swipe_to, y}, s.swipe_seconds, 0.0, lift);
}

render::Rect photos_rect(Driver& d) {
    const CaptureStyle& s = d.style();
    const double x = d.window_width() / 2.0;
    d.set_pointer({x, d.window_height() * s.page_scroll_from});
    d.touch_swipe({x, d.window_height() * s.page_scroll_to}, s.page_scroll_seconds, s.swipe_hold_seconds, true);
    d.settle();
    const std::optional<render::Rect> r = d.find(s.photos_target);
    if (!r) {
        throw std::runtime_error("no widget named " + std::string(s.photos_target));
    }
    return *r;
}

void photo_swipe(Driver& d, const render::Rect& r, bool lift) {
    const CaptureStyle& s = d.style();
    const double y = r.y + r.height / 2.0;
    d.set_pointer({r.x + r.width * s.swipe_from, y});
    d.touch_swipe({r.x + r.width * s.swipe_to, y}, s.swipe_seconds, 0.0, lift);
}

const std::map<std::string, Scene>& files_stills() {
    static const std::map<std::string, Scene> scenes{
        {"pages_swiping", [](Driver& d, examples::components::Gallery&) { page_swipe(d, false); }},
        {"pages_swiped",
         [](Driver& d, examples::components::Gallery&) {
             page_swipe(d, true);
             d.settle();
         }},
        {"viewer_photos",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Viewer");
             photos_rect(d);
         }},
        {"viewer_photo_swiping",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Viewer");
             photo_swipe(d, photos_rect(d), false);
         }},
        {"viewer_photo_swiped",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Viewer");
             photo_swipe(d, photos_rect(d), true);
             d.settle();
         }},
        {"files", [](Driver& d, examples::components::Gallery&) { files_view(d); }},
        {"viewer", [](Driver& d, examples::components::Gallery&) { show_page(d, "Viewer"); }},
        {"viewer_zoomed",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Viewer");
             viewer_pinch(d);
             d.settle();
         }},
        {"viewer_panned",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Viewer");
             viewer_pinch(d);
             d.settle();
             viewer_pan(d);
             d.settle();
         }},
        {"viewer_double_tap",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Viewer");
             viewer_double_tap(d, 0.25, 0.25);
             d.settle();
         }},
        {"viewer_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Viewer");
             click_named(d, "Enabled", 0.4);
             d.move_to({720.0, 845.0}, 0.3);
             viewer_pinch(d);
             d.settle();
         }},
        {"files_file_menu",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             d.move_to(menu_entry(g, "File"), 0.3);
             d.click();
             d.wait(0.4);
             d.move_to(menu_entry(g, "Open recent"), 0.3);
             d.wait(0.8);
             d.move_to(menu_entry(g, "budget.xlsx"), 0.3);
         }},
        {"files_edit_menu",
         [](Driver& d, examples::components::Gallery&) {
             files_view(d);
             d.chord(Key::e, false, false, true, "Alt+E");
             d.wait(0.3);
             d.key(Key::down);
             d.key(Key::down);
             d.key(Key::down);
         }},
        {"files_context_edge",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             const widgets::AccessibleNode t = table_of(g);
             d.move_to({t.bounds.x + t.bounds.width - 12.0, t.bounds.y + t.bounds.height - 12.0}, 0.4);
             d.right_click();
             d.wait(0.3);
             d.move_to(menu_entry(g, "Copy"), 0.3);
         }},
        {"files_sorted",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             const render::Rect size = column_header(g, "Size");
             d.move_to({size.x + size.width / 2.0, size.y + size.height / 2.0}, 0.3);
             d.click();
             d.wait(0.2);
             d.click();
             d.wait(0.3);
             d.move_to(table_row(g, 1), 0.3);
             d.click();
             d.move_to(table_row(g, 3), 0.3);
             d.modified_click(false, true);
             d.move_to(table_row(g, 5), 0.3);
             d.modified_click(false, true);
             d.move_to(table_row(g, 8), 0.3);
             d.modified_click(true, true);
             d.move_to({720.0, 845.0}, 0.2);
         }},
        {"files_resized",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             const render::Rect name = column_header(g, "Name");
             d.move_to({name.x + name.width - 1.0, name.y + name.height / 2.0}, 0.3);
             d.drag_to({name.x + name.width + 140.0, name.y + name.height / 2.0}, 0.6);
             const render::Rect kind = column_header(g, "Kind");
             d.move_to({kind.x + kind.width - 1.0, kind.y + kind.height / 2.0}, 0.3);
             d.drag_to({kind.x + kind.width - 50.0, kind.y + kind.height / 2.0}, 0.4);
         }},
        {"files_scrolled",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             d.move_to(table_row(g, 2), 0.3);
             d.click();
             d.key(Key::end);
             d.wait(0.4);
             d.key(Key::up, true);
             d.key(Key::up, true);
         }},
        {"files_dense",
         [](Driver& d, examples::components::Gallery&) {
             files_view(d);
             d.chord(Key::v, false, false, true, "Alt+V");
             d.key(Key::right);
             d.key(Key::down);
             d.key(Key::down);
             d.key(Key::enter);
             d.wait(0.6);
         }},
        {"files_disabled",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             d.move_to(table_row(g, 1), 0.3);
             d.click();
             toggle_enabled(d);
         }},
    };
    return scenes;
}

const std::map<std::string, Scene>& files_videos() {
    static const std::map<std::string, Scene> scenes{
        {"pages_swipe",
         [](Driver& d, examples::components::Gallery&) {
             d.wait(0.4);
             page_swipe(d, true);
             d.wait(0.8);
             page_swipe(d, true);
             d.wait(0.8);
             show_page(d, "Basics");
             d.wait(0.4);
         }},
        {"viewer_photos",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Viewer");
             const render::Rect r = photos_rect(d);
             d.wait(0.4);
             photo_swipe(d, r, true);
             d.wait(0.8);
             photo_swipe(d, r, true);
             d.wait(0.8);
         }},
        {"viewer_pinch",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Viewer");
             viewer_pinch(d);
             d.wait(0.6);
             viewer_pan(d);
             d.wait(1.0);
             viewer_double_tap(d, 0.5, 0.5);
             d.wait(0.8);
             viewer_double_tap(d, 0.25, 0.25);
             d.wait(0.8);
         }},
        {"files_menubar",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             files_menus_by_pointer(d, g);
             d.move_to({720.0, 845.0}, 0.4);
             files_menus_by_keyboard(d);
         }},
        {"files_context",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             d.move_to(table_row(g, 3), 0.6);
             d.right_click();
             d.wait(1.0);
             d.key(Key::escape);
             d.wait(0.4);
             const widgets::AccessibleNode t = table_of(g);
             d.move_to({t.bounds.x + t.bounds.width - 14.0, t.bounds.y + t.bounds.height - 14.0}, 1.0);
             d.right_click();
             d.wait(1.0);
             d.move_to(menu_entry(g, "Copy"), 0.6);
             d.wait(0.3);
             d.click();
             d.wait(0.8);
             d.move_to(table_row(g, 0, 420.0), 0.8);
             d.right_click();
             d.wait(0.6);
             d.move_to(menu_entry(g, "Paste"), 0.5);
             d.wait(0.3);
             d.click();
             d.wait(0.6);
             d.move_to(table_row(g, 5), 0.6);
             d.click();
             d.key(Key::down);
             d.wait(0.3);
             d.chord(Key::f10, true, false, false, "Shift+F10");
             d.wait(0.8);
             d.key(Key::down);
             d.wait(0.3);
             d.key(Key::enter);
             d.wait(0.8);
             d.key(Key::menu);
             d.wait(0.8);
             d.key(Key::escape);
             d.wait(0.6);
         }},
        {"files_table",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             const render::Rect size = column_header(g, "Size");
             d.move_to({size.x + size.width / 2.0, size.y + size.height / 2.0}, 0.8);
             d.wait(0.4);
             d.click();
             d.wait(0.8);
             d.click();
             d.wait(0.8);
             const render::Rect modified = column_header(g, "Modified");
             d.move_to({modified.x + 40.0, modified.y + modified.height / 2.0}, 0.6);
             d.click();
             d.wait(0.8);
             const render::Rect name = column_header(g, "Name");
             d.move_to({name.x + name.width - 1.0, name.y + name.height / 2.0}, 0.8);
             d.wait(0.3);
             d.drag_to({name.x + name.width + 120.0, name.y + name.height / 2.0}, 1.0);
             d.wait(0.3);
             const render::Rect kind = column_header(g, "Kind");
             d.move_to({kind.x + kind.width - 1.0, kind.y + kind.height / 2.0}, 0.6);
             d.drag_to({kind.x + kind.width - 60.0, kind.y + kind.height / 2.0}, 0.8);
             d.wait(0.4);
             d.move_to(table_row(g, 1), 0.6);
             d.click();
             d.wait(0.4);
             d.move_to(table_row(g, 3), 0.4);
             d.modified_click(false, true);
             d.wait(0.4);
             d.move_to(table_row(g, 4), 0.4);
             d.modified_click(false, true);
             d.wait(0.4);
             d.move_to(table_row(g, 8), 0.4);
             d.modified_click(true, true);
             d.wait(0.6);
             d.move_to(table_row(g, 3), 0.4);
             d.modified_click(false, true);
             d.wait(0.6);
             d.move_to(table_row(g, 6), 0.4);
             d.modified_click(true, false);
             d.wait(0.8);
             d.key(Key::down, true);
             d.wait(0.3);
             d.key(Key::down, true);
             d.wait(0.6);
             d.move_to(table_row(g, 4), 0.6);
             for (double dy : {1.0, 1.0, 1.0, -1.0, -1.0, -1.0}) {
                 d.shift_wheel(dy);
                 d.wait(0.25);
             }
             d.wait(0.6);
         }},
        {"files_scroll",
         [](Driver& d, examples::components::Gallery& g) {
             files_view(d);
             d.move_to(table_row(g, 2), 0.6);
             for (int i = 0; i < 12; ++i) {
                 d.wheel(1.0);
                 d.wait(0.08);
             }
             d.wait(0.6);
             const widgets::AccessibleNode t = table_of(g);
             const render::Point thumb{t.bounds.x + t.bounds.width - 5.0, t.bounds.y + t.children.front().bounds.height + 8.0};
             d.move_to(thumb, 0.8);
             d.wait(0.4);
             d.drag_to({thumb.x, t.bounds.y + t.bounds.height * 0.5}, 1.6);
             d.wait(0.6);
             d.move_to(table_row(g, 3), 0.6);
             d.click();
             d.wait(0.3);
             d.key(Key::page_down);
             d.wait(0.4);
             d.key(Key::page_down);
             d.wait(0.4);
             d.key(Key::end);
             d.wait(0.8);
             d.key(Key::page_up, true);
             d.wait(0.8);
             d.key(Key::home);
             d.wait(1.0);
         }},
    };
    return scenes;
}

render::Point font_button(examples::components::Gallery& g) {
    const auto nodes = g.accessible_nodes();
    std::optional<widgets::AccessibleNode> found;
    widgets::visit_accessible(nodes, [&](const widgets::AccessibleNode& n) {
        if (n.role == widgets::AccessibleRole::button && n.name.starts_with("Font, ")) {
            found = n;
        }
    });
    if (!found) {
        throw std::runtime_error("no font button");
    }
    return {found->bounds.x + found->bounds.width / 2.0, found->bounds.y + found->bounds.height / 2.0};
}

void choose_font(Driver& d, examples::components::Gallery& g, const std::string& name) {
    d.move_to(font_button(g), 0.8);
    d.wait(0.2);
    d.click();
    d.wait(0.6);
    d.move_to(menu_entry(g, name), 0.6);
    d.wait(0.3);
    d.click();
    d.wait(1.4);
}

void tour_pages(Driver& d) {
    d.move_to({1100.0, 90.0}, 0.5);
    d.move_to("Dates", 0.5);
    d.click();
    d.wait(0.5);
    d.move_to("日本語", 0.6);
    d.click();
    d.wait(1.2);
    d.move_to("Desktop", 0.7);
    d.click();
    d.wait(0.5);
    d.move_to("Files", 0.6);
    d.click();
    d.wait(1.2);
    d.move_to("Basics", 0.7);
    d.click();
    d.wait(0.6);
}

void sweep_width(Driver& d, double to, double seconds) {
    const double from = d.window_width();
    const int n = std::max(1, static_cast<int>(std::lround(seconds * d.fps())));
    for (int i = 1; i <= n; ++i) {
        const double t = static_cast<double>(i) / n;
        const double k = t * t * (3.0 - 2.0 * t);
        d.set_window_width(from + (to - from) * k);
        d.frame();
    }
}

const std::map<std::string, Scene>& gallery_videos() {
    static const std::map<std::string, Scene> scenes{
        {"gallery_pages",
         [](Driver& d, examples::components::Gallery&) {
             d.wait(0.6);
             for (const char* name : {"Chips", "Dates", "Sheets", "Progress", "Adaptive", "Desktop", "Basics"}) {
                 d.move_to(name, 0.6);
                 d.click();
                 d.wait(1.0);
             }
         }},
        {"gallery_pages_keys",
         [](Driver& d, examples::components::Gallery& g) {
             d.wait(0.6);
             tab_to(d, g, "Pages");
             d.wait(0.6);
             const auto nodes = g.accessible_nodes();
             const auto row = widgets::find_accessible(
                 nodes, [](const widgets::AccessibleNode& n) { return n.name == "Pages" && n.role == widgets::AccessibleRole::tab_list; });
             if (!row) {
                 throw std::runtime_error("no page tabs");
             }
             for (std::size_t i = 1; i < row->children.size(); ++i) {
                 d.key(Key::right);
                 d.wait(0.5);
             }
             d.key(Key::home, false, "Home");
             d.wait(1.2);
         }},
        {"gallery_resize",
         [](Driver& d, examples::components::Gallery&) {
             const CaptureStyle& style = d.style();
             const auto below = [&style](layout::WindowWidthClass c) {
                 return layout::window_breakpoints.width[static_cast<std::size_t>(c)] - style.sweep_below_breakpoint;
             };
             d.wait(0.6);
             for (const double w : {below(layout::WindowWidthClass::large), below(layout::WindowWidthClass::expanded),
                                    below(layout::WindowWidthClass::medium), style.sweep_narrowest}) {
                 if (w < d.window_width()) {
                     sweep_width(d, w, style.sweep_step_seconds);
                     d.wait(0.8);
                 }
             }
             sweep_width(d, d.stage_width(), style.sweep_back_seconds);
             d.wait(0.8);
         }},
        {"times_resize",
         [](Driver& d, examples::components::Gallery&) {
             const CaptureStyle& style = d.style();
             show_page(d, "Times");
             d.wait(0.6);
             sweep_width(d, style.sweep_narrowest, style.sweep_back_seconds);
             d.wait(0.8);
             sweep_width(d, d.stage_width(), style.sweep_back_seconds);
             d.wait(0.8);
         }},
    };
    return scenes;
}

const std::map<std::string, Scene>& font_stills() {
    static const std::map<std::string, Scene> scenes{
        {"fonts_menu",
         [](Driver& d, examples::components::Gallery& g) {
             d.move_to(font_button(g), 0.3);
             d.click();
             d.wait(0.6);
             d.move_to(menu_entry(g, "Roboto Flex"), 0.3);
         }},
    };
    return scenes;
}

const std::map<std::string, Scene>& font_videos() {
    static const std::map<std::string, Scene> scenes{
        {"fonts_switch",
         [](Driver& d, examples::components::Gallery& g) {
             d.wait(0.8);
             for (const char* name : {"Roboto Flex", "OS fonts", "Google Sans Flex"}) {
                 choose_font(d, g, name);
                 tour_pages(d);
             }
             d.wait(0.6);
         }},
        {"fonts_keyboard",
         [](Driver& d, examples::components::Gallery& g) {
             d.move_to({1180.0, 470.0}, 0.4);
             d.wait(0.6);
             tab_to(d, g, "Font, Google Sans Flex");
             d.wait(0.6);
             const int downs[] = {1, 2, 0};
             for (int n : downs) {
                 d.key(Key::enter);
                 d.wait(0.6);
                 for (int i = 0; i < n; ++i) {
                     d.key(Key::down);
                     d.wait(0.35);
                 }
                 d.key(Key::enter);
                 d.wait(2.4);
             }
         }},
        {"fonts_japanese",
         [](Driver& d, examples::components::Gallery&) {
             d.wait(0.4);
             d.move_to("Name", 0.6);
             d.click();
             d.wait(0.4);
             d.type("山田太郎（例）", 0.18);
             d.wait(0.6);
             d.key(Key::tab);
             d.wait(0.4);
             d.type("東京での予定 Tokyo 2026", 0.12);
             d.wait(0.8);
             d.move_to("Dialog", 0.7);
             d.click();
             d.wait(0.8);
             d.move_to("Email", 0.6);
             d.click();
             d.type("例えば user@example.com", 0.1);
             d.wait(1.0);
             d.key(Key::escape);
             d.wait(0.8);
             d.move_to("Dates", 0.7);
             d.click();
             d.wait(0.5);
             d.move_to("日本語", 0.6);
             d.click();
             d.wait(1.6);
         }},
    };
    return scenes;
}

const Scene* find_scene(std::initializer_list<const std::map<std::string, Scene>*> maps, const std::string& name) {
    for (const auto* m : maps) {
        if (const auto it = m->find(name); it != m->end()) {
            return &it->second;
        }
    }
    return nullptr;
}

bool contains_point(const render::Rect& r, double x, double y) {
    return x >= r.x && x <= r.x + r.width && y >= r.y && y <= r.y + r.height;
}

int block_of(const std::vector<render::Rect>& blocks, const render::Rect& r) {
    const double cx = r.x + r.width / 2.0;
    const double cy = r.y + r.height / 2.0;
    int best = -1;
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        if (contains_point(blocks[i], cx, cy) &&
            (best < 0 || blocks[i].width * blocks[i].height < blocks[static_cast<std::size_t>(best)].width * blocks[static_cast<std::size_t>(best)].height)) {
            best = static_cast<int>(i);
        }
    }
    return best;
}

struct PageCheck {
    std::vector<std::string> problems;
    std::vector<std::string> known;
};

PageCheck check_page(const std::string& scene, examples::components::Gallery& g, double window_width, const CaptureStyle& style) {
    const double tolerance = style.overlap_tolerance;
    std::vector<std::string> problems;
    std::vector<std::string> known;
    auto listed = [&](std::string_view name) {
        return std::any_of(style.known_overflows.begin(), style.known_overflows.end(), [&](const auto& k) { return k.first == scene && k.second == name; });
    };
    auto describe = [](const widgets::AccessibleNode& n) {
        return std::string(widgets::role_name(n.role)) + " \"" + n.name + "\" at " + std::to_string(static_cast<int>(n.bounds.x)) + "," +
               std::to_string(static_cast<int>(n.bounds.y)) + " " + std::to_string(static_cast<int>(n.bounds.width)) + "x" +
               std::to_string(static_cast<int>(n.bounds.height));
    };
    std::vector<widgets::AccessibleNode> nodes;
    for (widgets::AccessibleNode& n : g.page_nodes()) {
        const bool wide = n.bounds.width > tolerance;
        const bool tall = n.bounds.height > tolerance;
        if (wide != tall) {
            problems.push_back(scene + ": " + describe(n) + " is squeezed to nothing");
        } else if (wide) {
            nodes.push_back(std::move(n));
        }
    }
    const std::vector<render::Rect> blocks = g.layout_blocks();
    for (const render::Rect& b : blocks) {
        if (b.x + b.width > window_width + tolerance) {
            problems.push_back(scene + ": a block at " + std::to_string(static_cast<int>(b.x)) + "," + std::to_string(static_cast<int>(b.y)) + " " +
                               std::to_string(static_cast<int>(b.width)) + " wide passes the right edge");
        }
    }
    double content_left = window_width;
    for (const widgets::AccessibleNode& n : nodes) {
        content_left = std::min(content_left, n.bounds.x);
    }
    for (const auto& [page, name] : style.known_overflows) {
        if (page == scene && std::none_of(nodes.begin(), nodes.end(), [name = name](const widgets::AccessibleNode& n) { return n.name == name; })) {
            problems.push_back(scene + ": the known overflow \"" + std::string(name) + "\" is no longer on the page");
        }
    }
    std::vector<int> groups;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const widgets::AccessibleNode& n = nodes[i];
        if (n.bounds.x + n.bounds.width > window_width + tolerance) {
            if (listed(n.name) && n.bounds.x <= content_left + tolerance) {
                known.push_back(scene + ": " + describe(n) + " is wider than the room from the content's left edge");
            } else {
                problems.push_back(scene + ": " + describe(n) + " passes the right edge");
            }
        }
        const int b = block_of(blocks, n.bounds);
        groups.push_back(b >= 0 ? b : -1 - static_cast<int>(i));
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        for (std::size_t k = i + 1; k < nodes.size(); ++k) {
            if (groups[i] == groups[k]) {
                continue;
            }
            const render::Rect& a = nodes[i].bounds;
            const render::Rect& b = nodes[k].bounds;
            const double w = std::min(a.x + a.width, b.x + b.width) - std::max(a.x, b.x);
            const double h = std::min(a.y + a.height, b.y + b.height) - std::max(a.y, b.y);
            if (w > tolerance && h > tolerance) {
                problems.push_back(scene + ": " + describe(nodes[i]) + " overlaps " + describe(nodes[k]));
            }
        }
    }
    return {problems, known};
}

std::optional<std::string> check_tap_redraw(Driver& d, const CaptureStyle& style) {
    d.set_cursor(false);
    d.set_pointer(d.centre_of(style.redraw_target));
    d.touch_tap();
    int frames = 0;
    while (d.animating() && frames < style.redraw_frame_limit) {
        d.frame();
        ++frames;
    }
    if (d.animating()) {
        return "tap redraw: still animating " + std::to_string(frames) + " frames after a touch tap on " + std::string(style.redraw_target);
    }
    const std::vector<std::uint8_t> idle = d.pixels();
    d.frame();
    const std::vector<std::uint8_t> next = d.pixels();
    int worst = 0;
    for (std::size_t i = 0; i < idle.size() && i < next.size(); ++i) {
        worst = std::max(worst, std::abs(static_cast<int>(idle[i]) - static_cast<int>(next[i])));
    }
    if (worst > style.redraw_tolerance) {
        return "tap redraw: a touch tap on " + std::string(style.redraw_target) + " left the frame idle, but the next frame differs by " +
               std::to_string(worst) + " levels, so the tap's result stays off screen until the next input";
    }
    return std::nullopt;
}

int settled_frame_change(Driver& d, const CaptureStyle& style) {
    int frames = 0;
    while (d.animating() && frames < style.redraw_frame_limit) {
        d.frame();
        ++frames;
    }
    const std::vector<std::uint8_t> idle = d.pixels();
    d.frame();
    const std::vector<std::uint8_t> next = d.pixels();
    int worst = 0;
    for (std::size_t i = 0; i < idle.size() && i < next.size(); ++i) {
        worst = std::max(worst, std::abs(static_cast<int>(idle[i]) - static_cast<int>(next[i])));
    }
    return worst;
}

std::optional<std::string> check_fling_redraw(Driver& d, const CaptureStyle& style) {
    d.set_cursor(false);
    const double x = d.window_width() / 2.0;
    for (int i = 0; i < style.fling_redraw_flings; ++i) {
        d.set_pointer({x, d.window_height() * style.fling_redraw_from});
        d.touch_fling_to({x, d.window_height() * style.fling_redraw_to}, style.fling_redraw_seconds);
        const int worst = settled_frame_change(d, style);
        if (d.animating()) {
            return "fling redraw: still animating after fling " + std::to_string(i + 1);
        }
        if (worst > style.redraw_tolerance) {
            return "fling redraw: fling " + std::to_string(i + 1) + " left the frame idle, but the next frame differs by " + std::to_string(worst) +
                   " levels, so the page stays drawn where the fling's last step began until the next input";
        }
    }
    return std::nullopt;
}

std::optional<std::string> check_dark_theme_host(const examples::components::GalleryFonts& fonts, const icons::IconFont& icon_font) {
    for (const bool start : {false, true}) {
        HeadlessHost host;
        examples::components::Gallery gallery(fonts, icon_font, host, examples::components::GalleryStart{start});
        if (host.dark_theme() != start) {
            return std::string("dark theme: a gallery started ") + (start ? "dark" : "light") + " did not tell its host";
        }
        gallery.set_dark(!start);
        if (host.dark_theme() != !start) {
            return std::string("dark theme: switching a gallery started ") + (start ? "dark" : "light") + " did not tell its host";
        }
    }
    return std::nullopt;
}

std::optional<std::string> check_back(const examples::components::GalleryFonts& gallery_fonts, const text::FontStack& fonts,
                                      const icons::IconFont& icon_font, WindowSize window, const CaptureStyle& style) {
    auto run = [&](std::string_view open, auto setup) -> std::optional<std::string> {
        HeadlessHost host;
        examples::components::Gallery gallery(gallery_fonts, icon_font, host);
        Driver d(gallery, fonts, 1.0, nullptr, window);
        d.settle();
        setup(d, gallery);
        d.settle();
        const int opened = open.empty() ? 0 : 1;
        for (int press = 0; press <= opened; ++press) {
            d.back();
            d.settle();
            const int expected = press < opened ? 0 : 1;
            if (host.system_backs() != expected) {
                return "back: press " + std::to_string(press + 1) + (open.empty() ? std::string(" with nothing open") : " after opening " + std::string(open)) +
                       " handed Back to the system " + std::to_string(host.system_backs()) + " times, expected " + std::to_string(expected);
            }
        }
        return std::nullopt;
    };
    if (auto p = run("", [](Driver&, examples::components::Gallery&) {})) {
        return p;
    }
    if (auto p = run(style.back_menu, [&](Driver& d, examples::components::Gallery& g) {
            tab_to(d, g, style.back_menu);
            d.key(Key::enter);
        })) {
        return p;
    }
    if (auto p = run(style.back_field, [&](Driver& d, examples::components::Gallery& g) { tab_to(d, g, style.back_field); })) {
        return p;
    }
    auto leaves = [&](const std::string& what, bool opened, auto setup) -> std::optional<std::string> {
        HeadlessHost host;
        examples::components::Gallery gallery(gallery_fonts, icon_font, host);
        Driver d(gallery, fonts, 1.0, nullptr, window);
        d.settle();
        setup(d, gallery);
        d.settle();
        const int limit = opened ? style.back_press_limit : 1;
        int presses = 0;
        while (host.system_backs() == 0 && presses < limit) {
            d.back();
            d.settle();
            ++presses;
            if (opened && presses == 1 && host.system_backs() != 0) {
                return "back: the first press after opening " + what + " was handed to the system";
            }
        }
        if (host.system_backs() != 1) {
            return "back: " + what + " handed Back to the system " + std::to_string(host.system_backs()) + " times within " +
                   std::to_string(limit) + " presses, expected once";
        }
        return std::nullopt;
    };
    for (const std::string_view name : style.page_scenes) {
        const Scene* scene = find_scene({&stills(), &pattern_stills(), &expressive_stills(), &adaptive_stills(), &files_stills()}, std::string(name));
        if (!scene) {
            return "back: no scene " + std::string(name);
        }
        if (auto p = leaves("the " + std::string(name) + " page", false, [&](Driver& d, examples::components::Gallery& g) { (*scene)(d, g); })) {
            return p;
        }
    }
    if (auto p = leaves(std::string(style.back_dialog), true, [&](Driver& d, examples::components::Gallery& g) {
            tab_to(d, g, style.back_dialog);
            d.key(Key::enter);
        })) {
        return p;
    }
    const Scene* sheet = find_scene({&stills(), &pattern_stills(), &expressive_stills(), &adaptive_stills(), &files_stills()}, std::string(style.back_sheet_scene));
    if (!sheet) {
        return "back: no scene " + std::string(style.back_sheet_scene);
    }
    if (auto p = leaves(std::string(style.back_sheet_scene), true, [&](Driver& d, examples::components::Gallery& g) { (*sheet)(d, g); })) {
        return p;
    }
    HeadlessHost host;
    examples::components::Gallery gallery(gallery_fonts, icon_font, host);
    Driver d(gallery, fonts, 1.0, nullptr, window);
    d.settle();
    d.key(Key::escape);
    d.settle();
    if (host.system_backs() != 0) {
        return std::string("back: Escape from a keyboard handed Back to the system");
    }
    return std::nullopt;
}

int usage() {
    std::string keys;
    for (const text::TypefaceSpec& t : text::typefaces) {
        keys += (keys.empty() ? "" : "|") + std::string(t.key);
    }
    std::fprintf(stderr,
                 "usage: components_capture [%s %s] [%s <width>x<height>] [%s] [%s] <mode> ...\n  components_capture size <scale>\n  components_capture still <scene> <light|dark> <scale> <out.bgra> [name]\n"
                 "  components_capture tree <scene> <light|dark> <scale>\n"
                 "  components_capture check\n"
                 "  components_capture video <scene> <light|dark> <scale>\n"
                 "  components_capture bench <scene> <light|dark> <scale> <start dp> <distance dp> <seconds> [repeats] [out.bgra]\n",
                 examples::typeface_option.data(), keys.c_str(), window_option.data(), examples::full_repaint_flag.data(), verify_repaint_flag.data());
    return 2;
}

}

int main(int argc, char** argv) {
    set_default_style(examples::sample_style());
    try {
        const text::Typeface typeface = examples::take_typeface(argc, argv);
        const WindowSize window = take_window(argc, argv);
        const bool full_repaint = examples::take_flag(argc, argv, examples::full_repaint_flag);
        const bool verify_repaint = examples::take_flag(argc, argv, verify_repaint_flag);
        if (argc < 2 || (argc < 3 && std::string_view(argv[1]) != "check")) {
            return usage();
        }
        const std::string mode = argv[1];
        if (mode == "check") {
            auto load = [](text::Typeface t) { return sdl3::typeface_stack(t, examples::res::embedded_fonts); };
            const text::FontStack fonts = load(typeface);
            const icons::IconFont icon_font = icons::IconFont::from_resource(examples::res::fonts::symbols);
            const CaptureStyle style;
            std::size_t count = 0;
            std::size_t known = 0;
            for (const std::string_view name : style.page_scenes) {
                const Scene* found = find_scene({&stills(), &pattern_stills(), &expressive_stills(), &adaptive_stills(), &files_stills()}, std::string(name));
                if (!found) {
                    throw std::runtime_error("no scene " + std::string(name));
                }
                HeadlessHost host;
                examples::components::Gallery gallery(examples::components::GalleryFonts{load, typeface}, icon_font, host);
                Driver driver(gallery, fonts, 1.0, nullptr, window);
                driver.settle();
                (*found)(driver, gallery);
                driver.settle();
                const PageCheck result = check_page(std::string(name), gallery, window.width, style);
                for (const std::string& k : result.known) {
                    std::printf("known: %s\n", k.c_str());
                    ++known;
                }
                for (const std::string& p : result.problems) {
                    std::printf("%s\n", p.c_str());
                    ++count;
                }
            }
            {
                HeadlessHost host;
                examples::components::Gallery gallery(examples::components::GalleryFonts{load, typeface}, icon_font, host);
                Driver driver(gallery, fonts, 1.0, nullptr, window);
                driver.settle();
                if (const auto problem = check_dark_theme_host(examples::components::GalleryFonts{load, typeface}, icon_font)) {
                    std::printf("%s\n", problem->c_str());
                    ++count;
                }
                if (const auto problem = check_back(examples::components::GalleryFonts{load, typeface}, fonts, icon_font, window, style)) {
                    std::printf("%s\n", problem->c_str());
                    ++count;
                }
                if (const auto problem = check_tap_redraw(driver, style)) {
                    std::printf("%s\n", problem->c_str());
                    ++count;
                }
                if (const auto problem = check_fling_redraw(driver, style)) {
                    std::printf("%s\n", problem->c_str());
                    ++count;
                }
            }
            std::printf("%d dp: %zu problems, %zu known overflows\n", static_cast<int>(window.width), count, known);
            return count == 0 ? 0 : 1;
        }
        if (mode == "size") {
            const double scale = std::stod(argv[2]);
            std::printf("%d %d\n", static_cast<int>(window.width * scale), static_cast<int>(window.height * scale));
            return 0;
        }
        if (argc < 5) {
            return usage();
        }
        const std::string scene = argv[2];
        const bool dark = std::string_view(argv[3]) == "dark";
        const double scale = std::stod(argv[4]);
        auto load = [](text::Typeface t) { return sdl3::typeface_stack(t, examples::res::embedded_fonts); };
        const text::FontStack fonts = load(typeface);
        const icons::IconFont icon_font = icons::IconFont::from_resource(examples::res::fonts::symbols);
        HeadlessHost host;
        examples::components::Gallery gallery(examples::components::GalleryFonts{load, typeface}, icon_font, host);
        gallery.set_dark(dark);

        if (mode == "still") {
            const Scene* found = find_scene({&stills(), &pattern_stills(), &expressive_stills(), &adaptive_stills(), &files_stills(), &font_stills()}, scene);
            if (!found || argc < 6) {
                return usage();
            }
            Driver driver(gallery, fonts, scale, nullptr, window);
            driver.settle();
            (*found)(driver, gallery);
            driver.wait(driver.still_seconds());
            driver.write_raw(argv[5]);
            if (argc > 6) {
                const std::optional<render::Rect> r = gallery.find(argv[6]);
                if (!r) {
                    throw std::runtime_error(std::string("no widget named ") + argv[6]);
                }
                std::printf("%d %d %d %d\n", static_cast<int>(r->x * scale), static_cast<int>(r->y * scale), static_cast<int>(r->width * scale),
                            static_cast<int>(r->height * scale));
            }
            return 0;
        }
        if (mode == "tree") {
            const Scene* found = find_scene({&stills(), &pattern_stills(), &expressive_stills(), &adaptive_stills(), &files_stills(), &font_stills()}, scene);
            if (!found) {
                return usage();
            }
            Driver driver(gallery, fonts, scale, nullptr, window);
            driver.settle();
            try {
                (*found)(driver, gallery);
            } catch (const std::exception& ex) {
                std::fprintf(stderr, "%s\n", ex.what());
            }
            driver.settle();
            std::fputs(widgets::describe(gallery.accessible_nodes()).c_str(), stdout);
            return 0;
        }
        if (mode == "bench") {
            const Scene* found = find_scene({&stills(), &pattern_stills(), &expressive_stills(), &adaptive_stills(), &files_stills(), &font_stills()}, scene);
            if (!found || argc < 8) {
                return usage();
            }
            const double start = std::stod(argv[5]);
            const double distance = std::stod(argv[6]);
            const double seconds = std::stod(argv[7]);
            const int repeats = argc > 8 ? std::stoi(argv[8]) : 1;
            Driver driver(gallery, fonts, scale, nullptr, window);
            driver.set_cursor(false);
            driver.set_full_repaint(full_repaint);
            driver.settle();
            (*found)(driver, gallery);
            driver.settle();
            const CaptureStyle& s = driver.style();
            const double x = driver.window_width() * s.bench_x;
            const double low = driver.window_height() * s.bench_from;
            const double reach = driver.window_height() * (s.bench_from - s.bench_to);
            for (double left = start; left > 0.0; left -= reach) {
                driver.set_pointer({x, low});
                driver.touch_swipe({x, low - std::min(left, reach)}, s.page_scroll_seconds, s.swipe_hold_seconds, true);
                driver.settle();
            }
            diagnostics::FrameProbe probe;
            diagnostics::DisplayInfo display;
            display.renderer = "headless";
            display.refresh_hz = s.bench_refresh_hz;
            display.window_width = driver.width();
            display.window_height = driver.height();
            display.canvas_width = driver.width();
            display.canvas_height = driver.height();
            for (int r = 0; r < repeats; ++r) {
                probe.set_switches({false, true});
                probe.set_display(display);
                driver.set_probe(&probe);
                driver.set_verify(verify_repaint);
                const bool up = r % 2 == 0;
                const double from = up ? low : low - distance;
                driver.set_pointer({x, from});
                driver.touch_swipe({x, up ? from - distance : low}, seconds, 0.0, false);
                driver.set_probe(nullptr);
                driver.set_verify(false);
                std::printf("%s\n", diagnostics::report_line(diagnostics::report_rows(probe.recent(), display, up ? "bench up" : "bench down")).c_str());
                probe.set_switches({});
                driver.wait(s.swipe_hold_seconds);
                driver.touch_lift();
                driver.settle();
            }
            if (verify_repaint) {
                std::printf("verify %zu frames, %llu pixels changed outside the rects\n", driver.verified_frames(),
                            static_cast<unsigned long long>(driver.outside_pixels()));
            }
            if (argc > 9) {
                driver.write_raw(argv[9]);
            }
            return verify_repaint && driver.outside_pixels() > 0 ? 1 : 0;
        }
        if (mode == "video") {
            const Scene* found = find_scene({&videos(), &pattern_videos(), &expressive_videos(), &adaptive_videos(), &files_videos(), &font_videos(), &gallery_videos()}, scene);
            if (!found) {
                return usage();
            }
            if (verify_repaint) {
                Driver driver(gallery, fonts, scale, nullptr, window);
                driver.set_full_repaint(full_repaint);
                driver.settle();
                driver.set_verify(true);
                (*found)(driver, gallery);
                driver.set_verify(false);
                std::printf("verify %zu frames, %llu pixels changed outside the rects\n", driver.verified_frames(),
                            static_cast<unsigned long long>(driver.outside_pixels()));
                return driver.outside_pixels() > 0 ? 1 : 0;
            }
#ifdef _WIN32
            _setmode(_fileno(stdout), _O_BINARY);
#endif
            Driver driver(gallery, fonts, scale, stdout, window);
            driver.settle();
            driver.set_recording(true);
            (*found)(driver, gallery);
            std::fflush(stdout);
            return 0;
        }
        return usage();
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
