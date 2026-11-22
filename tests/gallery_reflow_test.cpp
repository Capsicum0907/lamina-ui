#include <doctest/doctest.h>

#include <cmath>
#include <memory>
#include <vector>

#include "components/reflow.hpp"

using namespace lamina;
using namespace lamina::examples::components;

namespace {

struct Box {
    render::Rect rect{};
    layout::NodePtr node(layout::Size s) {
        return layout::fixed(s, [this](const render::Rect& r) { rect = r; });
    }
};

constexpr double frame_dt = 1.0 / 60.0;

layout::WindowSizeClass width_class(layout::WindowWidthClass c) {
    return layout::compute_window_size_class(layout::window_breakpoints.width[static_cast<std::size_t>(c)], 800.0);
}

void frame(Reflow& reflow, layout::Node& root, double width) {
    layout::run(root, {0.0, 0.0, width, 2000.0});
    reflow.step(frame_dt);
}

double distance(const render::Rect& a, const render::Rect& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

}

TEST_CASE("one column below expanded stacks every section, the right column's below the left's") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::medium);
    Reflow reflow(sc);
    Box a, b, c;
    auto columns = std::make_shared<Columns>(reflow, std::vector<std::vector<layout::NodePtr>>{{a.node({100, 40}), b.node({120, 30})}, {c.node({90, 50})}},
                                             48.0, 16.0);
    auto root = reflow.root(columns);
    frame(reflow, *root, 700.0);
    CHECK(a.rect.y == 0.0);
    CHECK(b.rect.y == 40.0 + 16.0);
    CHECK(c.rect.x == 0.0);
    CHECK(c.rect.y == 40.0 + 16.0 + 30.0 + 16.0);
    CHECK(root->measured().height == 40.0 + 16.0 + 30.0 + 16.0 + 50.0);
}

TEST_CASE("from expanded the columns sit side by side at their natural widths when they fit") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::expanded);
    Reflow reflow(sc);
    Box a, b, c;
    auto columns = std::make_shared<Columns>(reflow, std::vector<std::vector<layout::NodePtr>>{{a.node({100, 40}), b.node({120, 30})}, {c.node({90, 50})}},
                                             48.0, 16.0);
    auto root = reflow.root(columns);
    frame(reflow, *root, 900.0);
    CHECK(c.rect.x == 120.0 + 48.0);
    CHECK(c.rect.y == 0.0);
    CHECK(root->measured().width == 120.0 + 48.0 + 90.0);
    CHECK(root->measured().height == 40.0 + 16.0 + 30.0);
}

TEST_CASE("columns that do not fit give up width from the widest first") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::expanded);
    Reflow reflow(sc);
    std::vector<double> seen(3, 0.0);
    auto probe = [&seen](std::size_t i, double natural) {
        return std::make_shared<layout::Leaf>(
            [&seen, i, natural](const layout::Constraints& c) {
                seen[i] = std::min(natural, c.max_width);
                return layout::Size{seen[i], 10.0};
            },
            [](const render::Rect&) {});
    };
    auto columns = std::make_shared<Columns>(reflow, std::vector<std::vector<layout::NodePtr>>{{probe(0, 600)}, {probe(1, 300)}, {probe(2, 100)}}, 20.0, 0.0);
    auto root = reflow.root(columns);
    frame(reflow, *root, 740.0);
    CHECK(seen[2] == doctest::Approx(100.0));
    CHECK(seen[0] == doctest::Approx(300.0));
    CHECK(seen[1] == doctest::Approx(300.0));
    frame(reflow, *root, 440.0);
    CHECK(seen[0] == doctest::Approx(150.0));
    CHECK(seen[1] == doctest::Approx(150.0));
    CHECK(seen[2] == doctest::Approx(100.0));
}

TEST_CASE("a caption sits beside its content from medium and above it on a compact window") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::medium);
    Reflow reflow(sc);
    Box cap, content;
    auto root = reflow.root(std::make_shared<Captioned>(reflow, cap.node({120, 20}), content.node({200, 40}), 8.0));
    frame(reflow, *root, 700.0);
    CHECK(content.rect.x == 120.0);
    CHECK(cap.rect.y == 10.0);
    CHECK(root->measured().height == 40.0);
    reflow.snap();
    sc = width_class(layout::WindowWidthClass::compact);
    frame(reflow, *root, 400.0);
    CHECK(content.rect.x == 0.0);
    CHECK(content.rect.y == 20.0 + 8.0);
    CHECK(cap.rect.y == 0.0);
    CHECK(root->measured().height == 20.0 + 8.0 + 40.0);
}

TEST_CASE("a wrapping row measures the height of all its lines and keeps its gaps") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::compact);
    Reflow reflow(sc);
    Box a, b, c;
    auto f = reflow.flow({a.node({60, 20}), b.node({60, 40}), c.node({60, 20})});
    f->spacing(16).line_spacing(8).cross_align(layout::CrossAlign::center);
    auto root = reflow.root(f);
    frame(reflow, *root, 140.0);
    CHECK(f->line_count() == 2);
    CHECK(b.rect.x == 60.0 + 16.0);
    CHECK(a.rect.y == 10.0);
    CHECK(c.rect.y == 40.0 + 8.0);
    CHECK(root->measured().height == 40.0 + 8.0 + 20.0);
}

