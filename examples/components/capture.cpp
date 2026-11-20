#include <cmath>
#include <cstdio>
#include <exception>
#include <fstream>
#include <functional>
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
            pointer_events_.push_back({PointerAction::move, pointer_.x, pointer_.y});
            frame();
        }
    }
    void move_to(std::string_view name, double seconds) { move_to(centre_of(name), seconds); }

    void click() {
        pointer_events_.push_back({PointerAction::down, pointer_.x, pointer_.y, 1});
        frame();
        wait(0.08);
        pointer_events_.push_back({PointerAction::up, pointer_.x, pointer_.y});
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

using Scene = std::function<void(Driver&, examples::components::Gallery&)>;

const std::map<std::string, Scene>& stills() {
    static const std::map<std::string, Scene> scenes{
        {"default", [](Driver&, examples::components::Gallery&) {}},
        {"chips", [](Driver& d, examples::components::Gallery&) { show_page(d, "Chips & cards"); }},
        {"chips_focus",
         [](Driver& d, examples::components::Gallery& g) {
             show_page(d, "Chips & cards");
             d.set_cursor(false);
             tab_to(d, g, "Filters");
             d.key(Key::right);
             d.wait(0.4);
         }},
        {"chips_hover",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Chips & cards");
             d.move_to("Kyoto", 0.3);
             d.wait(0.4);
         }},
        {"chips_disabled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "Chips & cards");
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
        {"bars", [](Driver& d, examples::components::Gallery&) { show_page(d, "App bars & badges"); }},
        {"bars_scrolled",
         [](Driver& d, examples::components::Gallery&) {
             show_page(d, "App bars & badges");
             for (const char* list : {"Inbox list", "Photos list", "Mail list", "Albums list"}) {
                 wheel_at(d, list, 1.0, 3, 0.1);
             }
             d.move_to({720.0, 845.0}, 0.2);
             d.wait(0.8);
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
             show_page(d, "Chips & cards");
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
             show_page(d, "Chips & cards");
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
             show_page(d, "Chips & cards");
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
             show_page(d, "App bars & badges");
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
             show_page(d, "App bars & badges");
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
            const auto it = stills().find(scene);
            if (it == stills().end() || argc < 6) {
                return usage();
            }
            Driver driver(gallery, fonts, scale, nullptr);
            driver.settle();
            it->second(driver, gallery);
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
            const auto it = stills().find(scene);
            if (it == stills().end()) {
                return usage();
            }
            Driver driver(gallery, fonts, scale, nullptr);
            driver.settle();
            try {
                it->second(driver, gallery);
            } catch (const std::exception& ex) {
                std::fprintf(stderr, "%s\n", ex.what());
            }
            driver.settle();
            std::fputs(widgets::describe(gallery.accessible_nodes()).c_str(), stdout);
            return 0;
        }
        if (mode == "video") {
            const auto it = videos().find(scene);
            if (it == videos().end()) {
                return usage();
            }
#ifdef _WIN32
            _setmode(_fileno(stdout), _O_BINARY);
#endif
            Driver driver(gallery, fonts, scale, stdout);
            driver.settle();
            driver.set_recording(true);
            it->second(driver, gallery);
            std::fflush(stdout);
            return 0;
        }
        return usage();
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
