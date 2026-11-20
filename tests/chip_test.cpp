#include <doctest/doctest.h>

#include <string>

#include "lamina/widgets/chip.hpp"
#include "lamina/widgets/chip_group.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using tokens::ChipKind;
using widgets::Chip;
using widgets::ChipOptions;

namespace {

icons::Icon icon(icons::Symbol s) {
    return {icon_font(), s};
}

double label_width(const char* text) {
    return text::layout_line(fonts(), text, tokens::text_style(tokens::type_style(tokens::TypeRole::label_large))).width;
}

render::Image avatar() {
    render::Image img{4, 4, std::vector<color::Argb>(16, 0xff3366cc)};
    return img;
}

input::KeyEvent key(input::Key k) {
    return {k};
}

}

TEST_CASE("chip tokens follow the chip token files and Chip.kt") {
    const auto& t = tokens::chip_tokens;
    CHECK(t.height == 32.0);
    CHECK(t.corner == tokens::CornerSize::small);
    CHECK(t.label == tokens::TypeRole::label_large);
    CHECK(t.icon_size == 18.0);
    CHECK(t.avatar_size == 24.0);
    CHECK(t.element_spacing == 8.0);
    CHECK(t.padding == 8.0);
    CHECK(t.input_edge_padding == 4.0);
    CHECK(t.outline_width == 1.0);
    const auto& assist = tokens::chip_style(ChipKind::assist, false);
    CHECK(assist.label == color::Role::on_surface);
    CHECK(assist.leading_icon == color::Role::primary);
    CHECK(assist.outline == color::Role::outline_variant);
    CHECK(assist.disabled_outline->opacity == 0.12);
    const auto& filter = tokens::chip_style(ChipKind::filter, false);
    CHECK(filter.selected_container == color::Role::secondary_container);
    CHECK(filter.selected_label == color::Role::on_secondary_container);
    CHECK(filter.elevation.hovered == 1.0);
    const auto& input = tokens::chip_style(ChipKind::input, true);
    CHECK(input.selected_leading_icon == color::Role::primary);
    CHECK(input.elevation.hovered == 0.0);
    const auto& elevated = tokens::chip_style(ChipKind::suggestion, true);
    CHECK(elevated.container == color::Role::surface_container_low);
    CHECK_FALSE(elevated.outline.has_value());
    CHECK(elevated.elevation.resting == 1.0);
    CHECK(elevated.elevation.hovered == 3.0);
    CHECK(elevated.elevation.disabled == 0.0);
}

TEST_CASE("a chip is 32 dp tall with 16 dp around the label, or 8 dp before an 18 dp icon") {
    Chip plain({"Plain", ChipKind::suggestion}, fonts());
    CHECK(plain.height() == 32.0);
    CHECK(plain.width() == doctest::Approx(label_width("Plain") + 32.0));
    Chip icon_chip({"Icon", ChipKind::assist, false, icon(res::icons::send)}, fonts());
    CHECK(icon_chip.width() == doctest::Approx(8.0 + 18.0 + 8.0 + label_width("Icon") + 16.0));
    icon_chip.set_origin({10.0, 20.0});
    CHECK(icon_chip.leading_rect()->x == doctest::Approx(18.0));
}

TEST_CASE("input chips pad 4 dp at a bare or avatar edge and 8 dp at an icon") {
    Chip bare({"Bare", ChipKind::input}, fonts());
    CHECK(bare.width() == doctest::Approx(4.0 + 8.0 + label_width("Bare") + 8.0 + 4.0));
    ChipOptions with_avatar{"Ada", ChipKind::input};
    with_avatar.avatar = avatar();
    with_avatar.trailing_icon = icon(res::icons::close);
    with_avatar.removable = true;
    Chip a(with_avatar, fonts());
    CHECK(a.width() == doctest::Approx(4.0 + 24.0 + 8.0 + label_width("Ada") + 8.0 + 18.0 + 8.0));
    ChipOptions bad{"Oops", ChipKind::input};
    bad.removable = true;
    CHECK_THROWS_AS(Chip(bad, fonts()), std::invalid_argument);
}

TEST_CASE("a filter chip toggles on click and grows its checkmark slot on the fast spatial spring") {
    ChipOptions o{"Starred", ChipKind::filter};
    o.selected_leading_icon = icon(res::icons::check);
    Chip c(o, fonts());
    c.set_origin({0.0, 0.0});
    const double unselected = c.width();
    CHECK(c.leading_presence() == 0.0);
    widgets::ChipResponse r;
    c.handle({input::PointerAction::down, 20.0, 16.0});
    r = c.handle({input::PointerAction::up, 20.0, 16.0});
    CHECK(r.clicked);
    CHECK(r.consumed);
    CHECK(c.selected());
    CHECK(c.accessible().role == widgets::AccessibleRole::checkbox);
    CHECK(c.accessible().state.toggle == widgets::AccessibleToggle::on);
    CHECK(c.target_width() == doctest::Approx(unselected + 18.0));
    c.step(frame_seconds);
    CHECK(c.width() > unselected);
    CHECK(c.width() < unselected + 18.0);
    settle(c);
    CHECK(c.width() == doctest::Approx(unselected + 18.0));
    c.handle({input::PointerAction::down, 20.0, 16.0});
    c.handle({input::PointerAction::up, 20.0, 16.0});
    CHECK_FALSE(c.selected());
    settle(c);
    CHECK(c.width() == doctest::Approx(unselected));
}

