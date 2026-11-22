#include "components/pages.hpp"
#include "components/base_page.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <utility>

#include "common/caption_cell.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/render/image.hpp"
#include "lamina/render/path.hpp"
#include "lamina/widgets/badge.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/card.hpp"
#include "lamina/widgets/chip.hpp"
#include "lamina/widgets/chip_group.hpp"
#include "lamina/widgets/divider.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/list_item.hpp"
#include "lamina/widgets/list_view.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/tooltip.hpp"
#include "lamina/widgets/top_app_bar.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

using icons::Symbol;
using tokens::ChipKind;
using tokens::TypeRole;
namespace res = examples::res;

struct PageLayout : BasePageLayout {
    double row_gap = 20.0;
    double column_space = 40.0;
    double chip_area_width = 600.0;
    double wrap_area_width = 480.0;
    double card_width = 152.0;
    double card_padding = 16.0;
    double card_text_gap = 4.0;
    double divider_area_width = 480.0;
    double vertical_divider_height = 32.0;
    double settings_width = 400.0;
    double standalone_width = 360.0;
    double big_list_width = 440.0;
    double list_height = 640.0;
    std::size_t big_list_rows = 10000;
    std::size_t long_row_every = 7;
    double frame_width = 300.0;
    double frame_height = 380.0;
    double frame_corner = 16.0;
    double frame_outline = 1.0;
    color::Role frame_outline_color = color::Role::outline_variant;
    std::size_t frame_rows = 40;
    double badge_area_width = 300.0;
    double bar_width = 420.0;
    double rail_height = 330.0;
    double badge_button_gap = 32.0;
    int avatar_pixels = 64;
    std::array<std::pair<color::Argb, color::Argb>, 4> avatar_colors{{
        {0xff6750a4, 0xffd0bcff},
        {0xff006a6a, 0xff9cf1f0},
        {0xff8c4a60, 0xffffd8e4},
        {0xff3f6837, 0xffbfeeb0},
    }};
    long long add_small = 1;
    long long add_large = 100;
};

const PageLayout page_layout;

render::Image avatar_image(std::size_t index) {
    const int n = page_layout.avatar_pixels;
    const auto [a, b] = page_layout.avatar_colors[index % page_layout.avatar_colors.size()];
    render::Image img{n, n, std::vector<color::Argb>(static_cast<std::size_t>(n * n))};
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const double t = static_cast<double>(x + y) / static_cast<double>(2 * n - 2);
            const double dx = x - n / 2.0;
            const double dy = y - n * 0.42;
            const bool head = dx * dx + dy * dy < (n * 0.2) * (n * 0.2);
            const bool body = y > n * 0.68 && std::abs(dx) < n * 0.34;
            img.pixels[static_cast<std::size_t>(y * n + x)] = head || body ? b : color::mix(a, b, t * 0.35);
        }
    }
    return img;
}


class ChipsPage final : public BasePage {
public:
    explicit ChipsPage(PageContext context) : BasePage(std::move(context)) {
        root = columns({{assist(), suggestion(), filter(), input(), wrapping()}, {cards(), card_states(), dividers()}}, page_layout.column_space,
                       page_layout.row_gap);
    }

private:
    widgets::Chip& chip(std::vector<layout::NodePtr>& row, widgets::ChipOptions options) {
        auto& c = controls.emplace_with<widgets::Chip>(
            [this](widgets::Chip& w, const widgets::ChipResponse& r) {
                if (r.clicked) {
                    ctx.report("Chip: " + w.label() + (w.options().kind == ChipKind::filter ? (w.selected() ? " on" : " off") : ""));
                }
            },
            std::move(options), ctx.fonts);
        focus_.add(c);
        enabler(c.accessible().name, c);
        row.push_back(widgets::node(c));
        return c;
    }

