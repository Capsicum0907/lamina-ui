#include <doctest/doctest.h>

#include <cmath>

#include "lamina/motion/paging.hpp"
#include "lamina/tokens/pager.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/pager.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/zoom_view.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;
using input::PointerKind;
using widgets::Pager;

static_assert(widgets::Steppable<Pager> && widgets::HandlesKeys<Pager> && widgets::Cancellable<Pager> && widgets::HoldsPointer<Pager> &&
              widgets::Focusable<Pager>);

namespace {

constexpr const tokens::PagerTokens& pt = tokens::pager;
constexpr render::Rect area{0.0, 0.0, 400.0, 600.0};
constexpr double move_seconds = 0.008;

Pager make(std::size_t page = 2, bool mouse = true) {
    Pager p({.count = 5, .page = page, .accessible_name = "Photos", .mouse_drag = mouse});
    p.set_bounds(area);
    return p;
}

struct Child {
    int downs = 0;
    int ups = 0;
    int leaves = 0;
    bool grabs = false;
    bool held = false;
    input::ScrollAxes waiting{};

    bool operator()(const input::PointerEvent& e) {
        switch (e.action) {
        case PointerAction::down:
            ++downs;
            held = grabs;
            waiting = e.waiting_scrollers;
            break;
        case PointerAction::up:
            ++ups;
            held = false;
            break;
        case PointerAction::leave:
            ++leaves;
            held = false;
            break;
        case PointerAction::move:
            break;
        }
        return held;
    }
};

struct Finger {
    PointerKind kind = PointerKind::touch;
    double time = 1.0;

    input::PointerEvent at(PointerAction a, double x, double y) {
        input::PointerEvent e{a, x, y, a == PointerAction::down ? 1 : 0, kind, time};
        time += move_seconds;
        return e;
    }
};

template <typename Deliver>
void swipe(Pager& p, Finger& f, Deliver& deliver, double x, double y, double dx, double dy, int steps, bool lift = true) {
    p.route(f.at(PointerAction::down, x, y), deliver);
    for (int i = 1; i <= steps; ++i) {
        p.route(f.at(PointerAction::move, x + dx * i / steps, y + dy * i / steps), deliver);
    }
    if (lift) {
        p.route(f.at(PointerAction::up, x + dx, y + dy), deliver);
        p.route(input::PointerEvent{PointerAction::leave, 0.0, 0.0}, deliver);
    }
}

void slow_swipe(Pager& p, Finger& f, Child& c, double dx, int steps) {
    swipe(p, f, c, 300.0, 300.0, dx, 0.0, steps, false);
    f.time += 0.2;
    p.route(f.at(PointerAction::up, 300.0 + dx, 300.0), c);
}

}

TEST_CASE("pager tokens come from the platform's ViewPager and the touch scroll") {
    CHECK(pt.touch_slop == 16.0);
    CHECK(pt.snap.min_fling_velocity == 400.0);
    CHECK(pt.snap.min_fling_distance == 25.0);
    CHECK(pt.snap.positional_threshold == 0.6);
    CHECK(pt.catch_distance == 2.0);
    CHECK(pt.beyond_viewport_pages == 1);
    CHECK(pt.horizontal_angle_limit_degrees == tokens::touch_scroll.horizontal_angle_limit_degrees);
    CHECK(pt.settle_spring.damping_ratio == 1.0);
}

TEST_CASE("without a fling the page changes after 60 % of a page in either direction") {
    const motion::PageSnapSpec& s = pt.snap;
    CHECK(motion::snap_page(2.59, 2, -200.0, 0.0, 5, s) == 2);
    CHECK(motion::snap_page(2.61, 2, -200.0, 0.0, 5, s) == 3);
    CHECK(motion::snap_page(1.41, 2, 200.0, 0.0, 5, s) == 2);
    CHECK(motion::snap_page(1.39, 2, 200.0, 0.0, 5, s) == 1);
    CHECK(motion::snap_page(2.0, 2, 0.0, 0.0, 5, s) == 2);
    CHECK(motion::snap_page(3.7, 2, -600.0, 0.0, 5, s) == 4);
}

