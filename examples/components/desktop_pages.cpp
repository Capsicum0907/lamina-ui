#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "components/base_page.hpp"
#include "components/demo_frame.hpp"
#include "components/files_view.hpp"
#include "components/pages.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/chip_group.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/list_item.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/numeric_field.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/segmented_button.hpp"
#include "lamina/widgets/split_pane.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/top_app_bar.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

struct DesktopLayout {
    double column_width = 324.0;
    double column_gap = 24.0;
    double item_gap = 10.0;
    std::array<int, 4> densities{0, -1, -2, -3};
    layout::Size editor{1392.0, 660.0};
    double assets_width = 260.0;
    double assets_min = 180.0;
    double stage_min = 360.0;
    double preview_min = 240.0;
    double inspector_width = 300.0;
    double inspector_min = 240.0;
    double top_height = 380.0;
    double top_min = 200.0;
    double timeline_min = 140.0;
    double pane_padding = 12.0;
    double field_gap = 8.0;
    double numeric_label_width = 72.0;
    double numeric_width = 112.0;
    tokens::Density editor_density{-2};
    color::Role pane = color::Role::surface_container_low;
    color::Role preview = color::Role::surface_container_highest;
    color::Role preview_art = color::Role::primary_container;
    double track_height = 44.0;
    double ruler_height = 28.0;
    double header_width = 120.0;
    double pixels_per_second = 80.0;
    double seconds = 40.0;
    double clip_inset = 6.0;
    double clip_corner = 8.0;
    double tick_major = 5.0;
    double tick_height = 8.0;
    double tick_width = 1.0;
    color::Role ruler = color::Role::surface_container;
    color::Role tick = color::Role::outline_variant;
    color::Role tick_label = color::Role::on_surface_variant;
    std::array<color::Role, 3> clip_colors{color::Role::primary_container, color::Role::secondary_container, color::Role::tertiary_container};
    std::array<color::Role, 3> clip_text{color::Role::on_primary_container, color::Role::on_secondary_container, color::Role::on_tertiary_container};
    tokens::TypeRole timeline_type = tokens::TypeRole::label_medium;
};

const DesktopLayout desktop_layout;

struct Clip {
    std::size_t track;
    double start;
    double length;
    const char* name;
};

constexpr std::array<const char*, 5> track_names{"Camera", "Lyrics", "Background", "Sparkles", "Audio"};
constexpr std::array<Clip, 12> clips{{
    {0, 0.0, 6.5, "cam-01 wide"},
    {0, 6.5, 9.0, "cam-02 close"},
    {0, 15.5, 12.0, "cam-03 pan"},
    {0, 27.5, 12.5, "cam-04 tilt"},
    {1, 2.0, 7.0, "First line"},
    {1, 10.0, 8.0, "Second line"},
    {1, 21.0, 10.0, "Chorus"},
    {2, 0.0, 22.0, "clouds.png"},
    {2, 22.0, 18.0, "night.png"},
    {3, 12.0, 4.0, "glitter"},
    {3, 30.0, 6.0, "stars"},
    {4, 0.0, 40.0, "demo_song.m4a"},
}};

class Timeline {
public:
    explicit Timeline(const text::FontStack& fonts) : fonts_(fonts) {
        const auto style = tokens::text_style(tokens::type_style(desktop_layout.timeline_type));
        for (const char* n : track_names) {
            names_.push_back(text::layout_line(fonts, n, style));
        }
        for (const Clip& c : clips) {
            clip_labels_.push_back(text::layout_line(fonts, c.name, style));
        }
        for (int s = 0; s <= static_cast<int>(desktop_layout.seconds); s += static_cast<int>(desktop_layout.tick_major)) {
            ticks_.push_back(text::layout_line(fonts, std::to_string(s / 60) + ":" + (s % 60 < 10 ? "0" : "") + std::to_string(s % 60), style));
        }
    }

