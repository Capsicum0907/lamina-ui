#include <doctest/doctest.h>

#include <memory>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/switch.hpp"
#include "restyle_support.hpp"

using namespace lamina;
using namespace lamina::test;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;

namespace {

constexpr render::Point origin{20.0, 20.0};

icons::Icon icon(icons::Symbol s = res::icons::favorite) {
    return {icon_font(), s};
}

template <typename W>
std::unique_ptr<W> placed(std::unique_ptr<W> w) {
    w->set_origin(origin);
    return w;
}

void check_round(const RestyleResult& r) {
    CHECK(r.built_under_default.pixels != r.built_under_test.pixels);
    CHECK(r.switched.bounds.x == r.built_under_test.bounds.x);
    CHECK(r.switched.bounds.y == r.built_under_test.bounds.y);
    CHECK(r.switched.bounds.width == r.built_under_test.bounds.width);
    CHECK(r.switched.bounds.height == r.built_under_test.bounds.height);
    CHECK(r.switched.pixels == r.built_under_test.pixels);
}

}

TEST_CASE("a button built under one style and switched to another paints as one built under the other") {
    check_round(restyle_round([] {
        return placed(std::make_unique<widgets::Button>(widgets::ButtonOptions{"Send", ButtonStyle::filled, ButtonSize::medium, ButtonShape::round}, fonts()));
    }));
}

TEST_CASE("a toggle button with the style's shapes follows a style change") {
    check_round(restyle_round([] {
        widgets::ButtonOptions o{"Bold", ButtonStyle::tonal, ButtonSize::small, ButtonShape::round};
        o.toggle = widgets::ButtonToggle{true};
        return placed(std::make_unique<widgets::Button>(o, fonts()));
    }));
}

TEST_CASE("an icon button follows a style change") {
    check_round(restyle_round([] {
        widgets::IconButtonOptions o{icon(), "Like", tokens::IconButtonStyle::filled, tokens::IconButtonSize::medium, ButtonShape::square};
        return placed(std::make_unique<widgets::IconButton>(o));
    }));
}

TEST_CASE("a checkbox, a switch and a radio button animate with the new style's springs after a style change") {
    check_round(restyle_motion_round([] { return placed(std::make_unique<widgets::Checkbox>(widgets::CheckboxOptions{})); },
                                     [](widgets::Checkbox& w) { w.set_state(tokens::CheckboxState::on); }));
    check_round(restyle_motion_round([] { return placed(std::make_unique<widgets::Switch>(widgets::SwitchOptions{})); },
                                     [](widgets::Switch& w) { w.set_checked(true); }));
    check_round(restyle_motion_round([] { return placed(std::make_unique<widgets::RadioButton>(widgets::RadioButtonOptions{})); },
                                     [](widgets::RadioButton& w) { w.set_selected(true); }));
}

TEST_CASE("a FAB animates its focus with the new style's springs after a style change") {
    check_round(restyle_motion_round([] { return placed(std::make_unique<widgets::Fab>(widgets::FabOptions{icon(), "Compose"})); },
                                     [](widgets::Fab& w) { w.set_focused(true); }));
}

TEST_CASE("a label lays its text out again in the new style's type on its next measure") {
    const widgets::LabelOptions options{"Settings", tokens::TypeRole::title_large};
    auto shot = [](widgets::Label& l) {
        l.measure(layout::Constraints{0.0, static_cast<double>(restyle_canvas_width), 0.0, static_cast<double>(restyle_canvas_height)});
        l.set_origin(origin);
        return shoot(l);
    };
    widgets::Label switched(options, fonts());
    const RestyleShot before = shot(switched);
    RestyleShot after;
    {
        const StyleScope scope(restyle_test_style());
        after = shot(switched);
    }
    RestyleShot fresh;
    {
        const StyleScope scope(restyle_test_style());
        widgets::Label built(options, fonts());
        fresh = shot(built);
    }
    check_round({before, fresh, after});
}