TEST_CASE("a fling needs both the distance and the velocity and goes to the page in its direction") {
    const motion::PageSnapSpec& s = pt.snap;
    CHECK(motion::snap_page(2.1, 2, -40.0, -500.0, 5, s) == 3);
    CHECK(motion::snap_page(1.9, 2, 40.0, 500.0, 5, s) == 1);
    CHECK(motion::snap_page(2.1, 2, -20.0, -500.0, 5, s) == 2);
    CHECK(motion::snap_page(2.1, 2, -40.0, -300.0, 5, s) == 2);
    CHECK(motion::snap_page(2.5, 2, -200.0, 500.0, 5, s) == 2);
    CHECK(motion::snap_page(3.9, 3, -200.0, -9000.0, 4, s) == 3);
    CHECK(motion::snap_page(0.0, 0, 200.0, 9000.0, 4, s) == 0);
}

TEST_CASE("the pages in view and the pages to build around them") {
    motion::PageRange r = motion::pages_in_view(2.0, 5);
    CHECK(r.first == 2);
    CHECK(r.last == 2);
    r = motion::pages_in_view(2.3, 5);
    CHECK(r.first == 2);
    CHECK(r.last == 3);
    r = motion::pages_around(2.3, 5, 1);
    CHECK(r.first == 1);
    CHECK(r.last == 4);
    r = motion::pages_around(0.0, 5, 1);
    CHECK(r.first == 0);
    CHECK(r.last == 1);
    CHECK(motion::pages_in_view(0.0, 0).empty);
    CHECK(motion::nearest_page(2.49, 5) == 2);
    CHECK(motion::nearest_page(2.5, 5) == 3);
}

TEST_CASE("a drag past the slop follows the finger and reports the page and its offset") {
    Pager p = make();
    Finger f;
    Child c;
    swipe(p, f, c, 300.0, 300.0, -100.0, 0.0, 10, false);
    CHECK(p.dragging());
    CHECK(p.holds_pointer());
    CHECK(p.position() == doctest::Approx(2.0 + (100.0 - pt.touch_slop) / area.width));
    CHECK(p.current_page() == 2);
    CHECK(p.current_page_offset_fraction() == doctest::Approx((100.0 - pt.touch_slop) / area.width));
    CHECK(c.downs == 1);
    CHECK(c.leaves == 1);
    CHECK(c.waiting.horizontal);
    CHECK(p.page_offset(3) == doctest::Approx(area.width - (100.0 - pt.touch_slop)));
    CHECK(p.pages_in_view().first == 2);
    CHECK(p.pages_in_view().last == 3);
    for (int i = 1; i <= 15; ++i) {
        p.route(f.at(PointerAction::move, 200.0 - 10.0 * i, 300.0), c);
    }
    CHECK(p.current_page() == 3);
    CHECK(p.current_page_offset_fraction() < 0.0);
}

TEST_CASE("a slow release goes back below 60 % and on above it") {
    Pager p = make();
    Finger f;
    Child c;
    slow_swipe(p, f, c, -(0.5 * area.width + pt.touch_slop), 10);
    CHECK(p.settling());
    CHECK(p.swiping());
    settle(p);
    CHECK(p.settled_page() == 2);
    CHECK(p.position() == 2.0);
    slow_swipe(p, f, c, -(0.65 * area.width + pt.touch_slop), 10);
    settle(p);
    CHECK(p.settled_page() == 3);
    CHECK(p.position() == 3.0);
    CHECK(!p.animating());
}

TEST_CASE("a quick short swipe flings to the next page and the settle never passes it") {
    Pager p = make();
    Finger f;
    Child c;
    swipe(p, f, c, 300.0, 300.0, -80.0, 0.0, 5);
    CHECK(p.target_page() == 3);
    double highest = 0.0;
    for (int i = 0; i < settle_frames; ++i) {
        p.step(frame_seconds);
        highest = std::max(highest, p.position());
    }
    CHECK(p.settled_page() == 3);
    CHECK(highest <= 3.0);
    swipe(p, f, c, 100.0, 300.0, 80.0, 0.0, 5);
    settle(p);
    CHECK(p.settled_page() == 2);
}

