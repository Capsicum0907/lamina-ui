#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/widgets/navigation_drawer.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::DrawerValue;

namespace {

constexpr const tokens::NavigationDrawerTokens& t = tokens::navigation_drawer_tokens;
constexpr render::Rect area{0.0, 0.0, 800.0, 600.0};

std::vector<widgets::DrawerEntry> entries() {
    const icons::Icon inbox{icon_font(), test::res::icons::add};
    return {widgets::drawer_headline("Mail"),
            {widgets::DrawerEntryKind::item, "Inbox", inbox, widgets::BadgeSpec{"24", "24 unread"}},
            {widgets::DrawerEntryKind::item, "Outbox", inbox},
            {widgets::DrawerEntryKind::item, "Spam", inbox, std::nullopt, false},
            widgets::drawer_divider(),
            widgets::drawer_headline("Labels"),
            {widgets::DrawerEntryKind::item, "Family"}};
}

struct Clock {
    double now = 0.0;
    input::PointerEvent at(PointerAction a, double x, double y) { return {a, x, y, 0, input::PointerKind::mouse, now}; }
};

template <typename W>
widgets::DrawerResponse drag(W& w, Clock& clock, double y, double from, double to, double seconds) {
    w.handle(clock.at(PointerAction::down, from, y));
    const int n = std::max(2, static_cast<int>(seconds / frame_seconds));
    for (int i = 1; i <= n; ++i) {
        clock.now += frame_seconds;
        w.handle(clock.at(PointerAction::move, from + (to - from) * i / n, y));
        w.step(frame_seconds);
    }
    return w.handle(clock.at(PointerAction::up, to, y));
}

widgets::NavigationDrawerOptions options() {
    return {{entries(), 1}};
}

}

TEST_CASE("a drawer sheet is 360 dp wide with 56 dp pills inset 12 dp, headlines and dividers") {
    widgets::DrawerSheet s({entries(), 1}, fonts());
    const layout::Size size = s.measure({0.0, 800.0, 0.0, 600.0});
    CHECK(size.width == doctest::Approx(t.max_width));
    CHECK(size.height == doctest::Approx(600.0));
    CHECK(s.measure({0.0, 300.0, 0.0, 600.0}).width == doctest::Approx(300.0));
    s.arrange({0.0, 0.0, size.width, size.height});
    const render::Rect inbox = s.item_rect(1);
    CHECK(inbox.x == doctest::Approx(t.item_inset));
    CHECK(inbox.width == doctest::Approx(t.max_width - 2.0 * t.item_inset));
    CHECK(inbox.height == doctest::Approx(t.item_height));
    CHECK(s.entry_rect(0).y == doctest::Approx(t.top_space));
    CHECK(s.entry_rect(0).height == doctest::Approx(t.headline_height));
    CHECK(s.entry_rect(4).height == doctest::Approx(t.divider_top_space + 1.0 + t.divider_bottom_space));
    const widgets::AccessibleNode a = s.accessible();
    CHECK(a.role == widgets::AccessibleRole::navigation);
    CHECK(a.name == "Navigation menu");
    REQUIRE(a.children.size() == 7);
    CHECK(a.children[0].role == widgets::AccessibleRole::heading);
    CHECK(a.children[1].state.selected == true);
    CHECK(a.children[1].description == "24 unread");
    CHECK_FALSE(a.children[3].state.enabled);
    CHECK(a.children[4].role == widgets::AccessibleRole::separator);
}

TEST_CASE("the drawer sheet is one Tab stop whose arrows skip headlines, dividers and disabled items") {
    widgets::DrawerSheet s({entries(), 1}, fonts());
    s.arrange({0.0, 0.0, 360.0, 600.0});
    s.set_focused(true);
    CHECK(s.focused_item() == 1u);
    s.handle(input::KeyEvent{Key::down, false});
    CHECK(s.focused_item() == 2u);
    s.handle(input::KeyEvent{Key::down, false});
    CHECK(s.focused_item() == 6u);
    s.handle(input::KeyEvent{Key::down, false});
    CHECK(s.focused_item() == 1u);
    s.handle(input::KeyEvent{Key::up, false});
    CHECK(s.focused_item() == 6u);
    const widgets::DrawerResponse r = s.handle(input::KeyEvent{Key::enter, false});
    CHECK(r.selected == 6u);
    CHECK(s.selected() == 6u);
}

TEST_CASE("clicking an enabled drawer item selects it and a disabled one does nothing") {
    widgets::DrawerSheet s({entries(), 1}, fonts());
    s.arrange({0.0, 0.0, 360.0, 600.0});
    const render::Rect outbox = s.item_rect(2);
    s.handle(input::PointerEvent{PointerAction::down, outbox.x + 20.0, outbox.y + 20.0});
    const auto r = s.handle(input::PointerEvent{PointerAction::up, outbox.x + 20.0, outbox.y + 20.0});
    CHECK(r.selected == 2u);
    const render::Rect spam = s.item_rect(3);
    s.handle(input::PointerEvent{PointerAction::down, spam.x + 20.0, spam.y + 20.0});
    const auto d = s.handle(input::PointerEvent{PointerAction::up, spam.x + 20.0, spam.y + 20.0});
    CHECK_FALSE(d.selected);
    CHECK(s.selected() == 2u);
}

