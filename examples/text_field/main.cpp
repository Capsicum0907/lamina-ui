#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "common/canvas.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/system_fonts.hpp"

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

}

int main(int argc, char** argv) {
    try {
        const Layout layout;
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const auto scheme = color::spec_2021().build(color::default_scheme_options);

        sdl3::Window window({"lamina-ui text field", 760, 560}, examples::window_canvas());
        std::vector<std::unique_ptr<widgets::TextField>> fields;
        fields.push_back(std::make_unique<widgets::TextField>(
            widgets::TextFieldOptions{.label = "\u540d\u524d", .multiline = false, .clipboard = &window.clipboard()}, fonts));
        fields.push_back(std::make_unique<widgets::TextField>(
            widgets::TextFieldOptions{.label = "\u30e1\u30c3\u30bb\u30fc\u30b8", .multiline = true, .clipboard = &window.clipboard()}, fonts));
        for (auto& f : fields) {
            f->set_width(layout.field_width);
        }
        widgets::Button send({"\u9001\u4fe1", tokens::ButtonStyle::filled, tokens::ButtonSize::medium, tokens::ButtonShape::round}, fonts);
        widgets::WidgetSet controls;
        for (auto& f : fields) {
            controls.add(*f);
        }
        controls.add(send);
        widgets::Label header("\u6587\u5b57\u306e\u5165\u529b", layout.header_type, layout.text, fonts, false);
        std::vector<std::unique_ptr<widgets::Label>> log;

        std::shared_ptr<layout::Linear> page;
        auto build = [&] {
            std::vector<layout::NodePtr> entries;
            for (auto& l : log) {
                entries.push_back(widgets::node(*l));
            }
            auto log_column = layout::column(std::move(entries));
            log_column->spacing(layout.log_gap).cross_align(layout::CrossAlign::stretch);
            log_column->self_align = layout::CrossAlign::stretch;
            std::vector<layout::NodePtr> children{widgets::node(header)};
            for (auto& f : fields) {
                children.push_back(widgets::node(*f));
            }
            children.push_back(widgets::node(send));
            children.push_back(log_column);
            page = layout::column(std::move(children));
            page->spacing(layout.gap).padding(layout::Insets::all(layout.margin)).fill_cross(true);
        };
        build();

        auto submit = [&] {
            const std::string name = fields[0]->text();
            const std::string message = fields[1]->text();
            if (message.empty()) {
                return;
            }
            log.push_back(std::make_unique<widgets::Label>((name.empty() ? std::string("?") : name) + ": " + message, layout.log_type, layout.log, fonts));
            fields[1]->set_text("");
            build();
        };

        auto place = [&](const sdl3::Frame& frame) { layout::run(*page, {0.0, 0.0, frame.width_dp, frame.height_dp}); };

        auto update = [&](const sdl3::Frame& frame) {
            place(frame);

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
                    if (k.key == input::Key::tab) {
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

            place(frame);

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
            return scheme[layout.background];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            header.paint(canvas, scheme);
            controls.step(frame.dt_seconds);
            controls.paint(canvas, scheme);
            for (auto& f : fields) {
                if (f->focused()) {
                    window.wake_after(f->next_blink_seconds());
                }
            }
            for (const auto& l : log) {
                l->paint(canvas, scheme);
            }
            return controls.animating();
        };

        sdl3::run_app(window, update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
