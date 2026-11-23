#include <doctest/doctest.h>

#include <numbers>

#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/split_button.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using tokens::ButtonSize;
using widgets::SplitButton;
using widgets::SplitButtonOptions;

namespace {

SplitButton make(ButtonSize size = ButtonSize::small, bool with_menu = true, const char* label = "Send") {
    SplitButtonOptions o{label, icons::Icon{icon_font(), res::icons::send}};
    o.size = size;
    o.trailing_name = "More send options";
    if (with_menu) {
        o.menu = {{"Schedule"}, {"Save draft"}, {"Discard"}};
    }
    SplitButton s(std::move(o), fonts(), icon_font());
    s.set_origin({10.0, 10.0});
    s.set_window({800.0, 600.0});
    return s;
}

template <typename W>
void click_on(SplitButton& s, const W& w) {
    const render::Rect r = w.bounds();
    const double x = r.x + r.width / 2.0;
    const double y = r.y + r.height / 2.0;
    s.handle({PointerAction::down, x, y});
    s.handle({PointerAction::up, x, y});
    s.handle({PointerAction::leave, 0.0, 0.0});
}

}

static_assert(widgets::Sized<SplitButton>);
static_assert(widgets::Disableable<SplitButton>);

TEST_CASE("split button tokens follow SplitButton{XSmall,Small,Medium,Large,XLarge}Tokens.kt and SplitButton.kt") {
    const auto& s = tokens::split_button_size(ButtonSize::small);
    CHECK(s.height == 40.0);
    CHECK(s.inner_corner == tokens::CornerSize::extra_small);
    CHECK(s.inner_pressed_corner == tokens::CornerSize::medium);
    CHECK(s.leading_leading_space == 16.0);
    CHECK(s.leading_trailing_space == 12.0);
    CHECK(s.trailing_icon_size == 22.0);
    const auto& l = tokens::split_button_size(ButtonSize::large);
    CHECK(l.inner_corner == tokens::CornerSize::small);
    CHECK(l.inner_pressed_corner == tokens::CornerSize::large_increased);
    CHECK(l.trailing_leading_space == 29.0);
    CHECK(tokens::split_button_size(ButtonSize::extra_large).trailing_icon_size == 50.0);
    CHECK(tokens::split_button_tokens.between_space == 2.0);
    CHECK(tokens::split_button_tokens.min_width == 48.0);
    CHECK(tokens::split_button_tokens.shape_spring.stiffness == tokens::motion_scheme.default_effects().stiffness);
}

TEST_CASE("the two halves sit 2 dp apart with full outer and small inner corners at every size") {
    for (std::size_t i = 0; i < static_cast<std::size_t>(ButtonSize::count); ++i) {
        const auto size = static_cast<ButtonSize>(i);
        auto s = make(size);
        const auto& tk = tokens::split_button_size(size);
        CHECK(s.height() == tk.height);
        CHECK(s.trailing().bounds().x == doctest::Approx(s.leading().bounds().x + s.leading().width() + 2.0));
        const double inner = tokens::corner_radius(tk.inner_corner, 0.0, 0.0);
        CHECK(s.leading().corner_radii().top_left == doctest::Approx(tk.height / 2.0));
        CHECK(s.leading().corner_radii().top_right == doctest::Approx(inner));
        CHECK(s.trailing().corner_radii().top_left == doctest::Approx(inner));
        CHECK(s.trailing().corner_radii().top_right == doctest::Approx(std::min(tk.height, s.trailing().width()) / 2.0));
        CHECK(s.trailing().width() >= 48.0);
    }
}

TEST_CASE("pressing a half widens its inner corners") {
    auto s = make();
    const render::Rect r = s.leading().bounds();
    s.handle({PointerAction::down, r.x + 10.0, r.y + 10.0});
    settle(s);
    CHECK(s.leading().corner_radii().top_right == doctest::Approx(12.0));
    s.cancel_interaction();
    settle(s);
    CHECK(s.leading().corner_radii().top_right == doctest::Approx(4.0));
}

TEST_CASE("checking the trailing half rounds it into a circle, turns the chevron and opens the menu") {
    auto s = make();
    CHECK(s.trailing().content_offset() == doctest::Approx(0.11 * (4.0 - 20.0)));
    click_on(s, s.trailing());
    CHECK(s.checked());
    CHECK(s.menu_open());
    settle(s);
    CHECK(s.trailing().corner_radii().top_left == doctest::Approx(20.0));
    CHECK(s.trailing().content_offset() == doctest::Approx(0.0));
    CHECK(s.chevron_rotation() == doctest::Approx(std::numbers::pi));
    const auto node = s.accessible();
    REQUIRE(node.children.size() >= 2);
    CHECK(node.children[1].value == "Expanded");
    CHECK(node.children[1].state.toggle == widgets::AccessibleToggle::on);
}

