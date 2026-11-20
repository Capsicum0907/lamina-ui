#include <doctest/doctest.h>

#include <string>

#include "lamina/layout/layout.hpp"
#include "lamina/motion/easing.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/widgets/divider.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using widgets::Divider;
using widgets::DividerInset;
using widgets::DividerOrientation;

TEST_CASE("divider tokens follow DividerTokens and the list divider spaces") {
    const auto& t = tokens::divider_tokens;
    CHECK(t.thickness == 1.0);
    CHECK(t.color == color::Role::outline_variant);
    CHECK(t.inset_start == 16.0);
    CHECK(t.inset_end == 16.0);
}

TEST_CASE("a horizontal divider fills the width it is given and is 1 dp tall") {
    Divider d;
    const layout::Size s = d.measure({0.0, 300.0, 0.0, layout::unbounded});
    CHECK(s.width == 300.0);
    CHECK(s.height == 1.0);
    d.arrange({10.0, 20.0, 300.0, 1.0});
    CHECK(d.line().x == 10.0);
    CHECK(d.line().width == 300.0);
}

TEST_CASE("a vertical divider fills the height it is given and is 1 dp wide") {
    Divider d({DividerOrientation::vertical});
    const layout::Size s = d.measure({0.0, layout::unbounded, 0.0, 48.0});
    CHECK(s.width == 1.0);
    CHECK(s.height == 48.0);
    d.arrange({5.0, 0.0, 1.0, 48.0});
    CHECK(d.line().height == 48.0);
}

TEST_CASE("an inset divider leaves 16 dp at the start, the end or both") {
    Divider start({DividerOrientation::horizontal, DividerInset::start});
    start.arrange({0.0, 0.0, 200.0, 1.0});
    CHECK(start.line().x == 16.0);
    CHECK(start.line().width == 184.0);
    Divider both({DividerOrientation::horizontal, DividerInset::both});
    both.arrange({0.0, 0.0, 200.0, 1.0});
    CHECK(both.line().x == 16.0);
    CHECK(both.line().width == 168.0);
    Divider end({DividerOrientation::vertical, DividerInset::end});
    end.arrange({0.0, 0.0, 1.0, 100.0});
    CHECK(end.line().y == 0.0);
    CHECK(end.line().height == 84.0);
}

TEST_CASE("a divider paints outline-variant and reports itself as a separator") {
    Divider d({DividerOrientation::horizontal, DividerInset::start});
    auto node = widgets::measured_node(d);
    auto col = layout::column({node});
    col->fill_cross(true);
    layout::run(*col, {0.0, 0.0, 64.0, 8.0});
    thorvg::Canvas canvas(64, 8);
    canvas.clear(0xffffffff);
    d.paint(canvas, scheme());
    CHECK(canvas.pixel(40, 0) == scheme()[color::Role::outline_variant]);
    CHECK(canvas.pixel(8, 0) == 0xffffffff);
    CHECK(d.accessible().role == widgets::AccessibleRole::separator);
    CHECK(std::string(widgets::role_name(widgets::AccessibleRole::separator)) == "separator");
    CHECK(std::string(widgets::role_name(widgets::AccessibleRole::heading)) == "heading");
}

TEST_CASE("cubic bezier easing and tweens follow the curve and end on the target") {
    const motion::CubicBezier linear{0.0, 0.0, 1.0, 1.0};
    CHECK(motion::ease(linear, 0.3) == doctest::Approx(0.3).epsilon(1e-5));
    const motion::CubicBezier fast_out_slow_in{0.4, 0.0, 0.2, 1.0};
    CHECK(motion::ease(fast_out_slow_in, 0.5) == doctest::Approx(0.7749).epsilon(1e-3));
    CHECK(motion::ease(fast_out_slow_in, 0.0) == 0.0);
    CHECK(motion::ease(fast_out_slow_in, 1.0) == 1.0);
    motion::Tweened v(0.0);
    v.animate_to(10.0, {0.12, fast_out_slow_in});
    CHECK_FALSE(v.settled());
    v.step(0.06);
    CHECK(v.value() == doctest::Approx(7.749).epsilon(1e-3));
    v.step(0.1);
    CHECK(v.settled());
    CHECK(v.value() == 10.0);
}
