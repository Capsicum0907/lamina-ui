#include <doctest/doctest.h>

#include "lamina/layout/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/grouped_menu.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using namespace lamina::widgets;

template <typename W>
constexpr bool interactive = Paintable<W> && Steppable<W> && HandlesPointer<W>;

static_assert(interactive<Button> && Sized<Button> && SizeSettable<Button> && !WidthSettable<Button> && !Measurable<Button>);
static_assert(interactive<IconButton> && Sized<IconButton> && !WidthSettable<IconButton> && !Measurable<IconButton>);
static_assert(interactive<Fab> && Sized<Fab> && !WidthSettable<Fab> && !Measurable<Fab>);
static_assert(interactive<Switch> && Sized<Switch> && !WidthSettable<Switch> && !Measurable<Switch>);
static_assert(interactive<Checkbox> && Sized<Checkbox> && !WidthSettable<Checkbox> && !Measurable<Checkbox>);
static_assert(interactive<RadioButton> && Sized<RadioButton> && !WidthSettable<RadioButton> && !Measurable<RadioButton>);
static_assert(interactive<Slider> && HandlesKeys<Slider> && WidthSettable<Slider> && !Measurable<Slider>);
static_assert(interactive<TabRow> && WidthSettable<TabRow> && !Measurable<TabRow>);
static_assert(interactive<NavigationBar> && WidthSettable<NavigationBar> && !Measurable<NavigationBar>);
static_assert(interactive<NavigationRail> && Measurable<NavigationRail> && !Sized<NavigationRail>);
static_assert(interactive<TextField> && HandlesKeys<TextField> && HandlesText<TextField> && Measurable<TextField> && WidthSettable<TextField>);
static_assert(Paintable<Label> && Measurable<Label> && !Steppable<Label> && !HandlesPointer<Label> && !Cancellable<Label>);
static_assert(interactive<Menu> && HandlesKeys<Menu> && Cancellable<Menu> && !Placeable<Menu>);
static_assert(interactive<GroupedMenu> && HandlesKeys<GroupedMenu> && Cancellable<GroupedMenu> && !Placeable<GroupedMenu>);
static_assert(interactive<Dialog> && HandlesKeys<Dialog> && !HandlesText<Dialog> && !Placeable<Dialog>);
static_assert(Cancellable<Button> && Cancellable<IconButton> && Cancellable<Fab> && Cancellable<Switch> && Cancellable<Checkbox> &&
              Cancellable<RadioButton> && Cancellable<Slider> && Cancellable<TabRow> && Cancellable<NavigationBar> &&
              Cancellable<NavigationRail> && Cancellable<TextField> && Cancellable<Dialog> && Cancellable<WidgetSet>);
static_assert(HandlesKeys<Button> && !HandlesText<Button> && !HandlesText<Slider>);
static_assert(Focusable<Button> && Focusable<IconButton> && Focusable<Fab> && Focusable<Switch> && Focusable<Checkbox> &&
              Focusable<RadioButton> && Focusable<TextField> && Focusable<Slider> && Focusable<TabRow> &&
              Focusable<NavigationBar> && Focusable<NavigationRail>);
static_assert(HandlesKeys<IconButton> && HandlesKeys<Fab> && HandlesKeys<Switch> && HandlesKeys<Checkbox> && HandlesKeys<RadioButton>);
static_assert(!Focusable<Label> && !Focusable<Dialog>);

namespace {

layout::NodePtr in_column(layout::NodePtr child, layout::CrossAlign align) {
    child->self_align = align;
    return layout::column({child});
}

}

TEST_CASE("node centers a sized widget vertically in the space it is given") {
    Button b({"OK"}, fonts());
    auto n = node(b);
    n->self_align = layout::CrossAlign::stretch;
    auto r = layout::row({n});
    r->fill_cross(true);
    layout::run(*r, {10.0, 20.0, 400.0, 200.0});
    CHECK(b.bounds().x == doctest::Approx(10.0));
    CHECK(b.bounds().y == doctest::Approx(20.0 + (200.0 - b.height()) / 2.0));
    CHECK(b.bounds().width == doctest::Approx(b.width()));
}

TEST_CASE("node gives a widget with set_width the whole width") {
    Slider s;
    auto c = in_column(node(s), layout::CrossAlign::start);
    layout::run(*c, {0.0, 0.0, 300.0, 100.0});
    CHECK(s.width() == doctest::Approx(300.0));
    CHECK(s.bounds().y == doctest::Approx(0.0));
}

TEST_CASE("node lets a text field keep its own width at its top left, even when stretched") {
    TextField f({.label = "Name"}, fonts());
    const double own = f.width();
    auto c = in_column(node(f), layout::CrossAlign::stretch);
    layout::run(*c, {5.0, 7.0, own + 200.0, 300.0});
    CHECK(f.width() == doctest::Approx(own));
    CHECK(f.bounds().x == doctest::Approx(5.0));
    CHECK(f.bounds().y == doctest::Approx(7.0));
}

TEST_CASE("node measures and arranges a label through its own measure and arrange") {
    Label l({"Some words that wrap", tokens::TypeRole::body_large, color::Role::on_surface, true}, fonts());
    auto c = in_column(node(l), layout::CrossAlign::start);
    layout::run(*c, {3.0, 4.0, 60.0, 300.0});
    CHECK(l.bounds().x == doctest::Approx(3.0));
    CHECK(l.bounds().y == doctest::Approx(4.0));
    CHECK(l.bounds().width <= 60.0);
    CHECK(l.bounds().height > 0.0);
}

TEST_CASE("a widget set paints a label among stepped widgets and does not count it as animating") {
    Label l({"Label"}, fonts());
    Button b({"OK"}, fonts());
    WidgetSet set;
    set.add(l);
    set.add(b);
    settle(set);
    CHECK_FALSE(set.animating());
}

TEST_CASE("filled_node gives a size-settable widget the whole cell it is placed in") {
    Button a({"1"}, fonts());
    Button b({"2"}, fonts());
    auto r = layout::row({layout::weighted(filled_node(a), 1.0), layout::weighted(filled_node(b), 1.0)});
    r->spacing(10.0).cross_align(layout::CrossAlign::stretch).fill_cross(true);
    layout::run(*r, {5.0, 6.0, 210.0, 70.0});
    CHECK(a.bounds().x == doctest::Approx(5.0));
    CHECK(a.bounds().y == doctest::Approx(6.0));
    CHECK(a.width() == doctest::Approx(100.0));
    CHECK(a.height() == doctest::Approx(70.0));
    CHECK(b.bounds().x == doctest::Approx(115.0));
    CHECK(b.width() == doctest::Approx(100.0));
}