TEST_CASE("choosing a menu item closes the menu and unchecks the trailing half") {
    auto s = make();
    click_on(s, s.trailing());
    settle(s);
    const render::Rect item = s.menu()->item_rect(1);
    s.handle({PointerAction::down, item.x + 10.0, item.y + item.height / 2.0});
    const auto r = s.handle({PointerAction::up, item.x + 10.0, item.y + item.height / 2.0});
    REQUIRE(r.chosen);
    CHECK(*r.chosen == 1);
    CHECK_FALSE(s.checked());
    CHECK_FALSE(s.menu_open());
    settle(s);
    CHECK(s.chevron_rotation() == doctest::Approx(0.0));
    CHECK(s.accessible().children[1].value == "Collapsed");
}

TEST_CASE("clicking outside dismisses the menu and unchecks, and clicking the trailing half again closes it") {
    auto s = make();
    click_on(s, s.trailing());
    REQUIRE(s.menu_open());
    s.handle({PointerAction::down, 700.0, 500.0});
    s.handle({PointerAction::up, 700.0, 500.0});
    CHECK_FALSE(s.menu_open());
    CHECK_FALSE(s.checked());
    click_on(s, s.trailing());
    REQUIRE(s.menu_open());
    click_on(s, s.trailing());
    CHECK_FALSE(s.menu_open());
    CHECK_FALSE(s.checked());
}

TEST_CASE("the leading half clicks without touching the trailing half") {
    auto s = make();
    const render::Rect r = s.leading().bounds();
    s.handle({PointerAction::down, r.x + 10.0, r.y + 10.0});
    const auto resp = s.handle({PointerAction::up, r.x + 10.0, r.y + 10.0});
    CHECK(resp.leading_clicked);
    CHECK_FALSE(s.checked());
}

TEST_CASE("the keyboard opens the menu from the trailing half and Escape returns there") {
    auto s = make();
    s.trailing().set_focused(true);
    CHECK(s.handle(input::KeyEvent{Key::enter}).trailing_toggled);
    REQUIRE(s.menu_open());
    CHECK(s.menu()->focused_item() == 0);
    CHECK(s.handle(input::KeyEvent{Key::escape}).consumed);
    CHECK_FALSE(s.menu_open());
    CHECK_FALSE(s.checked());
    CHECK(s.trailing().focused());
}

TEST_CASE("without a menu the trailing half is a plain toggle") {
    auto s = make(ButtonSize::medium, false);
    click_on(s, s.trailing());
    CHECK(s.checked());
    CHECK(s.menu() == nullptr);
    click_on(s, s.trailing());
    CHECK_FALSE(s.checked());
}

TEST_CASE("a disabled split button ignores both halves") {
    auto s = make();
    s.set_enabled(false);
    click_on(s, s.trailing());
    CHECK_FALSE(s.checked());
    CHECK_FALSE(s.accessible().state.enabled);
    CHECK_FALSE(s.accessible().children[0].state.enabled);
}

TEST_CASE("the trailing half needs a name") {
    SplitButtonOptions o{"Send"};
    CHECK_THROWS_AS(SplitButton(std::move(o), fonts(), icon_font()), std::invalid_argument);
}

TEST_CASE("the leading half is named by its label and the group by the split button's name") {
    SplitButtonOptions o{"Send"};
    o.trailing_name = "More";
    o.accessible_name = "Send options";
    SplitButton s(std::move(o), fonts(), icon_font());
    const auto node = s.accessible();
    CHECK(node.name == "Send options");
    CHECK(node.children[0].name == "Send");
}

TEST_CASE("a split button gives the trailing half its width first and shrinks the leading half to the rest") {
    SplitButton s = make(ButtonSize::extra_large, false, "Send to everyone");
    const auto& t = tokens::split_button_tokens;
    const double natural = s.natural_width();
    const double trailing = s.trailing().width();
    CHECK(s.width() == doctest::Approx(natural));
    CHECK(s.measure({0.0, layout::unbounded, 0.0, layout::unbounded}).width == doctest::Approx(natural));
    CHECK(s.measure({0.0, natural + 50.0, 0.0, layout::unbounded}).width == doctest::Approx(natural));
    CHECK(s.measure({0.0, natural - 50.0, 0.0, layout::unbounded}).width == doctest::Approx(natural - 50.0));
    CHECK(s.measure({0.0, 1.0, 0.0, layout::unbounded}).width == doctest::Approx(s.min_width()));

    s.arrange({0.0, 0.0, natural - 50.0, s.height()});
    CHECK(s.width() == doctest::Approx(natural - 50.0));
    CHECK(s.trailing().width() == doctest::Approx(trailing));
    CHECK(s.trailing().bounds().x == doctest::Approx(s.leading().bounds().x + s.leading().width() + t.between_space));
    CHECK(s.trailing().bounds().x + s.trailing().width() == doctest::Approx(natural - 50.0));
    CHECK(s.leading().label_ellipsized());
    const widgets::ButtonPadding pad = s.leading().padding();
    const double icon = s.leading().icon_size() + tokens::button_size(ButtonSize::extra_large).icon_label_space;
    CHECK(s.leading().shown_label().width <= s.leading().width() - pad.leading - pad.trailing - icon + 1e-9);

    s.arrange({0.0, 0.0, natural, s.height()});
    CHECK_FALSE(s.leading().label_ellipsized());
    CHECK(s.width() == doctest::Approx(natural));
}

