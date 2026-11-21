#include "components/files_view.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <tuple>
#include <utility>

#include "components/demo_frame.hpp"
#include "lamina/widgets/context_menu.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/menu_bar.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/shortcuts.hpp"
#include "lamina/widgets/table.hpp"

namespace lamina::examples::components {

namespace {

using input::Key;
using widgets::KeyChord;

struct FilesLayout {
    layout::Size frame{1392.0, 660.0};
    std::size_t rows = 100000;
    tokens::Density density{-2};
    std::array<int, 3> densities{0, -2, -4};
    double status_height = 32.0;
    double status_padding = 16.0;
    tokens::TypeRole status_type = tokens::TypeRole::body_small;
    color::Role status_color = color::Role::on_surface_variant;
    color::Role status_container = color::Role::surface_container_low;
    double keyboard_point_inset = 16.0;
    std::array<double, 8> widths{320.0, 170.0, 110.0, 180.0, 180.0, 130.0, 150.0, 250.0};
    int first_day = 19723;
    int day_span = 1050;
};

const FilesLayout files_layout;

struct Prefix {
    const char* name;
    const char* extension;
    const char* kind;
};

constexpr std::array<Prefix, 6> prefixes{{
    {"Clip ", ".mp4", "MPEG-4 video"},
    {"IMG_", ".jpg", "JPEG image"},
    {"Invoice ", ".xlsx", "Spreadsheet"},
    {"Notes ", ".txt", "Text document"},
    {"Report ", ".pdf", "PDF document"},
    {"Track ", ".m4a", "MPEG-4 audio"},
}};
constexpr std::array<const char*, 5> owners{"Ada", "Grace", "Linus", "Margaret", "Ken"};
constexpr std::array<const char*, 6> tags{"", "work", "family, 2026", "archive", "draft, review", "travel"};
constexpr std::array<const char*, 8> titles{"Name", "Kind", "Size", "Modified", "Created", "Dimensions", "Owner", "Tags"};

std::uint64_t mix(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

struct FileRow {
    std::size_t prefix;
    std::uint64_t bytes;
    int modified;
    int created;
    std::size_t owner;
    std::size_t tags;
};

FileRow file_row(std::size_t i) {
    const std::uint64_t h = mix(i);
    const std::size_t prefix = static_cast<std::size_t>(h % prefixes.size());
    const int minutes = files_layout.day_span * 24 * 60;
    const int modified = static_cast<int>((h >> 8) % static_cast<std::uint64_t>(minutes));
    const int created = std::max(0, modified - static_cast<int>((h >> 24) % 200000));
    return {prefix, 4096 + (h >> 32) % 48000000, modified, created, static_cast<std::size_t>((h >> 40) % owners.size()),
            static_cast<std::size_t>((h >> 48) % tags.size())};
}

std::string format_minutes(int minutes) {
    const int z = files_layout.first_day + minutes / (24 * 60) + 719468;
    const int era = z / 146097;
    const int doe = z - era * 146097;
    const int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int mp = (5 * doy + 2) / 153;
    const int d = doy - (153 * mp + 2) / 5 + 1;
    const int m = mp < 10 ? mp + 3 : mp - 9;
    const int y = yoe + era * 400 + (m <= 2 ? 1 : 0);
    char out[32];
    std::snprintf(out, sizeof out, "%04d-%02d-%02d %02d:%02d", y, m, d, (minutes / 60) % 24, minutes % 60);
    return out;
}

std::string format_bytes(std::uint64_t bytes) {
    char out[32];
    if (bytes < 1024 * 1024) {
        std::snprintf(out, sizeof out, "%llu KB", static_cast<unsigned long long>((bytes + 1023) / 1024));
    } else {
        std::snprintf(out, sizeof out, "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    }
    return out;
}

std::string file_cell(std::size_t row, std::size_t column) {
    const FileRow f = file_row(row);
    const Prefix& p = prefixes[f.prefix];
    switch (column) {
    case 0: {
        char number[16];
        std::snprintf(number, sizeof number, "%06zu", row + 1);
        return std::string(p.name) + number + p.extension;
    }
    case 1: return p.kind;
    case 2: return format_bytes(f.bytes);
    case 3: return format_minutes(f.modified);
    case 4: return format_minutes(f.created);
    case 5: return f.prefix <= 1 ? std::to_string(1280 + (f.bytes % 5) * 160) + " × " + std::to_string(720 + (f.bytes % 5) * 90) : "—";
    case 6: return owners[f.owner];
    default: return tags[f.tags];
    }
}

std::string with_separators(std::size_t n) {
    std::string s = std::to_string(n);
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) {
        s.insert(static_cast<std::size_t>(i), ",");
    }
    return s;
}

}

struct FilesView::Impl {
    explicit Impl(const PageContext& c);

    void command(const std::string& id, std::string text, std::optional<KeyChord> chord, std::function<void()> also = {}) {
        commands.add(id, chord, [this, text = std::move(text), also = std::move(also)] {
            if (also) {
                also();
            }
            ctx.report("Command: " + text);
        });
    }
    void open_context(input::InputSource source);
    void choose(const widgets::MenuResponse& r);
    void update_status();

    PageContext ctx;
    DemoFrame frame{files_layout.frame};
    widgets::ShortcutRegistry commands;
    std::unique_ptr<widgets::Table> table;
    std::unique_ptr<widgets::MenuBar> bar;
    std::unique_ptr<widgets::Menu> context_menu;
    std::unique_ptr<widgets::ContextMenuArea> area;
    std::unique_ptr<widgets::Label> status;
    widgets::FocusManager focus;
    render::Rect status_rect{0.0, 0.0, 0.0, 0.0};
    std::string status_text;
    layout::NodePtr root;
    layout::Size window{0.0, 0.0};
};

FilesView::Impl::Impl(const PageContext& context) : ctx(context) {
    const FilesLayout& l = files_layout;
    std::vector<widgets::TableColumn> columns;
    for (std::size_t i = 0; i < titles.size(); ++i) {
        widgets::TableColumn col{.title = titles[i], .width = l.widths[i]};
        switch (i) {
        case 0:
            col.less = [](std::size_t a, std::size_t b) {
                return std::tuple(file_row(a).prefix, a) < std::tuple(file_row(b).prefix, b);
            };
            break;
        case 1: col.less = [](std::size_t a, std::size_t b) { return file_row(a).prefix < file_row(b).prefix; }; break;
        case 2:
            col.align = widgets::ColumnAlign::end;
            col.less = [](std::size_t a, std::size_t b) { return file_row(a).bytes < file_row(b).bytes; };
            break;
        case 3: col.less = [](std::size_t a, std::size_t b) { return file_row(a).modified < file_row(b).modified; }; break;
        case 4: col.less = [](std::size_t a, std::size_t b) { return file_row(a).created < file_row(b).created; }; break;
        case 5: col.align = widgets::ColumnAlign::end; col.sortable = false; break;
        case 6: col.less = [](std::size_t a, std::size_t b) { return std::string_view(owners[file_row(a).owner]) < owners[file_row(b).owner]; }; break;
        default: col.sortable = false; break;
        }
        columns.push_back(std::move(col));
    }
    table = std::make_unique<widgets::Table>(
        widgets::TableOptions{.columns = std::move(columns), .row_count = l.rows, .cell = file_cell, .accessible_name = "File list", .density = l.density,
                              .icon_font = ctx.icon_font},
        ctx.fonts);
    focus.add(*table);

    command("new", "New window", KeyChord{Key::n, true});
    command("open", "Open", KeyChord{Key::o, true});
    command("save", "Save", KeyChord{Key::s, true});
    command("save-as", "Save as", KeyChord{Key::s, true, true});
    command("close", "Close", KeyChord{Key::w, true});
    command("recent-1", "Open holiday.jpg", std::nullopt);
    command("recent-2", "Open budget.xlsx", std::nullopt);
    command("recent-3", "Open lyrics.txt", std::nullopt);
    command("undo", "Undo", KeyChord{Key::z, true});
    command("redo", "Redo", KeyChord{Key::y, true});
    command("cut", "Cut", KeyChord{Key::x, true}, [this] { commands.set_enabled("paste", true); });
    command("copy", "Copy", KeyChord{Key::c, true}, [this] { commands.set_enabled("paste", true); });
    command("paste", "Paste", KeyChord{Key::v, true});
    command("delete", "Delete", KeyChord{Key::delete_forward});
    command("rename", "Rename", KeyChord{Key::f2});
    command("select-all", "Select all", KeyChord{Key::a, true}, [this] { table->select_all(); });
    command("properties", "Properties", KeyChord{Key::enter, false, false, true});
    command("refresh", "Refresh", KeyChord{Key::f5}, [this] { table->refresh(); });
    command("open-item", "Open the file", std::nullopt);
    for (std::size_t i = 0; i < l.densities.size(); ++i) {
        const int d = l.densities[i];
        command("density-" + std::to_string(i), "Density " + std::to_string(d), std::nullopt,
                [this, d] { table->set_density(tokens::Density{d}); });
    }
    for (std::size_t c : {std::size_t{0}, std::size_t{2}, std::size_t{3}}) {
        command(std::string("sort-") + titles[c], std::string("Sort by ") + titles[c], std::nullopt,
                [this, c] { table->sort_by(c, widgets::SortDirection::ascending); });
    }
    command("shortcuts", "Keyboard shortcuts", KeyChord{Key::f1});
    command("about", "About lamina-ui", std::nullopt);
    commands.set_enabled("paste", false);

    const auto item = [](std::string label, std::string id, bool divider = false, std::optional<Key> key = std::nullopt) {
        return widgets::MenuItem{.label = std::move(label), .access_key = key, .command = std::move(id), .divider_before = divider};
    };
    widgets::MenuItem recent{.label = "Open recent",
                             .access_key = Key::r,
                             .submenu = {item("holiday.jpg", "recent-1"), item("budget.xlsx", "recent-2"), item("lyrics.txt", "recent-3")}};
    widgets::MenuItem density{.label = "Density", .submenu = {item("Default", "density-0"), item("Compact", "density-1"), item("Dense", "density-2")}};
    widgets::MenuItem sort{.label = "Sort by", .submenu = {item("Name", "sort-Name"), item("Size", "sort-Size"), item("Modified", "sort-Modified")}};
    bar = std::make_unique<widgets::MenuBar>(
        widgets::MenuBarOptions{
            .menus = {{"File", {item("New window", "new"), item("Open", "open"), recent, item("Save", "save", true), item("Save as", "save-as", false, Key::a),
                                item("Close", "close", true)}},
                      {"Edit", {item("Undo", "undo"), item("Redo", "redo"), item("Cut", "cut", true, Key::t), item("Copy", "copy"), item("Paste", "paste"),
                                item("Delete", "delete"), item("Select all", "select-all", true)}},
                      {"View", {density, sort, item("Refresh", "refresh", true, Key::r)}},
                      {"Help", {item("Keyboard shortcuts", "shortcuts"), item("About lamina-ui", "about", true)}}},
            .accessible_name = "Menu bar",
            .density = l.density,
            .icon_font = ctx.icon_font,
            .commands = &commands},
        ctx.fonts);

    std::vector<widgets::MenuItem> context_items{item("Open", "open-item"), item("Rename", "rename", false, Key::m), item("Cut", "cut", true, Key::t),
                                                 item("Copy", "copy"), item("Paste", "paste"), item("Delete", "delete"),
                                                 item("Properties", "properties", true, Key::r)};
    widgets::apply_shortcuts(context_items, commands);
    context_menu = std::make_unique<widgets::Menu>(
        widgets::MenuOptions{.items = std::move(context_items), .accessible_name = "File actions", .density = l.density, .icon_font = ctx.icon_font},
        ctx.fonts);
    area = std::make_unique<widgets::ContextMenuArea>(
        *context_menu,
        widgets::ContextMenuAreaOptions{
            .area = [this] { return table->bounds(); },
            .focused = [this] { return table->focused(); },
            .keyboard_point =
                [this]() -> std::optional<render::Point> {
                    const std::optional<std::size_t> r = table->focused_row();
                    if (!r) {
                        return std::nullopt;
                    }
                    const render::Rect rr = table->row_rect(*r);
                    return render::Point{table->viewport().x + files_layout.keyboard_point_inset, rr.y + rr.height};
                },
            .before_open =
                [this](render::Point p) {
                    table->handle(input::ContextClick{p.x, std::min(p.y, table->viewport().y + table->viewport().height - 1.0) - 1.0});
                    widgets::sync_commands(*context_menu, commands);
                }});

    status = std::make_unique<widgets::Label>(widgets::LabelOptions{"", l.status_type, l.status_color, false}, ctx.fonts);
    update_status();

    auto content = std::make_shared<layout::Leaf>([](const layout::Constraints& k) { return layout::Size{k.max_width, k.max_height}; },
                                                  [this](const render::Rect& r) {
                                                      bar->set_width(r.width);
                                                      bar->set_origin({r.x, r.y});
                                                      const double top = r.y + bar->height();
                                                      const double bottom = r.y + r.height - files_layout.status_height;
                                                      table->arrange({r.x, top, r.width, std::max(0.0, bottom - top)});
                                                      status_rect = {r.x, bottom, r.width, files_layout.status_height};
                                                      status->measure({0.0, r.width, 0.0, files_layout.status_height});
                                                      status->set_origin({r.x + files_layout.status_padding,
                                                                          bottom + (files_layout.status_height - status->bounds().height) / 2.0});
                                                  });
    root = frame.node(content);
}

void FilesView::Impl::update_status() {
    std::string text = with_separators(table->row_count()) + " items";
    if (table->selected_count() > 0) {
        text += "  ·  " + with_separators(table->selected_count()) + " selected";
    }
    if (const auto s = table->sort()) {
        text += std::string("  ·  sorted by ") + titles[s->column] + (s->direction == widgets::SortDirection::ascending ? ", ascending" : ", descending");
    }
    if (text != status_text) {
        status_text = text;
        status->set_text(text);
        status->measure({0.0, status_rect.width, 0.0, status_rect.height});
        status->set_origin({status_rect.x + files_layout.status_padding, status_rect.y + (status_rect.height - status->bounds().height) / 2.0});
    }
}

void FilesView::Impl::choose(const widgets::MenuResponse& r) {
    if (!r.command.empty() && commands.contains(r.command)) {
        commands.run(r.command);
    }
}

FilesView::FilesView(const PageContext& context) : impl_(std::make_unique<Impl>(context)) {}
FilesView::~FilesView() = default;

layout::NodePtr FilesView::node() {
    return impl_->root;
}

widgets::FocusManager& FilesView::focus() {
    return impl_->focus;
}

bool FilesView::handle(const input::PointerEvent& e) {
    Impl& v = *impl_;
    if (v.bar->handle(e).consumed) {
        v.table->handle(input::PointerEvent{input::PointerAction::leave, 0.0, 0.0});
        return true;
    }
    if (v.context_menu->is_open()) {
        const widgets::MenuResponse r = v.context_menu->handle(e);
        v.choose(r);
        return r.consumed;
    }
    const widgets::TableResponse r = v.table->handle(e);
    if (r.activated) {
        v.commands.run("open-item");
    }
    return r.consumed;
}

bool FilesView::handle(const input::KeyEvent& e) {
    Impl& v = *impl_;
    if (v.context_menu->is_open()) {
        const widgets::MenuResponse r = v.context_menu->handle(e);
        v.choose(r);
        return true;
    }
    if (v.bar->handle(e).consumed) {
        return true;
    }
    if (v.area->handle(e)) {
        return true;
    }
    if (!v.focus.claims_key(e) && v.commands.handle(e)) {
        return true;
    }
    const widgets::TableResponse r = v.table->handle(e);
    if (r.activated) {
        v.commands.run("open-item");
    }
    return r.consumed;
}

bool FilesView::handle(const input::WheelEvent& e) {
    if (impl_->bar->engaged() || impl_->context_menu->is_open()) {
        return true;
    }
    return impl_->table->handle(e);
}

bool FilesView::handle(const input::ContextClick& e) {
    Impl& v = *impl_;
    if (v.bar->engaged()) {
        v.bar->cancel_interaction();
    }
    if (v.context_menu->is_open()) {
        v.context_menu->close();
    }
    return v.area->handle(e);
}

void FilesView::step(double dt) {
    Impl& v = *impl_;
    v.bar->step(dt);
    v.table->step(dt);
    v.context_menu->step(dt);
    v.update_status();
}

bool FilesView::animating() const {
    return impl_->bar->animating() || impl_->table->animating() || impl_->context_menu->animating();
}

void FilesView::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const Impl& v = *impl_;
    v.frame.paint_background(canvas, scheme);
    v.frame.begin_clip(canvas);
    v.table->paint(canvas, scheme);
    canvas.fill_rect(v.status_rect, scheme[files_layout.status_container]);
    v.status->paint(canvas, scheme);
    v.bar->paint(canvas, scheme);
    v.frame.end_clip(canvas);
    v.frame.paint_border(canvas, scheme);
}

void FilesView::paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    impl_->bar->paint_menus(canvas, scheme);
    impl_->context_menu->paint(canvas, scheme);
}

std::vector<widgets::AccessibleNode> FilesView::accessible_nodes() const {
    const Impl& v = *impl_;
    std::vector<widgets::AccessibleNode> nodes{v.bar->accessible(), v.table->accessible(), v.status->accessible()};
    if (auto m = v.context_menu->accessible()) {
        nodes.push_back(std::move(*m));
    }
    return nodes;
}

void FilesView::set_enabled(bool enabled) {
    impl_->bar->set_enabled(enabled);
    impl_->table->set_enabled(enabled);
    if (!enabled) {
        impl_->context_menu->close();
    }
}

void FilesView::cancel_interaction() {
    impl_->bar->cancel_interaction();
    impl_->context_menu->close();
    impl_->table->cancel_interaction();
}

void FilesView::set_window(layout::Size window) {
    impl_->window = window;
    impl_->bar->set_window(window);
    impl_->area->set_window(window);
}

std::optional<input::Cursor> FilesView::cursor() const {
    return impl_->table->cursor();
}

}
