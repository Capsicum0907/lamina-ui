#include <doctest/doctest.h>

#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::ScrollArea;

static_assert(widgets::Paintable<ScrollArea> && widgets::Steppable<ScrollArea> && widgets::HandlesPointer<ScrollArea> &&
              widgets::HandlesKeys<ScrollArea> && widgets::Cancellable<ScrollArea>);

namespace {

constexpr const tokens::ScrollAreaTokens& t = tokens::scroll_area_tokens;

ScrollArea make(double content = 1000.0) {
    ScrollArea s;
    s.set_viewport({10.0, 20.0, 200.0, 300.0});
    s.set_content_height(content);
    return s;
}

void wheel(ScrollArea& s, double notches) {
    s.handle(input::WheelEvent{100.0, 100.0, 0.0, notches});
}

}

TEST_CASE("scroll area tokens") {
    CHECK(t.min_thumb_length == 16.0);
    CHECK(t.bar == color::Role::on_surface);
    CHECK(t.offset_spring.damping_ratio == 1.0);
    CHECK(t.bar_thickness < t.hovered_bar_thickness);
}

TEST_CASE("the wheel scrolls by a step per notch and settles on the target") {
    auto s = make();
    wheel(s, 1.0);
    CHECK(s.target_offset() == doctest::Approx(t.wheel_step));
    CHECK(s.offset() == 0.0);
    s.step(frame_seconds);
    CHECK(s.offset() > 0.0);
    CHECK(s.offset() < t.wheel_step);
    settle(s);
    CHECK(s.offset() == doctest::Approx(t.wheel_step));
    wheel(s, -0.5);
    settle(s);
    CHECK(s.offset() == doctest::Approx(t.wheel_step / 2.0));
}

TEST_CASE("the offset never passes its target on the way") {
    auto s = make();
    wheel(s, 3.0);
    double highest = 0.0;
    for (int i = 0; i < settle_frames; ++i) {
        s.step(frame_seconds);
        highest = std::max(highest, s.offset());
    }
    CHECK(highest <= 3.0 * t.wheel_step + 1e-9);
}

TEST_CASE("the offset stays between the top and the end of the content") {
    auto s = make(1000.0);
    CHECK(s.max_offset() == doctest::Approx(700.0));
    wheel(s, -2.0);
    CHECK(s.target_offset() == 0.0);
    wheel(s, 100.0);
    CHECK(s.target_offset() == doctest::Approx(700.0));
    settle(s);
    s.set_content_height(500.0);
    CHECK(s.target_offset() == doctest::Approx(200.0));
    CHECK(s.offset() == doctest::Approx(700.0));
    settle(s);
    CHECK(s.offset() == doctest::Approx(200.0));
    s.set_content_height(100.0);
    CHECK(s.max_offset() == 0.0);
    CHECK(s.target_offset() == 0.0);
}

TEST_CASE("a wheel outside the viewport or over content that fits is left alone") {
    auto s = make();
    CHECK_FALSE(s.handle(input::WheelEvent{300.0, 100.0, 0.0, 1.0}));
    CHECK(s.target_offset() == 0.0);
    auto short_content = make(200.0);
    CHECK_FALSE(short_content.handle(input::WheelEvent{100.0, 100.0, 0.0, 1.0}));
}

TEST_CASE("Page Down and Page Up move by the viewport less the overlap") {
    auto s = make();
    CHECK(s.handle(input::KeyEvent{Key::page_down, false}));
    CHECK(s.target_offset() == doctest::Approx(300.0 - t.page_overlap));
    CHECK(s.handle(input::KeyEvent{Key::page_up, false}));
    CHECK(s.target_offset() == 0.0);
    CHECK_FALSE(s.handle(input::KeyEvent{Key::down, false}));
}

TEST_CASE("ensure_visible scrolls as little as it can") {
    auto s = make();
    s.ensure_visible(100.0, 150.0);
    CHECK(s.target_offset() == 0.0);
    s.ensure_visible(400.0, 450.0);
    CHECK(s.target_offset() == doctest::Approx(150.0));
    s.ensure_visible(120.0, 160.0);
    CHECK(s.target_offset() == doctest::Approx(120.0));
    s.ensure_visible(200.0, 600.0);
    CHECK(s.target_offset() == doctest::Approx(200.0));
}

TEST_CASE("pointer events reach the content in its coordinates, and only inside the viewport") {
    auto s = make();
    s.jump_to(50.0);
    const input::PointerEvent in = s.to_content({PointerAction::down, 30.0, 40.0, 2});
    CHECK(in.action == PointerAction::down);
    CHECK(in.x_dp == doctest::Approx(20.0));
    CHECK(in.y_dp == doctest::Approx(70.0));
    CHECK(in.clicks == 2);
    CHECK(s.to_content({PointerAction::up, 30.0, 10.0}).action == PointerAction::leave);
    CHECK(s.to_content({PointerAction::move, 205.0, 100.0}).action == PointerAction::leave);
    const render::Rect w = s.to_window({20.0, 70.0, 5.0, 6.0});
    CHECK(w.x == doctest::Approx(30.0));
    CHECK(w.y == doctest::Approx(40.0));
}