    widgets::ChipGroup& group(widgets::ChipGroupOptions options) {
        auto& g = controls.emplace_with<widgets::ChipGroup>(
            [this](widgets::ChipGroup& w, const widgets::ChipGroupResponse& r) {
                if (r.removed) {
                    ctx.report("Removed: " + r.removed_label);
                } else if (r.clicked) {
                    const widgets::Chip& c = w.chip(*r.clicked);
                    ctx.report("Chip: " + c.label() + (c.selected() ? " on" : " off"));
                }
            },
            std::move(options), ctx.fonts);
        focus_.add(g);
        return g;
    }

    layout::NodePtr assist() {
        std::vector<layout::NodePtr> row;
        chip(row, {"Add to calendar", ChipKind::assist, false, icon(res::icons::event)});
        chip(row, {"Share", ChipKind::assist, true, icon(res::icons::share)});
        chip(row, {"Directions", ChipKind::assist, false, icon(res::icons::location_on), std::nullopt, std::nullopt, std::nullopt, false, false,
                   false});
        return captioned("Assist", items(std::move(row)));
    }

    layout::NodePtr suggestion() {
        std::vector<layout::NodePtr> row;
        chip(row, {"Tomorrow", ChipKind::suggestion});
        chip(row, {"Search flights", ChipKind::suggestion, false, icon(res::icons::flight)});
        chip(row, {"Next week", ChipKind::suggestion, true});
        chip(row, {"Later", ChipKind::suggestion, false, std::nullopt, std::nullopt, std::nullopt, std::nullopt, false, false, false});
        return captioned("Suggestion", items(std::move(row)));
    }

    layout::NodePtr filter() {
        widgets::ChipGroupOptions o;
        o.accessible_name = "Filters";
        o.chips.push_back({"Starred", ChipKind::filter, false, std::nullopt, icon(res::icons::check)});
        o.chips.back().selected = true;
        o.chips.push_back({"Unread", ChipKind::filter, false, std::nullopt, icon(res::icons::check)});
        o.chips.push_back({"Has files", ChipKind::filter, false, icon(res::icons::attach_file), icon(res::icons::check)});
        o.chips.push_back({"Elevated", ChipKind::filter, true, std::nullopt, icon(res::icons::check)});
        o.chips.push_back({"Archived", ChipKind::filter, false, std::nullopt, icon(res::icons::check)});
        o.chips.back().enabled = false;
        auto& g = group(std::move(o));
        return captioned("Filter", width_limited(widgets::measured_node(g), page_layout.chip_area_width));
    }

    layout::NodePtr input() {
        widgets::ChipGroupOptions o;
        o.accessible_name = "Recipients";
        const char* people[] = {"Ada", "Grace", "Alan"};
        for (std::size_t i = 0; i < std::size(people); ++i) {
            widgets::ChipOptions c{people[i], ChipKind::input};
            c.avatar = avatar_image(i);
            c.trailing_icon = icon(res::icons::close);
            c.removable = true;
            c.selected = i == 1;
            o.chips.push_back(std::move(c));
        }
        widgets::ChipOptions place{"Lisbon", ChipKind::input, false, icon(res::icons::location_on)};
        place.trailing_icon = icon(res::icons::close);
        place.removable = true;
        o.chips.push_back(place);
        widgets::ChipOptions plain{"Tokyo", ChipKind::input};
        plain.trailing_icon = icon(res::icons::close);
        plain.removable = true;
        o.chips.push_back(plain);
        widgets::ChipOptions off{"Paris", ChipKind::input};
        off.trailing_icon = icon(res::icons::close);
        off.removable = true;
        off.enabled = false;
        o.chips.push_back(off);
        auto& g = group(std::move(o));
        return captioned("Input", width_limited(widgets::measured_node(g), page_layout.chip_area_width));
    }

    layout::NodePtr wrapping() {
        widgets::ChipGroupOptions o;
        o.accessible_name = "Topics";
        o.single_selection = true;
        for (const char* topic : {"Music", "Podcasts", "Audiobooks", "News", "Sports", "Comedy", "Science", "History", "Travel", "Food",
                                  "Design", "Technology"}) {
            o.chips.push_back({topic, ChipKind::filter, false, std::nullopt, icon(res::icons::check)});
        }
        auto& g = group(std::move(o));
        return captioned("Chip group", width_limited(widgets::measured_node(g), page_layout.wrap_area_width));
    }

