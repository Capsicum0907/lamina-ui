#include <doctest/doctest.h>

#include <vector>

#include "lamina/widgets/bottom_sheet.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/modal_navigation_rail.hpp"
#include "lamina/widgets/navigation_drawer.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/overlay_stack.hpp"
#include "lamina/widgets/search_bar.hpp"
#include "lamina/widgets/side_sheet.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;

namespace {

constexpr render::Rect area{0.0, 0.0, 412.0, 800.0};

template <typename W>
void run(W& w) {
    for (int i = 0; i < settle_frames; ++i) {
        w.step(frame_seconds);
    }
}

}

TEST_CASE("a modal bottom sheet opened from the keyboard keeps its focus when pushed and gives it back when it closes") {
    widgets::Button opener({"Open"}, fonts());
    widgets::FocusManager page;
    page.add(opener);
    widgets::OverlayStack overlay;
    overlay.set_focus(page);
    opener.set_focused(true);
    widgets::BottomSheet sheet({});
    sheet.set_area(area);
    sheet.show(input::InputSource::keyboard);
    overlay.push(sheet, [](const widgets::BottomSheetResponse&) {});
    run(sheet);
    CHECK_FALSE(opener.focused());
    CHECK(sheet.drag_handle()->focused());
    overlay.handle(input::KeyEvent{Key::escape, false});
    run(sheet);
    overlay.handle(input::KeyEvent{Key::escape, false});
    run(sheet);
    CHECK_FALSE(sheet.is_open());
    overlay.handle(input::KeyEvent{Key::tab, false});
    CHECK(opener.focused());
}

TEST_CASE("a modal side sheet and a modal drawer keep their own focus through an overlay push") {
    widgets::Button opener({"Open"}, fonts());
    widgets::FocusManager page;
    page.add(opener);
    widgets::OverlayStack overlay;
    overlay.set_focus(page);
    opener.set_focused(true);
    widgets::SideSheet side({.headline = "Filters", .close = widgets::SideSheetIcon{{icon_font(), test::res::icons::close}, "Close"}}, fonts());
    side.set_area(area);
    side.open(input::InputSource::keyboard);
    overlay.push(side, [](const widgets::SideSheetResponse&) {});
    run(side);
    CHECK(side.close_button()->focused());
    overlay.handle(input::KeyEvent{Key::escape, false});
    run(side);
    overlay.handle(input::KeyEvent{Key::tab, false});
    CHECK(opener.focused());

    widgets::ModalNavigationDrawer drawer({{{{widgets::DrawerEntryKind::item, "Inbox"}, {widgets::DrawerEntryKind::item, "Outbox"}}, 0}}, fonts());
    drawer.set_area(area);
    drawer.open(input::InputSource::keyboard);
    overlay.push(drawer, [](const widgets::DrawerResponse&) {});
    run(drawer);
    CHECK(drawer.sheet().focused());
    CHECK_FALSE(opener.focused());
}

TEST_CASE("an expanded search bar takes the focus back after the overlay push clears the page's focus") {
    widgets::SearchBar bar({.placeholder = "Search",
                            .search_icon = {icon_font(), test::res::icons::search},
                            .back = {{icon_font(), test::res::icons::home}, "Back"},
                            .results = {{.headline = "Kyoto"}, {.headline = "Kobe"}}},
                           fonts());
    bar.set_area(area);
    bar.arrange({16.0, 8.0, 380.0, 56.0});
    widgets::FocusManager page;
    bar.add_focus_stops(page);
    widgets::OverlayStack overlay;
    overlay.set_focus(page);
    bar.field().set_focused(true);
    const widgets::SearchBarResponse r = bar.handle(input::KeyEvent{Key::down, false});
    REQUIRE(r.expanded);
    overlay.push(bar, [](const widgets::SearchBarResponse&) {});
    CHECK_FALSE(bar.field().focused());
    bar.step(frame_seconds);
    CHECK(bar.field().focused());
    overlay.handle(input::TextEvent{input::TextEventKind::commit, "k", 0});
    CHECK(bar.query() == "k");
    overlay.handle(input::KeyEvent{Key::escape, false});
    run(bar);
    CHECK_FALSE(bar.is_open());
    overlay.handle(input::KeyEvent{Key::tab, false});
    CHECK(bar.field().focused());
}

TEST_CASE("a modal rail opened from the keyboard keeps the focus on the rail after the overlay push") {
    const icons::Icon home{icon_font(), test::res::icons::home};
    widgets::ModalNavigationRail rail({{{{home, "Home"}, {home, "Saved"}}, 0}}, fonts());
    rail.set_area(area);
    rail.measure({0.0, area.width, 0.0, area.height});
    rail.arrange({0.0, 0.0, 96.0, area.height});
    widgets::FocusManager page;
    page.add(rail);
    widgets::OverlayStack overlay;
    overlay.set_focus(page);
    rail.set_focused(true);
    rail.expand(input::InputSource::keyboard);
    overlay.push(rail, [](const widgets::ModalRailResponse&) {});
    CHECK_FALSE(rail.focused());
    rail.step(frame_seconds);
    CHECK(rail.focused());
    overlay.handle(input::KeyEvent{Key::escape, false});
    run(rail);
    CHECK_FALSE(rail.is_open());
    overlay.handle(input::KeyEvent{Key::tab, false});
    CHECK(rail.focused());
}
