#include <doctest/doctest.h>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "lamina/widgets/grouped_menu.hpp"
#include "lamina/widgets/list_view.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_drawer.hpp"
#include "restyle_support.hpp"

using namespace lamina;
using namespace lamina::test;
using widgets::ListItemAction;
using widgets::ListItemSpec;

namespace {

constexpr int list_width = 360;
constexpr int list_height = 240;
constexpr int menu_window_width = 400;
constexpr int menu_window_height = 300;
constexpr render::Rect menu_anchor{20.0, 40.0, 40.0, 40.0};
constexpr int drawer_width = 800;
constexpr int drawer_height = 400;
constexpr render::Rect content_card{24.0, 24.0, 300.0, 120.0};
constexpr int stray_pixel_ratio = 100;

Style with_dividers() {
    Style s = m3_expressive_style();
    s.list.row_dividers = true;
    return s;
}

Style with_foreground_selection() {
    Style s = m3_expressive_style();
    s.selection.mark = tokens::SelectionMark::foreground;
    return s;
}

Style with_plain_foreground_selection() {
    Style s = with_foreground_selection();
    s.selection.emphasized_label = false;
    return s;
}

Style with_outlined_containers() {
    Style s = m3_expressive_style();
    s.containers.fill = tokens::ContainerFill::outlined;
    return s;
}

Style with_receding_layers() {
    Style s = m3_expressive_style();
    s.layering.mode = tokens::Layering::recede;
    return s;
}

std::vector<ListItemSpec> rows() {
    return {{"Inbox", "", "", {}, {}, ListItemAction::select, true}, {"Starred", "", "", {}, {}, ListItemAction::select}, {"Sent"}};
}

std::unique_ptr<thorvg::Canvas> paint_list(const Style& style, std::optional<bool> dividers, const char* shot) {
    const StyleScope scope(style);
    widgets::ListViewOptions o{rows(), true};
    o.dividers = dividers;
    widgets::ListView list(o, fonts());
    list.set_viewport({0.0, 0.0, static_cast<double>(list_width), static_cast<double>(list_height)});
    settle_widget(list);
    auto canvas = std::make_unique<thorvg::Canvas>(list_width, list_height);
    canvas->clear(restyle_background);
    list.paint(*canvas, scheme());
    if (shot) {
        save_shot(*canvas, shot);
    }
    return canvas;
}

int divider_pixels(const Style& style, std::optional<bool> dividers, const char* shot) {
    return count_color(*paint_list(style, dividers, shot), scheme()[style.divider.color]);
}

std::unique_ptr<widgets::NavigationBar> bar_at(std::size_t selected) {
    std::vector<widgets::NavigationItem> items;
    for (auto [symbol, label] : {std::pair{res::icons::home, "Home"}, std::pair{res::icons::search, "Search"},
                                 std::pair{res::icons::settings, "Settings"}}) {
        items.push_back({icons::Icon{icon_font(), symbol}, label});
    }
    auto b = std::make_unique<widgets::NavigationBar>(widgets::NavigationBarOptions{items, selected}, fonts());
    b->set_width(list_width);
    return b;
}

RestyleCanvas bar_canvas(const widgets::NavigationBar& b) {
    return {list_width, static_cast<int>(b.height())};
}

int differing_pixels(const RestyleShot& a, const RestyleShot& b, const render::Rect& r) {
    int n = 0;
    for (int y = static_cast<int>(r.y); y < static_cast<int>(r.y + r.height) && y < a.canvas.height; ++y) {
        for (int x = static_cast<int>(r.x); x < static_cast<int>(r.x + r.width) && x < a.canvas.width; ++x) {
            const std::size_t i = static_cast<std::size_t>(y * a.canvas.width + x);
            n += a.pixels[i] != b.pixels[i] ? 1 : 0;
        }
    }
    return n;
}

std::unique_ptr<thorvg::Canvas> paint_menu(const Style& style, const char* shot) {
    const StyleScope scope(style);
    widgets::Menu m({{{"Edit", icons::Icon{icon_font(), res::icons::edit}}, {"Share"}, {"Delete"}}}, fonts());
    m.open(menu_anchor, {static_cast<double>(menu_window_width), static_cast<double>(menu_window_height)});
    settle_widget(m);
    auto canvas = std::make_unique<thorvg::Canvas>(menu_window_width, menu_window_height);
    canvas->clear(restyle_background);
    m.paint(*canvas, scheme());
    save_shot(*canvas, shot);
    return canvas;
}

template <typename Make>
std::pair<RestyleShot, RestyleShot> switched_and_built(const Style& style, Make make, RestyleCanvas size) {
    std::pair<RestyleShot, RestyleShot> r;
    auto w = make();
    settle_widget(*w);
    {
        const StyleScope scope(style);
        settle_widget(*w);
        r.first = shoot(*w, size);
    }
    const StyleScope scope(style);
    auto b = make();
    settle_widget(*b);
    r.second = shoot(*b, size);
    return r;
}

void check_switched_as_built(const std::pair<RestyleShot, RestyleShot>& r) {
    CHECK(r.first.bounds.x == r.second.bounds.x);
    CHECK(r.first.bounds.y == r.second.bounds.y);
    CHECK(r.first.bounds.width == r.second.bounds.width);
    CHECK(r.first.bounds.height == r.second.bounds.height);
    CHECK((r.first.pixels == r.second.pixels));
}

constexpr RestyleCanvas menu_canvas{menu_window_width, menu_window_height};

std::unique_ptr<widgets::GroupedMenu> grouped_menu() {
    widgets::GroupedMenuOptions o{{{{{"Bold"}, {"Italic", std::nullopt, std::nullopt, std::nullopt, true}}, "Style"}, {{{"Copy"}, {"Paste"}}}}};
    auto m = std::make_unique<widgets::GroupedMenu>(std::move(o), fonts());
    m->open(menu_anchor, {static_cast<double>(menu_window_width), static_cast<double>(menu_window_height)});
    return m;
}

std::vector<widgets::DrawerEntry> drawer_entries() {
    const icons::Icon inbox{icon_font(), res::icons::add};
    return {widgets::drawer_headline("Mail"), {widgets::DrawerEntryKind::item, "Inbox", inbox}, {widgets::DrawerEntryKind::item, "Outbox", inbox}};
}

double paint_drawer(const Style& style, const char* shot) {
    const StyleScope scope(style);
    widgets::ModalNavigationDrawer d({{drawer_entries(), 1}}, fonts());
    d.set_area({0.0, 0.0, static_cast<double>(drawer_width), static_cast<double>(drawer_height)});
    d.open();
    settle_widget(d);
    thorvg::Canvas canvas(drawer_width, drawer_height);
    canvas.clear(scheme()[color::Role::surface]);
    const double shift = d.content_shift();
    canvas.fill_rect({content_card.x + shift, content_card.y, content_card.width, content_card.height}, scheme()[color::Role::primary_container]);
    d.paint(canvas, scheme());
    save_shot(canvas, shot);
    return shift;
}

}

