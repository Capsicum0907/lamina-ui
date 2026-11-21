#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <vector>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/content_host.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/pattern_strings.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;
using widgets::FocusDirection;

namespace {

struct Boxes {
    widgets::WidgetSet widgets;
    widgets::FocusManager focus;
    std::vector<widgets::Checkbox*> boxes;
    layout::NodePtr node;

    explicit Boxes(int count, double gap = 8.0) {
        std::vector<layout::NodePtr> nodes;
        for (int i = 0; i < count; ++i) {
            auto& box = widgets.emplace<widgets::Checkbox>();
            focus.add(box);
            boxes.push_back(&box);
            nodes.push_back(widgets::node(box));
        }
        auto c = layout::column(std::move(nodes));
        c->spacing(gap);
        node = c;
    }

    widgets::ContentSlot slot() { return {node, &widgets, &focus}; }
};

}

TEST_CASE("chained focus moves through each part in order and wraps when asked") {
    Boxes a(2);
    Boxes b(1);
    const std::vector<widgets::FocusManager*> parts{&a.focus, &b.focus};
    CHECK(widgets::chain_focus(parts, FocusDirection::next, true));
    CHECK(a.boxes[0]->focused());
    widgets::chain_focus(parts, FocusDirection::next, true);
    CHECK(a.boxes[1]->focused());
    widgets::chain_focus(parts, FocusDirection::next, true);
    CHECK(b.boxes[0]->focused());
    CHECK_FALSE(a.boxes[1]->focused());
    widgets::chain_focus(parts, FocusDirection::next, true);
    CHECK(a.boxes[0]->focused());
    CHECK_FALSE(b.boxes[0]->focused());
    widgets::chain_focus(parts, FocusDirection::previous, true);
    CHECK(b.boxes[0]->focused());
}

TEST_CASE("chained focus reports leaving the last part when it does not wrap") {
    Boxes a(1);
    Boxes b(1);
    const std::vector<widgets::FocusManager*> parts{&a.focus, &b.focus};
    widgets::chain_focus(parts, FocusDirection::previous, false);
    CHECK(b.boxes[0]->focused());
    CHECK_FALSE(widgets::chain_focus(parts, FocusDirection::next, false));
}

TEST_CASE("hosted content scrolls when it is taller than its viewport and gets pointer events in its own coordinates") {
    Boxes c(20);
    widgets::ContentHost host(c.slot());
    const double h = host.measure(200.0);
    CHECK(h > 300.0);
    host.place({50.0, 100.0, 200.0, 300.0});
    CHECK(host.scrolls());
    const render::Rect first = c.boxes[0]->bounds();
    CHECK(first.y == doctest::Approx(0.0));
    const double x = 50.0 + first.x + first.width / 2.0;
    const double y = 100.0 + first.y + first.height / 2.0;
    host.handle(input::PointerEvent{PointerAction::down, x, y});
    host.handle(input::PointerEvent{PointerAction::up, x, y});
    CHECK(c.boxes[0]->state() == tokens::CheckboxState::on);
    std::vector<widgets::AccessibleNode> nodes;
    host.collect(nodes);
    REQUIRE(nodes.size() == 20);
    CHECK(nodes[0].bounds.y == doctest::Approx(100.0 + first.y));
}

TEST_CASE("hosted content keeps the focused widget in view") {
    Boxes c(20);
    widgets::ContentHost host(c.slot());
    host.place({0.0, 0.0, 200.0, 200.0});
    for (int i = 0; i < 19; ++i) {
        c.focus.advance(FocusDirection::next);
    }
    host.reveal_focus();
    settle(host);
    CHECK(host.scroll().offset() > 0.0);
    CHECK(c.boxes[18]->focused());
}

TEST_CASE("the pattern strings follow Compose in English and Japanese") {
    CHECK(widgets::english_us_patterns().bottom_sheet_pane_title == "Bottom Sheet");
    CHECK(widgets::english_gb_patterns().bottom_sheet_pane_title == "Bottom sheet");
    CHECK(widgets::japanese_patterns().close_drawer == "ナビゲーションメニューを閉じる");
    CHECK(widgets::japanese_patterns().search == "検索");
}
