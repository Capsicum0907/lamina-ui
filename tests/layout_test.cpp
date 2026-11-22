#include <doctest/doctest.h>

#include "lamina/layout/layout.hpp"

using namespace lamina;
using namespace lamina::layout;

namespace {

struct Box {
    render::Rect rect{};
    NodePtr node(Size s) {
        return fixed(s, [this](const render::Rect& r) { rect = r; });
    }
};

}

TEST_CASE("a row places children left to right with spacing and padding") {
    Box a, b;
    auto r = row({a.node({30, 10}), b.node({20, 40})});
    r->spacing(5).padding(Insets::all(2));
    run(*r, {0, 0, 500, 500});
    CHECK(r->measured().width == 30 + 5 + 20 + 4);
    CHECK(r->measured().height == 40 + 4);
    CHECK(a.rect.x == 2);
    CHECK(b.rect.x == 2 + 30 + 5);
    CHECK(a.rect.y == 2);
}

TEST_CASE("weights share the space left over") {
    Box a, b, c;
    auto r = row({a.node({100, 10}), weighted(b.node({0, 10}), 1.0), weighted(c.node({0, 10}), 3.0)});
    run(*r, {0, 0, 500, 100});
    CHECK(r->measured().width == 500);
    CHECK(b.rect.width == doctest::Approx(100));
    CHECK(c.rect.width == doctest::Approx(300));
    CHECK(c.rect.x == doctest::Approx(200));
}

TEST_CASE("main axis alignment distributes free space") {
    Box a, b;
    auto r = row({a.node({10, 10}), b.node({10, 10})});
    r->fill_main(true).main_align(MainAlign::space_between);
    run(*r, {0, 0, 100, 10});
    CHECK(a.rect.x == 0);
    CHECK(b.rect.x == 90);

    r->main_align(MainAlign::center);
    run(*r, {0, 0, 100, 10});
    CHECK(a.rect.x == 40);

    r->main_align(MainAlign::end);
    run(*r, {0, 0, 100, 10});
    CHECK(b.rect.x == 90);
}

TEST_CASE("cross alignment centers or stretches") {
    Box a, b;
    auto c = column({a.node({10, 10}), b.node({30, 10})});
    c->cross_align(CrossAlign::center);
    run(*c, {0, 0, 100, 100});
    CHECK(a.rect.x == 10);
    c->cross_align(CrossAlign::stretch);
    run(*c, {0, 0, 100, 100});
    CHECK(a.rect.width == 30);
    CHECK(b.rect.width == 30);
    c->fill_cross(true);
    run(*c, {0, 0, 100, 100});
    CHECK(a.rect.width == 100);
    CHECK(b.rect.width == 100);
}

TEST_CASE("a stretched child is measured with the full cross size") {
    double seen_width = 0.0;
    auto leaf = std::make_shared<Leaf>(
        [&seen_width](const Constraints& c) {
            seen_width = c.max_width;
            return Size{c.max_width, 1000.0 / c.max_width};
        },
        [](const render::Rect&) {});
    auto c = column({leaf});
    c->cross_align(CrossAlign::stretch).fill_cross(true).padding(Insets::symmetric(10, 0));
    run(*c, {0, 0, 120, 500});
    CHECK(seen_width == 100);
    CHECK(c->measured().height == doctest::Approx(10));
}

TEST_CASE("a stretched row is as tall as its tallest child, not as tall as the space") {
    Box a, b;
    auto r = row({a.node({10, 20}), b.node({10, 50})});
    r->cross_align(CrossAlign::stretch);
    run(*r, {0, 0, 300, 1000});
    CHECK(r->measured().height == 50);
    CHECK(a.rect.height == 50);
}

TEST_CASE("a flow wraps children that do not fit onto further lines") {
    Box a, b, c;
    auto f = flow({a.node({60, 10}), b.node({50, 20}), c.node({70, 10})});
    f->spacing(5).line_spacing(3);
    run(*f, {0, 0, 120, 500});
    CHECK(f->line_count() == 2);
    CHECK(f->measured().width == 115);
    CHECK(f->measured().height == 20 + 3 + 10);
    CHECK(a.rect.x == 0);
    CHECK(b.rect.x == 65);
    CHECK(c.rect.x == 0);
    CHECK(c.rect.y == 23);
}

TEST_CASE("an end-to-start flow places the first child at the end of each line") {
    Box a, b, c;
    auto f = flow({a.node({60, 10}), b.node({50, 20}), c.node({70, 10})});
    f->spacing(5).direction(FlowDirection::end_to_start);
    run(*f, {0, 0, 120, 500});
    CHECK(a.rect.x + a.rect.width == 115);
    CHECK(b.rect.x + b.rect.width + 5 == a.rect.x);
    CHECK(c.rect.x + c.rect.width == 115);
    run(*f, {0, 0, 500, 500});
    CHECK(f->line_count() == 1);
    CHECK(c.rect.x + c.rect.width + 5 == b.rect.x);
}

TEST_CASE("a flow aligns each child within the height of its own line") {
    Box a, b, c, d;
    auto f = flow({a.node({60, 10}), b.node({50, 30}), c.node({70, 20}), d.node({40, 8})});
    f->spacing(5).line_spacing(4);
    run(*f, {0, 0, 120, 500});
    CHECK(a.rect.y == 0);
    CHECK(d.rect.y == 34);

    f->cross_align(CrossAlign::center);
    run(*f, {0, 0, 120, 500});
    CHECK(a.rect.y == 10);
    CHECK(b.rect.y == 0);
    CHECK(c.rect.y == 34);
    CHECK(d.rect.y == 34 + 6);

    f->cross_align(CrossAlign::end);
    run(*f, {0, 0, 120, 500});
    CHECK(a.rect.y + a.rect.height == 30);
    CHECK(d.rect.y + d.rect.height == 34 + 20);

    f->cross_align(CrossAlign::stretch);
    run(*f, {0, 0, 120, 500});
    CHECK(a.rect.height == 30);
    CHECK(d.rect.height == 20);
    CHECK(f->measured().height == 30 + 4 + 20);
}

TEST_CASE("a child's own alignment overrides the flow's") {
    Box a, b, c;
    NodePtr top = a.node({20, 10});
    top->self_align = CrossAlign::start;
    auto f = flow({top, b.node({20, 30}), c.node({20, 10})});
    f->cross_align(CrossAlign::center);
    run(*f, {0, 0, 100, 100});
    CHECK(a.rect.y == 0);
    CHECK(c.rect.y == 10);
}

TEST_CASE("a flow reports where each of its lines starts") {
    Box a, b, c, d;
    auto f = flow({a.node({60, 10}), b.node({50, 10}), c.node({70, 10}), d.node({40, 10})});
    f->spacing(5);
    run(*f, {0, 0, 120, 500});
    REQUIRE(f->line_count() == 2);
    CHECK(f->line_start(0) == 0);
    CHECK(f->line_start(1) == 2);
    run(*f, {0, 0, 80, 500});
    REQUIRE(f->line_count() == 4);
    CHECK(f->line_start(1) == 1);
    CHECK(f->line_start(3) == 3);
    run(*f, {0, 0, 400, 500});
    CHECK(f->line_count() == 1);
}
