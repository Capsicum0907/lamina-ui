#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstdlib>

#include "lamina/input/caption.hpp"
#include "lamina/tokens/title_bar.hpp"
#include "lamina/widgets/title_bar.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::CaptionHit;
using input::PointerAction;
using widgets::TitleBarAction;

namespace {

widgets::TitleBar make_bar(double width) {
    widgets::TitleBar bar({"Title", icon_font()}, fonts());
    bar.set_width(width);
    bar.set_origin({0.0, 0.0});
    return bar;
}

render::Point center(const render::Rect& r) {
    return {r.x + r.width / 2.0, r.y + r.height / 2.0};
}

constexpr double ink_tolerance = 1.0;

int channel_distance(color::Argb a, color::Argb b) {
    int d = 0;
    for (int shift : {0, 8, 16}) {
        d = std::max(d, std::abs(static_cast<int>((a >> shift) & 0xff) - static_cast<int>((b >> shift) & 0xff)));
    }
    return d;
}

render::Rect ink_box(render::Canvas& canvas, const render::Rect& cell, color::Argb background, color::Argb ink) {
    const int half_coverage = channel_distance(ink, background) / 2;
    int left = static_cast<int>(cell.x + cell.width);
    int top = static_cast<int>(cell.y + cell.height);
    int right = static_cast<int>(cell.x);
    int bottom = static_cast<int>(cell.y);
    for (int y = static_cast<int>(cell.y); y < static_cast<int>(cell.y + cell.height); ++y) {
        for (int x = static_cast<int>(cell.x); x < static_cast<int>(cell.x + cell.width); ++x) {
            if (channel_distance(canvas.pixel(x, y), background) > half_coverage) {
                left = std::min(left, x);
                top = std::min(top, y);
                right = std::max(right, x + 1);
                bottom = std::max(bottom, y + 1);
            }
        }
    }
    return {static_cast<double>(left), static_cast<double>(top), static_cast<double>(right - left), static_cast<double>(bottom - top)};
}

}

TEST_CASE("caption hit testing puts buttons and interactive rects above the drag area") {
    input::CaptionRegions r;
    r.drag = {0.0, 0.0, 400.0, 40.0};
    r.minimize = {268.0, 0.0, 44.0, 40.0};
    r.maximize = {312.0, 0.0, 44.0, 40.0};
    r.close = {356.0, 0.0, 44.0, 40.0};
    r.interactive = {{100.0, 4.0, 32.0, 32.0}};
    CHECK(input::hit_test(r, {10.0, 10.0}) == CaptionHit::drag);
    CHECK(input::hit_test(r, {110.0, 10.0}) == CaptionHit::client);
    CHECK(input::hit_test(r, {110.0, 2.0}) == CaptionHit::drag);
    CHECK(input::hit_test(r, {280.0, 20.0}) == CaptionHit::minimize);
    CHECK(input::hit_test(r, {320.0, 20.0}) == CaptionHit::maximize);
    CHECK(input::hit_test(r, {399.0, 39.0}) == CaptionHit::close);
    CHECK(input::hit_test(r, {10.0, 41.0}) == CaptionHit::client);
}

TEST_CASE("the title bar puts its three buttons at the end and reports them as regions") {
    const auto& t = tokens::title_bar_tokens;
    const widgets::TitleBar bar = make_bar(600.0);
    CHECK(bar.height() == t.height);
    const render::Rect close = bar.button_rect(TitleBarAction::close);
    CHECK(close.x + close.width == doctest::Approx(600.0 - t.trailing_space));
    CHECK(bar.button_rect(TitleBarAction::toggle_maximize).x == doctest::Approx(close.x - t.button_width));
    CHECK(bar.button_rect(TitleBarAction::minimize).x == doctest::Approx(close.x - 2.0 * t.button_width));

    const std::array interactive{render::Rect{200.0, 4.0, 32.0, 32.0}};
    const input::CaptionRegions r = bar.regions(interactive);
    CHECK(input::hit_test(r, center(bar.button_rect(TitleBarAction::toggle_maximize))) == CaptionHit::maximize);
    CHECK(input::hit_test(r, {210.0, 20.0}) == CaptionHit::client);
    CHECK(input::hit_test(r, {100.0, 20.0}) == CaptionHit::drag);
}

TEST_CASE("clicking a title bar button reports its action") {
    widgets::TitleBar bar = make_bar(600.0);
    for (TitleBarAction a : {TitleBarAction::minimize, TitleBarAction::toggle_maximize, TitleBarAction::close}) {
        const render::Point p = center(bar.button_rect(a));
        CHECK(bar.handle({PointerAction::down, p.x, p.y}) == TitleBarAction::none);
        CHECK(bar.handle({PointerAction::up, p.x, p.y}) == a);
        bar.handle({PointerAction::leave, 0.0, 0.0});
    }
    bar.handle({PointerAction::down, 100.0, 20.0});
    CHECK(bar.handle({PointerAction::up, 100.0, 20.0}) == TitleBarAction::none);
}

TEST_CASE("the title bar paints its container and the close button turns to the error color on hover") {
    const auto& t = tokens::title_bar_tokens;
    widgets::TitleBar bar = make_bar(300.0);
    const render::Point p = center(bar.button_rect(TitleBarAction::close));
    bar.handle({PointerAction::move, p.x, p.y});
    settle(bar);
    thorvg::Canvas canvas(300, static_cast<int>(t.height));
    canvas.clear(0);
    bar.paint(canvas, scheme());
    CHECK(count_color(canvas, scheme()[t.container]) > 0);
    CHECK(count_color(canvas, scheme()[t.close_container]) > 0);
}

TEST_CASE("the caption glyphs match the close glyph's ink box, restored and maximized") {
    const auto& t = tokens::title_bar_tokens;
    for (bool maximized : {false, true}) {
        CAPTURE(maximized);
        widgets::TitleBar bar = make_bar(300.0);
        bar.set_maximized(maximized);
        thorvg::Canvas canvas(300, static_cast<int>(t.height));
        canvas.clear(0);
        bar.paint(canvas, scheme());
        const color::Argb background = scheme()[t.container];
        const color::Argb ink = scheme()[t.icon];
        const render::Rect close = ink_box(canvas, bar.button_rect(TitleBarAction::close), background, ink);
        const render::Rect minimize = ink_box(canvas, bar.button_rect(TitleBarAction::minimize), background, ink);
        const render::Rect maximize = ink_box(canvas, bar.button_rect(TitleBarAction::toggle_maximize), background, ink);
        REQUIRE(close.width > 0.0);
        CHECK(std::abs(minimize.width - close.width) <= ink_tolerance);
        CHECK(minimize.height < close.height);
        CHECK(std::abs(maximize.width - close.width) <= ink_tolerance);
        CHECK(std::abs(maximize.height - close.height) <= ink_tolerance);
        for (TitleBarAction a : {TitleBarAction::minimize, TitleBarAction::toggle_maximize, TitleBarAction::close}) {
            CAPTURE(static_cast<int>(a));
            const render::Rect box = ink_box(canvas, bar.button_rect(a), background, ink);
            const render::Point c = center(bar.button_rect(a));
            CHECK(std::abs(box.x + box.width / 2.0 - c.x) <= ink_tolerance);
            CHECK(std::abs(box.y + box.height / 2.0 - c.y) <= ink_tolerance);
        }
    }
}
