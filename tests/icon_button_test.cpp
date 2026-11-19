#include <doctest/doctest.h>

#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina_thorvg/canvas.hpp"

using namespace lamina;
using input::PointerAction;
using tokens::IconButtonSize;
using tokens::IconButtonStyle;
using tokens::IconButtonWidth;

namespace {

const icons::IconFont& icon_font() {
    static const icons::IconFont font = icons::IconFont::from_directory(LAMINA_ICONS_DIR);
    return font;
}

icons::Icon icon(icons::Symbol s = icons::Symbol::favorite) {
    return {icon_font(), s};
}

template <typename W>
void settle(W& w) {
    for (int i = 0; i < 240; ++i) {
        w.step(1.0 / 60.0);
    }
}

}

TEST_CASE("icon button sizes follow the size tokens") {
    CHECK(tokens::icon_button_size(IconButtonSize::extra_small).height == 32.0);
    CHECK(tokens::icon_button_size(IconButtonSize::small).height == 40.0);
    CHECK(tokens::icon_button_size(IconButtonSize::medium).height == 56.0);
    CHECK(tokens::icon_button_size(IconButtonSize::large).height == 96.0);
    CHECK(tokens::icon_button_size(IconButtonSize::extra_large).height == 136.0);
    CHECK(tokens::icon_button_size(IconButtonSize::large).icon_size == 32.0);
    CHECK(tokens::icon_button_width(IconButtonSize::small, IconButtonWidth::narrow) == 32.0);
    CHECK(tokens::icon_button_width(IconButtonSize::small, IconButtonWidth::wide) == 52.0);
    CHECK(tokens::icon_button_width(IconButtonSize::extra_large, IconButtonWidth::wide) == 184.0);
}

TEST_CASE("a uniform icon button is square at every size") {
    for (auto size : {IconButtonSize::extra_small, IconButtonSize::small, IconButtonSize::medium, IconButtonSize::large,
                      IconButtonSize::extra_large}) {
        widgets::IconButton b({icon(), IconButtonStyle::filled, size});
        CHECK(b.width() == b.height());
    }
}

TEST_CASE("icon button colors follow the token files") {
    CHECK(tokens::icon_button_style(IconButtonStyle::filled).plain.container == color::Role::primary);
    CHECK(tokens::icon_button_style(IconButtonStyle::filled).unchecked.container == color::Role::surface_container);
    CHECK(tokens::icon_button_style(IconButtonStyle::outlined).checked.container == color::Role::inverse_surface);
    CHECK_FALSE(tokens::icon_button_style(IconButtonStyle::outlined).checked.outline.has_value());
    CHECK(tokens::icon_button_style(IconButtonStyle::standard).checked.content == color::Role::primary);
}

TEST_CASE("a toggleable icon button flips on click, a plain one does not") {
    widgets::IconButton toggle({icon(), IconButtonStyle::tonal, IconButtonSize::small, tokens::ButtonShape::round,
                                IconButtonWidth::uniform, true});
    toggle.set_origin({10.0, 10.0});
    CHECK_FALSE(toggle.checked());
    toggle.handle({PointerAction::down, 30.0, 30.0});
    CHECK(toggle.handle({PointerAction::up, 30.0, 30.0}));
    CHECK(toggle.checked());
    toggle.handle({PointerAction::down, 30.0, 30.0});
    toggle.handle({PointerAction::up, 30.0, 30.0});
    CHECK_FALSE(toggle.checked());

    widgets::IconButton plain({icon(), IconButtonStyle::filled});
    plain.set_origin({10.0, 10.0});
    plain.handle({PointerAction::down, 30.0, 30.0});
    CHECK(plain.handle({PointerAction::up, 30.0, 30.0}));
    CHECK_FALSE(plain.checked());
}

TEST_CASE("an icon button reaches the 48 dp touch target") {
    widgets::IconButton b({icon(), IconButtonStyle::filled, IconButtonSize::small});
    b.set_origin({10.0, 10.0});
    b.handle({PointerAction::down, 7.0, 30.0});
    CHECK(b.handle({PointerAction::up, 7.0, 30.0}));
}

TEST_CASE("pressing a round icon button squares its corners") {
    widgets::IconButton b({icon(), IconButtonStyle::filled, IconButtonSize::medium});
    b.set_origin({0.0, 0.0});
    thorvg::Canvas canvas(60, 60);
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme);
    CHECK(canvas.pixel(3, 3) == 0xffffffff);
    CHECK(canvas.pixel(28, 4) != 0xffffffff);
    b.handle({PointerAction::down, 28.0, 28.0});
    settle(b);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme);
    CHECK(canvas.pixel(3, 3) != 0xffffffff);
}

TEST_CASE("the icon of an icon button is drawn in the content color") {
    widgets::IconButton b({icon(icons::Symbol::add), IconButtonStyle::standard, IconButtonSize::medium});
    thorvg::Canvas canvas(56, 56);
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme);
    CHECK(canvas.pixel(28, 28) == scheme[color::Role::on_surface_variant]);
    CHECK(canvas.pixel(4, 28) == 0xffffffff);
}

TEST_CASE("FAB sizes and the Compose overrides") {
    CHECK(tokens::fab_size(tokens::FabSize::baseline).size == 56.0);
    CHECK(tokens::fab_size(tokens::FabSize::medium).size == 80.0);
    CHECK(tokens::fab_size(tokens::FabSize::large).size == 96.0);
    CHECK(tokens::fab_size(tokens::FabSize::medium).corner == tokens::CornerSize::large_increased);
    CHECK(tokens::fab_size(tokens::FabSize::large).icon_size == 36.0);
    CHECK(tokens::fab_style(tokens::FabStyle::primary_container).elevation == 3);
    CHECK(tokens::fab_style(tokens::FabStyle::primary_container).hovered_elevation == 4);
}

TEST_CASE("a FAB clicks and paints its container") {
    widgets::Fab fab({icon(icons::Symbol::edit)});
    fab.set_origin({20.0, 20.0});
    fab.handle({PointerAction::down, 40.0, 40.0});
    CHECK(fab.handle({PointerAction::up, 40.0, 40.0}));
    fab.handle({PointerAction::leave, 0.0, 0.0});
    settle(fab);
    thorvg::Canvas canvas(100, 100);
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    canvas.clear(0xffffffff);
    fab.paint(canvas, scheme);
    CHECK(canvas.pixel(25, 48) == scheme[color::Role::primary_container]);
}