TEST_CASE("a modal drawer opens over a scrim and closes on Escape or a click on the scrim") {
    widgets::ModalNavigationDrawer d(options(), fonts());
    d.set_area(area);
    CHECK_FALSE(d.visible());
    CHECK(d.offset() == doctest::Approx(-t.max_width));
    d.open(input::InputSource::keyboard);
    settle(d);
    CHECK(d.current_value() == DrawerValue::open);
    CHECK(d.offset() == doctest::Approx(0.0));
    CHECK(d.scrim_alpha() == doctest::Approx(1.0));
    CHECK(d.sheet().focused_item() == 1u);
    const auto esc = d.handle(input::KeyEvent{Key::escape, false});
    CHECK(esc.closed);
    settle(d);
    CHECK_FALSE(d.visible());
    d.open();
    settle(d);
    Clock clock;
    d.handle(clock.at(PointerAction::down, 600.0, 300.0));
    const auto r = d.handle(clock.at(PointerAction::up, 600.0, 300.0));
    CHECK(r.closed);
    CHECK(d.target_value() == DrawerValue::closed);
}

TEST_CASE("dragging across closed content opens the drawer once it passes half way or flings past 400 dp/s") {
    widgets::ModalNavigationDrawer d(options(), fonts());
    d.set_area(area);
    Clock clock;
    d.handle(clock.at(PointerAction::down, 20.0, 300.0));
    clock.now += 0.5;
    auto r = d.handle(clock.at(PointerAction::move, 20.0 + 100.0, 300.0));
    CHECK(r.took_over);
    CHECK(d.offset() == doctest::Approx(-t.max_width + 100.0 - tokens::touch_slop));
    clock.now += 0.5;
    d.handle(clock.at(PointerAction::up, 120.0, 300.0));
    settle(d);
    CHECK(d.current_value() == DrawerValue::closed);
    drag(d, clock, 300.0, 20.0, 260.0, 1.0);
    settle(d);
    CHECK(d.current_value() == DrawerValue::open);
    const auto fling = drag(d, clock, 300.0, 300.0, 240.0, 0.06);
    CHECK(fling.closed);
    CHECK(d.target_value() == DrawerValue::closed);
}

TEST_CASE("an opening drawer overshoots and its container stretches to the window edge") {
    widgets::ModalNavigationDrawer d(options(), fonts());
    d.set_area(area);
    d.open();
    double furthest = -t.max_width;
    thorvg::Canvas canvas(800, 600);
    bool checked = false;
    for (int i = 0; i < 120; ++i) {
        d.step(frame_seconds);
        furthest = std::max(furthest, d.offset());
        if (!checked && d.offset() > 2.0) {
            canvas.clear(0xff000000);
            d.paint(canvas, scheme());
            canvas.finish();
            const color::Argb left = canvas.pixel(0, 300);
            const color::Argb inside = canvas.pixel(100, 300);
            CHECK(left == inside);
            checked = true;
        }
    }
    CHECK(furthest > 0.0);
    CHECK(checked);
}

TEST_CASE("selecting an item in a modal drawer reports it") {
    widgets::ModalNavigationDrawer d(options(), fonts());
    d.set_area(area);
    d.snap_to(DrawerValue::open);
    const render::Rect outbox = d.sheet().item_rect(2);
    Clock clock;
    d.handle(clock.at(PointerAction::down, outbox.x + 30.0, outbox.y + 28.0));
    const auto r = d.handle(clock.at(PointerAction::up, outbox.x + 30.0, outbox.y + 28.0));
    CHECK(r.selected == 2u);
    const auto node = d.accessible();
    REQUIRE(node);
    CHECK(node->state.modal);
    CHECK(widgets::find_accessible(node->children, "Close navigation menu"));
}

TEST_CASE("a dismissible drawer pushes its content aside") {
    auto content = layout::spacer({10.0, 10.0});
    widgets::DismissibleNavigationDrawer d(options(), content, fonts());
    d.measure({0.0, 800.0, 0.0, 600.0});
    d.arrange(area);
    CHECK(d.content_rect().x == doctest::Approx(0.0));
    d.open();
    settle(d);
    CHECK(d.content_rect().x == doctest::Approx(t.max_width));
    CHECK(d.sheet().bounds().x == doctest::Approx(0.0));
    d.set_focused(true);
    const auto r = d.handle(input::KeyEvent{Key::escape, false});
    CHECK(r.closed);
    settle(d);
    CHECK(d.content_rect().x == doctest::Approx(0.0));
}

TEST_CASE("a permanent drawer is a sheet in a row before the content") {
    widgets::DrawerSheet s({entries(), 1, widgets::DrawerSheetStyle::standard}, fonts());
    auto content = layout::spacer({10.0, 10.0});
    auto row = widgets::permanent_drawer(s, content);
    row->measure({0.0, 1000.0, 0.0, 600.0});
    row->arrange({0.0, 0.0, 1000.0, 600.0});
    CHECK(s.bounds().width == doctest::Approx(t.max_width));
    CHECK(s.bounds().height == doctest::Approx(600.0));
}
