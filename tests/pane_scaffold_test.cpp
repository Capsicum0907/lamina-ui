#include <doctest/doctest.h>

#include <memory>
#include <string>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/pane_scaffold.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::BackNavigationBehavior;
using widgets::PaneAdaptedValue;
using widgets::PaneRole;
namespace ld = widgets::list_detail_pane;
namespace sp = widgets::supporting_pane;

static_assert(widgets::Measurable<widgets::ThreePaneScaffold> && widgets::Paintable<widgets::ThreePaneScaffold> &&
              widgets::Steppable<widgets::ThreePaneScaffold> && widgets::HandlesPointer<widgets::ThreePaneScaffold> &&
              widgets::HandlesKeys<widgets::ThreePaneScaffold> && widgets::FocusGated<widgets::ThreePaneScaffold> &&
              widgets::Disableable<widgets::ThreePaneScaffold>);

namespace {

constexpr const tokens::PaneScaffoldTokens& t = tokens::pane_scaffold_tokens;

widgets::WindowAdaptiveInfo info(double w, double h = 800.0) {
    return {layout::compute_window_size_class(w, h)};
}

struct PaneBody {
    widgets::WidgetSet widgets;
    widgets::FocusManager focus;
    widgets::Button* button = nullptr;
    layout::NodePtr node;

    explicit PaneBody(const std::string& label) {
        button = &widgets.emplace<widgets::Button>(widgets::ButtonOptions{.label = label}, fonts());
        focus.add(*button);
        auto c = layout::column({widgets::node(*button)});
        node = c;
    }
    widgets::ContentSlot slot() { return {node, &widgets, &focus}; }
};

struct Fixture {
    PaneBody list{"List"};
    PaneBody detail{"Detail"};
    widgets::ThreePaneScaffoldNavigator navigator;
    std::unique_ptr<widgets::ThreePaneScaffold> scaffold;

    explicit Fixture(double width, widgets::ThreePaneScaffoldOptions options = {})
        : navigator(widgets::list_detail_pane_navigator(widgets::calculate_pane_scaffold_directive(info(width)))) {
        options.panes[static_cast<std::size_t>(ld::list)] = list.slot();
        options.panes[static_cast<std::size_t>(ld::detail)] = detail.slot();
        scaffold = std::make_unique<widgets::ThreePaneScaffold>(std::move(options), navigator);
        arrange(width);
    }
    void arrange(double width) {
        scaffold->measure(layout::Constraints::tight({width, 600.0}));
        scaffold->arrange({0.0, 0.0, width, 600.0});
    }
};

}

TEST_CASE("pane scaffold directives follow the window width classes") {
    CHECK(widgets::calculate_pane_scaffold_directive(info(500.0)).max_horizontal_partitions == 1);
    CHECK(widgets::calculate_pane_scaffold_directive(info(700.0)).max_horizontal_partitions == 1);
    const widgets::PaneScaffoldDirective expanded = widgets::calculate_pane_scaffold_directive(info(1000.0));
    CHECK(expanded.max_horizontal_partitions == 2);
    CHECK(expanded.horizontal_partition_spacer == 24.0);
    CHECK(expanded.default_pane_preferred_width == 360.0);
    const widgets::PaneScaffoldDirective large = widgets::calculate_pane_scaffold_directive(info(1300.0));
    CHECK(large.max_horizontal_partitions == 3);
    CHECK(large.default_pane_preferred_width == 412.0);
    CHECK(widgets::calculate_pane_scaffold_directive(info(500.0, 950.0)).max_vertical_partitions == 2);
    CHECK(widgets::calculate_pane_scaffold_directive_with_two_panes_on_medium_width(info(700.0)).max_horizontal_partitions == 2);
}

