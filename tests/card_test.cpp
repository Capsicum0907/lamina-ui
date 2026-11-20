#include <doctest/doctest.h>

#include "lamina/layout/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/card.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/list_item.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using tokens::CardStyle;
using widgets::Card;
using widgets::CardOptions;

namespace {

layout::NodePtr box(double w, double h) {
    return layout::spacer({w, h});
}

void place(Card& c, double w = 200.0, double h = 100.0) {
    c.measure(layout::Constraints::tight({w, h}));
    c.arrange({0.0, 0.0, w, h});
}

color::Argb composite(color::Role role, double opacity, color::Role over) {
    return color::source_over(color::scale_alpha(scheme()[role], opacity), scheme()[over]);
}

}

TEST_CASE("card tokens follow the card token files and Card.kt") {
    CHECK(tokens::card_tokens.corner == tokens::CornerSize::medium);
    CHECK(tokens::card_tokens.outline_width == 1.0);
    const auto& elevated = tokens::card_style(CardStyle::elevated);
    CHECK(elevated.container == color::Role::surface_container_low);
    CHECK(elevated.elevation.resting == 1.0);
    CHECK(elevated.elevation.hovered == 3.0);
    CHECK(elevated.elevation.disabled == 1.0);
    const auto& filled = tokens::card_style(CardStyle::filled);
    CHECK(filled.container == color::Role::surface_container_highest);
    CHECK(filled.disabled_container.role == color::Role::surface_variant);
    CHECK(filled.disabled_container.over == color::Role::surface_container_highest);
    CHECK(filled.elevation.hovered == 1.0);
    const auto& outlined = tokens::card_style(CardStyle::outlined);
    CHECK(outlined.outline == color::Role::outline_variant);
    CHECK(outlined.disabled_outline->over == color::Role::surface_container_low);
    CHECK(outlined.elevation.hovered == 0.0);
}

TEST_CASE("a card takes the size of its content and paints its container with 12 dp corners") {
    Card card({CardStyle::filled, false, true, box(120.0, 60.0)});
    const layout::Size s = card.measure({0.0, 400.0, 0.0, 400.0});
    CHECK(s.width == 120.0);
    CHECK(s.height == 60.0);
    card.arrange({0.0, 0.0, 120.0, 60.0});
    CHECK(card.radius() == 12.0);
    thorvg::Canvas canvas(120, 60);
    canvas.clear(0xffffffff);
    card.paint(canvas, scheme());
    CHECK(canvas.pixel(60, 30) == scheme()[color::Role::surface_container_highest]);
    CHECK(canvas.pixel(0, 0) == 0xffffffff);
}

TEST_CASE("an outlined card draws a 1 dp outline-variant border, and its disabled border over surface-container-low") {
    Card card({CardStyle::outlined});
    place(card);
    thorvg::Canvas canvas(200, 100);
    canvas.clear(0xffffffff);
    card.paint(canvas, scheme());
    CHECK(canvas.pixel(100, 0) == scheme()[color::Role::outline_variant]);
    CHECK(canvas.pixel(100, 50) == scheme()[color::Role::surface]);
    card.set_enabled(false);
    settle(card);
    canvas.clear(0xffffffff);
    card.paint(canvas, scheme());
    CHECK(canvas.pixel(100, 0) == composite(color::Role::outline, 0.12, color::Role::surface_container_low));
}

TEST_CASE("a non-clickable card takes no clicks or focus and is a group") {
    Card card({CardStyle::elevated});
    place(card);
    CHECK_FALSE(click(card, 50.0, 50.0));
    CHECK_FALSE(card.can_focus());
    CHECK(card.accessible().role == widgets::AccessibleRole::group);
    card.handle({input::PointerAction::move, 50.0, 50.0});
    settle(card);
    CHECK(card.elevation() == 1.0);
}

