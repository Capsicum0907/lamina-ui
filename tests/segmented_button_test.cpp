#include <doctest/doctest.h>

#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/segmented_button.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::Segment;
using widgets::SegmentedButtonRow;
using widgets::SegmentedButtonRowOptions;
using widgets::SegmentSelection;

namespace {

double label_width(const char* text) {
    return text::layout_line(fonts(), text, tokens::text_style(tokens::type_style(tokens::TypeRole::label_large))).width;
}

SegmentedButtonRow row(SegmentSelection selection, std::vector<Segment> segments) {
    SegmentedButtonRow r({std::move(segments), selection, "Row"}, fonts(), icon_font());
    r.set_origin({0.0, 0.0});
    return r;
}

std::vector<Segment> days() {
    return {{"Day"}, {"Week", std::nullopt, true}, {"Month"}, {"Year"}};
}

void press_segment(SegmentedButtonRow& r, std::size_t i) {
    const render::Rect b = r.segment_bounds(i);
    const double x = b.x + b.width / 2.0;
    const double y = b.y + b.height / 2.0;
    r.handle({PointerAction::down, x, y});
    r.handle({PointerAction::up, x, y});
    r.handle({PointerAction::leave, 0.0, 0.0});
}

input::KeyEvent key(Key k) {
    return {k};
}

}

static_assert(widgets::Sized<SegmentedButtonRow>);
static_assert(widgets::Focusable<SegmentedButtonRow>);
static_assert(widgets::FocusGated<SegmentedButtonRow>);
static_assert(widgets::Disableable<SegmentedButtonRow>);

TEST_CASE("segmented button tokens follow OutlinedSegmentedButtonTokens.kt and SegmentedButton.kt") {
    const auto& t = tokens::segmented_button_tokens;
    CHECK(t.height == 40.0);
    CHECK(t.min_width == 58.0);
    CHECK(t.horizontal_padding == 12.0);
    CHECK(t.outline_width == 1.0);
    CHECK(t.icon_size == 18.0);
    CHECK(t.icon_spacing == 8.0);
    CHECK(t.corner == tokens::CornerSize::full);
    CHECK(t.label == tokens::TypeRole::label_large);
    CHECK(t.active_container == color::Role::secondary_container);
    CHECK(t.active_content == color::Role::on_secondary_container);
    CHECK(t.inactive_content == color::Role::on_surface);
    CHECK(t.outline == color::Role::outline);
    CHECK(t.disabled_outline.opacity == 0.12);
    CHECK(t.disabled_content.opacity == 0.38);
}

TEST_CASE("segments share the width of the widest one and overlap their borders by 1 dp") {
    auto r = row(SegmentSelection::single, days());
    const double widest = 12.0 + 18.0 + 8.0 + label_width("Month") + 12.0;
    CHECK(r.segment_width() == doctest::Approx(std::max(58.0, widest)));
    CHECK(r.height() == 40.0);
    CHECK(r.width() == doctest::Approx(4.0 * r.segment_width() - 3.0));
    CHECK(r.segment_bounds(1).x == doctest::Approx(r.segment_width() - 1.0));
    auto tiny = row(SegmentSelection::single, {{"A"}, {"B"}});
    CHECK(tiny.segment_width() == doctest::Approx(std::max(58.0, 50.0 + std::max(label_width("A"), label_width("B")))));
}

TEST_CASE("a single-choice row keeps one segment selected") {
    auto r = row(SegmentSelection::single, {{"A", std::nullopt, true}, {"B", std::nullopt, true}, {"C"}});
    CHECK(r.selected() == 0);
    CHECK_FALSE(r.checked(1));
    press_segment(r, 2);
    CHECK(r.selected() == 2);
    CHECK_FALSE(r.checked(0));
    press_segment(r, 2);
    CHECK(r.selected() == 2);
}

TEST_CASE("a multi-choice row toggles each segment") {
    auto r = row(SegmentSelection::multiple, {{"A", std::nullopt, true}, {"B"}, {"C"}});
    press_segment(r, 1);
    press_segment(r, 0);
    CHECK(r.checked_segments() == std::vector<std::size_t>{1});
}

