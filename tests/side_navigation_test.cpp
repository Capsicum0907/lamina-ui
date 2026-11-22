#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

#include "lamina/tokens/side_navigation.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/side_navigation.hpp"
#include "navigation_checks.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;

static_assert(widgets::Paintable<widgets::SideNavigation> && widgets::Steppable<widgets::SideNavigation> &&
              widgets::HandlesPointer<widgets::SideNavigation> && widgets::HandlesKeys<widgets::SideNavigation> &&
              widgets::FocusGated<widgets::SideNavigation> && widgets::Densifiable<widgets::SideNavigation>);

namespace {

constexpr const tokens::SideNavigationTokens& t = tokens::side_navigation_tokens;
constexpr double tolerance = navigation_tolerance;

std::vector<widgets::NavigationItem> items() {
    std::vector<widgets::NavigationItem> out;
    for (auto [symbol, label] : {std::pair{res::icons::home, "Home"}, std::pair{res::icons::search, "Search"},
                                 std::pair{res::icons::settings, "Settings"}, std::pair{res::icons::favorite, "Favorites"}}) {
        out.push_back({icons::Icon{icon_font(), symbol}, label});
    }
    return out;
}

widgets::SideNavigation make(bool expanded, bool drawer, tokens::Density density = {}) {
    widgets::SideNavigation side({items(), 1, expanded, drawer, "Navigation", density}, fonts());
    side.measure({0.0, 1440.0, 800.0, 800.0});
    side.arrange({0.0, 0.0, side.width(), 800.0});
    return side;
}

bool same(const render::Rect& a, const render::Rect& b) {
    return same_rect(a, b);
}

std::vector<render::Rect> icons_of(const widgets::SideNavigation& s) {
    std::vector<render::Rect> out;
    for (std::size_t i = 0; i < s.count(); ++i) {
        out.push_back(s.icon_rect(i));
    }
    return out;
}

int run(widgets::SideNavigation& s, const std::function<void()>& each) {
    int n = 0;
    while (s.animating() && n < settle_frames) {
        s.step(frame_seconds);
        ++n;
        each();
    }
    return n;
}

void check_painted_once(const widgets::SideNavigation& s) {
    RecordingCanvas canvas(1500, 800);
    s.paint(canvas, scheme());
    const PaintedItems p = painted_items(canvas.fills, s, {0.0, 0.0, 0.0, 0.0});
    for (const RecordedFill& f : p.unknown) {
        FAIL_CHECK("a fill at " << f.bounds.x << ", " << f.bounds.y << " (" << f.bounds.width << " x " << f.bounds.height << ") belongs to no item");
    }
    for (std::size_t i = 0; i < s.count(); ++i) {
        CHECK(p.icons[i] == 1);
    }
}

}

TEST_CASE("side navigation tokens reuse the rail's and the drawer's values and add a critically damped spring of lamina-ui's own") {
    CHECK(t.collapsed_width == 80.0);
    CHECK(t.expanded_min_width == 220.0);
    CHECK(t.expanded_max_width == 360.0);
    CHECK(t.drawer_width == 360.0);
    CHECK(t.top_space == 12.0);
    CHECK(t.item_inset == 12.0);
    CHECK(t.indicator_padding == 16.0);
    CHECK(t.collapsed_indicator_width == 56.0);
    CHECK(t.collapsed_indicator_height == 32.0);
    CHECK(t.icon_label_space == 12.0);
    CHECK(t.collapsed_width == 2.0 * t.item_inset + t.collapsed_indicator_width);
    CHECK(t.collapsed_indicator_width == 2.0 * t.indicator_padding + t.icon_size);
    CHECK(t.expansion.damping_ratio == 1.0);
    CHECK(t.expansion.stiffness == tokens::motion_scheme.default_spatial().stiffness);
}

TEST_CASE("the icons sit at the same place in the collapsed rail, the wide rail and the drawer at every density") {
    for (const int scale : {0, -1, -2, -4}) {
        CAPTURE(scale);
        const tokens::Density density{scale};
        auto collapsed = make(false, false, density);
        auto rail = make(true, false, density);
        auto drawer = make(true, true, density);
        auto collapsed_from_drawer = make(false, true, density);
        CHECK(collapsed.width() == doctest::Approx(80.0));
        CHECK(collapsed_from_drawer.width() == doctest::Approx(80.0));
        CHECK(rail.width() == doctest::Approx(220.0));
        CHECK(drawer.width() == doctest::Approx(360.0));
        const double row = tokens::density_tokens.drawer_item.at(density);
        for (std::size_t i = 0; i < collapsed.count(); ++i) {
            const render::Rect icon = collapsed.icon_rect(i);
            CHECK(icon.x == doctest::Approx(28.0));
            CHECK(icon.y == doctest::Approx(t.top_space + row * static_cast<double>(i) + (row - 24.0) / 2.0));
            CHECK(same(rail.icon_rect(i), icon));
            CHECK(same(drawer.icon_rect(i), icon));
            CHECK(same(collapsed_from_drawer.icon_rect(i), icon));
        }
        const render::Rect pill = collapsed.indicator_rect(1);
        const render::Rect icon = collapsed.icon_rect(1);
        CHECK(pill.width == doctest::Approx(56.0));
        CHECK(pill.height == doctest::Approx(32.0));
        CHECK(pill.x + pill.width / 2.0 == doctest::Approx(icon.x + icon.width / 2.0));
        CHECK(pill.y + pill.height / 2.0 == doctest::Approx(icon.y + icon.height / 2.0));
        const render::Rect full = drawer.indicator_rect(1);
        CHECK(full.x == doctest::Approx(12.0));
        CHECK(full.width == doctest::Approx(360.0 - 24.0));
        CHECK(full.height == doctest::Approx(row));
        const render::Rect hug = rail.indicator_rect(1);
        CHECK(hug.x == doctest::Approx(12.0));
        CHECK(hug.height == doctest::Approx(row));
        CHECK(hug.width < 220.0 - 24.0);
        CHECK(rail.label_rect(1).x + rail.label_rect(1).width + t.indicator_padding == doctest::Approx(hug.x + hug.width));
    }
}