TEST_CASE("a tap reaches the child and the pager does not move") {
    Pager p = make();
    Finger f;
    Child c;
    swipe(p, f, c, 300.0, 300.0, -4.0, 2.0, 2);
    CHECK(c.downs == 1);
    CHECK(c.ups == 1);
    CHECK(p.position() == 2.0);
    CHECK(!p.dragging());
}

TEST_CASE("a vertical drag is left to the page, also when it turns sideways later") {
    Pager p = make();
    Finger f;
    Child c;
    p.route(f.at(PointerAction::down, 300.0, 300.0), c);
    for (int i = 1; i <= 5; ++i) {
        p.route(f.at(PointerAction::move, 300.0, 300.0 + 6.0 * i), c);
    }
    for (int i = 1; i <= 10; ++i) {
        p.route(f.at(PointerAction::move, 300.0 - 20.0 * i, 330.0), c);
    }
    CHECK(!p.dragging());
    CHECK(p.position() == 2.0);
    CHECK(c.leaves == 0);
}

TEST_CASE("a drag at 20 degrees pages and one at 40 degrees does not") {
    const double rad = 3.14159265358979323846 / 180.0;
    for (const double degrees : {20.0, 40.0}) {
        Pager p = make();
        Finger f;
        Child c;
        swipe(p, f, c, 300.0, 200.0, -30.0 * std::cos(degrees * rad), 30.0 * std::sin(degrees * rad), 6, false);
        CHECK(p.dragging() == (degrees < pt.horizontal_angle_limit_degrees));
    }
}

TEST_CASE("an inner vertical scroller takes a 40 degree drag and leaves a 20 degree drag to the pager") {
    const double rad = 3.14159265358979323846 / 180.0;
    for (const double degrees : {20.0, 40.0}) {
        Pager p = make();
        widgets::ScrollArea inner;
        inner.set_viewport(area);
        inner.set_content_height(3000.0);
        Finger f;
        auto deliver = [&](const input::PointerEvent& e) { return inner.route(e, [](const input::PointerEvent&) {}) || inner.dragging(); };
        swipe(p, f, deliver, 300.0, 200.0, -60.0 * std::cos(degrees * rad), 60.0 * std::sin(degrees * rad), 12, false);
        if (degrees < pt.horizontal_angle_limit_degrees) {
            CHECK(p.dragging());
            CHECK(inner.offset() == 0.0);
        } else {
            CHECK(!p.dragging());
            CHECK(inner.dragging());
            CHECK(p.position() == 2.0);
        }
    }
}

TEST_CASE("a child that holds the press keeps the drag") {
    Pager p = make();
    Finger f;
    Child c;
    c.grabs = true;
    swipe(p, f, c, 300.0, 300.0, -200.0, 0.0, 10);
    CHECK(p.position() == 2.0);
    CHECK(c.leaves == 1);
    CHECK(c.ups == 1);
}

TEST_CASE("a drag toward the outside of the first or last page is not taken") {
    Pager p = make(0);
    Finger f;
    Child c;
    swipe(p, f, c, 100.0, 300.0, 200.0, 0.0, 10, false);
    CHECK(!p.dragging());
    CHECK(c.leaves == 0);
    CHECK(!p.can_move(10.0));
    CHECK(p.can_move(-10.0));
    Pager q = make(4);
    CHECK(!q.can_move(-10.0));
    CHECK(q.can_move(10.0));
}

TEST_CASE("a touch during a settle catches the pager and the child only gets a leave") {
    Pager p = make();
    Finger f;
    Child c;
    slow_swipe(p, f, c, -(0.65 * area.width + pt.touch_slop), 10);
    p.step(frame_seconds);
    REQUIRE(p.settling());
    const double at = p.position();
    c = {};
    CHECK(p.route(f.at(PointerAction::down, 200.0, 300.0), c));
    CHECK(p.dragging());
    CHECK(!p.settling());
    CHECK(c.downs == 0);
    CHECK(c.leaves == 1);
    p.route(f.at(PointerAction::move, 220.0, 300.0), c);
    CHECK(p.position() == doctest::Approx(at - 20.0 / area.width));
}