TEST_CASE("the scaffold value gives the partitions to the latest destinations first") {
    const auto& s = widgets::list_detail_adapt_strategies;
    const widgets::PaneScaffoldValue one = widgets::calculate_pane_scaffold_value(1, s, {{ld::list}});
    CHECK(one[ld::list].kind == PaneAdaptedValue::Kind::expanded);
    CHECK(one[ld::detail].kind == PaneAdaptedValue::Kind::hidden);
    const widgets::PaneScaffoldValue detail = widgets::calculate_pane_scaffold_value(1, s, {{ld::list}, {ld::detail, 3}});
    CHECK(detail[ld::detail].kind == PaneAdaptedValue::Kind::expanded);
    CHECK(detail[ld::list].kind == PaneAdaptedValue::Kind::hidden);
    const widgets::PaneScaffoldValue two = widgets::calculate_pane_scaffold_value(2, s, {{ld::list}});
    CHECK(two[ld::list].kind == PaneAdaptedValue::Kind::expanded);
    CHECK(two[ld::detail].kind == PaneAdaptedValue::Kind::expanded);
    CHECK(two[ld::extra].kind == PaneAdaptedValue::Kind::hidden);
    const widgets::PaneScaffoldValue reflowed =
        widgets::calculate_pane_scaffold_value(1, widgets::supporting_pane_adapt_strategies, {{sp::main}}, 2);
    CHECK(reflowed[sp::main].kind == PaneAdaptedValue::Kind::expanded);
    CHECK(reflowed[sp::supporting].kind == PaneAdaptedValue::Kind::reflowed);
    CHECK(reflowed[sp::supporting].reflow_under == sp::main);
}

TEST_CASE("the navigator goes back by the chosen behavior") {
    widgets::ThreePaneScaffoldNavigator n = widgets::list_detail_pane_navigator(widgets::calculate_pane_scaffold_directive(info(500.0)));
    CHECK_FALSE(n.can_navigate_back());
    n.navigate_to(ld::detail, 1);
    n.navigate_to(ld::detail, 2);
    CHECK(n.value()[ld::detail].shown());
    CHECK(n.can_navigate_back());
    CHECK(n.peek_previous_value()[ld::list].shown());
    CHECK(n.can_navigate_back(BackNavigationBehavior::pop_latest));
    widgets::ThreePaneScaffoldNavigator copy = n;
    CHECK(copy.navigate_back(BackNavigationBehavior::pop_latest));
    CHECK(copy.current_destination()->content == 1);
    CHECK(copy.navigate_back(BackNavigationBehavior::pop_until_content_change));
    CHECK(copy.current_destination()->pane == ld::list);
    CHECK(n.navigate_back());
    CHECK(n.history().size() == 1);
    CHECK(n.value()[ld::list].shown());
    CHECK_FALSE(n.value()[ld::detail].shown());
    n.set_directive(widgets::calculate_pane_scaffold_directive(info(1000.0)));
    n.navigate_to(ld::detail, 4);
    CHECK_FALSE(n.can_navigate_back());
    CHECK(n.can_navigate_back(BackNavigationBehavior::pop_latest));
}

TEST_CASE("one pane on a compact window and two side by side from the expanded width") {
    Fixture compact(500.0);
    CHECK(compact.scaffold->pane_shown(ld::list));
    CHECK_FALSE(compact.scaffold->pane_shown(ld::detail));
    CHECK(compact.scaffold->pane_rect(ld::list).width == 500.0);
    Fixture wide(1000.0);
    CHECK(wide.scaffold->pane_shown(ld::list));
    CHECK(wide.scaffold->pane_shown(ld::detail));
    CHECK(wide.scaffold->pane_rect(ld::list).width == 360.0);
    CHECK(wide.scaffold->pane_rect(ld::detail).x == 384.0);
    CHECK(wide.scaffold->pane_rect(ld::detail).width == 1000.0 - 384.0);
    CHECK(wide.list.button->bounds().x == 0.0);
    CHECK(wide.detail.button->bounds().x == 384.0);
}

TEST_CASE("navigating to the detail on one pane slides it in from the right and Escape goes back") {
    Fixture f(500.0);
    f.navigator.navigate_to(ld::detail, 0);
    f.arrange(500.0);
    CHECK(f.scaffold->pane_shown(ld::detail));
    CHECK(f.scaffold->pane_rect(ld::detail).x == doctest::Approx(500.0));
    CHECK(f.scaffold->pane_visible(ld::list));
    for (int i = 0; i < 6; ++i) {
        f.scaffold->step(frame_seconds);
    }
    const double mid = f.scaffold->pane_rect(ld::detail).x;
    CHECK(mid < 500.0);
    CHECK(mid > 0.0);
    CHECK(f.scaffold->animating());
    settle(*f.scaffold);
    CHECK(f.scaffold->pane_rect(ld::detail).x == doctest::Approx(0.0).epsilon(0.01));
    CHECK_FALSE(f.scaffold->pane_visible(ld::list));
    CHECK(f.detail.button->bounds().x == doctest::Approx(0.0).epsilon(0.01));
    const widgets::PaneScaffoldResponse r = f.scaffold->handle(input::KeyEvent{Key::escape, false});
    CHECK(r.navigated_back);
    f.arrange(500.0);
    settle(*f.scaffold);
    CHECK(f.scaffold->pane_shown(ld::list));
    CHECK_FALSE(f.scaffold->pane_shown(ld::detail));
    CHECK_FALSE(f.scaffold->handle(input::KeyEvent{Key::escape, false}).consumed);
}

