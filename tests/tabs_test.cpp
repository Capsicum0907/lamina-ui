#include <doctest/doctest.h>

#include <string>

#include "lamina/widgets/tabs.hpp"
#include "lamina_thorvg/canvas.hpp"

using namespace lamina;
using platform::PointerAction;
using tokens::TabStyle;

namespace {

const text::FontStack& fonts() {
    static const text::FontStack stack({text::Font::from_file(std::string(LAMINA_TEST_FONTS_DIR) + "/lamina-test-sans.ttf")});
    return stack;
}

const icons::IconFont& icon_font() {
    static const icons::IconFont font = icons::IconFont::from_directory(LAMINA_ICONS_DIR);
    return font;
}

const color::ColorScheme& scheme() {
    static const color::ColorScheme s = color::spec_2021().build(color::default_scheme_options);
    return s;
}

void settle(widgets::TabRow& row) {
    for (int i = 0; i < 240; ++i) {
        row.step(1.0 / 60.0);
    }
}

bool click(widgets::TabRow& row, double x, double y) {
    row.handle({PointerAction::down, x, y});
    const bool changed = row.handle({PointerAction::up, x, y});
    row.handle({PointerAction::leave, 0.0, 0.0});
    return changed;
}

widgets::TabRow make(TabStyle style, bool icons = false) {
    std::vector<widgets::TabItem> items;
    for (const char* label : {"One", "Two", "Three"}) {
        widgets::TabItem item{label};
        if (icons) {
            item.icon = icons::Icon{icon_font(), icons::Symbol::home};
        }
        items.push_back(item);
    }
    widgets::TabRow row({items, style}, fonts());
    row.set_width(300.0);
    return row;
}

}

TEST_CASE("tab tokens follow the navigation tab tokens and Tab.kt") {
    const auto& t = tokens::tab_tokens;
    CHECK(t.height == 48.0);
    CHECK(t.icon_and_label_height == 72.0);
    CHECK(t.horizontal_text_padding == 16.0);
    CHECK(t.label == tokens::TypeRole::title_small);
    CHECK(t.divider == color::Role::outline_variant);
    const auto& primary = tokens::tab_style(TabStyle::primary);
    CHECK(primary.active_content == color::Role::primary);
    CHECK(primary.inactive_content == color::Role::on_surface_variant);
    CHECK(primary.indicator_height == 3.0);
    CHECK(primary.indicator_matches_content);
    const auto& secondary = tokens::tab_style(TabStyle::secondary);
    CHECK(secondary.active_content == color::Role::on_surface);
    CHECK_FALSE(secondary.indicator_matches_content);
}

TEST_CASE("a tab row is 48 dp high, or 72 dp with icons above the labels") {
    CHECK(make(TabStyle::primary).height() == 48.0);
    CHECK(make(TabStyle::primary, true).height() == 72.0);
    std::vector<widgets::TabItem> items{{"A", icons::Icon{icon_font(), icons::Symbol::home}}};
    widgets::TabRow leading({items, TabStyle::primary, 0, widgets::TabIconPosition::leading}, fonts());
    CHECK(leading.height() == 48.0);
}

TEST_CASE("clicking a tab selects it and the indicator slides there with a spring") {
    auto row = make(TabStyle::secondary);
    CHECK(row.selected() == 0);
    CHECK(row.indicator_rect().x == doctest::Approx(0.0));
    CHECK(row.indicator_rect().width == doctest::Approx(100.0));
    CHECK(click(row, 250.0, 24.0));
    CHECK(row.selected() == 2);
    CHECK(row.animating());
    row.step(1.0 / 60.0);
    const double moving = row.indicator_rect().x;
    CHECK(moving > 0.0);
    CHECK(moving < 200.0);
    settle(row);
    CHECK(row.indicator_rect().x == doctest::Approx(200.0));
    CHECK(row.selection(2) == doctest::Approx(1.0));
    CHECK(row.selection(0) == doctest::Approx(0.0));
    CHECK_FALSE(click(row, 250.0, 24.0));
    CHECK_FALSE(click(row, 150.0, 100.0));
    CHECK(row.selected() == 2);
}

TEST_CASE("the primary indicator matches the label width, at least 24 dp, centered on the tab") {
    auto row = make(TabStyle::primary);
    const auto label = text::layout_line(fonts(), "Two", text::text_style(tokens::type_style(tokens::TypeRole::title_small)));
    row.select(1);
    settle(row);
    const render::Rect r = row.indicator_rect();
    CHECK(r.width == doctest::Approx(std::max(label.width, 24.0)));
    CHECK(r.x + r.width / 2.0 == doctest::Approx(150.0));
    CHECK(r.y == doctest::Approx(45.0));
}

TEST_CASE("the first layout places the indicator without animating") {
    std::vector<widgets::TabItem> items{{"A"}, {"B"}};
    widgets::TabRow row({items, TabStyle::secondary, 1}, fonts());
    row.set_width(200.0);
    CHECK_FALSE(row.animating());
    CHECK(row.indicator_rect().x == doctest::Approx(100.0));
}

TEST_CASE("the tab row paints the indicator, the divider and the selected label color") {
    auto row = make(TabStyle::secondary);
    render::ThorVGCanvas canvas(300, 48);
    canvas.clear(0xffffffff);
    row.paint(canvas, scheme());
    CHECK(canvas.pixel(50, 46) == scheme()[color::Role::primary]);
    CHECK(canvas.pixel(150, 47) == scheme()[color::Role::outline_variant]);
    CHECK(canvas.pixel(150, 4) == scheme()[color::Role::surface]);
}
