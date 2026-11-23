#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "lamina/widgets/search_bar.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;

namespace {

constexpr const tokens::SearchTokens& t = tokens::search_tokens;
constexpr render::Rect phone{0.0, 0.0, 412.0, 800.0};
constexpr render::Rect desktop{0.0, 0.0, 1200.0, 800.0};

std::vector<widgets::ListItemSpec> results(int n) {
    std::vector<widgets::ListItemSpec> out;
    for (int i = 0; i < n; ++i) {
        out.push_back({.headline = "Result " + std::to_string(i), .supporting = "Additional info", .transparent = true});
    }
    return out;
}

widgets::SearchBarOptions options(int n = 4) {
    return {.placeholder = "Search",
            .search_icon = {icon_font(), test::res::icons::search},
            .back = {{icon_font(), test::res::icons::home}, "Back"},
            .trailing = widgets::SearchBarIcon{{icon_font(), test::res::icons::star}, "Mic"},
            .results = results(n)};
}

void place(widgets::SearchBar& s, const render::Rect& area) {
    s.set_area(area);
    s.arrange({area.x + 16.0, area.y + 8.0, area.width - 32.0, t.height});
}

void click(widgets::SearchBar& s, double x, double y) {
    s.handle(input::PointerEvent{PointerAction::down, x, y, 1});
    s.handle(input::PointerEvent{PointerAction::up, x, y});
}

}

TEST_CASE("a collapsed search bar is a 56 dp pill between 360 and 720 dp wide with room for its icons") {
    widgets::SearchBar s(options(), fonts());
    CHECK(s.measure({0.0, 1000.0, 0.0, 100.0}).width == doctest::Approx(t.max_width));
    CHECK(s.measure({0.0, 500.0, 0.0, 100.0}).width == doctest::Approx(500.0));
    CHECK(s.measure({0.0, 1000.0, 0.0, 100.0}).height == doctest::Approx(t.height));
    place(s, desktop);
    CHECK(s.bounds().width == doctest::Approx(t.max_width));
    CHECK(s.corner_radius() == doctest::Approx(t.height / 2.0));
    CHECK(s.field().placeholder_shown());
    const render::Rect trailing = s.trailing_button()->bounds();
    CHECK(trailing.x + trailing.width / 2.0 == doctest::Approx(s.bounds().x + s.bounds().width - 28.0));
}

TEST_CASE("clicking the bar on a compact window expands it to fill the window with square corners") {
    widgets::SearchBar s(options(), fonts());
    place(s, phone);
    CHECK(s.full_screen());
    click(s, 200.0, 36.0);
    CHECK(s.expanded());
    CHECK(s.field().focused());
    for (int i = 0; i < 6; ++i) {
        s.step(frame_seconds);
    }
    CHECK(s.progress() > 0.0);
    CHECK(s.progress() < 1.0);
    settle(s);
    const render::Rect surface = s.surface();
    CHECK(surface.x == doctest::Approx(phone.x));
    CHECK(surface.width == doctest::Approx(phone.width));
    CHECK(surface.height == doctest::Approx(phone.height));
    CHECK(s.corner_radius() == doctest::Approx(0.0));
    CHECK(s.field_rect().y == doctest::Approx(t.vertical_padding));
    CHECK(s.field_rect().width == doctest::Approx(phone.width));
    CHECK(s.results_rect().y == doctest::Approx(t.vertical_padding * 2.0 + t.height + 1.0));
}

TEST_CASE("on a wide window the expanded view docks below the bar, at least 240 dp and at most two thirds of the window") {
    widgets::SearchBar few(options(1), fonts());
    place(few, desktop);
    CHECK_FALSE(few.full_screen());
    few.expand();
    settle(few);
    CHECK(few.surface().x == doctest::Approx(few.bounds().x));
    CHECK(few.surface().width == doctest::Approx(few.bounds().width));
    CHECK(few.surface().height == doctest::Approx(t.docked_min_height));
    CHECK(few.corner_radius() == doctest::Approx(28.0));
    widgets::SearchBar many(options(40), fonts());
    place(many, desktop);
    many.expand();
    settle(many);
    CHECK(many.surface().height == doctest::Approx(desktop.height * 2.0 / 3.0));
}