TEST_CASE("after a scroll under a still pointer, rehover gives a move at the new place once") {
    auto s = make();
    s.handle(input::PointerEvent{PointerAction::move, 50.0, 100.0});
    CHECK_FALSE(s.rehover());
    wheel(s, 1.0);
    settle(s);
    const auto e = s.rehover();
    REQUIRE(e);
    CHECK(e->action == PointerAction::move);
    CHECK(e->y_dp == doctest::Approx(80.0 + t.wheel_step));
    CHECK_FALSE(s.rehover());
}

TEST_CASE("the bar shows while scrolling and hides after the delay") {
    auto s = make();
    CHECK(s.bar_visibility() == 0.0);
    CHECK(s.next_wake_seconds() < 0.0);
    wheel(s, 1.0);
    int frames = 0;
    while (s.animating() && frames < settle_frames) {
        s.step(frame_seconds);
        ++frames;
    }
    CHECK(frames * frame_seconds < t.hide_delay_seconds);
    CHECK(s.bar_visibility() == doctest::Approx(1.0));
    const double wake = s.next_wake_seconds();
    CHECK(wake > 0.0);
    CHECK(wake <= t.hide_delay_seconds);
    s.step(wake + frame_seconds);
    CHECK(s.animating());
    settle(s);
    CHECK(s.bar_visibility() == doctest::Approx(0.0));
    CHECK(s.next_wake_seconds() < 0.0);
}

TEST_CASE("the thumb is as long as the visible share and grows thicker under the pointer") {
    auto s = make(1200.0);
    const auto thumb = s.thumb();
    REQUIRE(thumb);
    const double track = 300.0 - 2.0 * t.bar_margin;
    CHECK(thumb->height == doctest::Approx(track * 300.0 / 1200.0));
    CHECK(thumb->width == doctest::Approx(t.bar_thickness));
    CHECK(thumb->x + thumb->width == doctest::Approx(210.0 - t.bar_margin));
    s.handle(input::PointerEvent{PointerAction::move, 208.0, 30.0});
    settle(s);
    CHECK(s.thumb()->width == doctest::Approx(t.hovered_bar_thickness));
    CHECK(s.bar_visibility() == doctest::Approx(1.0));
    CHECK(s.next_wake_seconds() < 0.0);
    CHECK_FALSE(make(100.0).thumb());
    CHECK(make(100000.0).thumb()->height == doctest::Approx(t.min_thumb_length));
}

TEST_CASE("dragging the thumb moves the content in proportion") {
    auto s = make(1200.0);
    const render::Rect thumb = *s.thumb();
    const double x = thumb.x + thumb.width / 2.0;
    const double y = thumb.y + thumb.height / 2.0;
    CHECK(s.handle(input::PointerEvent{PointerAction::down, x, y}));
    CHECK(s.to_content({PointerAction::move, 50.0, 50.0}).action == PointerAction::leave);
    const double travel = 300.0 - 2.0 * t.bar_margin - thumb.height;
    CHECK(s.handle(input::PointerEvent{PointerAction::move, x, y + travel / 2.0}));
    CHECK(s.offset() == doctest::Approx(450.0));
    CHECK(s.handle(input::PointerEvent{PointerAction::move, x, y + 2.0 * travel}));
    CHECK(s.offset() == doctest::Approx(900.0));
    CHECK(s.handle(input::PointerEvent{PointerAction::up, x, y}));
    CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::move, 50.0, 50.0}));
}

TEST_CASE("a press on the track pages toward it") {
    auto s = make(1200.0);
    CHECK(s.handle(input::PointerEvent{PointerAction::down, 207.0, 300.0}));
    CHECK(s.target_offset() == doctest::Approx(300.0 - t.page_overlap));
}

TEST_CASE("cancel_interaction ends a drag") {
    auto s = make(1200.0);
    const render::Rect thumb = *s.thumb();
    s.handle(input::PointerEvent{PointerAction::down, thumb.x + 1.0, thumb.y + 1.0});
    s.cancel_interaction();
    CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::move, thumb.x + 1.0, thumb.y + 100.0}));
    CHECK(s.offset() == 0.0);
}