TEST_CASE("a split button's leading half keeps its paddings and room for an ellipsis at its narrowest") {
    SplitButton s = make(ButtonSize::extra_large, false, "Send to everyone");
    const auto& size = tokens::split_button_size(ButtonSize::extra_large);
    s.set_width(1.0);
    CHECK(s.width() == doctest::Approx(s.min_width()));
    CHECK(s.leading().width() >= tokens::split_button_tokens.min_width);
    const widgets::ButtonPadding pad = s.leading().padding();
    CHECK(pad.leading == size.leading_leading_space);
    CHECK(pad.trailing == size.leading_trailing_space);
    CHECK(s.leading().shown_label().width > 0.0);
}

TEST_CASE("a split button's leading half follows a gradual resize without jumps") {
    SplitButton s = make(ButtonSize::extra_large, false, "Send to everyone");
    const double from = s.natural_width() + 20.0;
    const double to = s.min_width() - 20.0;
    s.arrange({0.0, 0.0, from, s.height()});
    double previous = s.leading().width();
    for (double w = from - 1.0; w >= to; w -= 1.0) {
        s.arrange({0.0, 0.0, w, s.height()});
        CHECK(previous - s.leading().width() <= 1.0 + 1e-9);
        CHECK(s.leading().width() <= previous + 1e-9);
        previous = s.leading().width();
    }
}

TEST_CASE("a button ellipsizes its label only when asked to") {
    widgets::ButtonOptions plain{"A rather long label"};
    widgets::Button b(plain, fonts());
    const double natural = b.width();
    b.set_size(natural - 40.0, b.height());
    CHECK_FALSE(b.label_ellipsized());
    CHECK(b.min_width() == doctest::Approx(b.intrinsic_width()));

    widgets::ButtonOptions opt{"A rather long label"};
    opt.ellipsize_label = true;
    widgets::Button e(opt, fonts());
    CHECK(e.min_width() < e.intrinsic_width());
    e.set_size(natural - 40.0, e.height());
    CHECK(e.label_ellipsized());
    CHECK(e.shown_label().width <= natural - 40.0 - e.padding().leading - e.padding().trailing + 1e-9);
    e.set_size(natural, e.height());
    CHECK_FALSE(e.label_ellipsized());
}

TEST_CASE("the menu opens under the trailing half, start-aligned with it, and end-aligned with it when there is no room to the right") {
    auto s = make(ButtonSize::large);
    click_on(s, s.trailing());
    settle(s);
    REQUIRE(s.menu_open());
    const render::Rect trail = s.trailing().bounds();
    render::Rect menu = s.menu()->bounds();
    CHECK(menu.x == doctest::Approx(trail.x));
    CHECK(menu.y == doctest::Approx(trail.y + trail.height));
    click_on(s, s.trailing());
    REQUIRE_FALSE(s.menu_open());
    s.set_origin({800.0 - s.width() - 10.0, 10.0});
    click_on(s, s.trailing());
    settle(s);
    const render::Rect moved = s.trailing().bounds();
    menu = s.menu()->bounds();
    CHECK(menu.x + menu.width == doctest::Approx(moved.x + moved.width));
    CHECK(menu.y == doctest::Approx(moved.y + moved.height));
}

TEST_CASE("without room below, the menu opens above the trailing half and leaves the split button uncovered") {
    const layout::Size phone{1080.0 / 2.625, 2400.0 / 2.625};
    auto s = make(ButtonSize::extra_large);
    s.set_window(phone);
    s.arrange({16.0, phone.height - 24.0 - s.height() - 100.0, phone.width - 32.0, s.height()});
    click_on(s, s.trailing());
    settle(s);
    REQUIRE(s.menu_open());
    const render::Rect trail = s.trailing().bounds();
    const render::Rect menu = s.menu()->bounds();
    CHECK(trail.y + trail.height + menu.height > phone.height - tokens::menu_tokens.window_vertical_margin);
    CHECK(menu.y + menu.height == doctest::Approx(trail.y));
    CHECK(menu.y + menu.height <= s.bounds().y + 1e-9);
    CHECK(menu.x == doctest::Approx(trail.x));
}