TEST_CASE("Down expands a focused bar, moves into the results and Up comes back to the query") {
    widgets::SearchBar s(options(), fonts());
    place(s, phone);
    s.field().set_focused(true);
    auto r = s.handle(input::KeyEvent{Key::down, false});
    CHECK(r.expanded);
    settle(s);
    s.handle(input::KeyEvent{Key::down, false});
    CHECK(s.results().focused());
    CHECK(s.results().focused_row() == 0u);
    s.handle(input::KeyEvent{Key::down, false});
    CHECK(s.results().focused_row() == 1u);
    r = s.handle(input::KeyEvent{Key::enter, false});
    CHECK(r.chosen == 1u);
    s.handle(input::KeyEvent{Key::up, false});
    s.handle(input::KeyEvent{Key::up, false});
    CHECK(s.field().focused());
    r = s.handle(input::KeyEvent{Key::escape, false});
    CHECK(r.collapsed);
    CHECK_FALSE(s.expanded());
    CHECK(s.field().focused());
}

TEST_CASE("typing into a focused collapsed bar expands it and Enter searches") {
    widgets::SearchBar s(options(), fonts());
    place(s, phone);
    s.field().set_focused(true);
    auto r = s.handle(input::TextEvent{input::TextEventKind::commit, "k", 0});
    CHECK(r.query_changed);
    CHECK(r.expanded);
    s.handle(input::TextEvent{input::TextEventKind::commit, "y", 0});
    CHECK(s.query() == "ky");
    r = s.handle(input::KeyEvent{Key::enter, false});
    REQUIRE(r.searched);
    CHECK(*r.searched == "ky");
}

TEST_CASE("clicking a result chooses it and the back button collapses the view") {
    widgets::SearchBar s(options(), fonts());
    place(s, phone);
    s.expand();
    settle(s);
    const render::Rect row = s.results().row_rect(2);
    s.handle(input::PointerEvent{PointerAction::down, row.x + 40.0, row.y + 20.0, 1});
    const auto r = s.handle(input::PointerEvent{PointerAction::up, row.x + 40.0, row.y + 20.0});
    CHECK(r.chosen == 2u);
    const render::Rect back = s.back_button().bounds();
    s.handle(input::PointerEvent{PointerAction::down, back.x + 20.0, back.y + 20.0, 1});
    const auto c = s.handle(input::PointerEvent{PointerAction::up, back.x + 20.0, back.y + 20.0});
    CHECK(c.collapsed);
}

