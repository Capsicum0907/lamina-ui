#include <doctest/doctest.h>

#include <array>

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