TEST_CASE("list dividers are off in the M3 style, on when the style turns them on, and an option overrides the style") {
    CHECK(divider_pixels(m3_expressive_style(), std::nullopt, "dividers_default") == 0);
    CHECK(divider_pixels(with_dividers(), std::nullopt, "dividers_on") > 0);
    CHECK(divider_pixels(with_dividers(), false, nullptr) == 0);
    CHECK(divider_pixels(m3_expressive_style(), true, nullptr) > 0);
}

TEST_CASE("with foreground selection a selected list row has no container and takes the selection color") {
    auto a = paint_list(m3_expressive_style(), std::nullopt, "selection_list_default");
    auto b = paint_list(with_foreground_selection(), std::nullopt, "selection_list_foreground");
    const Style& m3 = m3_expressive_style();
    CHECK(count_color(*a, scheme()[m3.list.selected_container]) > 0);
    CHECK(count_color(*b, scheme()[m3.list.selected_container]) == 0);
    CHECK(count_color(*b, scheme()[m3.selection.foreground]) > count_color(*a, scheme()[m3.selection.foreground]));
}

TEST_CASE("with foreground selection the navigation bar drops the pill and underlines the selected label") {
    auto paint = [](const Style& style, const char* shot) {
        const StyleScope scope(style);
        auto b = bar_at(0);
        settle_widget(*b);
        thorvg::Canvas canvas(list_width, static_cast<int>(b->height()));
        canvas.clear(restyle_background);
        b->paint(canvas, scheme());
        save_shot(canvas, shot);
        return std::pair{count_color(canvas, scheme()[style.navigation_bar.indicator]), count_color(canvas, scheme()[style.selection.foreground])};
    };
    const auto [m3_pill, m3_mark] = paint(m3_expressive_style(), "selection_bar_default");
    const auto [fg_pill, fg_mark] = paint(with_foreground_selection(), "selection_bar_foreground");
    CHECK(m3_pill > 0);
    CHECK(fg_pill == 0);
    CHECK(fg_mark > m3_mark);
}

