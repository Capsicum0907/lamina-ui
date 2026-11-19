#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/canvas.hpp"
#include "common/options.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/overlay_stack.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/snackbar.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/title_bar.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/resources.hpp"
#include "lamina_sdl3/system_fonts.hpp"
#include "todo/engine.hpp"
#include "todo/layout.hpp"
#include "todo/row.hpp"
#include "todo/storage.hpp"

using namespace lamina;
using examples::todo::ItemId;
using examples::todo::RowAction;
using examples::todo::RowPart;
using examples::todo::TaskRow;
using input::Key;

namespace {

struct Config {
    sdl3::WindowOptions window{"lamina-ui to-do", 420, 720};
    const char* bar_title = "To-do";
    color::SchemeOptions scheme{.seed = color::default_scheme_options.seed, .dark = false, .variant = color::Variant::tonal_spot, .contrast = 0.0};
    color::Role background = color::Role::surface;
    std::string organization = "lamina-ui";
    std::string application = "todo";
    std::string file_name = "todo.txt";
    std::string data_dir_option = "--data-dir";
    examples::todo::LayoutSettings layout;
    examples::todo::RowStyle row;
    examples::todo::HeaderStyle header;
    std::string field_label = "Add a task";
    icons::Symbol add_icon = icons::Symbol::add;
    tokens::IconButtonStyle add_style = tokens::IconButtonStyle::filled;
    tokens::IconButtonSize add_size = tokens::IconButtonSize::medium;
    std::string empty_message = "Nothing to do";
    icons::Symbol empty_icon = icons::Symbol::task_alt;
    double empty_icon_size = 48.0;
    double empty_gap = 12.0;
    tokens::TypeRole empty_type = tokens::TypeRole::body_large;
    color::Role empty_color = color::Role::on_surface_variant;
    std::string deleted_message = "Task deleted";
    std::string cleared_one_message = "1 completed task deleted";
    std::string cleared_message = "{} completed tasks deleted";
    widgets::SnackbarOptions snackbar{"Undo"};
    widgets::DialogOptions clear_dialog{"Delete completed tasks?", "Every completed task will be deleted. You can undo it right after.",
                                        "Delete", "Cancel"};
    motion::SpringSpec fade_spring = tokens::motion_scheme.default_effects();
};

struct Base {
    std::function<void()> cancel;
    void cancel_interaction() { cancel(); }
};

void report_problems(const examples::todo::LoadReport& report, const std::filesystem::path& file) {
    for (const auto& [line, problem] : report.problems) {
        const char* what = problem == examples::todo::LineProblem::no_marker      ? "no [ ] or [x] marker; kept as an open task"
                           : problem == examples::todo::LineProblem::empty_text ? "no text; skipped"
                                                                                : "invalid UTF-8; replaced";
        std::fprintf(stderr, "lamina-ui to-do: %s line %zu: %s\n", file.string().c_str(), line, what);
    }
}

}