TEST_CASE("content is drawn shifted by the offset and clipped to the viewport, also with a clip inside") {
    constexpr color::Argb red = 0xffff0000;
    constexpr color::Argb blue = 0xff0000ff;
    auto s = make();
    s.jump_to(100.0);
    thorvg::Canvas canvas(240, 360);
    canvas.clear(0xffffffff);
    s.begin_content(canvas);
    canvas.fill_rect({0.0, 0.0, 200.0, 1000.0}, red);
    canvas.push_clip({0.0, 150.0, 300.0, 20.0});
    canvas.fill_rect({0.0, 0.0, 300.0, 1000.0}, blue);
    canvas.pop_clip();
    s.end_content(canvas);
    CHECK(canvas.pixel(100, 10) == 0xffffffff);
    CHECK(canvas.pixel(100, 30) == red);
    CHECK(canvas.pixel(100, 75) == blue);
    CHECK(canvas.pixel(220, 75) == 0xffffffff);
    CHECK(canvas.pixel(100, 330) == 0xffffffff);
}

TEST_CASE("the bar is painted only while visible") {
    auto s = make();
    thorvg::Canvas canvas(240, 360);
    canvas.clear(0xffffffff);
    s.paint(canvas, scheme());
    CHECK(canvas.pixel(206, 30) == 0xffffffff);
    wheel(s, 0.0);
    s.handle(input::PointerEvent{PointerAction::move, 208.0, 30.0});
    settle(s);
    canvas.clear(0xffffffff);
    s.paint(canvas, scheme());
    CHECK(canvas.pixel(204, 30) != 0xffffffff);
}

TEST_CASE("a scroll area scrolls sideways with the horizontal wheel and with Shift and the wheel") {
    ScrollArea s = make(1000.0);
    s.set_content_width(800.0);
    CHECK(s.max_offset_x() == 600.0);
    s.handle(input::WheelEvent{100.0, 100.0, 1.0, 0.0});
    settle(s);
    CHECK(s.offset_x() == doctest::Approx(t.wheel_step));
    CHECK(s.offset() == 0.0);
    s.handle(input::WheelEvent{100.0, 100.0, 0.0, 2.0, true});
    settle(s);
    CHECK(s.offset_x() == doctest::Approx(3.0 * t.wheel_step));
    CHECK(s.offset() == 0.0);
    wheel(s, 1.0);
    settle(s);
    CHECK(s.offset() == doctest::Approx(t.wheel_step));
    s.handle(input::WheelEvent{100.0, 100.0, 50.0, 0.0});
    settle(s);
    CHECK(s.offset_x() == 600.0);
}

TEST_CASE("Shift and the wheel still scroll a scroll area that cannot scroll sideways") {
    ScrollArea s = make(1000.0);
    CHECK(s.handle(input::WheelEvent{100.0, 100.0, 0.0, 1.0, true}));
    settle(s);
    CHECK(s.offset() == doctest::Approx(t.wheel_step));
}

TEST_CASE("content coordinates follow both offsets") {
    ScrollArea s = make(1000.0);
    s.set_content_width(800.0);
    s.jump_to(100.0);
    s.jump_to_x(50.0);
    const input::PointerEvent in = s.to_content({PointerAction::move, 20.0, 30.0});
    CHECK(in.x_dp == 60.0);
    CHECK(in.y_dp == 110.0);
    const render::Rect r = s.to_window({60.0, 110.0, 10.0, 10.0});
    CHECK(r.x == 20.0);
    CHECK(r.y == 30.0);
    s.ensure_visible_x(700.0, 760.0);
    settle(s);
    CHECK(s.offset_x() == 560.0);
}

TEST_CASE("the horizontal bar runs along the bottom and drags the content sideways") {
    ScrollArea s = make(1000.0);
    s.set_content_width(800.0);
    s.scroll_by_x(1.0);
    settle(s);
    s.jump_to_x(0.0);
    const std::optional<render::Rect> thumb = s.thumb_x();
    REQUIRE(thumb.has_value());
    CHECK(thumb->y + thumb->height == doctest::Approx(20.0 + 300.0 - t.bar_margin));
    CHECK(thumb->width < 200.0);
    const std::optional<render::Rect> vertical = s.thumb();
    REQUIRE(vertical.has_value());
    CHECK(vertical->y + vertical->height <= 20.0 + 300.0 - t.bar_hit_width);
    const double y = 20.0 + 300.0 - t.bar_margin - 2.0;
    const double x = thumb->x + thumb->width / 2.0;
    CHECK(s.handle(input::PointerEvent{PointerAction::down, x, y}));
    CHECK(s.dragging());
    CHECK(s.to_content({PointerAction::move, x, y}).action == PointerAction::leave);
    s.handle(input::PointerEvent{PointerAction::move, x + 20.0, y});
    CHECK(s.offset_x() > 0.0);
    s.handle(input::PointerEvent{PointerAction::up, x + 20.0, y});
    CHECK_FALSE(s.dragging());
}