TEST_CASE("a filter chip with a leading icon swaps it for the selected one at once") {
    ChipOptions o{"Home", ChipKind::filter, false, icon(res::icons::home), icon(res::icons::check)};
    Chip c(o, fonts());
    const double w = c.width();
    c.set_selected(true);
    CHECK(c.width() == doctest::Approx(w));
    CHECK(c.leading_presence() == 1.0);
}

TEST_CASE("assist and suggestion chips are buttons and keep no selection") {
    Chip c({"Help", ChipKind::assist}, fonts());
    c.set_selected(true);
    CHECK_FALSE(c.selected());
    CHECK(c.accessible().role == widgets::AccessibleRole::button);
    CHECK_FALSE(c.accessible().state.toggle.has_value());
}

TEST_CASE("an elevated chip tweens to its hover elevation in 120 ms and snaps when disabled") {
    Chip c({"Lift", ChipKind::assist, true}, fonts());
    c.set_origin({0.0, 0.0});
    CHECK(c.elevation() == 1.0);
    c.handle({input::PointerAction::move, 10.0, 16.0});
    c.step(0.06);
    CHECK(c.elevation() > 1.0);
    CHECK(c.elevation() < 3.0);
    c.step(0.07);
    CHECK(c.elevation() == doctest::Approx(3.0));
    c.handle({input::PointerAction::leave, 0.0, 0.0});
    c.step(0.06);
    CHECK(c.elevation() < 3.0);
    c.step(0.1);
    CHECK(c.elevation() == doctest::Approx(1.0));
    c.set_enabled(false);
    CHECK(c.elevation() == 0.0);
}

TEST_CASE("a removable input chip reports a click on its trailing icon as a removal, and Delete too") {
    ChipOptions o{"Ada", ChipKind::input};
    o.trailing_icon = icon(res::icons::close);
    o.removable = true;
    Chip c(o, fonts());
    c.set_origin({0.0, 0.0});
    const render::Rect trail = *c.trailing_rect();
    c.handle({input::PointerAction::down, trail.x + 9.0, 16.0});
    const widgets::ChipResponse r = c.handle({input::PointerAction::up, trail.x + 9.0, 16.0});
    CHECK(r.removed);
    CHECK_FALSE(r.clicked);
    CHECK_FALSE(c.selected());
    REQUIRE(c.accessible().children.size() == 1);
    CHECK(c.accessible().children[0].name == "Remove Ada");
    c.set_focused(true);
    CHECK(c.handle(key(input::Key::delete_forward)).removed);
    CHECK(c.handle(key(input::Key::backspace)).removed);
    CHECK(c.handle(key(input::Key::space)).clicked);
    CHECK(c.selected());
}

TEST_CASE("a disabled chip takes no input and paints the disabled colors") {
    Chip c({"Off", ChipKind::assist, false, std::nullopt, std::nullopt, std::nullopt, std::nullopt, false, false, false}, fonts());
    c.set_origin({0.0, 0.0});
    CHECK_FALSE(c.can_focus());
    c.handle({input::PointerAction::down, 10.0, 16.0});
    CHECK_FALSE(c.handle({input::PointerAction::up, 10.0, 16.0}).clicked);
    CHECK_FALSE(c.accessible().state.enabled);
    thorvg::Canvas canvas(static_cast<int>(c.width()) + 2, 34);
    canvas.clear(scheme()[color::Role::surface]);
    c.paint(canvas, scheme());
    const color::Argb outline = color::source_over(color::scale_alpha(scheme()[color::Role::on_surface], 0.12), scheme()[color::Role::surface]);
    const color::Argb pixel = canvas.pixel(16, 0);
    CHECK(std::abs(static_cast<int>((pixel >> 16) & 0xff) - static_cast<int>((outline >> 16) & 0xff)) <= 2);
}

TEST_CASE("a selected filter chip paints the secondary container without an outline") {
    ChipOptions o{"On", ChipKind::filter};
    o.selected = true;
    Chip c(o, fonts());
    c.set_origin({0.0, 0.0});
    thorvg::Canvas canvas(static_cast<int>(c.width()) + 2, 34);
    canvas.clear(0xffffffff);
    c.paint(canvas, scheme());
    CHECK(canvas.pixel(4, 16) == scheme()[color::Role::secondary_container]);
    CHECK(canvas.pixel(12, 0) == scheme()[color::Role::secondary_container]);
}

