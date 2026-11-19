#include <doctest/doctest.h>

#include <string>
#include <type_traits>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/grouped_menu.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;

static_assert(std::is_copy_assignable_v<widgets::Button>);
static_assert(std::is_copy_assignable_v<widgets::GroupedMenu>);
static_assert(std::is_copy_assignable_v<widgets::Label>);
static_assert(std::is_copy_assignable_v<widgets::Menu>);
static_assert(std::is_copy_assignable_v<widgets::NavigationBar>);
static_assert(std::is_copy_assignable_v<widgets::NavigationRail>);
static_assert(std::is_copy_assignable_v<widgets::TabRow>);
static_assert(std::is_copy_assignable_v<widgets::TextField>);

TEST_CASE("a widget keeps its fonts after the stack it was built from is gone") {
    const auto build = [] {
        const text::FontStack temporary({text::Font::from_file(std::string(LAMINA_TEST_FONTS_DIR) + "/lamina-test-sans.ttf")});
        return widgets::Label("Fonts", tokens::TypeRole::body_large, color::Role::on_surface, temporary, false);
    };
    widgets::Label owned = build();
    widgets::Label shared("Fonts", tokens::TypeRole::body_large, color::Role::on_surface, fonts(), false);
    owned.measure({0.0, 200.0, 0.0, 80.0});
    shared.measure({0.0, 200.0, 0.0, 80.0});
    thorvg::Canvas a(200, 80);
    thorvg::Canvas b(200, 80);
    a.clear(scheme()[color::Role::surface]);
    b.clear(scheme()[color::Role::surface]);
    owned.paint(a, scheme());
    shared.paint(b, scheme());
    CHECK(count_color(a, scheme()[color::Role::on_surface]) > 0);
    int differing = 0;
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            differing += a.pixel(x, y) != b.pixel(x, y) ? 1 : 0;
        }
    }
    CHECK(differing == 0);
}
