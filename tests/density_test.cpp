#include <doctest/doctest.h>

#include "lamina/tokens/density.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/chip.hpp"
#include "lamina/widgets/chip_group.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/list_item.hpp"
#include "lamina/widgets/list_view.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_drawer.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/segmented_button.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/top_app_bar.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using tokens::Density;

namespace {

icons::Icon icon() {
    return {icon_font(), res::icons::favorite};
}

}

TEST_CASE("each density step removes 4 dp and stops at the component's minimum") {
    const tokens::DenseLength l{40.0, 28.0};
    CHECK(l.at(Density{0}) == 40.0);
    CHECK(l.at(Density{-1}) == 36.0);
    CHECK(l.at(Density{-2}) == 32.0);
    CHECK(l.at(Density{-3}) == 28.0);
    CHECK(l.at(Density{-4}) == 28.0);
    CHECK(l.at(Density{2}) == 40.0);
    CHECK(l.minimum_scale() == -3);
    CHECK(tokens::density_tokens.text_field.minimum_scale() == -5);
    CHECK(tokens::density_tokens.chip.minimum_scale() == -2);
    CHECK(tokens::density_tokens.tab.at(Density{-5}) == 30.0);
}

TEST_CASE("a dense control drops the 48 dp touch target") {
    CHECK(tokens::minimum_target(Density{0}) == tokens::minimum_interactive_size);
    CHECK(tokens::minimum_target(Density{-1}) == 0.0);
    const render::Rect r{10.0, 10.0, 28.0, 28.0};
    const render::Rect normal = widgets::touch_target(r, tokens::minimum_target(Density{0}));
    CHECK(normal.width == 48.0);
    CHECK(normal.x == 0.0);
    const render::Rect dense = widgets::touch_target(r, tokens::minimum_target(Density{-2}));
    CHECK(dense.width == 28.0);
    CHECK(dense.x == 10.0);
}

TEST_CASE("density shortens buttons of the small sizes and leaves the large ones") {
    for (auto size : {tokens::ButtonSize::extra_small, tokens::ButtonSize::small, tokens::ButtonSize::medium, tokens::ButtonSize::large,
                      tokens::ButtonSize::extra_large}) {
        widgets::Button normal({.label = "Save", .size = size}, fonts());
        CHECK(normal.height() == tokens::button_size(size).height);
        for (int s = -1; s >= -4; --s) {
            widgets::Button dense({.label = "Save", .size = size, .density = Density{s}}, fonts());
            CHECK(dense.height() == tokens::button_density(size).at(Density{s}));
            CHECK(dense.width() == normal.width());
        }
    }
    widgets::Button b({.label = "Save"}, fonts());
    b.set_density(Density{-3});
    CHECK(b.height() == 28.0);
    CHECK(b.corner_radii().top_left == 14.0);
    b.set_density(Density{0});
    CHECK(b.height() == 40.0);
}

TEST_CASE("a dense icon button shrinks in both directions and takes clicks only on itself") {
    widgets::IconButton b({icon(), "Like"});
    CHECK(b.width() == 40.0);
    b.set_density(Density{-3});
    CHECK(b.height() == 28.0);
    CHECK(b.width() == 28.0);
    b.set_origin({100.0, 100.0});
    CHECK(click(b, 114.0, 114.0));
    CHECK_FALSE(click(b, 96.0, 114.0));
    b.set_density(Density{0});
    CHECK(click(b, 96.0, 114.0));
    widgets::IconButton narrow({icon(), "Like", tokens::IconButtonStyle::standard, tokens::IconButtonSize::small, tokens::ButtonShape::round,
                                tokens::IconButtonWidth::narrow});
    narrow.set_density(Density{-3});
    CHECK(narrow.width() == tokens::icon_button_size(tokens::IconButtonSize::small).icon_size);
}

TEST_CASE("a widget set passes its density to every member that takes one") {
    widgets::WidgetSet set;
    auto& b = set.emplace<widgets::Button>(widgets::ButtonOptions{.label = "Save"}, fonts());
    auto& i = set.emplace<widgets::IconButton>(widgets::IconButtonOptions{icon(), "Like"});
    widgets::WidgetSet inner;
    auto& c = inner.emplace<widgets::Button>(widgets::ButtonOptions{.label = "Open"}, fonts());
    set.add(inner);
    set.set_density(Density{-2});
    CHECK(set.density() == Density{-2});
    CHECK(b.density() == Density{-2});
    CHECK(i.density() == Density{-2});
    CHECK(c.height() == 32.0);
    static_assert(widgets::Densifiable<widgets::Button>);
    static_assert(widgets::Densifiable<widgets::IconButton>);
    static_assert(widgets::Densifiable<widgets::WidgetSet>);
}

TEST_CASE("a dense text field is shorter and keeps its text and label centred") {
    widgets::TextField normal({.label = "Name"}, fonts());
    CHECK(normal.height() == 56.0);
    widgets::TextField compact({.label = "Name", .min_width = 120.0, .density = Density{-3}}, fonts());
    CHECK(compact.height() == 44.0);
    compact.set_width(120.0);
    CHECK(compact.width() == 120.0);
    compact.set_density(Density{-6});
    CHECK(compact.height() == 36.0);
    compact.set_origin({0.0, 0.0});
    compact.set_focused(true);
    compact.handle(input::TextEvent{input::TextEventKind::commit, "Ada", 0});
    const render::Rect caret = compact.caret_area();
    CHECK(caret.y + caret.height / 2.0 == doctest::Approx(18.0).epsilon(0.05));
    widgets::TextField notes({.label = "Notes", .multiline = true, .density = Density{-2}}, fonts());
    CHECK(notes.height() == 48.0);
}