    widgets::Card& card(std::vector<layout::NodePtr>& row, tokens::CardStyle style, const char* title, const char* text, bool clickable,
                        bool enabled, const char* action) {
        auto& set = *contents.emplace_back(std::make_unique<widgets::WidgetSet>());
        auto& heading_label = set.emplace<widgets::Label>(widgets::LabelOptions{title, TypeRole::title_medium, color::Role::on_surface}, ctx.fonts);
        auto& body = set.emplace<widgets::Label>(widgets::LabelOptions{text, TypeRole::body_medium, color::Role::on_surface_variant}, ctx.fonts);
        std::vector<layout::NodePtr> nodes{widgets::node(heading_label), widgets::node(body)};
        widgets::Button* button = nullptr;
        if (action) {
            button = &set.emplace_with<widgets::Button>(
                [this, title](bool c) {
                    if (c) {
                        ctx.report(std::string("Card button: ") + title);
                    }
                },
                widgets::ButtonOptions{action, tokens::ButtonStyle::text}, ctx.fonts);
            nodes.push_back(widgets::node(*button));
        }
        auto column = layout::column(std::move(nodes));
        column->padding(layout::Insets::all(page_layout.card_padding)).spacing(page_layout.card_text_gap).fill_cross(true);
        auto& c = controls.emplace_with<widgets::Card>(
            [this, title](widgets::Card&, bool clicked) {
                if (clicked) {
                    ctx.report(std::string("Card: ") + title);
                }
            },
            widgets::CardOptions{style, clickable, enabled, column, &set, title});
        if (clickable) {
            focus_.add(c);
            enabler(title, c);
        }
        if (button) {
            focus_.add(*button);
        }
        row.push_back(width_limited(widgets::measured_node(c), page_layout.card_width));
        return c;
    }

    layout::NodePtr cards() {
        std::vector<layout::NodePtr> row;
        card(row, tokens::CardStyle::elevated, "Kyoto", "Temples, gardens and tea in the old capital.", true, true, nullptr);
        card(row, tokens::CardStyle::filled, "Porto", "Tiled streets, trams and the river at sunset.", true, true, nullptr);
        card(row, tokens::CardStyle::outlined, "Reykjavik", "Hot springs and the northern lights.", true, true, "Open");
        auto r = items(std::move(row));
        r->cross_align(layout::CrossAlign::start);
        return captioned("Cards", r);
    }

    layout::NodePtr card_states() {
        std::vector<layout::NodePtr> row;
        card(row, tokens::CardStyle::elevated, "Static", "A card without a click shows content only.", false, true, nullptr);
        card(row, tokens::CardStyle::filled, "Disabled", "A disabled card fades its content.", true, false, nullptr);
        card(row, tokens::CardStyle::outlined, "Outlined off", "Its border fades too.", true, false, nullptr);
        auto r = items(std::move(row));
        r->cross_align(layout::CrossAlign::start);
        return captioned("Card states", r);
    }

