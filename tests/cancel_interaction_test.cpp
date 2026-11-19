#include <doctest/doctest.h>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;

namespace {

render::Point center(const render::Rect& r) {
    return {r.x + r.width / 2.0, r.y + r.height / 2.0};
}

template <typename W>
auto press_cancel_release(W& w, const render::Rect& target) {
    const render::Point p = center(target);
    w.handle({PointerAction::down, p.x, p.y});
    w.cancel_interaction();
    return w.handle({PointerAction::up, p.x, p.y});
}

template <typename W>
void check_cancelled_click(W& w) {
    CHECK_FALSE(press_cancel_release(w, w.bounds()));
    settle(w);
    CHECK_FALSE(w.animating());
}

std::vector<widgets::NavigationItem> items() {
    std::vector<widgets::NavigationItem> list;
    for (auto [symbol, label] : {std::pair{icons::Symbol::home, "Home"}, std::pair{icons::Symbol::search, "Search"}}) {
        list.push_back({icons::Icon{icon_font(), symbol}, label});
    }
    return list;
}

}

TEST_CASE("cancel_interaction keeps a press on a checkbox, radio button, switch or FAB from clicking") {
    widgets::Checkbox checkbox;
    check_cancelled_click(checkbox);
    CHECK(checkbox.state() == tokens::CheckboxState::off);
    widgets::RadioButton radio;
    check_cancelled_click(radio);
    CHECK_FALSE(radio.selected());
    widgets::Switch toggle;
    check_cancelled_click(toggle);
    CHECK_FALSE(toggle.checked());
    widgets::Fab fab({icons::Icon{icon_font(), icons::Symbol::edit}});
    check_cancelled_click(fab);
}

TEST_CASE("cancel_interaction ends a slider drag and restores the handle width at once") {
    widgets::Slider s({0.5});
    s.set_width(300.0);
    const render::Rect b = s.bounds();
    s.handle({PointerAction::down, b.x + 30.0, center(b).y});
    REQUIRE(s.dragging());
    const double value = s.value();
    s.cancel_interaction();
    CHECK_FALSE(s.dragging());
    CHECK(s.handle_width() == tokens::slider_tokens.handle_width);
    CHECK_FALSE(s.handle({PointerAction::up, b.x + 250.0, center(b).y}));
    CHECK(s.value() == value);
}

TEST_CASE("cancel_interaction keeps a press on another tab or destination from selecting it") {
    widgets::TabRow tabs({{{"One"}, {"Two"}}}, fonts());
    tabs.set_width(400.0);
    CHECK_FALSE(press_cancel_release(tabs, tabs.tab_rect(1)));
    CHECK(tabs.selected() == 0);

    widgets::NavigationBar bar({items()}, fonts());
    bar.set_width(400.0);
    CHECK_FALSE(press_cancel_release(bar, bar.item_rect(1)));
    CHECK(bar.selected() == 0);

    widgets::NavigationRail rail({items()}, fonts());
    rail.measure({0.0, 800.0});
    rail.arrange({0.0, 0.0, rail.width(), 600.0});
    CHECK_FALSE(press_cancel_release(rail, rail.item_rect(1)));
    CHECK(rail.selected() == 0);
    settle(rail);
    CHECK_FALSE(rail.animating());
}

TEST_CASE("cancel_interaction ends a text field's drag selection but keeps its focus") {
    widgets::TextField f({.label = "Name"}, fonts());
    f.set_text("hello world");
    const render::Rect b = f.bounds();
    f.handle({PointerAction::down, b.x + 20.0, center(b).y, 1});
    REQUIRE(f.focused());
    const std::size_t cursor = f.cursor();
    f.cancel_interaction();
    f.handle({PointerAction::move, b.x + b.width - 20.0, center(b).y});
    CHECK(f.cursor() == cursor);
    CHECK(f.anchor() == cursor);
    CHECK(f.focused());
}

TEST_CASE("cancel_interaction keeps a press on a dialog button from closing it") {
    widgets::Dialog d({"Title", "Text", "OK", "Cancel"}, fonts());
    d.open({800.0, 600.0});
    settle(d);
    CHECK(press_cancel_release(d, d.confirm_bounds()) == widgets::DialogResult::none);
    CHECK(d.is_open());
}

TEST_CASE("a widget set passes cancel_interaction to the members that have it") {
    widgets::Label label({"Label"}, fonts());
    widgets::Button button({"OK"}, fonts());
    widgets::WidgetSet set;
    set.add(label);
    set.add(button);
    const render::Point p = center(button.bounds());
    button.handle({PointerAction::down, p.x, p.y});
    set.cancel_interaction();
    CHECK_FALSE(button.handle({PointerAction::up, p.x, p.y}));
}