int main(int argc, char** argv) {
    try {
        const Config config;
        const bool want_title_bar = examples::take_flag(argc, argv, examples::title_bar_flag);
        const std::optional<std::string> data_dir = examples::take_option(argc, argv, config.data_dir_option);
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const icons::IconFont icon_font = sdl3::bundled_icon_font();
        const color::ColorScheme scheme = color::spec_2025().build(config.scheme);
        if (data_dir) {
            std::filesystem::create_directories(*data_dir);
        }
        const std::filesystem::path directory =
            data_dir ? std::filesystem::path(*data_dir) : sdl3::preference_directory(config.organization, config.application);
        const std::filesystem::path file = directory / config.file_name;

        examples::todo::LoadReport load_report;
        const std::optional<std::string> saved = examples::todo::read_file(file);
        examples::todo::TodoList list = saved ? examples::todo::TodoList::load(*saved, &load_report) : examples::todo::TodoList{};
        report_problems(load_report, file);
        std::uint64_t saved_revision = list.revision();

        sdl3::WindowOptions window_options = config.window;
        window_options.height_dp = examples::window_height_dp(config.window.height_dp, want_title_bar);
        window_options.client_title_bar = want_title_bar;
        sdl3::Window window(window_options, examples::window_canvas());
        widgets::TitleBar title_bar({config.bar_title, icon_font, nullptr, config.background}, fonts);
        const double bar_height = window.client_title_bar() ? title_bar.height() : 0.0;
        widgets::TextField input({.label = config.field_label, .multiline = false, .clipboard = &window.clipboard()}, fonts);
        widgets::IconButton add_button({icons::Icon{icon_font, config.add_icon}, config.add_style, config.add_size});
        examples::todo::LayoutSettings layout_settings = config.layout;
        layout_settings.add_size = add_button.width();
        layout_settings.field_height = input.height();
        const examples::todo::TodoLayout layout(layout_settings);
        examples::todo::SectionHeader header(config.header, config.row, fonts, icon_font);
        widgets::ScrollArea scroll;
        widgets::Dialog dialog(config.clear_dialog, fonts);
        widgets::Snackbar snackbar(config.snackbar, fonts);
        widgets::Label empty_label({config.empty_message, config.empty_type, config.empty_color, false}, fonts);
        motion::Animated empty_presence(list.items().empty() ? 1.0 : 0.0, config.fade_spring);
        widgets::WidgetSet rows;
        std::function<void(TaskRow&, const RowAction&)> on_row;
        std::vector<ItemId> fresh;
        bool expanded = true;
        input.set_focused(true);

        auto first_row = [&](auto&& pick) -> TaskRow* {
            TaskRow* found = nullptr;
            rows.for_each<TaskRow>([&](TaskRow& r) {
                if (!found && pick(r)) {
                    found = &r;
                }
            });
            return found;
        };
        auto find_row = [&](ItemId id) { return first_row([&](const TaskRow& r) { return r.id() == id && !r.leaving(); }); };
        auto sync_rows = [&] {
            rows.for_each<TaskRow>([&](TaskRow& r) {
                if (!r.leaving() && !list.find(r.id())) {
                    r.leave();
                }
            });
            for (const examples::todo::Item& item : list.items()) {
                if (TaskRow* r = find_row(item.id)) {
                    r->sync(item);
                    continue;
                }
                if (TaskRow* ghost = first_row([&](const TaskRow& r) { return r.id() == item.id; })) {
                    ghost->revive();
                    ghost->sync(item);
                    continue;
                }
                rows.emplace_with<TaskRow>([&on_row](TaskRow& r, const RowAction& a) { on_row(r, a); }, item, config.row, fonts, icon_font,
                                           &window.clipboard());
                fresh.push_back(item.id);
            }
        };
        sync_rows();

        auto display_order = [&] {
            std::vector<TaskRow*> out;
            for (const ItemId id : list.active()) {
                if (TaskRow* r = find_row(id)) {
                    out.push_back(r);
                }
            }
            if (expanded) {
                for (const ItemId id : list.completed()) {
                    if (TaskRow* r = find_row(id)) {
                        out.push_back(r);
                    }
                }
            }
            return out;
        };
        auto focused_row = [&] { return first_row([](const TaskRow& r) { return !r.leaving() && r.focused(); }); };
        auto focus_holder = [&] { return first_row([](const TaskRow& r) { return !r.leaving() && r.focused_part().has_value(); }); };
        auto reveal = [&](TaskRow& r) { scroll.ensure_visible(r.target_y(), r.target_y() + r.target_height()); };

        widgets::OverlayStack overlays;
        Base base{[&] {
            input.cancel_interaction();
            add_button.cancel_interaction();
            header.cancel_interaction();
            scroll.cancel_interaction();
            rows.cancel_interaction();
        }};
        overlays.add_base(base);

        auto undo = [&] {
            const std::vector<ItemId> restored = list.undo();
            if (!restored.empty()) {
                snackbar.dismiss();
            }
        };
        auto on_snackbar = [&](const widgets::SnackbarResponse& r) {
            if (r.action) {
                undo();
            }
        };
        auto notify = [&](std::string message) {
            snackbar.show(std::move(message));
            overlays.push(snackbar, on_snackbar, widgets::Modality::modeless);
        };
        auto on_dialog = [&](widgets::DialogResult r) {
            if (r != widgets::DialogResult::confirmed) {
                return;
            }
            const std::size_t n = list.clear_completed();
            if (n == 1) {
                notify(config.cleared_one_message);
            } else if (n > 1) {
                notify(std::vformat(config.cleared_message, std::make_format_args(n)));
            }
        };
        auto add_from_input = [&] {
            if (list.add(input.text())) {
                input.set_text("");
                scroll.scroll_to(0.0);
            }
        };
        auto commit = [&](TaskRow& r) {
            const std::string text = r.end_edit();
            list.edit(r.id(), text);
        };
        widgets::FocusManager focus;
        auto rebuild_focus = [&] {
            focus.clear();
            focus.add(input);
            focus.add(add_button);
            auto add_row = [&](TaskRow& r) {
                focus.add(r.checkbox());
                focus.add(r);
                focus.add(r.remove_button());
            };
            for (const ItemId id : list.active()) {
                if (TaskRow* r = find_row(id)) {
                    add_row(*r);
                }
            }
            if (!list.completed().empty()) {
                focus.add(header);
                focus.add(header.clear_button());
            }
            if (expanded) {
                for (const ItemId id : list.completed()) {
                    if (TaskRow* r = find_row(id)) {
                        add_row(*r);
                    }
                }
            }
            focus.add(snackbar);
        };
        overlays.set_focus(focus);
        auto focus_input = [&] {
            rebuild_focus();
            focus.clear_focus();
            input.set_focused(true);
        };
        auto pass_focus = [&](TaskRow& r) {
            const std::optional<RowPart> part = r.focused_part();
            if (!part) {
                return;
            }
            const std::vector<TaskRow*> order = display_order();
            const auto it = std::find(order.begin(), order.end(), &r);
            TaskRow* next = nullptr;
            if (it != order.end() && it + 1 != order.end()) {
                next = *(it + 1);
            } else if (it != order.begin() && it != order.end()) {
                next = *(it - 1);
            }
            r.clear_focus();
            if (next) {
                next->focus_part(*part);
                reveal(*next);
            } else {
                focus_input();
            }
        };
        auto remove = [&](TaskRow& r) {
            pass_focus(r);
            if (list.remove(r.id())) {
                notify(config.deleted_message);
            }
        };
        auto move_within_section = [&](TaskRow& r, int direction) {
            const std::vector<ItemId> section = r.done() ? list.completed() : list.active();
            const auto it = std::find(section.begin(), section.end(), r.id());
            if (it == section.end()) {
                return;
            }
            if (direction < 0 && it != section.begin()) {
                list.move(r.id(), *list.index_of(*(it - 1)));
            } else if (direction > 0 && it + 1 != section.end()) {
                list.move(r.id(), *list.index_of(*(it + 1)));
            }
        };
        auto step_focus = [&](TaskRow& r, int direction) {
            const std::vector<TaskRow*> order = display_order();
            const auto it = std::find(order.begin(), order.end(), &r);
            if (it == order.end()) {
                return;
            }
            if (direction < 0) {
                r.set_focused(false);
                if (it == order.begin()) {
                    input.set_focused(true);
                } else {
                    (*(it - 1))->set_focused(true);
                    reveal(**(it - 1));
                }
            } else if (it + 1 != order.end()) {
                r.set_focused(false);
                (*(it + 1))->set_focused(true);
                reveal(**(it + 1));
            }
        };
        auto start_edit = [&](TaskRow& r, const std::optional<input::PointerEvent>& press) {
            rows.for_each<TaskRow>([&](TaskRow& other) {
                if (&other != &r && other.editing()) {
                    commit(other);
                }
            });
            rebuild_focus();
            focus.clear_focus();
            rows.for_each<TaskRow>([](TaskRow& other) { other.clear_focus(); });
            r.begin_edit(press);
        };
        auto commit_unfocused = [&] {
            rows.for_each<TaskRow>([&](TaskRow& r) {
                if (r.editing() && !r.editor()->focused()) {
                    commit(r);
                }
            });
        };
        on_row = [&](TaskRow& r, const RowAction& a) {
            if (a.toggled) {
                if (r.editing()) {
                    commit(r);
                }
                list.toggle(r.id());
            }
            if (a.remove) {
                remove(r);
            }
            if (a.edit) {
                start_edit(r, a.edit);
            }
        };

        auto save_if_changed = [&] {
            if (list.revision() == saved_revision) {
                return;
            }
            try {
                examples::todo::write_file_replacing(file, list.save());
            } catch (const std::exception& ex) {
                std::fprintf(stderr, "lamina-ui to-do: could not save %s: %s\n", file.string().c_str(), ex.what());
            }
            saved_revision = list.revision();
        };

        bool first = true;
        std::uint64_t placed_revision = list.revision();
        bool placed_expanded = expanded;
        std::size_t placed_editing = 0;

        auto update = [&](const sdl3::Frame& frame) {
            rebuild_focus();
            const double body_height = frame.height_dp - bar_height;
            const render::Rect body{0.0, bar_height, frame.width_dp, body_height};
            if (window.client_title_bar()) {
                title_bar.set_maximized(frame.maximized);
                title_bar.set_active(frame.active);
                title_bar.set_width(frame.width_dp);
                title_bar.set_origin({0.0, 0.0});
                window.set_caption_regions(title_bar.regions());
                for (const auto& e : frame.pointer) {
                    switch (title_bar.handle(e)) {
                    case widgets::TitleBarAction::minimize:
                        window.minimize();
                        break;
                    case widgets::TitleBarAction::toggle_maximize:
                        window.toggle_maximize();
                        break;
                    case widgets::TitleBarAction::close:
                        window.close();
                        break;
                    case widgets::TitleBarAction::none:
                        break;
                    }
                }
                title_bar.step(frame.dt_seconds);
            }
            const examples::todo::Column column = layout.column(frame.width_dp);
            rows.for_each<TaskRow>([&](TaskRow& r) { r.set_column(column); });

            for (const auto& e : frame.pointer) {
                if (window.client_title_bar() && e.action == input::PointerAction::down && e.y_dp < bar_height) {
                    continue;
                }
                if (overlays.handle(e)) {
                    continue;
                }
                if (e.action == input::PointerAction::down) {
                    rows.for_each<TaskRow>([](TaskRow& r) {
                        if (!r.editing()) {
                            r.set_focused(false);
                        }
                    });
                }
                input.handle(e);
                if (add_button.handle(e)) {
                    add_from_input();
                    input.set_focused(true);
                }
                if (scroll.handle(e)) {
                    continue;
                }
                const input::PointerEvent c = scroll.to_content(e);
                const examples::todo::HeaderAction h = header.handle(c);
                if (h.toggled) {
                    expanded = !expanded;
                    header.set_expanded(expanded);
                }
                if (h.clear) {
                    dialog.open_in(body);
                    overlays.push(dialog, on_dialog);
                }
                rows.handle(c);
            }
            commit_unfocused();
            for (const auto& w : frame.wheel) {
                if (!dialog.is_open()) {
                    scroll.handle(w);
                }
            }

            for (const auto& k : frame.keys) {
                const bool snackbar_focused = snackbar.focused();
                if (overlays.handle(k)) {
                    if (snackbar_focused && !snackbar.focused()) {
                        focus_input();
                    }
                    continue;
                }
                TaskRow* editing = first_row([](const TaskRow& r) { return r.editing() && r.editor()->focused(); });
                widgets::TextField* typing = editing ? editing->editor() : input.focused() ? &input : nullptr;
                if (typing && typing->handles_history_key(k)) {
                    typing->handle(k);
                    continue;
                }
                if (k.ctrl && !k.shift && k.key == Key::z) {
                    if (k.repeat) {
                        continue;
                    }
                    undo();
                    if (snackbar_focused && !snackbar.focused()) {
                        focus_input();
                    }
                    continue;
                }
                if (editing && k.key == Key::escape) {
                    editing->end_edit();
                    continue;
                }
                if (k.key == Key::tab) {
                    rebuild_focus();
                    if (!focus.handle(k)) {
                        input.set_focused(true);
                    }
                    commit_unfocused();
                    if (TaskRow* f = focus_holder()) {
                        reveal(*f);
                    }
                    continue;
                }
                if (editing) {
                    if (editing->editor()->handle(k)) {
                        commit(*editing);
                    }
                    continue;
                }
                if (add_button.handle(k)) {
                    add_from_input();
                    continue;
                }
                const examples::todo::HeaderAction header_action = header.handle(k);
                if (header_action.toggled) {
                    expanded = !expanded;
                    header.set_expanded(expanded);
                    continue;
                }
                if (header_action.clear) {
                    dialog.open_in(body, input::InputSource::keyboard);
                    overlays.push(dialog, on_dialog);
                    continue;
                }
                bool row_used = false;
                for (TaskRow* r : display_order()) {
                    const examples::todo::RowAction a = r->handle(k);
                    if (a.toggled) {
                        pass_focus(*r);
                        list.toggle(r->id());
                    }
                    if (a.remove) {
                        remove(*r);
                    }
                    if (a.toggled || a.remove) {
                        row_used = true;
                        break;
                    }
                }
                if (row_used) {
                    continue;
                }
                if (input.focused()) {
                    if (input.handle(k)) {
                        add_from_input();
                    } else if (k.key == Key::down && !k.ctrl && !k.shift) {
                        const std::vector<TaskRow*> order = display_order();
                        if (!order.empty()) {
                            rebuild_focus();
                            focus.clear_focus();
                            order.front()->set_focused(true);
                            reveal(*order.front());
                        }
                    } else {
                        scroll.handle(k);
                    }
                    continue;
                }
                TaskRow* f = focused_row();
                if (!f) {
                    if (k.key == Key::escape && (focus_holder() || add_button.focused() || header.focused() || header.clear_button().focused())) {
                        focus_input();
                    } else {
                        scroll.handle(k);
                    }
                    continue;
                }
                switch (k.key) {
                case Key::enter: start_edit(*f, std::nullopt); break;
                case Key::space:
                    pass_focus(*f);
                    list.toggle(f->id());
                    break;
                case Key::delete_forward: remove(*f); break;
                case Key::up:
                case Key::down: {
                    const int direction = k.key == Key::up ? -1 : 1;
                    if (k.ctrl) {
                        move_within_section(*f, direction);
                    } else {
                        step_focus(*f, direction);
                    }
                    break;
                }
                case Key::escape: focus_input(); break;
                default: scroll.handle(k); break;
                }
            }
            for (const auto& t : frame.text) {
                if (overlays.handle(t)) {
                    continue;
                }
                input.handle(t);
                rows.for_each<TaskRow>([&](TaskRow& r) {
                    if (r.editing()) {
                        r.editor()->handle(t);
                    }
                });
            }

            save_if_changed();
            sync_rows();
            rows.for_each<TaskRow>([&](TaskRow& r) { r.set_column(column); });

            std::size_t editing_count = 0;
            rows.for_each<TaskRow>([&](const TaskRow& r) { editing_count += r.editing() ? 1 : 0; });
            const bool structural =
                list.revision() != placed_revision || expanded != placed_expanded || editing_count != placed_editing;
            placed_revision = list.revision();
            placed_expanded = expanded;
            placed_editing = editing_count;

            examples::todo::ListInput sizes;
            sizes.expanded = expanded;
            const std::vector<ItemId> active = list.active();
            const std::vector<ItemId> completed = list.completed();
            for (const ItemId id : active) {
                sizes.active.push_back(find_row(id)->target_height());
            }
            for (const ItemId id : completed) {
                sizes.completed.push_back(find_row(id)->target_height());
            }
            examples::todo::TodoFrame placed = layout.place(frame.width_dp, body_height, sizes);
            for (examples::todo::Box* b : {&placed.field, &placed.add, &placed.viewport}) {
                b->y += bar_height;
            }
            auto put = [&](ItemId id, const examples::todo::RowPlace& p) {
                TaskRow* r = find_row(id);
                const auto it = std::find(fresh.begin(), fresh.end(), id);
                if (it == fresh.end()) {
                    r->place(p, structural);
                    return;
                }
                fresh.erase(it);
                if (first) {
                    r->snap(p);
                } else {
                    r->appear(p.y, p.height);
                    r->place(p, true);
                }
            };
            for (std::size_t i = 0; i < active.size(); ++i) {
                put(active[i], placed.active[i]);
            }
            for (std::size_t i = 0; i < completed.size(); ++i) {
                put(completed[i], placed.completed[i]);
            }
            header.set_count(completed.size());
            header.place(placed.header, column, structural);
            scroll.set_viewport({placed.viewport.x, placed.viewport.y, placed.viewport.width, placed.viewport.height});
            scroll.set_content_height(placed.content_height);
            input.set_width(placed.field.width);
            input.set_origin({placed.field.x, placed.field.y});
            add_button.set_origin({placed.add.x, placed.add.y});
            snackbar.set_area(body);
            if (dialog.visible()) {
                dialog.set_area(body);
            }
            empty_presence.animate_to(list.items().empty() ? 1.0 : 0.0);
            if (first) {
                empty_presence.snap_to(empty_presence.target());
            }
            if (const auto e = scroll.rehover()) {
                header.handle(*e);
                rows.handle(*e);
            }
            first = false;

            const double dt = frame.dt_seconds;
            rows.step(dt);
            rows.for_each<TaskRow>([&](TaskRow& r) {
                if (r.gone()) {
                    rows.remove(r);
                }
            });
            rebuild_focus();
            header.step(dt);
            scroll.step(dt);
            input.step(dt);
            add_button.step(dt);
            dialog.step(dt);
            snackbar.step(dt);
            empty_presence.step(dt);

            std::optional<render::Rect> caret;
            std::optional<double> wake;
            auto wake_at = [&](double seconds) {
                if (seconds >= 0.0) {
                    wake = wake ? std::min(*wake, seconds) : seconds;
                }
            };
            if (input.focused()) {
                caret = input.caret_area();
                wake_at(input.next_blink_seconds());
            }
            rows.for_each<TaskRow>([&](const TaskRow& r) {
                if (r.editing() && r.editor()->focused()) {
                    caret = scroll.to_window(r.editor()->caret_area());
                    wake_at(r.editor()->next_blink_seconds());
                }
            });
            wake_at(scroll.next_wake_seconds());
            wake_at(snackbar.next_wake_seconds());
            window.set_text_input(dialog.is_open() ? std::nullopt : caret);
            if (wake) {
                window.wake_after(*wake);
            }
            return scheme[config.background];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame&) {
            input.paint(canvas, scheme);
            add_button.paint(canvas, scheme);

            scroll.begin_content(canvas);
            header.paint(canvas, scheme);
            const widgets::WidgetSet& painted = rows;
            painted.for_each<TaskRow>([&](const TaskRow& r) {
                if (r.leaving()) {
                    r.paint(canvas, scheme);
                }
            });
            painted.for_each<TaskRow>([&](const TaskRow& r) {
                if (!r.leaving() && !r.moving()) {
                    r.paint(canvas, scheme);
                }
            });
            painted.for_each<TaskRow>([&](const TaskRow& r) {
                if (!r.leaving() && r.moving()) {
                    r.paint(canvas, scheme);
                }
            });
            scroll.end_content(canvas);
            scroll.paint(canvas, scheme);

            const double e = std::clamp(empty_presence.value(), 0.0, 1.0);
            if (e > 0.0) {
                const color::ColorScheme faded = color::faded(scheme, e);
                const render::Rect v = scroll.viewport();
                const layout::Size text = empty_label.measure({0.0, v.width, 0.0, v.height});
                const double block = config.empty_icon_size + config.empty_gap + text.height;
                const double top = v.y + (v.height - block) / 2.0;
                icon_font.draw(canvas, config.empty_icon, {v.x + (v.width - config.empty_icon_size) / 2.0, top},
                               config.empty_icon_size, faded[config.empty_color]);
                empty_label.set_origin({v.x + (v.width - text.width) / 2.0, top + config.empty_icon_size + config.empty_gap});
                empty_label.paint(canvas, faded);
            }

            if (window.client_title_bar()) {
                title_bar.paint(canvas, scheme);
            }
            snackbar.paint(canvas, scheme);
            dialog.paint(canvas, scheme);

            bool animating = (window.client_title_bar() && title_bar.animating()) || header.animating() || scroll.animating() || input.animating() || add_button.animating() ||
                             dialog.animating() || snackbar.animating() || !empty_presence.settled() || rows.animating();
            return animating;
        };

        sdl3::run_app(window, update, paint);
        rows.for_each<TaskRow>([&](TaskRow& r) {
            if (r.editing()) {
                commit(r);
            }
        });
        save_if_changed();
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