TEST_CASE("a chip group wraps its chips with 8 dp between them and between lines") {
    widgets::ChipGroupOptions o;
    for (const char* label : {"Chip 1", "Chip 2", "Chip 3", "Chip 4", "Chip 5", "Chip 6"}) {
        o.chips.push_back({label, ChipKind::suggestion});
    }
    widgets::ChipGroup g(o, fonts());
    const double one = label_width("Chip 1") + 32.0;
    const double two = label_width("Chip 2") + 32.0;
    const layout::Size s = g.measure({0.0, one + 8.0 + two + 1.0, 0.0, layout::unbounded});
    g.arrange({0.0, 0.0, s.width, s.height});
    CHECK(g.line_count() == 3);
    CHECK(g.chip(1).bounds().x == doctest::Approx(one + 8.0));
    CHECK(g.chip(2).bounds().y == doctest::Approx(40.0));
    CHECK(s.height == doctest::Approx(32.0 * 3 + 16.0));
}

TEST_CASE("a chip group is one Tab stop whose arrows move between enabled chips, wrapping") {
    widgets::ChipGroupOptions o;
    o.single_selection = true;
    o.accessible_name = "Sizes";
    for (const char* label : {"S", "M", "L"}) {
        o.chips.push_back({label, ChipKind::filter});
    }
    widgets::ChipGroup g(o, fonts());
    g.arrange({0.0, 0.0, 400.0, 32.0});
    g.set_chip_enabled(1, false);
    widgets::FocusManager focus;
    focus.add(g);
    CHECK(focus.move_focus(widgets::FocusDirection::next));
    CHECK(g.focused_chip() == 0u);
    CHECK(g.chip(0).focused());
    CHECK(g.handle(key(input::Key::right)).consumed);
    CHECK(g.focused_chip() == 2u);
    g.handle(key(input::Key::right));
    CHECK(g.focused_chip() == 0u);
    g.handle(key(input::Key::end));
    CHECK(g.focused_chip() == 2u);
    const widgets::ChipGroupResponse r = g.handle(key(input::Key::space));
    CHECK(r.clicked == 2u);
    CHECK(g.chip(2).selected());
    g.handle(key(input::Key::home));
    g.handle(key(input::Key::enter));
    CHECK(g.chip(0).selected());
    CHECK_FALSE(g.chip(2).selected());
    CHECK(g.accessible().children.size() == 3);
    CHECK(g.accessible().name == "Sizes");
}

TEST_CASE("removing an input chip from a group fades it out while the next chips slide into its place") {
    widgets::ChipGroupOptions o;
    for (const char* label : {"Ada", "Grace", "Alan"}) {
        ChipOptions c{label, ChipKind::input};
        c.trailing_icon = icon(res::icons::close);
        c.removable = true;
        o.chips.push_back(c);
    }
    widgets::ChipGroup g(o, fonts());
    g.measure({0.0, 600.0, 0.0, layout::unbounded});
    g.arrange({0.0, 0.0, 600.0, 32.0});
    const double grace_x = g.chip(1).bounds().x;
    const double alan_x = g.chip(2).bounds().x;
    g.set_focused(true);
    g.handle(key(input::Key::right));
    const widgets::ChipGroupResponse r = g.handle(key(input::Key::delete_forward));
    CHECK(r.removed == 1u);
    CHECK(r.removed_label == "Grace");
    CHECK(g.size() == 2);
    CHECK(g.focused_chip() == 1u);
    CHECK(g.chip(1).label() == "Alan");
    g.step(frame_seconds);
    CHECK(g.chip(1).bounds().x < alan_x);
    CHECK(g.chip(1).bounds().x > grace_x);
    settle(g);
    CHECK(g.chip(1).bounds().x == doctest::Approx(grace_x));
    CHECK_FALSE(g.animating());
    CHECK(g.accessible().children.size() == 2);
}

TEST_CASE("chips and chip groups satisfy the widget concepts") {
    static_assert(widgets::Focusable<Chip>);
    static_assert(widgets::FocusGated<Chip>);
    static_assert(widgets::Disableable<Chip>);
    static_assert(widgets::Accessible<Chip>);
    static_assert(widgets::Sized<Chip>);
    static_assert(widgets::Measurable<widgets::ChipGroup>);
    static_assert(widgets::FocusGated<widgets::ChipGroup>);
    static_assert(widgets::Disableable<widgets::ChipGroup>);
    widgets::WidgetSet set;
    auto& c = set.emplace<Chip>(ChipOptions{"In a set"}, fonts());
    c.set_origin({0.0, 0.0});
    set.handle(input::PointerEvent{input::PointerAction::down, 10.0, 10.0});
    CHECK(set.handle(input::PointerEvent{input::PointerAction::up, 10.0, 10.0}));
    set.set_enabled(false);
    CHECK_FALSE(c.enabled());
}