    layout::NodePtr dividers() {
        auto& full_label = controls.emplace<widgets::Label>(widgets::LabelOptions{"Full-bleed", TypeRole::body_medium, color::Role::on_surface}, ctx.fonts);
        auto& full = controls.emplace<widgets::Divider>(widgets::DividerOptions{});
        auto& inset_label = controls.emplace<widgets::Label>(widgets::LabelOptions{"Inset at both ends", TypeRole::body_medium, color::Role::on_surface},
                                                             ctx.fonts);
        auto& inset = controls.emplace<widgets::Divider>(widgets::DividerOptions{widgets::DividerOrientation::horizontal, widgets::DividerInset::both});
        auto& start_label =
            controls.emplace<widgets::Label>(widgets::LabelOptions{"Inset at the start", TypeRole::body_medium, color::Role::on_surface}, ctx.fonts);
        auto& start = controls.emplace<widgets::Divider>(widgets::DividerOptions{widgets::DividerOrientation::horizontal, widgets::DividerInset::start});
        auto& left = controls.emplace<widgets::Label>(widgets::LabelOptions{"Left", TypeRole::body_medium, color::Role::on_surface}, ctx.fonts);
        auto& vertical = controls.emplace<widgets::Divider>(widgets::DividerOptions{widgets::DividerOrientation::vertical});
        auto& right = controls.emplace<widgets::Label>(widgets::LabelOptions{"Right", TypeRole::body_medium, color::Role::on_surface}, ctx.fonts);
        auto split = layout::row({widgets::node(left), sized(widgets::measured_node(vertical), 1.0, page_layout.vertical_divider_height),
                                  widgets::node(right)});
        split->spacing(page_layout.column_gap).cross_align(layout::CrossAlign::center);
        auto column = layout::column({widgets::node(full_label), widgets::measured_node(full), widgets::node(inset_label),
                                      widgets::measured_node(inset), widgets::node(start_label), widgets::measured_node(start), split});
        column->spacing(page_layout.heading_gap).fill_cross(true);
        return captioned("Divider", width_limited(column, page_layout.divider_area_width));
    }
};

widgets::ListItemSpec spec(std::string headline, std::string supporting = {}, std::string overline = {}) {
    return {std::move(headline), std::move(supporting), std::move(overline)};
}

class ListsPage final : public BasePage {
public:
    explicit ListsPage(PageContext context) : BasePage(std::move(context)) {
        auto settings_list = settings();
        auto standalone_card = standalone();
        auto messages_list = messages();
        root = columns({{settings_list}, {standalone_card}, {messages_list}}, page_layout.column_space, page_layout.row_gap);
    }

    bool handle(const input::WheelEvent& e) override {
        bool used = false;
        controls.for_each<widgets::ListView>([&](widgets::ListView& l) { used = l.handle(e) || used; });
        return used;
    }

    std::optional<double> next_wake_seconds() const override {
        std::optional<double> wake = BasePage::next_wake_seconds();
        controls.for_each<widgets::ListView>([&](const widgets::ListView& l) {
            const double w = l.next_wake_seconds();
            if (w >= 0.0) {
                wake = wake ? std::min(*wake, w) : w;
            }
        });
        return wake;
    }

private:
    widgets::ListView& list(std::vector<widgets::ListItemSpec> items, bool single, const char* name) {
        auto& l = controls.emplace_with<widgets::ListView>(
            [this](widgets::ListView& w, const widgets::ListViewResponse& r) {
                if (r.clicked) {
                    const widgets::ListItemSpec& s = w.item(*r.clicked);
                    ctx.report("List: " + s.headline + (s.action == widgets::ListItemAction::click ? "" : (s.selected ? " on" : " off")));
                }
            },
            widgets::ListViewOptions{std::move(items), single, name}, ctx.fonts);
        focus_.add(l);
        return l;
    }

