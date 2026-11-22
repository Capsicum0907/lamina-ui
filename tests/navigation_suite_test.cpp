#include <doctest/doctest.h>

#include <memory>

#include "lamina/widgets/navigation_suite.hpp"
#include "lamina/widgets/protocol.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::NavigationSuiteType;

static_assert(widgets::Measurable<widgets::NavigationSuiteScaffold> && widgets::Paintable<widgets::NavigationSuiteScaffold> &&
              widgets::Steppable<widgets::NavigationSuiteScaffold> && widgets::HandlesPointer<widgets::NavigationSuiteScaffold> &&
              widgets::HandlesKeys<widgets::NavigationSuiteScaffold> && widgets::FocusGated<widgets::NavigationSuiteScaffold> &&
              widgets::Densifiable<widgets::NavigationSuiteScaffold>);

namespace {

widgets::WindowAdaptiveInfo info(double w, double h = 800.0) {
    return {layout::compute_window_size_class(w, h)};
}

std::vector<widgets::NavigationItem> items() {
    return {{{icon_font(), res::icons::favorite}, "Home"}, {{icon_font(), res::icons::favorite}, "Files"}, {{icon_font(), res::icons::favorite}, "Shared"}};
}

struct Content {
    render::Rect rect{0.0, 0.0, 0.0, 0.0};
};

void place(widgets::NavigationSuiteScaffold& s, double w, double h = 700.0) {
    s.measure(layout::Constraints::tight({w, h}));
    s.arrange({0.0, 0.0, w, h});
}

}

TEST_CASE("Compose's chooser picks a bar on compact windows and on short ones and a collapsed rail otherwise") {
    CHECK(widgets::navigation_suite_type(info(400.0)) == NavigationSuiteType::short_navigation_bar_compact);
    CHECK(widgets::navigation_suite_type(info(700.0)) == NavigationSuiteType::wide_navigation_rail_collapsed);
    CHECK(widgets::navigation_suite_type(info(700.0, 400.0)) == NavigationSuiteType::short_navigation_bar_medium);
    CHECK(widgets::navigation_suite_type(info(1700.0)) == NavigationSuiteType::wide_navigation_rail_collapsed);
}

TEST_CASE("the widening chooser goes from bar to rail to expanded rail to drawer") {
    CHECK(widgets::navigation_suite_type(info(400.0), widgets::widening_navigation_suite) == NavigationSuiteType::short_navigation_bar_compact);
    CHECK(widgets::navigation_suite_type(info(700.0), widgets::widening_navigation_suite) == NavigationSuiteType::wide_navigation_rail_collapsed);
    CHECK(widgets::navigation_suite_type(info(1000.0), widgets::widening_navigation_suite) == NavigationSuiteType::wide_navigation_rail_expanded);
    CHECK(widgets::navigation_suite_type(info(1300.0), widgets::widening_navigation_suite) == NavigationSuiteType::navigation_drawer);
    CHECK(widgets::navigation_suite_type(info(1300.0, 400.0), widgets::widening_navigation_suite) ==
          NavigationSuiteType::short_navigation_bar_medium);
}

TEST_CASE("the content takes the room the bar or the rail leaves") {
    Content c;
    auto node = std::make_shared<layout::Leaf>([](const layout::Constraints& k) { return layout::Size{k.min_width, k.min_height}; },
                                               [&c](const render::Rect& r) { c.rect = r; });
    widgets::NavigationSuiteScaffold s({.items = items(), .content = node, .type = NavigationSuiteType::short_navigation_bar_compact}, fonts());
    place(s, 400.0);
    CHECK(c.rect.width == 400.0);
    CHECK(c.rect.height == doctest::Approx(700.0 - s.bar().height()));
    CHECK(s.bar().bounds().y == doctest::Approx(700.0 - s.bar().height()));
    s.update(info(700.0));
    CHECK(s.type() == NavigationSuiteType::wide_navigation_rail_collapsed);
    place(s, 700.0);
    for (int i = 0; i < 4; ++i) {
        s.step(frame_seconds);
    }
    CHECK(s.animating());
    CHECK(s.bar_progress() < 1.0);
    CHECK(s.side_progress() > 0.0);
    settle(s);
    place(s, 700.0);
    CHECK(c.rect.x == doctest::Approx(96.0));
    CHECK(c.rect.height == doctest::Approx(700.0));
    s.update(info(1300.0));
    settle(s);
    place(s, 1300.0);
    CHECK(s.type() == NavigationSuiteType::navigation_drawer);
    CHECK(c.rect.x == doctest::Approx(360.0));
    CHECK(s.accessible_nodes().size() == 1);
}

TEST_CASE("choosing a destination on one component selects it on all of them") {
    widgets::NavigationSuiteScaffold s({.items = items(), .type = NavigationSuiteType::wide_navigation_rail_collapsed}, fonts());
    place(s, 700.0);
    settle(s);
    const render::Rect r = s.rail().item_rect(2);
    s.handle(input::PointerEvent{PointerAction::down, r.x + r.width / 2.0, r.y + r.height / 2.0});
    const widgets::NavigationSuiteResponse resp = s.handle(input::PointerEvent{PointerAction::up, r.x + r.width / 2.0, r.y + r.height / 2.0});
    REQUIRE(resp.selected.has_value());
    CHECK(*resp.selected == 2);
    CHECK(s.selected() == 2);
    CHECK(s.bar().selected() == 2);
    CHECK(s.drawer().selected() == 2);
}

TEST_CASE("the focus follows the navigation component when the type changes") {
    widgets::NavigationSuiteScaffold s({.items = items(), .type = NavigationSuiteType::short_navigation_bar_compact}, fonts());
    place(s, 400.0);
    s.set_focused(true);
    CHECK(s.bar().focused());
    s.set_type(NavigationSuiteType::wide_navigation_rail_collapsed);
    CHECK_FALSE(s.bar().focused());
    CHECK(s.rail().focused());
    CHECK(s.focused());
    s.set_density(tokens::Density{-3});
    CHECK(s.bar().height() == 52.0);
}

TEST_CASE("labels turn off on the bar and the rail together, and snap_to_targets lands a type change and the labels at once") {
    Content c;
    auto node = std::make_shared<layout::Leaf>([](const layout::Constraints& k) { return layout::Size{k.min_width, k.min_height}; },
                                               [&c](const render::Rect& r) { c.rect = r; });
    widgets::NavigationSuiteScaffold s({.items = items(), .content = node, .type = NavigationSuiteType::navigation_drawer, .labels = false}, fonts());
    CHECK_FALSE(s.labels());
    CHECK_FALSE(s.rail().labels());
    CHECK_FALSE(s.bar().labels());
    place(s, 1300.0);
    s.set_type(NavigationSuiteType::wide_navigation_rail_collapsed);
    s.snap_to_targets();
    CHECK_FALSE(s.animating());
    CHECK(c.rect.x == doctest::Approx(80.0));
    s.set_labels(true);
    CHECK(s.rail().labels());
    CHECK(s.bar().labels());
    settle(s);
    place(s, 1300.0);
    CHECK(c.rect.x == doctest::Approx(96.0));
}