TEST_CASE("a hidden pane takes no pointer and leaves the focus and the accessibility tree") {
    Fixture f(500.0);
    widgets::FocusManager page;
    page.add_group(f.list.focus, [&] { return f.scaffold->pane_shown(ld::list); });
    page.add_group(f.detail.focus, [&] { return f.scaffold->pane_shown(ld::detail); });
    CHECK(page.handle(input::KeyEvent{Key::tab, false}));
    CHECK(f.list.button->focused());
    CHECK(page.handle(input::KeyEvent{Key::tab, false}));
    CHECK(f.list.button->focused());
    CHECK(f.scaffold->accessible_nodes().size() == 1);
    CHECK(f.scaffold->accessible_nodes()[0].name == widgets::english_us_patterns().secondary_pane);
    f.navigator.navigate_to(ld::detail, 0);
    f.arrange(500.0);
    CHECK_FALSE(f.list.button->focused());
    CHECK(f.detail.button->focused());
    settle(*f.scaffold);
    const render::Rect b = f.detail.button->bounds();
    CHECK(f.scaffold->handle(input::PointerEvent{PointerAction::down, b.x + 4.0, b.y + 4.0}).consumed == false);
    CHECK(f.detail.button->pressed());
    CHECK_FALSE(f.list.button->pressed());
}

TEST_CASE("the pane expansion handle drags the split and settles on the closest anchor") {
    widgets::ThreePaneScaffoldOptions o;
    o.expansion_handle = true;
    o.anchors = {widgets::PaneExpansionAnchor::proportion(0.0), widgets::PaneExpansionAnchor::proportion(0.5),
                 widgets::PaneExpansionAnchor::from_end(300.0)};
    Fixture f(1000.0, o);
    REQUIRE(f.scaffold->handle_rect().has_value());
    const render::Rect h = *f.scaffold->handle_rect();
    CHECK(h.x + h.width / 2.0 == doctest::Approx(372.0));
    CHECK(f.scaffold->can_focus());
    const double y = h.y + h.height / 2.0;
    f.scaffold->handle(input::PointerEvent{PointerAction::move, 372.0, y});
    REQUIRE(f.scaffold->cursor().has_value());
    CHECK(f.scaffold->handle(input::PointerEvent{PointerAction::down, 372.0, y, 1, input::PointerKind::mouse, 1.0}).consumed);
    for (int i = 1; i <= 10; ++i) {
        f.scaffold->handle(input::PointerEvent{PointerAction::move, 372.0 + 10.0 * i, y, 0, input::PointerKind::mouse, 1.0 + 0.05 * i});
        f.scaffold->step(0.05);
    }
    CHECK(f.scaffold->expansion().dragging());
    CHECK(f.scaffold->pane_rect(ld::list).width == doctest::Approx(472.0 - 12.0));
    f.scaffold->handle(input::PointerEvent{PointerAction::up, 472.0, y, 0, input::PointerKind::mouse, 1.55});
    settle(*f.scaffold);
    REQUIRE(f.scaffold->expansion().current_anchor().has_value());
    CHECK(*f.scaffold->expansion().current_anchor() == widgets::PaneExpansionAnchor::proportion(0.5));
    CHECK(f.scaffold->pane_rect(ld::list).width == doctest::Approx(500.0 - 12.0));
    CHECK(f.scaffold->pane_rect(ld::detail).x == doctest::Approx(512.0));
    const std::vector<widgets::AccessibleNode> nodes = f.scaffold->accessible_nodes();
    CHECK(nodes.back().role == widgets::AccessibleRole::separator);
    CHECK(nodes.back().value == "Current pane split, 50 percent");
    CHECK(nodes.back().description == "Change pane split to 300 DPs from end");
}