TEST_CASE("the checkmark scales in while the label slides 13 dp, and vanishes at once") {
    auto r = row(SegmentSelection::multiple, {{"A"}, {"B"}});
    CHECK(r.label_offset(0) == doctest::Approx(-13.0));
    CHECK(r.check_presence(0) == 0.0);
    press_segment(r, 0);
    r.step(frame_seconds);
    CHECK(r.check_presence(0) > 0.0);
    CHECK(r.check_presence(0) < 1.0);
    CHECK(r.label_offset(0) > -13.0);
    settle(r);
    CHECK(r.check_presence(0) == doctest::Approx(1.0));
    CHECK(r.label_offset(0) == doctest::Approx(0.0));
    press_segment(r, 0);
    CHECK(r.check_presence(0) == 0.0);
    r.step(frame_seconds);
    CHECK(r.label_offset(0) < 0.0);
    CHECK(r.label_offset(0) > -13.0);
}

TEST_CASE("a segment with an icon keeps the label still and cross-fades the icon with the checkmark") {
    auto r = row(SegmentSelection::single, {{"Walk", icons::Icon{icon_font(), res::icons::home}}, {"Drive"}});
    CHECK(r.label_offset(0) == 0.0);
    press_segment(r, 0);
    settle(r);
    CHECK(r.label_offset(0) == 0.0);
}

TEST_CASE("the row is one Tab stop; arrows select in a single-choice row and move the focus in a multi-choice one") {
    auto single = row(SegmentSelection::single, days());
    widgets::FocusManager focus;
    focus.add(single);
    focus.handle(key(Key::tab));
    CHECK(single.focused_segment() == 1);
    CHECK(single.handle(key(Key::right)).clicked == 2);
    CHECK(single.selected() == 2);
    single.handle(key(Key::end));
    CHECK(single.selected() == 3);
    single.handle(key(Key::right));
    CHECK(single.selected() == 0);
    single.handle(key(Key::home));
    CHECK(single.selected() == 0);

    auto multi = row(SegmentSelection::multiple, days());
    multi.set_focused(true);
    CHECK(multi.focused_segment() == 0);
    multi.handle(key(Key::right));
    CHECK(multi.focused_segment() == 1);
    CHECK(multi.checked(1));
    CHECK(multi.handle(key(Key::space)).clicked == 1);
    CHECK_FALSE(multi.checked(1));
    multi.handle(key(Key::enter));
    CHECK(multi.checked(1));
}

TEST_CASE("disabled segments are skipped and keep their own setting when the row is enabled again") {
    auto r = row(SegmentSelection::single, {{"A", std::nullopt, true}, {"B", std::nullopt, false, false}, {"C"}});
    r.set_focused(true);
    r.handle(key(Key::right));
    CHECK(r.selected() == 2);
    press_segment(r, 1);
    CHECK(r.selected() == 2);
    r.set_enabled(false);
    CHECK_FALSE(r.can_focus());
    CHECK_FALSE(r.focused());
    press_segment(r, 0);
    CHECK(r.selected() == 2);
    r.set_enabled(true);
    CHECK(r.segment_enabled(0));
    CHECK_FALSE(r.segment_enabled(1));
}

TEST_CASE("segments report radio buttons or toggle buttons with their state") {
    auto single = row(SegmentSelection::single, days());
    const widgets::AccessibleNode n = single.accessible();
    CHECK(n.role == widgets::AccessibleRole::group);
    CHECK(n.children.size() == 4);
    CHECK(n.children[1].role == widgets::AccessibleRole::radio_button);
    CHECK(n.children[1].state.selected == true);
    CHECK(n.children[0].name == "Day");
    auto multi = row(SegmentSelection::multiple, {{"Bold", std::nullopt, true}, {"Italic"}});
    CHECK(multi.accessible().children[0].role == widgets::AccessibleRole::toggle_button);
    CHECK(multi.accessible().children[0].state.toggle == widgets::AccessibleToggle::on);
    CHECK_THROWS_AS(row(SegmentSelection::single, {{"", icons::Icon{icon_font(), res::icons::home}}}), std::invalid_argument);
}

TEST_CASE("a checked segment fills with secondary-container and draws over its neighbours") {
    auto r = row(SegmentSelection::single, {{"A", std::nullopt, true}, {"B"}});
    settle(r);
    thorvg::Canvas canvas(static_cast<int>(r.width()) + 2, 42);
    canvas.clear(scheme()[color::Role::surface]);
    r.paint(canvas, scheme());
    canvas.finish();
    const render::Rect a = r.segment_bounds(0);
    CHECK(canvas.pixel(static_cast<int>(a.x + a.width - 8.0), 20) == scheme()[color::Role::secondary_container]);
    const render::Rect b = r.segment_bounds(1);
    CHECK(canvas.pixel(static_cast<int>(b.x + b.width - 6.0), 20) == scheme()[color::Role::surface]);
}
