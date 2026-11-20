#include <doctest/doctest.h>

#include <stdexcept>
#include <vector>

#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/title_bar.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using widgets::AccessibleRole;
using widgets::AccessibleToggle;

namespace {

icons::Icon icon(icons::Symbol s = res::icons::add) {
    return {icon_font(), s};
}

}

static_assert(widgets::Accessible<widgets::Button> && widgets::Accessible<widgets::IconButton> && widgets::Accessible<widgets::Fab>);
static_assert(widgets::Accessible<widgets::Checkbox> && widgets::Accessible<widgets::Switch> && widgets::Accessible<widgets::RadioButton>);
static_assert(widgets::Accessible<widgets::Slider> && widgets::Accessible<widgets::TabRow> && widgets::Accessible<widgets::TextField>);
static_assert(widgets::Accessible<widgets::Label> && widgets::Accessible<widgets::Menu> && widgets::Accessible<widgets::Dialog>);
static_assert(widgets::Accessible<widgets::NavigationBar> && widgets::Accessible<widgets::TitleBar>);

TEST_CASE("controls without visible text refuse to be built without an accessible name") {
    CHECK_THROWS_AS(widgets::IconButton({icon(), ""}), std::invalid_argument);
    CHECK_THROWS_AS(widgets::Fab({icon(), ""}), std::invalid_argument);
    CHECK_THROWS_AS(widgets::Button({"", tokens::ButtonStyle::filled, tokens::ButtonSize::small, tokens::ButtonShape::round, icon()}, fonts()),
                    std::invalid_argument);
    CHECK_THROWS_AS(widgets::TabRow({{{"", icon()}}}, fonts()), std::invalid_argument);
    CHECK_THROWS_AS(widgets::NavigationBar({{{icon(), ""}}}, fonts()), std::invalid_argument);
    widgets::IconButton named({icon(), "Add"});
    CHECK(named.accessible_name() == "Add");
    CHECK_THROWS_AS(named.set_accessible_name(""), std::invalid_argument);
    widgets::Button labelled({"Send"}, fonts());
    CHECK(labelled.accessible().name == "Send");
}

TEST_CASE("each control reports its role, name and state") {
    widgets::IconButton toggle({icon(), "Favorite", tokens::IconButtonStyle::filled, tokens::IconButtonSize::small, tokens::ButtonShape::round,
                                tokens::IconButtonWidth::uniform, true, true});
    const widgets::AccessibleNode t = toggle.accessible();
    CHECK(t.role == AccessibleRole::toggle_button);
    CHECK(t.state.toggle == AccessibleToggle::on);
    CHECK(t.state.focusable);

    widgets::Checkbox mixed({tokens::CheckboxState::indeterminate, true, "Select all"});
    CHECK(mixed.accessible().state.toggle == AccessibleToggle::mixed);
    CHECK(mixed.accessible().name == "Select all");

    widgets::Slider slider({.value = 0.25, .min = 0.0, .max = 2.0, .accessible_name = "Volume"});
    const widgets::AccessibleNode s = slider.accessible();
    CHECK(s.role == AccessibleRole::slider);
    REQUIRE(s.state.range);
    CHECK(s.state.range->value == 0.25);
    CHECK(s.state.range->max == 2.0);

    widgets::RadioButton radio({.selected = true, .enabled = false, .accessible_name = "Small"});
    CHECK(radio.accessible().state.selected == true);
    CHECK_FALSE(radio.accessible().state.enabled);
    CHECK_FALSE(radio.accessible().state.focusable);

    widgets::TextField field({.label = "Name", .placeholder = "Jane"}, fonts());
    field.set_text("Ada");
    const widgets::AccessibleNode f = field.accessible();
    CHECK(f.role == AccessibleRole::text_field);
    CHECK(f.name == "Name");
    CHECK(f.value == "Ada");
    CHECK(f.description == "Jane");
    widgets::TextField search({.placeholder = "Search"}, fonts());
    CHECK(search.accessible().name == "Search");
}