    layout::Size measure(const layout::Constraints& c) const {
        return c.constrain({std::isfinite(c.max_width) ? c.max_width : c.min_width, std::isfinite(c.max_height) ? c.max_height : c.min_height});
    }
    void arrange(const render::Rect& r) {
        const DesktopLayout& l = desktop_layout;
        rect_ = r;
        scroll_.set_viewport({r.x + l.header_width, r.y + l.ruler_height, std::max(0.0, r.width - l.header_width), std::max(0.0, r.height - l.ruler_height)});
        scroll_.set_content_width(l.seconds * l.pixels_per_second);
        scroll_.set_content_height(static_cast<double>(track_names.size()) * l.track_height);
    }
    render::Rect bounds() const { return rect_; }
    widgets::ScrollArea& scroll() { return scroll_; }
    const widgets::ScrollArea& scroll() const { return scroll_; }

    bool handle(const input::WheelEvent& e) { return scroll_.handle(e); }
    bool handle(const input::PointerEvent& e) { return scroll_.handle(e); }
    void cancel_interaction() { scroll_.cancel_interaction(); }
    void step(double dt) { scroll_.step(dt); }
    bool animating() const { return scroll_.animating(); }
    widgets::AccessibleNode accessible() const {
        widgets::AccessibleNode n{widgets::AccessibleRole::group, "Timeline", rect_};
        n.value = std::to_string(static_cast<int>(std::lround(scroll_.offset_x() / desktop_layout.pixels_per_second))) + " s";
        return n;
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
        const DesktopLayout& l = desktop_layout;
        const render::Rect v = scroll_.viewport();
        canvas.save();
        canvas.push_clip(rect_);
        canvas.fill_rect({rect_.x, rect_.y, rect_.width, l.ruler_height}, scheme[l.ruler]);
        canvas.push_clip({v.x, rect_.y, v.width, l.ruler_height});
        for (std::size_t i = 0; i < ticks_.size(); ++i) {
            const double x = v.x - scroll_.offset_x() + static_cast<double>(i) * l.tick_major * l.pixels_per_second;
            canvas.fill_rect({x, rect_.y + l.ruler_height - l.tick_height, l.tick_width, l.tick_height}, scheme[l.tick]);
            text::draw_line(canvas, fonts_, ticks_[i], {x + 4.0, rect_.y + (l.ruler_height - l.tick_height - ticks_[i].box_height()) / 2.0 + 2.0},
                            scheme[l.tick_label]);
        }
        canvas.pop_clip();
        canvas.push_clip({rect_.x, v.y, l.header_width, v.height});
        for (std::size_t i = 0; i < names_.size(); ++i) {
            const double y = v.y - scroll_.offset() + static_cast<double>(i) * l.track_height;
            text::draw_line(canvas, fonts_, names_[i], {rect_.x + 12.0, y + (l.track_height - names_[i].box_height()) / 2.0}, scheme[l.tick_label]);
        }
        canvas.pop_clip();
        scroll_.begin_content(canvas);
        for (std::size_t i = 0; i <= names_.size(); ++i) {
            canvas.fill_rect({0.0, static_cast<double>(i) * l.track_height, l.seconds * l.pixels_per_second, l.tick_width}, scheme[l.tick]);
        }
        for (std::size_t i = 0; i < clips.size(); ++i) {
            const Clip& c = clips[i];
            const render::Rect r{c.start * l.pixels_per_second + 1.0, static_cast<double>(c.track) * l.track_height + l.clip_inset,
                                 c.length * l.pixels_per_second - 2.0, l.track_height - 2.0 * l.clip_inset};
            const std::size_t k = c.track % l.clip_colors.size();
            canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(l.clip_corner)), scheme[l.clip_colors[k]]);
            canvas.save();
            canvas.push_clip(r);
            text::draw_line(canvas, fonts_, clip_labels_[i], {r.x + 8.0, r.y + (r.height - clip_labels_[i].box_height()) / 2.0}, scheme[l.clip_text[k]]);
            canvas.pop_clip();
            canvas.restore();
        }
        scroll_.end_content(canvas);
        scroll_.paint(canvas, scheme);
        canvas.pop_clip();
        canvas.restore();
    }

