#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "lamina/widgets/navigation_suite.hpp"
#include "lamina/widgets/protocol.hpp"
#include "navigation_checks.hpp"
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
    CHECK(c.rect.x == doctest::Approx(80.0));
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
    const render::Rect r = s.side().item_rect(2);
    s.handle(input::PointerEvent{PointerAction::down, r.x + r.width / 2.0, r.y + r.height / 2.0});
    const widgets::NavigationSuiteResponse resp = s.handle(input::PointerEvent{PointerAction::up, r.x + r.width / 2.0, r.y + r.height / 2.0});
    REQUIRE(resp.selected.has_value());
    CHECK(*resp.selected == 2);
    CHECK(s.selected() == 2);
    CHECK(s.bar().selected() == 2);
    CHECK(s.side().selected() == 2);
}

TEST_CASE("the focus follows the navigation component when the type changes") {
    widgets::NavigationSuiteScaffold s({.items = items(), .type = NavigationSuiteType::short_navigation_bar_compact}, fonts());
    place(s, 400.0);
    s.set_focused(true);
    CHECK(s.bar().focused());
    s.set_type(NavigationSuiteType::wide_navigation_rail_collapsed);
    CHECK_FALSE(s.bar().focused());
    CHECK(s.side().focused());
    CHECK(s.focused());
    s.set_density(tokens::Density{-3});
    CHECK(s.bar().height() == 52.0);
}

TEST_CASE("labels turn off on the bar and collapse the side navigation to its icons, and snap_to_targets lands a type change and the labels at once") {
    Content c;
    auto node = std::make_shared<layout::Leaf>([](const layout::Constraints& k) { return layout::Size{k.min_width, k.min_height}; },
                                               [&c](const render::Rect& r) { c.rect = r; });
    widgets::NavigationSuiteScaffold s({.items = items(), .content = node, .type = NavigationSuiteType::navigation_drawer, .labels = false}, fonts());
    CHECK_FALSE(s.labels());
    CHECK_FALSE(s.side().expanded());
    CHECK_FALSE(s.bar().labels());
    place(s, 1300.0);
    s.set_type(NavigationSuiteType::wide_navigation_rail_collapsed);
    s.snap_to_targets();
    CHECK_FALSE(s.animating());
    CHECK(c.rect.x == doctest::Approx(80.0));
    s.set_labels(true);
    CHECK_FALSE(s.side().expanded());
    CHECK(s.bar().labels());
    settle(s);
    place(s, 1300.0);
    CHECK(c.rect.x == doctest::Approx(80.0));
    s.set_type(NavigationSuiteType::navigation_drawer);
    CHECK(s.side().expanded());
    settle(s);
    place(s, 1300.0);
    CHECK(c.rect.x == doctest::Approx(360.0));
    s.set_labels(false);
    settle(s);
    place(s, 1300.0);
    CHECK(s.type() == NavigationSuiteType::navigation_drawer);
    CHECK(c.rect.x == doctest::Approx(80.0));
}

namespace {

std::vector<render::Rect> icons_of(const widgets::NavigationSuiteScaffold& s) {
    std::vector<render::Rect> out;
    for (std::size_t i = 0; i < s.side().count(); ++i) {
        out.push_back(s.side().icon_rect(i));
    }
    return out;
}

void check_painted_once(const widgets::NavigationSuiteScaffold& s) {
    RecordingCanvas canvas(1800, 800);
    s.paint(canvas, scheme());
    const render::Rect bar = s.bar_progress() > 0.0 ? s.bar().bounds() : render::Rect{0.0, 0.0, 0.0, 0.0};
    const PaintedItems p = painted_items(canvas.fills, s.side(), bar);
    for (const RecordedFill& f : p.unknown) {
        FAIL_CHECK("a fill at " << f.bounds.x << ", " << f.bounds.y << " (" << f.bounds.width << " x " << f.bounds.height << ") belongs to no item");
    }
    for (std::size_t i = 0; i < s.side().count(); ++i) {
        CHECK(p.icons[i] == 1);
    }
}

void check_frames(widgets::NavigationSuiteScaffold& s, double width, const std::vector<render::Rect>& icons) {
    int frames = 0;
    while (s.animating() && frames < settle_frames) {
        s.step(frame_seconds);
        place(s, width);
        ++frames;
        const std::vector<render::Rect> now = icons_of(s);
        for (std::size_t i = 0; i < icons.size(); ++i) {
            CHECK(same_rect(now[i], icons[i]));
        }
        check_painted_once(s);
    }
    CHECK(frames >= 6);
}

}

TEST_CASE("collapsing and expanding the drawer, and crossing the drawer's width while expanded, keep every icon in place and draw it once") {
    widgets::NavigationSuiteScaffold s({.items = items(), .type = NavigationSuiteType::navigation_drawer}, fonts());
    place(s, 1440.0);
    s.snap_to_targets();
    place(s, 1440.0);
    const std::vector<render::Rect> icons = icons_of(s);
    CHECK(s.side_width() == doctest::Approx(360.0));
    s.set_labels(false);
    check_frames(s, 1440.0, icons);
    CHECK(s.side_width() == doctest::Approx(80.0));
    s.set_labels(true);
    check_frames(s, 1440.0, icons);
    CHECK(s.side_width() == doctest::Approx(360.0));
    s.update(info(1199.0));
    CHECK(s.type() == NavigationSuiteType::wide_navigation_rail_expanded);
    check_frames(s, 1199.0, icons);
    CHECK(s.side_width() == doctest::Approx(220.0));
    CHECK(s.side().label_opacity() == 1.0);
    s.update(info(1200.0));
    check_frames(s, 1200.0, icons);
    CHECK(s.side_width() == doctest::Approx(360.0));
    s.update(info(900.0));
    s.set_labels(false);
    check_frames(s, 900.0, icons);
    s.update(info(1600.0));
    for (int i = 0; i < settle_frames; ++i) {
        s.step(frame_seconds);
        place(s, 1600.0);
        CHECK(s.side_width() == doctest::Approx(80.0));
        CHECK(same_rect(icons_of(s)[0], icons[0]));
    }
}

TEST_CASE("the bar and the side navigation still slide past each other across 600 dp, the side icons keeping their rows") {
    widgets::NavigationSuiteScaffold s({.items = items(), .type = NavigationSuiteType::wide_navigation_rail_collapsed}, fonts());
    place(s, 700.0);
    s.snap_to_targets();
    place(s, 700.0);
    const std::vector<render::Rect> icons = icons_of(s);
    s.update(info(599.0));
    double bar = s.bar_progress();
    double side = s.side_progress();
    int frames = 0;
    while (s.animating() && frames < settle_frames) {
        s.step(frame_seconds);
        place(s, 599.0);
        ++frames;
        const double hidden = 1.0 - std::clamp(s.side_progress(), 0.0, 1.0);
        for (std::size_t i = 0; i < icons.size(); ++i) {
            CHECK(s.side().icon_rect(i).y == doctest::Approx(icons[i].y));
            CHECK(s.side().icon_rect(i).x == doctest::Approx(icons[i].x - hidden * s.side().width()));
        }
        CHECK(s.bar_progress() >= bar - 0.05);
        CHECK(s.side_progress() <= side + 0.05);
        bar = s.bar_progress();
        side = s.side_progress();
    }
    CHECK(frames >= 6);
    s.step(frame_seconds);
    place(s, 599.0);
    CHECK(s.type() == NavigationSuiteType::short_navigation_bar_compact);
    CHECK(s.bar_progress() == doctest::Approx(1.0));
    CHECK(s.side_width() == doctest::Approx(0.0));
}
