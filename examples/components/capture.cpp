#include <chrono>
#include <cmath>
#include <cstdio>
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

#include "components/gallery.hpp"
#include "lamina/calendar/date.hpp"
#include "lamina/calendar/locale.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina_sdl3/system_fonts.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "res.hpp"

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
};

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

private:
    MemoryClipboard clipboard_;
};

class Driver {
public:
    Driver(examples::components::Gallery& gallery, const text::FontStack& fonts, double scale, std::FILE* out)
        : gallery_(gallery),
          fonts_(fonts),
          scale_(scale),
          out_(out),
          canvas_(static_cast<int>(examples::components::window_width_dp * scale), static_cast<int>(examples::components::window_height_dp * scale)) {}

    void set_recording(bool on) { recording_ = on; }
    void set_cursor(bool on) { cursor_ = on; }
    render::Point pointer() const { return pointer_; }

    void frame() {
        sdl3::Frame f{1.0 / style_.fps,
                      static_cast<double>(examples::components::window_width_dp),
                      static_cast<double>(examples::components::window_height_dp),
                      scale_,
                      false,
                      true,
                      std::move(pointer_events_),
                      std::move(wheel_events_),
                      std::move(key_events_),
                      std::move(text_events_)};
        pointer_events_.clear();
        wheel_events_.clear();
        key_events_.clear();
        text_events_.clear();
        const color::Argb background = gallery_.update(f);
        canvas_.clear(background);
        canvas_.save();
        canvas_.scale(scale_);
        gallery_.paint(canvas_, f);
        paint_overlay();
        canvas_.restore();
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
    input::PointerEvent at(PointerAction action, int clicks = 0) const {
        return {action, pointer_.x, pointer_.y, clicks, input::PointerKind::mouse, clock_};
    }

    static std::string key_name(Key k, bool shift) {
        static const std::map<Key, std::string> names{{Key::tab, "Tab"},      {Key::space, "Space"}, {Key::enter, "Enter"},
                                                      {Key::escape, "Esc"},   {Key::left, "←"},  {Key::right, "→"},
                                                      {Key::up, "↑"},    {Key::down, "↓"},  {Key::backspace, "Backspace"},
                                                      {Key::page_down, "PgDn"},  {Key::page_up, "PgUp"},
                                                      {Key::home, "Home"},       {Key::end, "End"},
                                                      {Key::delete_forward, "Delete"}};
        const auto it = names.find(k);
        const std::string base = it == names.end() ? "?" : it->second;
        return shift ? "Shift+" + base : base;
    }

    void badge(std::string text) {
        badge_text_ = std::move(text);
        badge_left_ = style_.key_badge_seconds;
    }

    void paint_overlay() {
        if (badge_left_ > 0.0 && !badge_text_.empty()) {
            const text::TextLine line = text::layout_line(fonts_, badge_text_, tokens::text_style(tokens::type_style(style_.badge_type)));
            const double h = line.box_height() + 2.0 * style_.key_badge_padding;
            const double w = line.width + 2.0 * style_.key_badge_padding;
            const render::Rect r{style_.key_badge_margin, examples::components::window_height_dp - style_.key_badge_margin - h, w, h};
            const double a = std::clamp(badge_left_ / 0.2, 0.0, 1.0);
            canvas_.fill_path(render::rounded_rect_path(r, render::uniform_radii(h / 2.0)), color::scale_alpha(style_.badge_fill, a));
            text::draw_line(canvas_, fonts_, line, {r.x + style_.key_badge_padding, r.y + style_.key_badge_padding},
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
            canvas_.fill_path(resize(1.5), style_.cursor_outline);
            canvas_.fill_path(resize(0.0), style_.cursor_fill);
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
            canvas_.fill_path(beam(1.5), style_.cursor_outline);
            canvas_.fill_path(beam(0.0), style_.cursor_fill);
            return;
        }
        canvas_.fill_path(arrow(1.5), style_.cursor_outline);
        canvas_.fill_path(arrow(0.0), style_.cursor_fill);
    }

    void write() {
        const render::PixelView px = canvas_.finish();
        for (int y = 0; y < px.height; ++y) {
            std::fwrite(px.data + px.stride * y, 4, static_cast<std::size_t>(px.width), out_);
        }
    }

    examples::components::Gallery& gallery_;
    text::FontStack fonts_;
    double scale_;
    std::FILE* out_;
    thorvg::Canvas canvas_;
    CaptureStyle style_;
    bool recording_ = false;
    bool cursor_ = true;
    render::Point pointer_{720.0, 820.0};
    std::vector<input::PointerEvent> pointer_events_;
    std::vector<input::WheelEvent> wheel_events_;
    std::vector<input::KeyEvent> key_events_;
    std::vector<input::TextEvent> text_events_;
    std::string badge_text_;
    double badge_left_ = 0.0;
    double clock_ = 0.0;
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

void show_page(Driver& d, std::string_view name) {
    d.set_cursor(false);
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
             d.move_to(centre_by(g, [](const widgets::AccessibleNode& n) { return n.name == "Progress" && n.role == widgets::AccessibleRole::slider; }), 0.8);
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

const Scene* find_scene(std::initializer_list<const std::map<std::string, Scene>*> maps, const std::string& name) {
    for (const auto* m : maps) {
        if (const auto it = m->find(name); it != m->end()) {
            return &it->second;
        }
    }
    return nullptr;
}

int usage() {
    std::fprintf(stderr,
                 "usage:\n  components_capture size <scale>\n  components_capture still <scene> <light|dark> <scale> <out.bgra> [name]\n"
                 "  components_capture tree <scene> <light|dark> <scale>\n"
                 "  components_capture video <scene> <light|dark> <scale>\n");
    return 2;
}

}

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            return usage();
        }
        const std::string mode = argv[1];
        if (mode == "size") {
            const double scale = std::stod(argv[2]);
            std::printf("%d %d\n", static_cast<int>(examples::components::window_width_dp * scale),
                        static_cast<int>(examples::components::window_height_dp * scale));
            return 0;
        }
        if (argc < 5) {
            return usage();
        }
        const std::string scene = argv[2];
        const bool dark = std::string_view(argv[3]) == "dark";
        const double scale = std::stod(argv[4]);
        const text::FontStack fonts = sdl3::font_stack_from_arguments(1, argv);
        const icons::IconFont icon_font = icons::IconFont::from_resource(examples::res::fonts::symbols);
        HeadlessHost host;
        examples::components::Gallery gallery(fonts, icon_font, host);
        gallery.set_dark(dark);

        if (mode == "still") {
            const Scene* found = find_scene({&stills(), &pattern_stills(), &expressive_stills(), &adaptive_stills()}, scene);
            if (!found || argc < 6) {
                return usage();
            }
            Driver driver(gallery, fonts, scale, nullptr);
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
            const Scene* found = find_scene({&stills(), &pattern_stills(), &expressive_stills(), &adaptive_stills()}, scene);
            if (!found) {
                return usage();
            }
            Driver driver(gallery, fonts, scale, nullptr);
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
        if (mode == "video") {
            const Scene* found = find_scene({&videos(), &pattern_videos(), &expressive_videos(), &adaptive_videos()}, scene);
            if (!found) {
                return usage();
            }
#ifdef _WIN32
            _setmode(_fileno(stdout), _O_BINARY);
#endif
            Driver driver(gallery, fonts, scale, stdout);
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