TEST_CASE("a touch at the end of a settle finishes it and goes to the child") {
    Pager p = make();
    Finger f;
    Child c;
    slow_swipe(p, f, c, -(0.65 * area.width + pt.touch_slop), 10);
    while (p.settling() && std::abs(3.0 - p.position()) * area.width > pt.catch_distance) {
        p.step(frame_seconds / 4.0);
    }
    REQUIRE(p.settling());
    c = {};
    CHECK(!p.route(f.at(PointerAction::down, 200.0, 300.0), c));
    CHECK(p.settled_page() == 3);
    CHECK(c.downs == 1);
}

TEST_CASE("animate_to_page jumps near a far page first and the swipe flag stays off") {
    Pager p = make(0);
    p.animate_to_page(4);
    CHECK(p.settling());
    CHECK(!p.swiping());
    CHECK(p.target_page() == 4);
    CHECK(p.position() == doctest::Approx(4.0 - static_cast<double>(pt.animate_jump_pages)));
    CHECK(p.pages_in_view().first == 3);
    settle(p);
    CHECK(p.settled_page() == 4);
    CHECK(p.position() == 4.0);
    p.scroll_to_page(1);
    CHECK(p.settled_page() == 1);
    CHECK(!p.animating());
}

TEST_CASE("the arrow keys page while the pager has the keyboard focus") {
    Pager p = make();
    CHECK(!p.handle(input::KeyEvent{input::Key::right, false}));
    p.set_focused(true);
    CHECK(p.focused());
    CHECK(p.handle(input::KeyEvent{input::Key::right, false}));
    CHECK(p.handle(input::KeyEvent{input::Key::right, false}));
    CHECK(p.target_page() == 4);
    CHECK(!p.handle(input::KeyEvent{input::Key::right, false}));
    CHECK(p.handle(input::KeyEvent{input::Key::home, false}));
    settle(p);
    CHECK(p.settled_page() == 0);
    Finger f;
    Child c;
    p.route(f.at(PointerAction::down, 10.0, 10.0), c);
    CHECK(!p.focused());
    const auto node = p.accessible();
    CHECK(node.role == widgets::AccessibleRole::group);
    CHECK(node.name == "Photos");
    REQUIRE(node.state.range);
    CHECK(node.state.range->max == 4.0);
}

TEST_CASE("the mouse drags a pager unless the pager turns it off") {
    for (const bool mouse : {true, false}) {
        Pager p = make(2, mouse);
        Finger f{PointerKind::mouse};
        Child c;
        swipe(p, f, c, 300.0, 300.0, -150.0, 0.0, 10, false);
        CHECK(p.dragging() == mouse);
    }
}

TEST_CASE("a leave during a drag settles on the nearest page") {
    Pager p = make();
    Finger f;
    Child c;
    swipe(p, f, c, 300.0, 300.0, -(0.7 * area.width + pt.touch_slop), 0.0, 10, false);
    p.route(input::PointerEvent{PointerAction::leave, 0.0, 0.0}, c);
    CHECK(!p.dragging());
    settle(p);
    CHECK(p.settled_page() == 3);
    p.set_enabled(false);
    swipe(p, f, c, 300.0, 300.0, -200.0, 0.0, 10, false);
    CHECK(!p.dragging());
}

namespace {

struct Photo {
    Pager pager = make();
    widgets::ZoomView zoom{{.accessible_name = "Photo", .content_width = 800.0, .content_height = 1200.0}};
    double time = 1.0;

    Photo() { zoom.set_bounds(pager.page_rect(pager.settled_page())); }