TEST_CASE("the first arrange places items at once") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::expanded);
    Reflow reflow(sc);
    Box a, b;
    auto columns = std::make_shared<Columns>(reflow, std::vector<std::vector<layout::NodePtr>>{{a.node({100, 40})}, {b.node({90, 50})}}, 48.0, 16.0);
    auto root = reflow.root(columns);
    layout::run(*root, {0.0, 0.0, 900.0, 2000.0});
    CHECK(b.rect.x == 148.0);
    CHECK_FALSE(reflow.animating());
}

TEST_CASE("crossing a breakpoint glides each section from where it was, with no jump in any frame") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::expanded);
    Reflow reflow(sc);
    Box a, b;
    auto columns = std::make_shared<Columns>(reflow, std::vector<std::vector<layout::NodePtr>>{{a.node({100, 40})}, {b.node({90, 50})}}, 48.0, 16.0);
    auto root = reflow.root(columns);
    frame(reflow, *root, 900.0);
    const render::Rect start = b.rect;
    const render::Rect goal{0.0, 40.0 + 16.0, 90.0, 50.0};
    sc = width_class(layout::WindowWidthClass::medium);
    render::Rect last = start;
    double largest = 0.0;
    int frames = 0;
    for (; frames < 600; ++frames) {
        frame(reflow, *root, 700.0);
        largest = std::max(largest, distance(last, b.rect));
        last = b.rect;
        if (!reflow.animating() && distance(b.rect, goal) < 1e-6) {
            break;
        }
    }
    const double total = distance(start, goal);
    CHECK(frames > 5);
    CHECK(largest < total / 4.0);
    CHECK(b.rect.x == doctest::Approx(goal.x));
    CHECK(b.rect.y == doctest::Approx(goal.y));
}

TEST_CASE("an item that moves to another line glides there") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::compact);
    Reflow reflow(sc);
    Box a, b, c;
    auto f = reflow.flow({a.node({60, 20}), b.node({60, 20}), c.node({60, 20})});
    f->spacing(10);
    auto root = reflow.root(f);
    frame(reflow, *root, 300.0);
    const render::Rect start = c.rect;
    frame(reflow, *root, 140.0);
    CHECK(distance(c.rect, start) < 20.0);
    for (int i = 0; i < 600 && reflow.animating(); ++i) {
        frame(reflow, *root, 140.0);
    }
    frame(reflow, *root, 140.0);
    CHECK(c.rect.x == doctest::Approx(0.0));
    CHECK(c.rect.y == doctest::Approx(20.0));
}

TEST_CASE("while the arrangement stays the same, items follow the window directly") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::expanded);
    Reflow reflow(sc);
    Box b;
    auto wide = std::make_shared<layout::Leaf>([](const layout::Constraints& c) { return layout::Size{std::min(600.0, c.max_width), 10.0}; },
                                               [](const render::Rect&) {});
    auto columns = std::make_shared<Columns>(reflow, std::vector<std::vector<layout::NodePtr>>{{wide}, {b.node({100, 10})}}, 20.0, 0.0);
    auto root = reflow.root(columns);
    frame(reflow, *root, 720.0);
    CHECK(b.rect.x == doctest::Approx(600.0 + 20.0));
    frame(reflow, *root, 600.0);
    CHECK(b.rect.x == doctest::Approx(480.0 + 20.0));
    CHECK_FALSE(reflow.animating());
}

TEST_CASE("a snap lands the next arrange without gliding") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::expanded);
    Reflow reflow(sc);
    Box a, b;
    auto columns = std::make_shared<Columns>(reflow, std::vector<std::vector<layout::NodePtr>>{{a.node({100, 40})}, {b.node({90, 50})}}, 48.0, 16.0);
    auto root = reflow.root(columns);
    frame(reflow, *root, 900.0);
    sc = width_class(layout::WindowWidthClass::compact);
    reflow.snap();
    frame(reflow, *root, 400.0);
    CHECK(b.rect.x == 0.0);
    CHECK(b.rect.y == 56.0);
    CHECK_FALSE(reflow.animating());
}

TEST_CASE("an item inside a gliding section moves with the section instead of trailing it") {
    layout::WindowSizeClass sc = width_class(layout::WindowWidthClass::expanded);
    Reflow reflow(sc);
    Box a, x, y;
    auto f = reflow.flow({x.node({30, 10}), y.node({30, 10})});
    f->spacing(10);
    auto columns = std::make_shared<Columns>(reflow, std::vector<std::vector<layout::NodePtr>>{{a.node({100, 40})}, {f}}, 48.0, 16.0);
    auto root = reflow.root(columns);
    frame(reflow, *root, 900.0);
    sc = width_class(layout::WindowWidthClass::medium);
    for (int i = 0; i < 20; ++i) {
        frame(reflow, *root, 700.0);
        CHECK(y.rect.x - x.rect.x == doctest::Approx(40.0));
        CHECK(y.rect.y == doctest::Approx(x.rect.y));
    }
}