private:
    text::FontStack fonts_;
    std::vector<text::TextLine> names_;
    std::vector<text::TextLine> clip_labels_;
    std::vector<text::TextLine> ticks_;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
    widgets::ScrollArea scroll_;
};

class Choice final : public layout::Node {
public:
    Choice(std::vector<layout::NodePtr> options, const std::size_t& current) : options_(std::move(options)), current_(current) {}

protected:
    layout::Size on_measure(const layout::Constraints& c) override { return options_[current_]->measure(c); }
    void on_arrange(const render::Rect& r) override { options_[current_]->arrange(r); }

private:
    std::vector<layout::NodePtr> options_;
    const std::size_t& current_;
};

struct DensityColumn {
    widgets::WidgetSet widgets;
    widgets::FocusManager focus;
    layout::NodePtr node;
};

template <typename W>
layout::NodePtr stretched(W& w) {
    return std::make_shared<layout::Leaf>([&w](const layout::Constraints& c) { return layout::Size{c.max_width, w.height()}; },
                                          [&w](const render::Rect& r) {
                                              w.set_width(r.width);
                                              w.set_origin({r.x, r.y});
                                          });
}

class DesktopPage final : public BasePage {
public:
    explicit DesktopPage(PageContext context)
        : BasePage(std::move(context)), editor_frame_(desktop_layout.editor), timeline_(ctx.fonts), files_(std::make_unique<FilesView>(ctx)) {
        const DesktopLayout& l = desktop_layout;
        views_ = &controls.emplace_with<widgets::SegmentedButtonRow>(
            [this](widgets::SegmentedButtonRow&, const widgets::SegmentedButtonResponse& r) {
                if (r.clicked) {
                    view_ = *r.clicked;
                    reflow.snap();
                    ctx.report(view_ == 0 ? "View: density" : view_ == 1 ? "View: editor" : "View: files");
                }
            },
            widgets::SegmentedButtonRowOptions{{{"Density", std::nullopt, true}, {"Editor"}, {"Files"}}, widgets::SegmentSelection::single, "View"}, ctx.fonts,
            ctx.icon_font);
        focus_.add(*views_);

        std::vector<layout::NodePtr> columns;
        for (int d : l.densities) {
            columns.push_back(build_column(d));
        }
        auto density_row = items(std::move(columns));
        density_row->spacing(l.column_gap).line_spacing(l.column_gap).cross_align(layout::CrossAlign::start);
        auto editor = editor_frame_.node(build_editor());
        auto body = std::make_shared<Choice>(std::vector<layout::NodePtr>{density_row, editor, files_->node()}, view_);
        root = this->columns({{widgets::node(*views_), body}}, l.column_gap, base_page_layout.heading_gap * 2.0);
        for (std::size_t i = 0; i < columns_.size(); ++i) {
            focus_.add_group(columns_[i]->focus, [this] { return view_ == 0; });
        }
        focus_.add_group(editor_focus_, [this] { return view_ == 1; });
        focus_.add_group(files_->focus(), [this] { return view_ == 2; });
    }

    bool handle(const input::PointerEvent& e) override {
        handle_tips(e);
        if (view_ == 2) {
            if (files_->handle(e)) {
                controls.handle(input::PointerEvent{input::PointerAction::leave, 0.0, 0.0});
                return true;
            }
            return handle_controls(e);
        }
        if (view_ == 0) {
            for (auto& c : columns_) {
                c->widgets.handle(e);
            }
            return handle_controls(e);
        }
        for (widgets::SplitPane* s : splits_) {
            if (s->handle(e)) {
                assets_.handle(input::PointerEvent{input::PointerAction::leave, 0.0, 0.0});
                inspector_.handle(input::PointerEvent{input::PointerAction::leave, 0.0, 0.0});
                return true;
            }
        }
        if (timeline_.handle(e)) {
            return true;
        }
        deliver(assets_, assets_rect_, e);
        deliver(inspector_, inspector_rect_, e);
        return handle_controls(e);
    }