    layout::NodePtr settings() {
        std::vector<widgets::ListItemSpec> items;
        auto wifi = spec("Wi-Fi");
        wifi.leading = widgets::ListSlot::of_icon(icon(res::icons::wifi));
        wifi.trailing = widgets::ListSlot::switch_control();
        wifi.action = widgets::ListItemAction::toggle;
        wifi.selected = true;
        items.push_back(wifi);
        auto ada = spec("Ada Lovelace", "Notes on the analytical engine");
        ada.leading = widgets::ListSlot::of_avatar(std::string("A"));
        ada.trailing = widgets::ListSlot::of_text("9:41");
        items.push_back(ada);
        auto photo = spec("Harbour at dawn", "Taken with the wide lens, before the boats went out to sea.", "Today");
        photo.leading = widgets::ListSlot::of_image(avatar_image(1));
        items.push_back(photo);
        for (const auto& [name, on] : std::array<std::pair<const char*, bool>, 2>{{{"Photos", false}, {"Videos", true}}}) {
            auto box = spec(name);
            box.leading = widgets::ListSlot::checkbox();
            box.action = widgets::ListItemAction::toggle;
            box.selected = on;
            items.push_back(box);
        }
        for (const auto& [name, on] : std::array<std::pair<const char*, bool>, 3>{{{"Small", false}, {"Medium", true}, {"Large", false}}}) {
            auto radio = spec(name);
            radio.leading = widgets::ListSlot::radio_button();
            radio.action = widgets::ListItemAction::select;
            radio.selected = on;
            items.push_back(radio);
        }
        auto off = spec("Bluetooth", "Turned off by the administrator");
        off.leading = widgets::ListSlot::of_icon(icon(res::icons::bluetooth));
        off.enabled = false;
        items.push_back(off);
        auto open = spec("Open settings");
        open.trailing = widgets::ListSlot::of_icon(icon(res::icons::open_in_new));
        items.push_back(open);
        auto version = spec("Version", "0.1.0");
        version.action = widgets::ListItemAction::none;
        items.push_back(version);
        auto& l = list(std::move(items), true, "Settings");
        return heading("Settings (ListView)", sized(widgets::measured_node(l), page_layout.settings_width, page_layout.list_height));
    }

    layout::NodePtr standalone() {
        auto& set = *contents.emplace_back(std::make_unique<widgets::WidgetSet>());
        std::vector<layout::NodePtr> nodes;
        auto add = [&](widgets::ListItemSpec s, bool divider) {
            auto& item = set.emplace_with<widgets::ListItem>(
                [this](widgets::ListItem& w, const widgets::ListItemResponse& r) {
                    if (r.clicked) {
                        ctx.report("List item: " + w.spec().headline);
                    }
                },
                std::move(s), ctx.fonts);
            focus_.add(item);
            nodes.push_back(widgets::measured_node(item));
            if (divider) {
                auto& d = set.emplace<widgets::Divider>(widgets::DividerOptions{widgets::DividerOrientation::horizontal, widgets::DividerInset::both});
                nodes.push_back(widgets::measured_node(d));
            }
        };
        auto inbox = spec("Inbox", "3 new messages");
        inbox.leading = widgets::ListSlot::of_icon(icon(res::icons::inbox));
        inbox.trailing = widgets::ListSlot::of_text("3");
        add(inbox, true);
        auto folder = spec("Projects", "Shared with 4 people");
        folder.leading = widgets::ListSlot::of_icon(icon(res::icons::folder));
        add(folder, true);
        auto event = spec("Design review", "Room 4 · bring the prototypes and the notes from last week's session", "Tomorrow 10:00");
        event.leading = widgets::ListSlot::of_icon(icon(res::icons::calendar_today));
        add(event, false);
        auto column = layout::column(std::move(nodes));
        column->fill_cross(true);
        auto& c = controls.emplace<widgets::Card>(widgets::CardOptions{tokens::CardStyle::outlined, false, true, column, &set, "Items"});
        return heading("List items in a card", width_limited(widgets::measured_node(c), page_layout.standalone_width));
    }

    layout::NodePtr messages() {
        std::vector<widgets::ListItemSpec> items;
        items.reserve(page_layout.big_list_rows);
        const char* names[] = {"Ada", "Grace", "Alan", "Edsger", "Barbara", "Donald", "Frances", "Ken"};
        for (std::size_t i = 0; i < page_layout.big_list_rows; ++i) {
            const char* who = names[i % std::size(names)];
            std::string text = i % page_layout.long_row_every == 0
                                   ? "A longer message that wraps onto a second line, so this row becomes a three-line item."
                                   : "Message " + std::to_string(i + 1) + " from " + who;
            auto s = spec(std::string(who) + " · " + std::to_string(i + 1), text);
            s.leading = widgets::ListSlot::of_avatar(std::string(1, who[0]));
            s.trailing = widgets::ListSlot::of_text(std::to_string(9 + i % 12) + ":" + (i % 6 < 5 ? std::to_string(10 + i % 50) : "05"));
            items.push_back(std::move(s));
        }
        auto& l = list(std::move(items), false, "Messages");
        return heading("10,000 rows (ListView)", sized(widgets::measured_node(l), page_layout.big_list_width, page_layout.list_height));
    }
};