TEST_CASE("Tab stays inside the expanded view") {
    widgets::SearchBar s(options(), fonts());
    place(s, phone);
    s.expand();
    settle(s);
    CHECK(s.field().focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(s.trailing_button()->focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(s.results().focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(s.back_button().focused());
}

TEST_CASE("the search field is named Search and says suggestions are below while expanded") {
    widgets::SearchBar s(options(), fonts());
    place(s, phone);
    auto node = s.accessible();
    REQUIRE(node);
    auto field = widgets::find_accessible(node->children, "Search");
    REQUIRE(field);
    CHECK(field->role == widgets::AccessibleRole::text_field);
    s.expand();
    settle(s);
    node = s.accessible();
    REQUIRE(node);
    CHECK(node->role == widgets::AccessibleRole::dialog);
    field = widgets::find_accessible(node->children, [](const widgets::AccessibleNode& n) { return n.role == widgets::AccessibleRole::text_field; });
    REQUIRE(field);
    CHECK(field->description == "Suggestions below");
}

namespace {

void tap(widgets::SearchBar& s, double x, double y, input::PointerKind kind) {
    s.handle(input::PointerEvent{PointerAction::down, x, y, 1, kind});
    s.handle(input::PointerEvent{PointerAction::up, x, y, 0, kind});
}

render::Point centre(const render::Rect& r) {
    return {r.x + r.width / 2.0, r.y + r.height / 2.0};
}

}

TEST_CASE("the back and trailing buttons of the expanded view never put the focus in the field") {
    for (const input::PointerKind kind : {input::PointerKind::mouse, input::PointerKind::touch}) {
        CAPTURE(static_cast<int>(kind));
        widgets::SearchBar s(options(), fonts());
        place(s, phone);
        s.expand();
        settle(s);
        REQUIRE(s.field().focused());
        tap(s, 200.0, phone.y + phone.height + 40.0, kind);
        CHECK_FALSE(s.field().focused());
        CHECK(s.expanded());
        const render::Point mic = centre(s.trailing_button()->bounds());
        s.handle(input::PointerEvent{PointerAction::down, mic.x, mic.y, 1, kind});
        const auto m = s.handle(input::PointerEvent{PointerAction::up, mic.x, mic.y, 0, kind});
        CHECK(m.trailing);
        CHECK_FALSE(s.field().focused());
        CHECK(s.expanded());
        const render::Point back = centre(s.back_button().bounds());
        s.handle(input::PointerEvent{PointerAction::down, back.x, back.y, 1, kind});
        const auto b = s.handle(input::PointerEvent{PointerAction::up, back.x, back.y, 0, kind});
        CHECK(b.collapsed);
        CHECK_FALSE(s.expanded());
        CHECK_FALSE(s.focused());
        settle(s);
        CHECK_FALSE(s.caret_area());
    }
}

TEST_CASE("the back button collapses a view whose field has the focus and takes the focus away") {
    for (const input::PointerKind kind : {input::PointerKind::mouse, input::PointerKind::touch}) {
        CAPTURE(static_cast<int>(kind));
        widgets::SearchBar s(options(), fonts());
        place(s, phone);
        s.expand();
        settle(s);
        REQUIRE(s.field().focused());
        const render::Point back = centre(s.back_button().bounds());
        tap(s, back.x, back.y, kind);
        CHECK_FALSE(s.expanded());
        CHECK_FALSE(s.focused());
    }
}

TEST_CASE("a press on the field of the expanded view keeps following the field when it moves over an icon") {
    widgets::SearchBar s(options(), fonts());
    place(s, phone);
    s.expand();
    settle(s);
    const render::Rect f = s.field_rect();
    const render::Point mic = centre(s.trailing_button()->bounds());
    s.handle(input::PointerEvent{PointerAction::down, f.x + f.width / 2.0, f.y + f.height / 2.0, 1});
    s.handle(input::PointerEvent{PointerAction::move, mic.x, mic.y});
    const auto r = s.handle(input::PointerEvent{PointerAction::up, mic.x, mic.y});
    CHECK_FALSE(r.trailing);
    CHECK(s.field().focused());
}

TEST_CASE("a press outside the docked view collapses it and is left for what lies under it") {
    for (const input::PointerKind kind : {input::PointerKind::mouse, input::PointerKind::touch}) {
        CAPTURE(static_cast<int>(kind));
        widgets::SearchBar s(options(), fonts());
        place(s, desktop);
        REQUIRE_FALSE(s.full_screen());
        s.expand();
        settle(s);
        const render::Rect view = s.surface();
        const double x = view.x / 2.0;
        const double y = view.y + view.height / 2.0;
        const auto down = s.handle(input::PointerEvent{PointerAction::down, x, y, 1, kind});
        CHECK(down.collapsed);
        CHECK_FALSE(down.consumed);
        CHECK_FALSE(s.expanded());
        CHECK_FALSE(s.focused());
        CHECK_FALSE(s.holds_pointer());
        CHECK(s.is_open());
        CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::move, x, y + 80.0, 0, kind}).consumed);
        CHECK_FALSE(s.holds_pointer());
        CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::up, x, y + 80.0, 0, kind}).consumed);
        settle(s);
        CHECK_FALSE(s.is_open());
    }
}

TEST_CASE("a press inside the docked view is its own until the pointer lifts") {
    widgets::SearchBar s(options(), fonts());
    place(s, desktop);
    s.expand();
    settle(s);
    CHECK_FALSE(s.holds_pointer());
    const render::Rect f = s.field_rect();
    const auto down = s.handle(input::PointerEvent{PointerAction::down, f.x + f.width / 2.0, f.y + f.height / 2.0, 1, input::PointerKind::touch});
    CHECK(down.consumed);
    CHECK(s.holds_pointer());
    s.handle(input::PointerEvent{PointerAction::up, f.x + f.width / 2.0, f.y + f.height / 2.0, 0, input::PointerKind::touch});
    CHECK_FALSE(s.holds_pointer());
    CHECK(s.expanded());
}

TEST_CASE("the full-screen view stays open on a press outside its area and leaves the press to what lies there") {
    widgets::SearchBar s(options(), fonts());
    place(s, phone);
    s.expand();
    settle(s);
    const double y = phone.y + phone.height + 40.0;
    const auto down = s.handle(input::PointerEvent{PointerAction::down, 200.0, y, 1, input::PointerKind::touch});
    CHECK_FALSE(down.consumed);
    CHECK_FALSE(down.collapsed);
    CHECK_FALSE(s.holds_pointer());
    CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::up, 200.0, y, 0, input::PointerKind::touch}).consumed);
    CHECK(s.expanded());
    CHECK_FALSE(s.handle(input::WheelEvent{200.0, y, 0.0, 1.0}));
    CHECK(s.handle(input::WheelEvent{200.0, 400.0, 0.0, 1.0}));
}
