#include <doctest/doctest.h>

#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/numeric_field.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::NumericField;

static_assert(widgets::Measurable<NumericField> && widgets::Paintable<NumericField> && widgets::Steppable<NumericField> &&
              widgets::HandlesPointer<NumericField> && widgets::HandlesKeys<NumericField> && widgets::HandlesText<NumericField> &&
              widgets::FocusGated<NumericField> && widgets::Disableable<NumericField> && widgets::Densifiable<NumericField> &&
              widgets::TextInput<NumericField>);

namespace {

constexpr const tokens::NumericFieldTokens& t = tokens::numeric_field_tokens;

input::KeyEvent key(Key k, bool shift = false) {
    return {k, shift};
}

NumericField make() {
    NumericField f({.label = "X", .value = 120.0, .min = 0.0, .max = 1920.0, .step = 1.0, .large_step = 10.0, .unit = "px"}, fonts());
    f.set_origin({10.0, 20.0});
    return f;
}

}

TEST_CASE("a numeric field is compact, puts its label at the start and shows its unit") {
    NumericField f = make();
    CHECK(f.height() == 40.0);
    CHECK(f.field_rect().width == t.width);
    CHECK(f.label_rect().x == 10.0);
    CHECK(f.field_rect().x > f.label_rect().x + f.label_rect().width);
    CHECK(f.formatted() == "120");
    CHECK(f.text() == "120");
    f.set_density(tokens::Density{-2});
    CHECK(f.height() == 32.0);
    f.set_density(tokens::Density{-9});
    CHECK(f.height() == 28.0);
    NumericField inside({.label = "W", .label_position = widgets::NumericLabelPosition::inside}, fonts());
    CHECK(inside.width() == t.width);
    CHECK(inside.label_rect().x == inside.field_rect().x + t.padding);
    const widgets::AccessibleNode n = f.accessible();
    CHECK(n.role == widgets::AccessibleRole::spin_button);
    CHECK(n.name == "X");
    CHECK(n.value == "120 px");
    CHECK(n.state.range->max == 1920.0);
}

TEST_CASE("dragging across a numeric field scrubs its value by steps within its range") {
    NumericField f = make();
    const render::Rect r = f.field_rect();
    const double y = r.y + r.height / 2.0;
    const double x = r.x + 20.0;
    f.handle(input::PointerEvent{PointerAction::move, x, y});
    REQUIRE(f.cursor().has_value());
    CHECK(*f.cursor() == input::Cursor::resize_horizontal);
    CHECK(f.handle(input::PointerEvent{PointerAction::down, x, y}).consumed);
    CHECK_FALSE(f.handle(input::PointerEvent{PointerAction::move, x + 2.0, y}).changed);
    CHECK(f.handle(input::PointerEvent{PointerAction::move, x + 10.0 * t.scrub_distance, y}).changed);
    CHECK(f.scrubbing());
    CHECK(f.value() == 130.0);
    f.handle(input::PointerEvent{PointerAction::move, x - 1000.0 * t.scrub_distance, y});
    CHECK(f.value() == 0.0);
    const widgets::NumericFieldResponse up = f.handle(input::PointerEvent{PointerAction::up, x - 1000.0 * t.scrub_distance, y});
    CHECK(up.committed);
    CHECK_FALSE(f.editing());
    const render::Rect l = f.label_rect();
    f.handle(input::PointerEvent{PointerAction::down, l.x + 2.0, y});
    f.handle(input::PointerEvent{PointerAction::move, l.x + 2.0 + 5.0 * t.scrub_distance, y});
    CHECK(f.value() == 5.0);
    f.handle(input::PointerEvent{PointerAction::up, l.x + 2.0 + 5.0 * t.scrub_distance, y});
}

TEST_CASE("a click edits the value, the arrow keys step it and Enter commits typed text") {
    NumericField f = make();
    const render::Rect r = f.field_rect();
    const double y = r.y + r.height / 2.0;
    f.handle(input::PointerEvent{PointerAction::down, r.x + 20.0, y});
    f.handle(input::PointerEvent{PointerAction::up, r.x + 20.0, y});
    CHECK(f.editing());
    CHECK(*f.cursor() == input::Cursor::text);
    CHECK(f.handle(key(Key::up)).changed);
    CHECK(f.value() == 121.0);
    CHECK(f.text() == "121");
    f.handle(key(Key::up, true));
    CHECK(f.value() == 131.0);
    f.handle(key(Key::page_down));
    CHECK(f.value() == 121.0);
    f.handle(input::TextEvent{input::TextEventKind::commit, "4a0x0", 0});
    CHECK(f.text() == "400");
    const widgets::NumericFieldResponse enter = f.handle(key(Key::enter));
    CHECK(enter.committed);
    CHECK(enter.changed);
    CHECK(f.value() == 400.0);
    f.handle(input::TextEvent{input::TextEventKind::commit, "99999", 0});
    f.handle(key(Key::enter));
    CHECK(f.value() == 1920.0);
}

TEST_CASE("text that is not a number marks the field invalid until it is fixed or Escape reverts it") {
    NumericField f = make();
    f.set_focused(true);
    f.handle(input::TextEvent{input::TextEventKind::commit, "1-2", 0});
    f.handle(key(Key::enter));
    CHECK(f.error());
    CHECK(f.value() == 120.0);
    CHECK(f.accessible().state.invalid);
    f.handle(key(Key::escape));
    CHECK_FALSE(f.error());
    CHECK(f.text() == "120");
    f.handle(input::TextEvent{input::TextEventKind::commit, "--", 0});
    f.set_focused(false);
    CHECK_FALSE(f.editing());
    CHECK(f.text() == "120");
}

TEST_CASE("a numeric field keeps its decimals and takes the focus from Tab") {
    NumericField f({.label = "Opacity", .value = 0.456, .min = 0.0, .max = 1.0, .step = 0.05, .decimals = 2}, fonts());
    CHECK(f.formatted() == "0.46");
    widgets::FocusManager m;
    m.add(f);
    CHECK(m.handle(input::KeyEvent{Key::tab, false}));
    CHECK(f.editing());
    f.handle(key(Key::up));
    CHECK(f.value() == doctest::Approx(0.51));
    CHECK(f.formatted() == "0.51");
}

TEST_CASE("a disabled numeric field neither scrubs nor edits") {
    NumericField f = make();
    f.set_enabled(false);
    const render::Rect r = f.field_rect();
    CHECK_FALSE(f.handle(input::PointerEvent{PointerAction::down, r.x + 10.0, r.y + 10.0}).consumed);
    f.set_focused(true);
    CHECK_FALSE(f.editing());
    CHECK_FALSE(f.cursor().has_value());
    thorvg::Canvas canvas(300, 100);
    canvas.clear(0xffffffff);
    f.paint(canvas, scheme());
}