    bool deliver(const input::PointerEvent& e) {
        zoom.handle(e);
        return zoom.holds_pointer();
    }
    void finger(input::FingerAction a, std::uint64_t id, double x, double y) { zoom.handle(input::FingerEvent{a, id, x, y, time}); }
    void touch(PointerAction a, double x, double y) {
        pager.route(input::PointerEvent{a, x, y, a == PointerAction::down ? 1 : 0, PointerKind::touch, time},
                    [this](const input::PointerEvent& e) { return deliver(e); });
        time += move_seconds;
    }
    void drag(double x, double y, double dx, int steps) {
        finger(input::FingerAction::down, 1, x, y);
        touch(PointerAction::down, x, y);
        for (int i = 1; i <= steps; ++i) {
            const double at = x + dx * i / steps;
            finger(input::FingerAction::move, 1, at, y);
            touch(PointerAction::move, at, y);
        }
    }
    void zoom_at(double scale, double x, double y) {
        zoom.zoom_to(scale, x, y);
        settle(zoom);
    }
};

}

TEST_CASE("a zoomed photo keeps a drag it can follow and the pager waits") {
    Photo ph;
    ph.zoom_at(2.0, 200.0, 300.0);
    const double x = ph.zoom.transform().x;
    ph.drag(300.0, 300.0, -120.0, 12);
    CHECK(ph.zoom.holds_pointer());
    CHECK(ph.zoom.transform().x < x);
    CHECK(!ph.pager.dragging());
    CHECK(ph.pager.position() == 2.0);
}

TEST_CASE("at the edge of a zoomed photo the pager takes the drag and the photo stays put") {
    Photo ph;
    ph.zoom_at(2.0, 400.0, 300.0);
    REQUIRE(ph.zoom.edges().right);
    const motion::ZoomTransform before = ph.zoom.transform();
    ph.drag(300.0, 300.0, -120.0, 12);
    CHECK(ph.pager.dragging());
    CHECK(ph.pager.position() > 2.0);
    CHECK(!ph.zoom.holds_pointer());
    CHECK(ph.zoom.transform().x == before.x);
    CHECK(ph.zoom.scale() == doctest::Approx(before.scale));
}

TEST_CASE("a photo at its fitted scale leaves every sideways drag to the pager") {
    Photo ph;
    ph.drag(300.0, 300.0, -120.0, 12);
    CHECK(ph.pager.dragging());
    CHECK(ph.zoom.scale() == 1.0);
}

TEST_CASE("two fingers belong to the zoom view and a finger added after the pager took the drag does not take it back") {
    Photo ph;
    ph.finger(input::FingerAction::down, 1, 150.0, 300.0);
    ph.touch(PointerAction::down, 150.0, 300.0);
    ph.finger(input::FingerAction::down, 2, 300.0, 300.0);
    for (int i = 1; i <= 10; ++i) {
        ph.finger(input::FingerAction::move, 1, 150.0 - 10.0 * i, 300.0);
        ph.finger(input::FingerAction::move, 2, 300.0 + 6.0 * i, 300.0);
        ph.touch(PointerAction::move, 150.0 - 10.0 * i, 300.0);
    }
    CHECK(ph.zoom.scale() > 1.0);
    CHECK(!ph.pager.dragging());
    CHECK(ph.pager.position() == 2.0);

    Photo second;
    second.drag(300.0, 300.0, -100.0, 10);
    REQUIRE(second.pager.dragging());
    second.finger(input::FingerAction::down, 2, 100.0, 300.0);
    for (int i = 1; i <= 10; ++i) {
        second.finger(input::FingerAction::move, 1, 200.0 - 5.0 * i, 300.0);
        second.finger(input::FingerAction::move, 2, 100.0 - 15.0 * i, 300.0);
        second.touch(PointerAction::move, 200.0 - 5.0 * i, 300.0);
    }
    CHECK(second.zoom.scale() == 1.0);
    CHECK(second.pager.dragging());
}

TEST_CASE("a touch that catches the pager's settle hands a zoomed photo's finger to the pager") {
    Photo ph;
    ph.pager.animate_to_page(3);
    ph.pager.step(frame_seconds);
    REQUIRE(ph.pager.settling());
    ph.zoom_at(2.0, 200.0, 300.0);
    const motion::ZoomTransform before = ph.zoom.transform();
    ph.drag(300.0, 300.0, -60.0, 6);
    CHECK(ph.pager.dragging());
    CHECK(!ph.zoom.holds_pointer());
    CHECK(ph.zoom.transform().x == before.x);
    CHECK(ph.zoom.transform().y == before.y);
}
