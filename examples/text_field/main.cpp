#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/platform/window.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/text_field.hpp"

using namespace lamina;

namespace {

struct Layout {
    double margin = 32.0;
    double gap = 24.0;
    double field_width = 360.0;
    double log_gap = 8.0;
    color::Role background = color::Role::surface;
    color::Role text = color::Role::on_surface;
    color::Role log = color::Role::on_surface_variant;
    tokens::TypeRole header_type = tokens::TypeRole::headline_small;
    tokens::TypeRole log_type = tokens::TypeRole::body_large;
};

const std::vector<std::string> default_fonts{"C:/Windows/Fonts/NotoSansJP-VF.ttf"};

}

int main(int argc, char** argv) {
    try {
        const Layout layout;
        text::FontStack fonts;
        for (int i = 1; i < argc; ++i) {
            fonts.add(text::Font::from_file(argv[i]));
        }
        if (fonts.empty()) {
            for (const auto& path : default_fonts) {
                fonts.add(text::Font::from_file(path));
            }
        }
        const auto scheme = color::spec_2021().build(color::default_scheme_options);
        const auto header_style = text::text_style(tokens::type_style(layout.header_type));
        const auto log_style = text::text_style(tokens::type_style(layout.log_type));

        std::vector<std::unique_ptr<widgets::TextField>> fields;
        fields.push_back(std::make_unique<widgets::TextField>("\u540d\u524d", fonts, layout.field_width));
        fields.push_back(std::make_unique<widgets::TextField>("\u30e1\u30c3\u30bb\u30fc\u30b8", fonts, layout.field_width));
        widgets::Button send({"\u9001\u4fe1", tokens::ButtonStyle::filled, tokens::ButtonSize::medium, tokens::ButtonShape::round}, fonts);
        const auto header = text::layout_line(fonts, "\u6587\u5b57\u306e\u5165\u529b", header_style);
        std::vector<text::TextLine> log;

        platform::Window window({"lamina-ui text field", 760, 560});
        platform::Frame frame;
        bool animating = true;

        auto submit = [&] {
            const std::string name = fields[0]->text();
            const std::string message = fields[1]->text();
            if (message.empty()) {
                return;
            }
            log.push_back(text::layout_line(fonts, (name.empty() ? std::string("?") : name) + ": " + message, log_style));
            fields[1]->set_text("");
        };

        while (window.next_frame(frame, animating)) {
            double y = layout.margin + header_style.line_height + layout.gap;
            for (auto& f : fields) {
                f->set_origin({layout.margin, y});
                y += f->bounds().height + layout.gap;
            }
            send.set_origin({layout.margin, y});
            y += send.height() + layout.gap;

            for (const auto& e : frame.pointer) {
                for (auto& f : fields) {
                    f->handle(e);
                }
                if (send.handle(e)) {
                    submit();
                }
            }
            for (const auto& k : frame.keys) {
                for (std::size_t i = 0; i < fields.size(); ++i) {
                    if (!fields[i]->focused()) {
                        continue;
                    }
                    if (k.key == platform::Key::tab) {
                        fields[i]->set_focused(false);
                        fields[(i + 1) % fields.size()]->set_focused(true);
                        break;
                    }
                    if (fields[i]->handle(k)) {
                        submit();
                    }
                    break;
                }
            }
            for (const auto& t : frame.text) {
                for (auto& f : fields) {
                    f->handle(t);
                }
            }

            const widgets::TextField* focused = nullptr;
            for (auto& f : fields) {
                if (f->focused()) {
                    focused = f.get();
                }
            }
            if (focused) {
                const render::Rect area = focused->caret_area();
                window.set_text_input(&area);
            } else {
                window.set_text_input(nullptr);
            }

            auto& canvas = window.canvas();
            const color::Argb background = scheme[layout.background];
            canvas.clear(background);
            canvas.save();
            canvas.scale(frame.scale);
            text::draw_line(canvas, fonts, header, {layout.margin, layout.margin}, scheme[layout.text]);
            animating = false;
            for (auto& f : fields) {
                f->step(frame.dt_seconds);
                f->paint(canvas, scheme, background);
                animating = animating || f->animating();
                if (f->focused()) {
                    window.wake_after(f->next_blink_seconds());
                }
            }
            send.step(frame.dt_seconds);
            send.paint(canvas, scheme);
            animating = animating || send.animating();
            for (const auto& line : log) {
                text::draw_line(canvas, fonts, line, {layout.margin, y}, scheme[layout.log]);
                y += log_style.line_height + layout.log_gap;
            }
            canvas.restore();
            window.present();
        }
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