TEST_CASE("a fast fling past the handle settles on the next anchor in its direction") {
    widgets::ThreePaneScaffoldOptions o;
    o.expansion_handle = true;
    o.anchors = {widgets::PaneExpansionAnchor::proportion(0.3), widgets::PaneExpansionAnchor::proportion(0.7)};
    o.initial_anchor = 0;
    Fixture f(1000.0, o);
    settle(*f.scaffold);
    const render::Rect h = *f.scaffold->handle_rect();
    CHECK(h.x + h.width / 2.0 == doctest::Approx(300.0));
    const double y = h.y + h.height / 2.0;
    f.scaffold->handle(input::PointerEvent{PointerAction::down, 300.0, y, 1, input::PointerKind::mouse, 1.0});
    f.scaffold->handle(input::PointerEvent{PointerAction::move, 330.0, y, 0, input::PointerKind::mouse, 1.02});
    f.scaffold->handle(input::PointerEvent{PointerAction::move, 360.0, y, 0, input::PointerKind::mouse, 1.04});
    f.scaffold->handle(input::PointerEvent{PointerAction::up, 360.0, y, 0, input::PointerKind::mouse, 1.04});
    settle(*f.scaffold);
    CHECK(*f.scaffold->expansion().current_anchor() == widgets::PaneExpansionAnchor::proportion(0.7));
}

TEST_CASE("the focused handle moves with the arrow keys and Enter goes to the next anchor") {
    widgets::ThreePaneScaffoldOptions o;
    o.expansion_handle = true;
    o.anchors = {widgets::PaneExpansionAnchor::proportion(0.25), widgets::PaneExpansionAnchor::proportion(0.75)};
    Fixture f(1000.0, o);
    f.scaffold->set_focused(true);
    CHECK(f.scaffold->focused());
    CHECK(f.scaffold->handle(input::KeyEvent{Key::right, false}).consumed);
    f.arrange(1000.0);
    CHECK(f.scaffold->pane_rect(ld::list).width == doctest::Approx(360.0 + t.keyboard_step));
    CHECK(f.scaffold->handle(input::KeyEvent{Key::enter, false}).consumed);
    settle(*f.scaffold);
    CHECK(*f.scaffold->expansion().current_anchor() == widgets::PaneExpansionAnchor::proportion(0.75));
    CHECK(f.scaffold->handle(input::KeyEvent{Key::enter, false}).consumed);
    settle(*f.scaffold);
    CHECK(*f.scaffold->expansion().current_anchor() == widgets::PaneExpansionAnchor::proportion(0.25));
}

TEST_CASE("pane names and handle strings are translated") {
    CHECK(widgets::japanese_patterns().pane_expansion_drag_handle == "ペインの展開のドラッグ ハンドル");
    CHECK(widgets::format_pattern(widgets::japanese_patterns().pane_split_action, "50 パーセント") == "ペインの分割を 50 パーセント に変更");
    CHECK(widgets::PaneExpansionAnchor::from_start(240.0).description(widgets::japanese_patterns()) == "開始地点から 240 DP");
}

TEST_CASE("a supporting pane scaffold puts the main pane first and reflows the supporting pane under it on a tall compact window") {
    PaneBody main_body{"Main"};
    PaneBody supporting{"Supporting"};
    widgets::ThreePaneScaffoldNavigator n = widgets::supporting_pane_navigator(widgets::calculate_pane_scaffold_directive(info(500.0, 950.0)));
    widgets::ThreePaneScaffoldOptions o{.order = widgets::supporting_pane_order};
    o.panes[static_cast<std::size_t>(sp::main)] = main_body.slot();
    o.panes[static_cast<std::size_t>(sp::supporting)] = supporting.slot();
    widgets::ThreePaneScaffold s(std::move(o), n);
    s.measure(layout::Constraints::tight({500.0, 950.0}));
    s.arrange({0.0, 0.0, 500.0, 950.0});
    CHECK(s.pane_shown(sp::supporting));
    CHECK(s.pane_rect(sp::supporting).y == doctest::Approx(950.0 - 420.0));
    CHECK(s.pane_rect(sp::main).height == doctest::Approx(950.0 - 420.0 - 24.0));
    n.set_directive(widgets::calculate_pane_scaffold_directive(info(1000.0, 800.0)));
    s.measure(layout::Constraints::tight({1000.0, 800.0}));
    s.arrange({0.0, 0.0, 1000.0, 800.0});
    CHECK(s.pane_rect(sp::main).x == 0.0);
    CHECK(s.pane_rect(sp::main).width == doctest::Approx(1000.0 - 360.0 - 24.0));
    CHECK(s.pane_rect(sp::supporting).x == doctest::Approx(1000.0 - 360.0));
}