TEST_CASE("a widget set gathers the nodes of its members, and the focus follows the widgets") {
    widgets::WidgetSet set;
    auto& a = set.emplace<widgets::Button>(widgets::ButtonOptions{"Open"}, fonts());
    auto& b = set.emplace<widgets::Switch>(widgets::SwitchOptions{.checked = true, .accessible_name = "Wi-Fi"});
    set.emplace<widgets::Label>(widgets::LabelOptions{"Settings"}, fonts());
    a.set_origin({10.0, 20.0});
    b.set_focused(true);
    const std::vector<widgets::AccessibleNode> nodes = set.accessible_nodes();
    REQUIRE(nodes.size() == 3);
    CHECK(nodes[0].name == "Open");
    CHECK(nodes[0].bounds.x == 10.0);
    CHECK(nodes[1].role == AccessibleRole::switch_control);
    CHECK(nodes[2].role == AccessibleRole::label);
    const auto focused = widgets::focused_accessible(nodes);
    REQUIRE(focused);
    CHECK(focused->name == "Wi-Fi");
    CHECK(widgets::find_accessible(nodes, "Settings"));
    CHECK_FALSE(widgets::find_accessible(nodes, "Missing"));
    a.set_enabled(false);
    CHECK_FALSE(widgets::find_accessible(set.accessible_nodes(), "Open")->state.enabled);
}

TEST_CASE("item rows report each item as a child") {
    widgets::TabRow tabs({{{"One"}, {"Two", std::nullopt, false}, {"", icon(), true, "Search"}}, tokens::TabStyle::primary, 0,
                          widgets::TabIconPosition::top, "Sections"},
                         fonts());
    tabs.set_width(300.0);
    const widgets::AccessibleNode row = tabs.accessible();
    CHECK(row.role == AccessibleRole::tab_list);
    CHECK(row.name == "Sections");
    REQUIRE(row.children.size() == 3);
    CHECK(row.children[0].state.selected == true);
    CHECK_FALSE(row.children[1].state.enabled);
    CHECK(row.children[2].name == "Search");
    CHECK(row.children[2].bounds.x == doctest::Approx(200.0));
}

TEST_CASE("an open menu reports its items, a closed one reports nothing") {
    widgets::Menu menu({{{"Cut"}, {"Copy", std::nullopt, std::nullopt, false}}}, fonts());
    CHECK_FALSE(menu.accessible());
    menu.open({10.0, 10.0, 40.0, 40.0}, {600.0, 600.0}, input::InputSource::keyboard);
    const auto node = menu.accessible();
    REQUIRE(node);
    CHECK(node->role == AccessibleRole::menu);
    REQUIRE(node->children.size() == 2);
    CHECK(node->children[0].state.focused);
    CHECK_FALSE(node->children[1].state.enabled);
}

TEST_CASE("a dialog reports its headline, its content in window coordinates and its actions") {
    widgets::WidgetSet content;
    auto& box = content.emplace<widgets::Checkbox>(widgets::CheckboxOptions{.accessible_name = "Remember"});
    widgets::Dialog d({"Sign in", "Use your account.", "OK", "Cancel", std::nullopt, std::nullopt,
                       widgets::DialogContent{widgets::node(box), &content, nullptr}},
                      fonts());
    CHECK_FALSE(d.accessible());
    d.open({800.0, 600.0});
    const auto node = d.accessible();
    REQUIRE(node);
    CHECK(node->role == AccessibleRole::dialog);
    CHECK(node->name == "Sign in");
    CHECK(node->state.modal);
    const auto remember = widgets::find_accessible(node->children, "Remember");
    REQUIRE(remember);
    CHECK(remember->bounds.x >= d.bounds().x);
    CHECK(remember->bounds.y >= d.bounds().y);
    CHECK(widgets::find_accessible(node->children, "Use your account."));
    CHECK(widgets::find_accessible(node->children, "OK"));
    CHECK(widgets::find_accessible(node->children, "Cancel"));
}

TEST_CASE("the title bar names its caption buttons, following the maximized state") {
    widgets::TitleBar bar({"Notes", icon_font()}, fonts());
    bar.set_width(600.0);
    CHECK(bar.accessible().children[1].name == "Maximize");
    bar.set_maximized(true);
    CHECK(bar.accessible().children[1].name == "Restore");
}

TEST_CASE("describe writes the tree one node per line") {
    widgets::Checkbox c({tokens::CheckboxState::on, false, "Agree"});
    CHECK(widgets::describe(c.accessible()) == "checkbox \"Agree\" disabled on\n");
}