TEST_CASE("with outlined containers a menu is filled with the outlined fill and drawn with an outline") {
    auto a = paint_menu(m3_expressive_style(), "containers_menu_default");
    auto b = paint_menu(with_outlined_containers(), "containers_menu_outlined");
    const Style& m3 = m3_expressive_style();
    CHECK(count_color(*a, scheme()[m3.menu.container]) > 0);
    CHECK(count_color(*b, scheme()[m3.menu.container]) * stray_pixel_ratio < count_color(*a, scheme()[m3.menu.container]));
    CHECK(count_color(*b, scheme()[m3.containers.outlined_fill]) > 0);
    CHECK(count_color(*b, scheme()[m3.containers.outline]) > count_color(*a, scheme()[m3.containers.outline]));
}

TEST_CASE("with receding layers a menu takes the receded container and dims what is behind it") {
    auto a = paint_menu(m3_expressive_style(), "layering_menu_default");
    auto b = paint_menu(with_receding_layers(), "layering_menu_recede");
    CHECK(a->pixel(menu_window_width - 1, menu_window_height - 1) == restyle_background);
    CHECK(b->pixel(menu_window_width - 1, menu_window_height - 1) != restyle_background);
    CHECK(count_color(*b, scheme()[m3_expressive_style().menu.container]) * stray_pixel_ratio <
          count_color(*a, scheme()[m3_expressive_style().menu.container]));
}

TEST_CASE("with receding layers an open modal drawer pushes the content by its width") {
    CHECK(paint_drawer(m3_expressive_style(), "layering_drawer_default") == 0.0);
    const double shift = paint_drawer(with_receding_layers(), "layering_drawer_recede");
    CHECK(shift > 0.0);
    CHECK(shift == doctest::Approx(m3_expressive_style().navigation_drawer.max_width * m3_expressive_style().layering.drawer_push));
}

TEST_CASE("with receding layers a grouped menu takes the receded container and dims what is behind it") {
    auto paint = [](const Style& style, const char* shot) {
        const StyleScope scope(style);
        auto m = grouped_menu();
        settle_widget(*m);
        auto canvas = std::make_unique<thorvg::Canvas>(menu_window_width, menu_window_height);
        canvas->clear(restyle_background);
        m->paint(*canvas, scheme());
        save_shot(*canvas, shot);
        return canvas;
    };
    auto a = paint(m3_expressive_style(), "layering_grouped_menu_default");
    auto b = paint(with_receding_layers(), "layering_grouped_menu_recede");
    CHECK(a->pixel(menu_window_width - 1, menu_window_height - 1) == restyle_background);
    CHECK(b->pixel(menu_window_width - 1, menu_window_height - 1) != restyle_background);
    CHECK(count_color(*b, scheme()[m3_expressive_style().grouped_menu.group_container]) * stray_pixel_ratio <
          count_color(*a, scheme()[m3_expressive_style().grouped_menu.group_container]));
}

TEST_CASE("a grouped menu built under the M3 style and switched to receding layers paints as one built under receding layers") {
    check_switched_as_built(switched_and_built(with_receding_layers(), grouped_menu, menu_canvas));
}
TEST_CASE("with foreground selection and an emphasized label the navigation bar sets only the selected label in the emphasized type") {
    auto shot = [](const Style& style) {
        const StyleScope scope(style);
        auto b = bar_at(0);
        settle_widget(*b);
        return std::pair{shoot(*b, bar_canvas(*b)), std::vector<render::Rect>{b->item_rect(0), b->item_rect(1), b->item_rect(2)}};
    };
    const auto [bold, rects] = shot(with_foreground_selection());
    const auto [plain, plain_rects] = shot(with_plain_foreground_selection());
    CHECK(differing_pixels(bold, plain, rects[0]) > 0);
    CHECK(differing_pixels(bold, plain, rects[1]) == 0);
    CHECK(differing_pixels(bold, plain, rects[2]) == 0);
}

TEST_CASE("with an emphasized label the navigation bar moves the emphasis to the item it selects") {
    const StyleScope scope(with_foreground_selection());
    auto moved = bar_at(0);
    settle_widget(*moved);
    moved->select(2);
    settle_widget(*moved);
    auto built = bar_at(2);
    settle_widget(*built);
    check_switched_as_built({shoot(*moved, bar_canvas(*moved)), shoot(*built, bar_canvas(*built))});
}

TEST_CASE("a navigation bar built under the M3 style and switched to foreground selection paints as one built under foreground selection") {
    const RestyleCanvas size{list_width, restyle_canvas_height};
    check_switched_as_built(switched_and_built(with_foreground_selection(), [] { return bar_at(1); }, size));
}