class BarsPage final : public BasePage {
public:
    explicit BarsPage(PageContext context) : BasePage(std::move(context)) {
        frame({"Inbox", tokens::TopAppBarKind::small, {}, nav(), actions(2), widgets::TopAppBarScroll::pinned});
        frame({"Photos", tokens::TopAppBarKind::center_aligned, {}, nav(), actions(1), widgets::TopAppBarScroll::enter_always});
        frame({"Mail", tokens::TopAppBarKind::medium, {}, nav(), actions(2), widgets::TopAppBarScroll::exit_until_collapsed});
        frame({"Albums", tokens::TopAppBarKind::large_flexible, "1,024 photos", nav(), actions(1), widgets::TopAppBarScroll::exit_until_collapsed});
        std::vector<layout::NodePtr> frame_nodes;
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            frame_nodes.push_back(layout::fixed({page_layout.frame_width, page_layout.frame_height}, [this, i](const render::Rect& r) {
                Frame& f = *frames_[i];
                f.rect = r;
                f.list->set_viewport(r);
                f.bar->arrange({r.x, r.y, r.width, f.bar->height()});
            }));
        }
        auto row = items(std::move(frame_nodes));
        row->spacing(page_layout.column_gap + page_layout.heading_gap).line_spacing(page_layout.column_gap + page_layout.heading_gap).cross_align(layout::CrossAlign::start);
        root = columns({{captioned("Top app bars", row), captioned("Badges", badges())}}, page_layout.column_space, page_layout.row_gap);
        refresh_badges();
    }

    bool handle(const input::PointerEvent& e) override {
        bool consumed = false;
        for (auto& t : tooltips) {
            consumed = t->handle(e).consumed || consumed;
        }
        if (consumed) {
            return true;
        }
        for (auto& f : frames_) {
            const widgets::TopAppBarResponse r = f->bar->handle(e);
            if (r.navigation) {
                ctx.report("App bar: back from " + f->bar->accessible().name);
            }
            if (r.action) {
                ctx.report("App bar: " + f->bar->action(*r.action).accessible_name());
            }
            f->list->handle(r.consumed ? input::PointerEvent{input::PointerAction::leave, 0.0, 0.0} : e);
        }
        return controls.handle(e);
    }

    bool handle(const input::KeyEvent& e) override {
        if (BasePage::handle(e)) {
            return true;
        }
        for (auto& f : frames_) {
            f->bar->handle(e);
            f->list->handle(e);
        }
        return false;
    }

    bool handle(const input::WheelEvent& e) override {
        bool used = false;
        for (auto& f : frames_) {
            used = f->list->handle(e) || used;
        }
        return used;
    }

    void step(double dt) override {
        BasePage::step(dt);
        for (auto& f : frames_) {
            f->list->step(dt);
            f->bar->step(dt);
            f->bar->follow(f->list->scroll_area());
            f->bar->arrange({f->rect.x, f->rect.y, f->rect.width, f->bar->height()});
        }
    }

    bool animating() const override {
        return BasePage::animating() ||
               std::any_of(frames_.begin(), frames_.end(), [](const auto& f) { return f->bar->animating() || f->list->animating(); });
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint(canvas, scheme);
        for (const auto& f : frames_) {
            const render::CornerRadii radii = render::uniform_radii(page_layout.frame_corner);
            canvas.push_clip(render::rounded_rect_path(f->rect, radii));
            canvas.fill_rect(f->rect, scheme[color::Role::surface]);
            f->list->paint(canvas, scheme);
            f->bar->paint(canvas, scheme);
            canvas.pop_clip();
            canvas.fill_path(render::ring_path(f->rect, page_layout.frame_corner, page_layout.frame_outline), scheme[page_layout.frame_outline_color],
                             render::FillRule::even_odd);
        }
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        for (const auto& f : frames_) {
            nodes.push_back(f->bar->accessible());
            nodes.push_back(f->list->accessible());
        }
        return nodes;
    }

    void set_enabled(bool enabled) override {
        BasePage::set_enabled(enabled);
        for (auto& f : frames_) {
            f->bar->set_enabled(enabled);
            f->list->set_enabled(enabled);
        }
    }

    void cancel_interaction() override {
        BasePage::cancel_interaction();
        for (auto& f : frames_) {
            f->bar->cancel_interaction();
            f->list->cancel_interaction();
        }
    }

    std::optional<double> next_wake_seconds() const override {
        std::optional<double> wake = BasePage::next_wake_seconds();
        for (const auto& f : frames_) {
            const double w = f->list->next_wake_seconds();
            if (w >= 0.0) {
                wake = wake ? std::min(*wake, w) : w;
            }
        }
        return wake;
    }