    bool handle(const input::KeyEvent& e) override {
        handle_tips(e);
        if (view_ == 2 && files_->handle(e)) {
            return true;
        }
        if (focus_.handle(e)) {
            return true;
        }
        if (view_ == 0) {
            bool consumed = false;
            for (auto& c : columns_) {
                consumed = c->widgets.handle(e) || consumed;
            }
            return consumed || controls.handle(e);
        }
        if (view_ == 2) {
            return controls.handle(e);
        }
        for (widgets::SplitPane* s : splits_) {
            if (s->handle(e)) {
                return true;
            }
        }
        const bool used = assets_.handle(e);
        return inspector_.handle(e) || used || controls.handle(e);
    }

    void handle(const input::TextEvent& e) override {
        if (view_ == 1) {
            inspector_.handle(e);
        } else {
            for (auto& c : columns_) {
                c->widgets.handle(e);
            }
        }
    }

    bool handle(const input::WheelEvent& e) override {
        if (view_ == 2) {
            return files_->handle(e);
        }
        return view_ == 1 && timeline_.handle(e);
    }

    void handle(const input::ContextClick& e) override {
        if (view_ == 2) {
            files_->handle(e);
        }
    }

    std::optional<render::Rect> caret_area() const override { return view_ == 1 ? editor_focus_.caret_area() : std::nullopt; }

    void step(double dt) override {
        BasePage::step(dt);
        for (auto& c : columns_) {
            c->widgets.step(dt);
        }
        assets_.step(dt);
        inspector_.step(dt);
        for (widgets::SplitPane* s : splits_) {
            s->step(dt);
        }
        timeline_.step(dt);
        files_->step(dt);
    }

    bool animating() const override {
        bool any = BasePage::animating() || assets_.animating() || inspector_.animating() || timeline_.animating() || (view_ == 2 && files_->animating());
        for (const auto& c : columns_) {
            any = any || c->widgets.animating();
        }
        for (const widgets::SplitPane* s : splits_) {
            any = any || s->animating();
        }
        return any;
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint(canvas, scheme);
        if (view_ == 0) {
            for (const auto& c : columns_) {
                c->widgets.paint(canvas, scheme);
            }
            return;
        }
        if (view_ == 2) {
            files_->paint(canvas, scheme);
            return;
        }
        const DesktopLayout& l = desktop_layout;
        editor_frame_.paint_background(canvas, scheme);
        editor_frame_.begin_clip(canvas);
        for (const render::Rect* r : {&assets_rect_, &inspector_rect_}) {
            canvas.fill_rect(*r, scheme[l.pane]);
        }
        canvas.fill_rect(preview_rect_, scheme[l.preview]);
        const double s = std::min(preview_rect_.width, preview_rect_.height) * 0.5;
        const render::Rect art{preview_rect_.x + (preview_rect_.width - s * 16.0 / 9.0) / 2.0, preview_rect_.y + (preview_rect_.height - s) / 2.0,
                               s * 16.0 / 9.0, s};
        canvas.fill_path(render::rounded_rect_path(art, render::uniform_radii(12.0)), scheme[l.preview_art]);
        for (const auto& [set, rect] : {std::pair{&assets_, &assets_rect_}, std::pair{&inspector_, &inspector_rect_}}) {
            canvas.save();
            canvas.push_clip(*rect);
            set->paint(canvas, scheme);
            canvas.pop_clip();
            canvas.restore();
        }
        timeline_.paint(canvas, scheme);
        for (const widgets::SplitPane* sp : splits_) {
            sp->paint(canvas, scheme);
        }
        editor_frame_.end_clip(canvas);
        editor_frame_.paint_border(canvas, scheme);
    }