TEST_CASE("dense list items shorten each line count down to its minimum") {
    widgets::ListItem one({.headline = "Wi-Fi"}, fonts());
    CHECK(one.height_for(360.0) == 56.0);
    one.set_density(Density{-4});
    CHECK(one.height_for(360.0) == 40.0);
    widgets::ListItem two({.headline = "Wi-Fi", .supporting = "Connected", .density = Density{-3}}, fonts());
    CHECK(two.height_for(360.0) == 60.0);
    widgets::ListItem three({.headline = "Wi-Fi", .supporting = "Connected", .overline = "Network", .density = Density{-3}}, fonts());
    CHECK(three.height_for(360.0) == 76.0);
    CHECK(widgets::list_item_estimate({.headline = "A", .density = Density{-2}}) == 48.0);

    widgets::ListView view({.items = {{.headline = "A"}, {.headline = "B"}, {.headline = "C"}}}, fonts());
    view.set_viewport({0.0, 0.0, 360.0, 400.0});
    CHECK(view.row_rect(2).y == 112.0);
    view.set_density(Density{-2});
    CHECK(view.row_rect(1).height == 48.0);
    CHECK(view.row_rect(2).y == 96.0);
}

TEST_CASE("dense chips, tabs and segmented buttons") {
    widgets::Chip chip({.label = "Filter", .density = Density{-1}}, fonts());
    CHECK(chip.height() == 28.0);
    chip.set_density(Density{-5});
    CHECK(chip.height() == 24.0);
    widgets::ChipGroup group({.chips = {{.label = "A"}, {.label = "B"}}}, fonts());
    group.set_density(Density{-2});
    CHECK(group.chip(1).height() == 24.0);

    widgets::TabRow tabs({.tabs = {{"One"}, {"Two"}}}, fonts());
    CHECK(tabs.height() == 48.0);
    tabs.set_density(Density{-3});
    CHECK(tabs.height() == 36.0);
    widgets::TabRow stacked({.tabs = {{"One", icon()}, {"Two", icon()}}, .density = Density{-2}}, fonts());
    CHECK(stacked.height() == 64.0);

    widgets::SegmentedButtonRow seg({.segments = {{"Day"}, {"Week"}}}, fonts(), icon_font());
    CHECK(seg.height() == 40.0);
    seg.set_density(Density{-3});
    CHECK(seg.height() == 28.0);
    seg.set_origin({0.0, 0.0});
    CHECK(seg.handle(input::PointerEvent{input::PointerAction::down, 10.0, 31.0}).consumed == false);
}

TEST_CASE("dense app bars and navigation") {
    widgets::TopAppBar bar({.title = "Inbox", .density = Density{-2}}, fonts());
    CHECK(bar.height() == 56.0);
    widgets::TopAppBar medium({.title = "Inbox", .kind = tokens::TopAppBarKind::medium}, fonts());
    const double tall = medium.height();
    medium.set_density(Density{-4});
    CHECK(medium.collapsed_height() == 48.0);
    CHECK(medium.height() == tall - 16.0);

    widgets::NavigationBar nav({.items = {{icon(), "Home"}, {icon(), "Files"}}}, fonts());
    CHECK(nav.height() == 64.0);
    nav.set_density(Density{-3});
    CHECK(nav.height() == 52.0);
    nav.set_width(200.0);
    CHECK(nav.indicator_rect(0).y == 0.0);

    widgets::NavigationRail rail({.items = {{icon(), "Home"}, {icon(), "Files"}}}, fonts());
    rail.measure({0.0, 400.0, 0.0, 600.0});
    rail.arrange({0.0, 0.0, rail.width(), 600.0});
    const double normal_gap = rail.item_rect(1).y - rail.item_rect(0).y;
    rail.set_density(Density{-3});
    rail.measure({0.0, 400.0, 0.0, 600.0});
    rail.arrange({0.0, 0.0, rail.width(), 600.0});
    CHECK(rail.item_rect(1).y - rail.item_rect(0).y == normal_gap - 12.0);

    widgets::DrawerSheet drawer({.entries = {{.label = "Inbox"}, {.label = "Sent"}}}, fonts());
    const double drawer_gap = drawer.item_rect(1).y - drawer.item_rect(0).y;
    CHECK(drawer_gap == 56.0);
    drawer.set_density(Density{-4});
    CHECK(drawer.item_rect(1).y - drawer.item_rect(0).y == 40.0);

    widgets::Menu menu({.items = {{"Cut"}, {"Copy"}, {"Paste"}}}, fonts());
    const double menu_h = menu.height();
    menu.set_density(Density{-2});
    CHECK(menu.item_height() == 40.0);
    CHECK(menu.height() == menu_h - 24.0);
}

TEST_CASE("every dense-capable widget takes a density from a widget set") {
    static_assert(widgets::Densifiable<widgets::TextField>);
    static_assert(widgets::Densifiable<widgets::ListItem>);
    static_assert(widgets::Densifiable<widgets::ListView>);
    static_assert(widgets::Densifiable<widgets::Chip>);
    static_assert(widgets::Densifiable<widgets::ChipGroup>);
    static_assert(widgets::Densifiable<widgets::TabRow>);
    static_assert(widgets::Densifiable<widgets::SegmentedButtonRow>);
    static_assert(widgets::Densifiable<widgets::TopAppBar>);
    static_assert(widgets::Densifiable<widgets::NavigationBar>);
    static_assert(widgets::Densifiable<widgets::NavigationRail>);
    static_assert(widgets::Densifiable<widgets::DrawerSheet>);
    static_assert(widgets::Densifiable<widgets::Menu>);
}