private:
    struct Frame {
        std::unique_ptr<widgets::TopAppBar> bar;
        std::unique_ptr<widgets::ListView> list;
        render::Rect rect{0.0, 0.0, 0.0, 0.0};
    };

    std::optional<widgets::IconButtonOptions> nav() const {
        return widgets::IconButtonOptions{icon(res::icons::arrow_back), "Back"};
    }

    std::vector<widgets::IconButtonOptions> actions(std::size_t n) const {
        std::vector<widgets::IconButtonOptions> out{{icon(res::icons::search), "Search"}, {icon(res::icons::more_vert), "More options"}};
        out.resize(n, out.back());
        return out;
    }

    void frame(widgets::TopAppBarOptions options) {
        auto f = std::make_unique<Frame>();
        const std::string name = options.title;
        f->bar = std::make_unique<widgets::TopAppBar>(std::move(options), ctx.fonts);
        std::vector<widgets::ListItemSpec> rows;
        for (std::size_t i = 0; i < page_layout.frame_rows; ++i) {
            auto s = spec(name + " " + std::to_string(i + 1), i % 2 == 0 ? "Supporting text" : "");
            s.leading = widgets::ListSlot::of_avatar(std::string(1, name[0]));
            rows.push_back(std::move(s));
        }
        widgets::ListViewOptions lo{std::move(rows), false, name + " list"};
        lo.top_inset = f->bar->expanded_height();
        f->list = std::make_unique<widgets::ListView>(std::move(lo), ctx.fonts);
        if (auto* n = f->bar->navigation_button()) {
            focus_.add(*n);
            name_tip(*n);
        }
        for (std::size_t i = 0; i < f->bar->action_count(); ++i) {
            focus_.add(f->bar->action(i));
            name_tip(f->bar->action(i));
        }
        focus_.add(*f->list);
        frames_.push_back(std::move(f));
    }

    std::vector<widgets::NavigationItem> nav_items() const {
        return {{icon(res::icons::mail), "Mail"}, {icon(res::icons::chat), "Chat"}, {icon(res::icons::groups), "Rooms"}, {icon(res::icons::videocam), "Meet"}};
    }

    layout::NodePtr badges() {
        std::vector<layout::NodePtr> buttons;
        auto icon_button = [&](Symbol s, const char* name) -> widgets::IconButton& {
            auto& b = controls.emplace<widgets::IconButton>(widgets::IconButtonOptions{icon(s), name});
            focus_.add(b);
            name_tip(b);
            buttons.push_back(widgets::node(b));
            return b;
        };
        dot_button_ = &icon_button(res::icons::notifications, "Notifications");
        mail_button_ = &icon_button(res::icons::mail, "Mail");
        many_button_ = &icon_button(res::icons::chat, "Chat");
        auto text_button = [&](const char* label, long long add) {
            auto& b = controls.emplace_with<widgets::Button>(
                [this, add](bool c) {
                    if (c) {
                        count_ = add == 0 ? 0 : count_ + add;
                        refresh_badges();
                        ctx.report("Mail count: " + std::to_string(count_));
                    }
                },
                widgets::ButtonOptions{label, tokens::ButtonStyle::outlined}, ctx.fonts);
            focus_.add(b);
            buttons.push_back(widgets::node(b));
        };
        auto icon_row = items(std::move(buttons));
        icon_row->spacing(page_layout.badge_button_gap);
        buttons.clear();
        text_button("Add 1", page_layout.add_small);
        text_button("Add 100", page_layout.add_large);
        text_button("Clear", 0);
        auto button_row = items({icon_row, items(std::move(buttons))});
        button_row->spacing(page_layout.badge_button_gap);
        bar_ = &controls.emplace<widgets::NavigationBar>(widgets::NavigationBarOptions{nav_items(), 0, "Sections"}, ctx.fonts);
        focus_.add(*bar_);
        rail_ = &controls.emplace<widgets::NavigationRail>(widgets::NavigationRailOptions{nav_items(), 0, false, nullptr, "Sections rail"}, ctx.fonts);
        focus_.add(*rail_);
        auto left = layout::column({button_row, sized(widgets::full_width_node(*bar_), page_layout.bar_width, bar_->height())});
        left->spacing(page_layout.row_gap);
        auto r = items({left, sized(widgets::measured_node(*rail_), tokens::navigation_rail_tokens.collapsed_width, page_layout.rail_height)});
        r->spacing(page_layout.column_space).cross_align(layout::CrossAlign::start);
        return r;
    }

    void refresh_badges() {
        const std::string count = widgets::badge_count(count_);
        const std::string described = count_ == 0 ? std::string() : count + " new messages";
        std::optional<widgets::BadgeSpec> mail;
        if (count_ > 0) {
            mail = widgets::BadgeSpec{count, described};
        }
        dot_button_->set_badge(widgets::Badge({{}, "New notifications"}, ctx.fonts));
        mail_button_->set_badge(mail ? std::optional<widgets::Badge>(widgets::Badge(*mail, ctx.fonts)) : std::nullopt);
        many_button_->set_badge(widgets::Badge({widgets::badge_count(1500), "More than 999 new chats"}, ctx.fonts));
        for (auto set : {std::function<void(std::size_t, std::optional<widgets::BadgeSpec>)>(
                             [this](std::size_t i, std::optional<widgets::BadgeSpec> b) { bar_->set_item_badge(i, std::move(b)); }),
                         std::function<void(std::size_t, std::optional<widgets::BadgeSpec>)>(
                             [this](std::size_t i, std::optional<widgets::BadgeSpec> b) { rail_->set_item_badge(i, std::move(b)); })}) {
            set(0, mail);
            set(1, widgets::BadgeSpec{{}, "New chats"});
            set(3, widgets::BadgeSpec{widgets::badge_count(1500), "More than 999 missed calls"});
        }
    }

    std::vector<std::unique_ptr<Frame>> frames_;
    widgets::IconButton* dot_button_ = nullptr;
    widgets::IconButton* mail_button_ = nullptr;
    widgets::IconButton* many_button_ = nullptr;
    widgets::NavigationBar* bar_ = nullptr;
    widgets::NavigationRail* rail_ = nullptr;
    long long count_ = 3;
};

}

std::unique_ptr<Page> make_chips_page(PageContext context) {
    return std::make_unique<ChipsPage>(std::move(context));
}

std::unique_ptr<Page> make_lists_page(PageContext context) {
    return std::make_unique<ListsPage>(std::move(context));
}

std::unique_ptr<Page> make_bars_page(PageContext context) {
    return std::make_unique<BarsPage>(std::move(context));
}

}