TEST_CASE("a clickable card needs a name, clicks, raises on hover and takes Space and Enter while focused") {
    CHECK_THROWS_AS(Card({CardStyle::elevated, true}), std::invalid_argument);
    Card card({CardStyle::elevated, true, true, nullptr, nullptr, "Trip to Kyoto"});
    place(card);
    CHECK(card.accessible().role == widgets::AccessibleRole::button);
    CHECK(card.accessible().name == "Trip to Kyoto");
    CHECK(click(card, 50.0, 50.0));
    card.handle({input::PointerAction::move, 50.0, 50.0});
    card.step(0.13);
    CHECK(card.elevation() == doctest::Approx(3.0));
    CHECK(card.state_layer() > 0.0);
    card.handle({input::PointerAction::down, 50.0, 50.0});
    card.step(0.13);
    CHECK(card.elevation() == doctest::Approx(1.0));
    card.cancel_interaction();
    CHECK_FALSE(card.handle(input::KeyEvent{input::Key::space}));
    card.set_focused(true);
    CHECK(card.handle(input::KeyEvent{input::Key::space}));
    CHECK(card.handle(input::KeyEvent{input::Key::enter}));
    card.set_enabled(false);
    CHECK_FALSE(card.focused());
    CHECK_FALSE(card.can_focus());
    CHECK_FALSE(click(card, 50.0, 50.0));
}

TEST_CASE("a filled card fades to its disabled container composited over surface-container-highest") {
    Card card({CardStyle::filled, true, false, nullptr, nullptr, "Off"});
    place(card);
    thorvg::Canvas canvas(200, 100);
    canvas.clear(0xffffffff);
    card.paint(canvas, scheme());
    CHECK(canvas.pixel(100, 50) == composite(color::Role::surface_variant, 0.38, color::Role::surface_container_highest));
}

TEST_CASE("a card holds any content, routes input to it and does not press under its controls") {
    widgets::WidgetSet content;
    auto& title = content.emplace<widgets::Label>(widgets::LabelOptions{"Kyoto", tokens::TypeRole::title_medium}, fonts());
    auto& button = content.emplace<widgets::Button>(widgets::ButtonOptions{"Book", tokens::ButtonStyle::text}, fonts());
    auto column = layout::column({widgets::node(title), widgets::node(button)});
    column->padding(layout::Insets::all(16.0)).spacing(8.0);
    Card card({CardStyle::outlined, true, true, column, &content, "Kyoto"});
    const layout::Size s = card.measure({0.0, 300.0, 0.0, 300.0});
    card.arrange({0.0, 0.0, s.width, s.height});
    CHECK(card.accessible().children.size() == 2);
    const render::Rect b = button.bounds();
    const double bx = b.x + b.width / 2.0;
    const double by = b.y + b.height / 2.0;
    card.handle({input::PointerAction::down, bx, by});
    CHECK(card.state_layer() == 0.0);
    CHECK_FALSE(card.handle({input::PointerAction::up, bx, by}));
    CHECK(click(card, 8.0, 8.0));
    card.set_enabled(false);
    CHECK_FALSE(button.enabled());
    card.set_enabled(true);
    CHECK(button.enabled());
}

TEST_CASE("an outlined card draws its border over content that fills it, as Modifier.border does") {
    widgets::WidgetSet content;
    auto& item = content.emplace<widgets::ListItem>(widgets::ListItemSpec{"Row"}, fonts());
    Card card({CardStyle::outlined, false, true, widgets::measured_node(item), &content});
    card.measure(layout::Constraints::tight({200.0, 56.0}));
    card.arrange({0.0, 0.0, 200.0, 56.0});
    thorvg::Canvas canvas(200, 56);
    canvas.clear(0xffffffff);
    card.paint(canvas, scheme());
    CHECK(canvas.pixel(100, 0) == scheme()[color::Role::outline_variant]);
    CHECK(canvas.pixel(0, 28) == scheme()[color::Role::outline_variant]);
}

TEST_CASE("cards satisfy the widget concepts") {
    static_assert(widgets::Measurable<Card>);
    static_assert(widgets::FocusGated<Card>);
    static_assert(widgets::Disableable<Card>);
    static_assert(widgets::Accessible<Card>);
    static_assert(widgets::Cancellable<Card>);
}