    void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint_overlays(canvas, scheme);
        if (view_ == 2) {
            files_->paint_overlays(canvas, scheme);
        }
    }

    void set_window(layout::Size window) override {
        BasePage::set_window(window);
        files_->set_window(window);
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        if (view_ == 2) {
            for (widgets::AccessibleNode& n : files_->accessible_nodes()) {
                nodes.push_back(std::move(n));
            }
            return nodes;
        }
        if (view_ == 0) {
            for (std::size_t i = 0; i < columns_.size(); ++i) {
                widgets::AccessibleNode g{widgets::AccessibleRole::group, "Density " + std::to_string(desktop_layout.densities[i]), column_rects_[i]};
                g.children = columns_[i]->widgets.accessible_nodes();
                nodes.push_back(std::move(g));
            }
            return nodes;
        }
        for (const widgets::WidgetSet* set : {&assets_, &inspector_}) {
            for (widgets::AccessibleNode& n : set->accessible_nodes()) {
                nodes.push_back(std::move(n));
            }
        }
        nodes.push_back(timeline_.accessible());
        for (const widgets::SplitPane* s : splits_) {
            nodes.push_back(s->accessible());
        }
        return nodes;
    }

    void set_enabled(bool enabled) override {
        BasePage::set_enabled(enabled);
        for (auto& c : columns_) {
            c->widgets.set_enabled(enabled);
        }
        assets_.set_enabled(enabled);
        inspector_.set_enabled(enabled);
        for (widgets::SplitPane* s : splits_) {
            s->set_enabled(enabled);
        }
        files_->set_enabled(enabled);
    }

    void cancel_interaction() override {
        BasePage::cancel_interaction();
        for (auto& c : columns_) {
            c->widgets.cancel_interaction();
        }
        assets_.cancel_interaction();
        inspector_.cancel_interaction();
        for (widgets::SplitPane* s : splits_) {
            s->cancel_interaction();
        }
        timeline_.cancel_interaction();
        files_->cancel_interaction();
    }

    std::optional<input::Cursor> cursor() const override {
        if (view_ == 2) {
            return files_->cursor();
        }
        if (view_ != 1) {
            return std::nullopt;
        }
        for (const widgets::SplitPane* s : splits_) {
            if (const auto c = s->cursor()) {
                return c;
            }
        }
        std::optional<input::Cursor> found;
        inspector_.for_each<widgets::NumericField>([&found](const widgets::NumericField& f) {
            if (!found) {
                found = f.cursor();
            }
        });
        return found;
    }