TEST_CASE("collapsing and expanding move no icon, draw each item once, and morph the width and the indicator monotonically") {
    for (const bool drawer : {true, false}) {
        CAPTURE(drawer);
        for (const bool expand : {false, true}) {
            CAPTURE(expand);
            auto s = make(!expand, drawer);
            const std::vector<render::Rect> icons = icons_of(s);
            const double from = s.width();
            s.set_expanded(expand);
            double width = s.width();
            render::Rect pill = s.indicator_rect(1);
            int moving = 0;
            const int frames = run(s, [&] {
                CHECK(icons_of(s).size() == icons.size());
                for (std::size_t i = 0; i < icons.size(); ++i) {
                    CHECK(same(s.icon_rect(i), icons[i]));
                }
                check_painted_once(s);
                const render::Rect now = s.indicator_rect(1);
                if (expand) {
                    CHECK(s.width() >= width - 1e-9);
                    CHECK(now.width >= pill.width - 1e-9);
                    CHECK(now.height >= pill.height - 1e-9);
                } else {
                    CHECK(s.width() <= width + 1e-9);
                    CHECK(now.width <= pill.width + 1e-9);
                    CHECK(now.height <= pill.height + 1e-9);
                }
                CHECK(now.x == doctest::Approx(12.0));
                CHECK(now.y + now.height / 2.0 == doctest::Approx(icons[1].y + icons[1].height / 2.0));
                if (std::abs(s.width() - width) > tolerance) {
                    ++moving;
                }
                width = s.width();
                pill = now;
            });
            CHECK(frames < settle_frames);
            CHECK(moving >= 6);
            CHECK(std::abs(s.width() - from) > 100.0);
        }
    }
}

TEST_CASE("the labels fade out in the first part of collapsing and in during the last part of expanding") {
    auto s = make(true, true);
    CHECK(s.label_opacity() == 1.0);
    s.set_expanded(false);
    std::vector<double> collapse;
    run(s, [&] { collapse.push_back(s.label_opacity()); });
    REQUIRE(collapse.size() > 6);
    const auto gone = std::find(collapse.begin(), collapse.end(), 0.0);
    REQUIRE(gone != collapse.end());
    CHECK(static_cast<std::size_t>(gone - collapse.begin()) < collapse.size() / 3);
    CHECK(std::is_sorted(collapse.rbegin(), collapse.rend()));
    s.set_expanded(true);
    std::vector<double> expand;
    std::vector<double> widths;
    run(s, [&] {
        expand.push_back(s.label_opacity());
        widths.push_back(s.width());
    });
    REQUIRE(expand.size() > 6);
    CHECK(std::is_sorted(expand.begin(), expand.end()));
    const auto shown = std::find_if(expand.begin(), expand.end(), [](double a) { return a > 0.0; });
    REQUIRE(shown != expand.end());
    const std::size_t first = static_cast<std::size_t>(shown - expand.begin());
    CHECK(first >= 3);
    CHECK(widths[first] - 80.0 > (360.0 - 80.0) / 2.0);
    CHECK(expand.back() == doctest::Approx(1.0));
}

TEST_CASE("the wide rail widens into the drawer with its labels shown, its icons in place and its indicator stretching to the row") {
    auto s = make(true, false);
    const std::vector<render::Rect> icons = icons_of(s);
    const double hug = s.indicator_rect(1).width;
    s.set_drawer(true);
    double width = s.width();
    double pill = hug;
    int moving = 0;
    run(s, [&] {
        CHECK(s.label_opacity() == 1.0);
        for (std::size_t i = 0; i < icons.size(); ++i) {
            CHECK(same(s.icon_rect(i), icons[i]));
        }
        check_painted_once(s);
        CHECK(s.width() >= width - 1e-9);
        CHECK(s.indicator_rect(1).width >= pill - 1e-9);
        moving += std::abs(s.width() - width) > tolerance ? 1 : 0;
        width = s.width();
        pill = s.indicator_rect(1).width;
    });
    CHECK(moving >= 6);
    CHECK(s.width() == doctest::Approx(360.0));
    CHECK(s.indicator_rect(1).width == doctest::Approx(336.0));
    s.set_drawer(false);
    settle(s);
    CHECK(s.width() == doctest::Approx(220.0));
    CHECK(s.indicator_rect(1).width == doctest::Approx(hug));
}

TEST_CASE("the side navigation selects by pointer and by keys and names its items") {
    auto s = make(false, false);
    settle(s);
    const render::Rect r = s.item_rect(3);
    CHECK(click(s, r.x + r.width / 2.0, r.y + r.height / 2.0));
    CHECK(s.selected() == 3);
    s.set_focused(true);
    CHECK(s.focused_item() == 3);
    s.handle(input::KeyEvent{input::Key::up});
    CHECK(s.focused_item() == 2);
    CHECK(s.handle(input::KeyEvent{input::Key::enter}));
    CHECK(s.selected() == 2);
    const widgets::AccessibleNode node = s.accessible();
    CHECK(node.role == widgets::AccessibleRole::navigation);
    REQUIRE(node.children.size() == 4);
    CHECK(node.children[2].name == "Settings");
    CHECK(node.children[2].state.selected);
    s.set_item_enabled(0, false);
    CHECK_FALSE(s.item_enabled(0));
}