private:
    static void deliver(widgets::WidgetSet& set, const render::Rect& area, const input::PointerEvent& e) {
        if (e.action != input::PointerAction::leave && widgets::contains(area, e.x_dp, e.y_dp)) {
            set.handle(e);
        } else if (e.action == input::PointerAction::down) {
            set.handle(input::PointerEvent{input::PointerAction::down, -1.0e9, -1.0e9});
        } else {
            set.handle(input::PointerEvent{input::PointerAction::leave, 0.0, 0.0});
        }
    }

    layout::NodePtr build_column(int d) {
        const DesktopLayout& l = desktop_layout;
        auto col = std::make_unique<DensityColumn>();
        widgets::WidgetSet& w = col->widgets;
        widgets::FocusManager& f = col->focus;
        auto& heading = w.emplace<widgets::Label>(
            widgets::LabelOptions{"Density " + std::to_string(d), base_page_layout.heading_type, base_page_layout.heading, false}, ctx.fonts);
        auto& bar = w.emplace<widgets::TopAppBar>(
            widgets::TopAppBarOptions{.title = "Inbox",
                                      .navigation = widgets::IconButtonOptions{icon(res::icons::menu), "Menu"},
                                      .actions = {widgets::IconButtonOptions{icon(res::icons::search), "Search"},
                                                  widgets::IconButtonOptions{icon(res::icons::more_vert), "More"}}},
            ctx.fonts);
        auto& tabs = w.emplace<widgets::TabRow>(widgets::TabRowOptions{{{"Mail"}, {"Chat"}, {"Meet"}}, tokens::TabStyle::secondary, 0,
                                                                       widgets::TabIconPosition::top, "Sections"},
                                                ctx.fonts);
        f.add(tabs);
        auto& seg = w.emplace<widgets::SegmentedButtonRow>(
            widgets::SegmentedButtonRowOptions{{{"Day", std::nullopt, true}, {"Week"}, {"Month"}}, widgets::SegmentSelection::single, "Range"},
            ctx.fonts, ctx.icon_font);
        f.add(seg);
        auto& save = w.emplace<widgets::Button>(widgets::ButtonOptions{.label = "Save"}, ctx.fonts);
        auto& share = w.emplace<widgets::Button>(widgets::ButtonOptions{.label = "Share", .style = tokens::ButtonStyle::tonal}, ctx.fonts);
        auto& edit = w.emplace<widgets::IconButton>(widgets::IconButtonOptions{icon(res::icons::edit), "Edit"});
        auto& add = w.emplace<widgets::IconButton>(widgets::IconButtonOptions{icon(res::icons::add), "Add", tokens::IconButtonStyle::filled});
        for (auto* b : {&save, &share}) {
            f.add(*b);
        }
        f.add(edit);
        f.add(add);
        gated_tip(edit, [this] { return view_ == 0; });
        gated_tip(add, [this] { return view_ == 0; });
        auto& field = w.emplace<widgets::TextField>(widgets::TextFieldOptions{.label = "Name", .clipboard = nullptr, .min_width = 120.0}, ctx.fonts);
        f.add(field);
        auto& chips = w.emplace<widgets::ChipGroup>(
            widgets::ChipGroupOptions{{{.label = "Recent", .kind = tokens::ChipKind::filter, .selected = true},
                                       {.label = "Starred", .kind = tokens::ChipKind::filter},
                                       {.label = "Mine", .kind = tokens::ChipKind::filter}},
                                      false,
                                      "Filters"},
            ctx.fonts);
        f.add(chips);
        std::vector<layout::NodePtr> list;
        const std::array<widgets::ListItemSpec, 3> specs{{
            {.headline = "Wi-Fi", .leading = widgets::ListSlot::of_icon(icon(res::icons::wifi)), .transparent = true},
            {.headline = "Bluetooth", .supporting = "Connected to headphones", .leading = widgets::ListSlot::of_icon(icon(res::icons::bluetooth)),
             .transparent = true},
            {.headline = "Location", .leading = widgets::ListSlot::of_icon(icon(res::icons::location_on)),
             .trailing = widgets::ListSlot::switch_control(), .action = widgets::ListItemAction::toggle, .transparent = true},
        }};
        for (const widgets::ListItemSpec& s : specs) {
            auto& item = w.emplace<widgets::ListItem>(s, ctx.fonts);
            f.add(item);
            list.push_back(widgets::node(item));
        }
        auto& nav = w.emplace<widgets::NavigationBar>(
            widgets::NavigationBarOptions{{{icon(res::icons::home), "Home"}, {icon(res::icons::folder), "Files"}, {icon(res::icons::person), "Me"}}},
            ctx.fonts);
        f.add(nav);
        w.set_density(tokens::Density{d});
        auto buttons = layout::row({widgets::node(save), widgets::node(share), widgets::node(edit), widgets::node(add)});
        buttons->spacing(l.item_gap).cross_align(layout::CrossAlign::center);
        auto list_col = layout::column(std::move(list));
        list_col->fill_cross(true);
        const std::size_t index = columns_.size();
        auto c = layout::column({widgets::node(heading), widgets::measured_node(bar), widgets::full_width_node(tabs), widgets::node(seg), buttons,
                                 stretched(field), widgets::measured_node(chips), list_col, widgets::full_width_node(nav)});
        c->spacing(l.item_gap).fill_cross(true);
        auto sized = std::make_shared<layout::Leaf>(
            [c](const layout::Constraints& k) {
                const layout::Size s = c->measure({desktop_layout.column_width, desktop_layout.column_width, 0.0, k.max_height});
                return layout::Size{desktop_layout.column_width, s.height};
            },
            [this, c, index](const render::Rect& r) {
                c->arrange(r);
                column_rects_[index] = r;
            });
        columns_.push_back(std::move(col));
        return sized;
    }

    layout::NodePtr build_editor() {
        const DesktopLayout& l = desktop_layout;
        std::vector<layout::NodePtr> assets;
        constexpr std::array<std::array<const char*, 2>, 6> files{{{"clouds.png", "Image, 1920 x 1080"},
                                                                    {"night.png", "Image, 1920 x 1080"},
                                                                    {"demo_song.m4a", "Audio, 3:12"},
                                                                    {"cam-01.mp4", "Video, 0:06"},
                                                                    {"cam-02.mp4", "Video, 0:09"},
                                                                    {"lyrics.txt", "Text, 24 lines"}}};
        const std::array<icons::Symbol, 6> symbols{res::icons::image, res::icons::image, res::icons::mic, res::icons::videocam, res::icons::videocam,
                                                   res::icons::text_fields};
        for (std::size_t i = 0; i < files.size(); ++i) {
            auto& item = assets_.emplace<widgets::ListItem>(
                widgets::ListItemSpec{.headline = files[i][0],
                                      .supporting = files[i][1],
                                      .leading = widgets::ListSlot::of_icon(icon(symbols[i])),
                                      .action = widgets::ListItemAction::select,
                                      .selected = i == 0,
                                      .transparent = true,
                                      .density = l.editor_density},
                ctx.fonts);
            editor_focus_.add(item);
            assets.push_back(widgets::node(item));
        }
        auto assets_col = layout::column(std::move(assets));
        assets_col->fill_cross(true).padding(layout::Insets::all(l.pane_padding / 2.0));
        auto assets_node = std::make_shared<layout::Leaf>(
            [assets_col](const layout::Constraints& c) {
                assets_col->measure({c.max_width, c.max_width, 0.0, c.max_height});
                return layout::Size{c.max_width, c.max_height};
            },
            [this, assets_col](const render::Rect& r) {
                assets_rect_ = r;
                assets_col->arrange({r.x, r.y, r.width, assets_col->measured().height});
            });

        auto preview_node = std::make_shared<layout::Leaf>([](const layout::Constraints& c) { return layout::Size{c.max_width, c.max_height}; },
                                                           [this](const render::Rect& r) { preview_rect_ = r; });

        struct Spec {
            const char* label;
            double value;
            double min;
            double max;
            double step;
            int decimals;
            const char* unit;
        };
        const std::array<Spec, 7> specs{{{"X", 960.0, -4000.0, 4000.0, 1.0, 0, "px"},
                                         {"Y", 540.0, -4000.0, 4000.0, 1.0, 0, "px"},
                                         {"Width", 1920.0, 1.0, 8000.0, 1.0, 0, "px"},
                                         {"Height", 1080.0, 1.0, 8000.0, 1.0, 0, "px"},
                                         {"Rotation", 0.0, -360.0, 360.0, 1.0, 0, "°"},
                                         {"Opacity", 100.0, 0.0, 100.0, 1.0, 0, "%"},
                                         {"Duration", 6.5, 0.1, 600.0, 0.1, 1, "s"}}};
        std::vector<layout::NodePtr> fields;
        auto& heading = inspector_.emplace<widgets::Label>(
            widgets::LabelOptions{"Layer: cam-01 wide", tokens::TypeRole::title_small, color::Role::on_surface, false}, ctx.fonts);
        fields.push_back(widgets::node(heading));
        for (const Spec& s : specs) {
            auto& field = inspector_.emplace_with<widgets::NumericField>(
                [this](widgets::NumericField& w, const widgets::NumericFieldResponse& r) {
                    if (r.committed && !w.error()) {
                        ctx.report(w.accessible().name + ": " + w.formatted());
                    }
                },
                widgets::NumericFieldOptions{.label = s.label,
                                             .value = s.value,
                                             .min = s.min,
                                             .max = s.max,
                                             .step = s.step,
                                             .large_step = s.step * 10.0,
                                             .decimals = s.decimals,
                                             .unit = s.unit,
                                             .width = l.numeric_width,
                                             .label_width = l.numeric_label_width,
                                             .clipboard = nullptr,
                                             .density = tokens::Density{-2}},
                ctx.fonts);
            editor_focus_.add(field);
            fields.push_back(widgets::node(field));
        }
        auto& blend = inspector_.emplace_with<widgets::NumericField>(
            [this](widgets::NumericField& w, const widgets::NumericFieldResponse& r) {
                if (r.committed) {
                    ctx.report(w.accessible().name + ": " + w.formatted());
                }
            },
            widgets::NumericFieldOptions{.label = "Blur",
                                         .value = 4.0,
                                         .min = 0.0,
                                         .max = 100.0,
                                         .unit = "px",
                                         .label_position = widgets::NumericLabelPosition::inside,
                                         .width = l.numeric_label_width + l.numeric_width + l.field_gap,
                                         .label_width = 36.0,
                                         .density = tokens::Density{-2}},
            ctx.fonts);
        editor_focus_.add(blend);
        fields.push_back(widgets::node(blend));
        auto inspector_col = layout::column(std::move(fields));
        inspector_col->spacing(l.field_gap).padding(layout::Insets::all(l.pane_padding));
        auto inspector_node = std::make_shared<layout::Leaf>(
            [inspector_col](const layout::Constraints& c) {
                inspector_col->measure({0.0, c.max_width, 0.0, c.max_height});
                return layout::Size{c.max_width, c.max_height};
            },
            [this, inspector_col](const render::Rect& r) {
                inspector_rect_ = r;
                inspector_col->arrange({r.x, r.y, r.width, inspector_col->measured().height});
            });
        auto timeline_node = widgets::measured_node(timeline_);

        top_split_ = std::make_unique<widgets::SplitPane>(widgets::SplitPaneOptions{.first = preview_node,
                                                                                    .second = inspector_node,
                                                                                    .first_min = l.preview_min,
                                                                                    .second_min = l.inspector_min,
                                                                                    .first_fraction = 0.6,
                                                                                    .accessible_name = "Preview and inspector"});
        stage_split_ = std::make_unique<widgets::SplitPane>(widgets::SplitPaneOptions{.axis = layout::Axis::vertical,
                                                                                      .first = widgets::measured_node(*top_split_),
                                                                                      .second = timeline_node,
                                                                                      .first_min = l.top_min,
                                                                                      .second_min = l.timeline_min,
                                                                                      .first_size = l.top_height,
                                                                                      .accessible_name = "Stage and timeline"});
        outer_split_ = std::make_unique<widgets::SplitPane>(widgets::SplitPaneOptions{.first = assets_node,
                                                                                      .second = widgets::measured_node(*stage_split_),
                                                                                      .first_min = l.assets_min,
                                                                                      .second_min = l.stage_min,
                                                                                      .first_size = l.assets_width,
                                                                                      .accessible_name = "Assets and stage"});
        splits_ = {outer_split_.get(), stage_split_.get(), top_split_.get()};
        editor_focus_.add(*outer_split_);
        editor_focus_.add(*stage_split_);
        editor_focus_.add(*top_split_);
        return widgets::measured_node(*outer_split_);
    }

    widgets::SegmentedButtonRow* views_ = nullptr;
    std::size_t view_ = 0;
    std::vector<std::unique_ptr<DensityColumn>> columns_;
    std::array<render::Rect, 4> column_rects_{};
    DemoFrame editor_frame_;
    widgets::WidgetSet assets_;
    widgets::WidgetSet inspector_;
    widgets::FocusManager editor_focus_;
    Timeline timeline_;
    std::unique_ptr<widgets::SplitPane> outer_split_;
    std::unique_ptr<widgets::SplitPane> stage_split_;
    std::unique_ptr<widgets::SplitPane> top_split_;
    std::vector<widgets::SplitPane*> splits_;
    std::unique_ptr<FilesView> files_;
    render::Rect assets_rect_{0.0, 0.0, 0.0, 0.0};
    render::Rect preview_rect_{0.0, 0.0, 0.0, 0.0};
    render::Rect inspector_rect_{0.0, 0.0, 0.0, 0.0};
};

}

std::unique_ptr<Page> make_desktop_page(PageContext context) {
    return std::make_unique<DesktopPage>(std::move(context));
}

}
