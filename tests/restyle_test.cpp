#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/button_group.hpp"
#include "lamina/widgets/card.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/chip.hpp"
#include "lamina/widgets/chip_group.hpp"
#include "lamina/widgets/extended_fab.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/list_view.hpp"
#include "lamina/widgets/modal_navigation_rail.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_drawer.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/navigation_suite.hpp"
#include "lamina/widgets/pager.hpp"
#include "lamina/widgets/pull_to_refresh.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/segmented_button.hpp"
#include "lamina/widgets/side_navigation.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/touch_scroll.hpp"
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

void save_round(const RestyleResult& r, const std::string& name) {
    thorvg::Canvas canvas(restyle_canvas_width, restyle_canvas_height * 3);
    const RestyleShot* shots[] = {&r.built_under_default, &r.switched, &r.built_under_test};
    for (int i = 0; i < 3; ++i) {
        for (int y = 0; y < restyle_canvas_height; ++y) {
            for (int x = 0; x < restyle_canvas_width; ++x) {
                canvas.fill_rect({static_cast<double>(x), static_cast<double>(y + i * restyle_canvas_height), 1.0, 1.0},
                                 shots[i]->pixels[static_cast<std::size_t>(y * restyle_canvas_width + x)]);
            }
        }
    }
    save_shot(canvas, name);
}

void check_round(const RestyleResult& r) {
    CHECK((r.built_under_default.pixels != r.built_under_test.pixels));
    CHECK(r.switched.bounds.x == r.built_under_test.bounds.x);
    CHECK(r.switched.bounds.y == r.built_under_test.bounds.y);
    CHECK(r.switched.bounds.width == r.built_under_test.bounds.width);
    CHECK(r.switched.bounds.height == r.built_under_test.bounds.height);
    CHECK((r.switched.pixels == r.built_under_test.pixels));
}

}

TEST_CASE("a button built under one style and switched to another paints as one built under the other") {
    const RestyleResult r = restyle_round([] {
        return placed(std::make_unique<widgets::Button>(widgets::ButtonOptions{"Send", ButtonStyle::filled, ButtonSize::medium, ButtonShape::round}, fonts()));
    });
    save_round(r, "restyle_button");
    check_round(r);
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

TEST_CASE("a list view measures its rows again and its rows lay out their text in the new style after a style change") {
    const RestyleResult r = restyle_round([] {
        std::vector<widgets::ListItemSpec> items{{"Inbox", "Three new messages", "", {}, {}, widgets::ListItemAction::select, true},
                                                 {"Starred", "", "Saved", {}, {widgets::ListSlotKind::checkbox}, widgets::ListItemAction::toggle},
                                                 {"Sent"}};
        auto list = std::make_unique<widgets::ListView>(widgets::ListViewOptions{items, true}, fonts());
        list->set_viewport({0.0, 0.0, static_cast<double>(restyle_canvas_width), static_cast<double>(restyle_canvas_height)});
        return list;
    });
    save_round(r, "restyle_list");
    check_round(r);
}

struct RelaidGroup {
    std::unique_ptr<widgets::ButtonGroup> g;
    void relayout() {
        const layout::Size size = g->measure({0.0, static_cast<double>(restyle_canvas_width), 0.0, static_cast<double>(restyle_canvas_height)});
        g->arrange({origin.x, origin.y, size.width, size.height});
    }
    void step(double dt) {
        g->step(dt);
        relayout();
    }
    render::Rect bounds() const { return g->bounds(); }
    void paint(render::Canvas& c, const color::ColorScheme& s) const { g->paint(c, s); }
};

TEST_CASE("a connected button group laid out again by its owner follows a style change") {
    const RestyleResult r = restyle_round([] {
        widgets::ButtonGroupOptions o{{{"Day"}, {"Week"}, {"Month"}}, widgets::ButtonGroupSelection::single, true};
        o.accessible_name = "Range";
        auto w = std::make_unique<RelaidGroup>(RelaidGroup{std::make_unique<widgets::ButtonGroup>(std::move(o), fonts(), icon_font())});
        w->relayout();
        return w;
    });
    save_round(r, "restyle_button_group");
    check_round(r);
}

TEST_CASE("an extended FAB lays out its label in the new style's type and collapses with the new style's springs") {
    auto make = [] { return placed(std::make_unique<widgets::ExtendedFab>(widgets::ExtendedFabOptions{"Compose", icon(res::icons::edit)}, fonts())); };
    const RestyleResult r = restyle_round(make);
    save_round(r, "restyle_extended_fab");
    check_round(r);
    check_round(restyle_motion_round(make, [](widgets::ExtendedFab& w) { w.set_expanded(false); }));
}

struct RelaidSegments {
    std::unique_ptr<widgets::SegmentedButtonRow> row;
    void relayout() {
        const layout::Size size = row->measure({0.0, static_cast<double>(restyle_canvas_width) - 2.0 * origin.x, 0.0, static_cast<double>(restyle_canvas_height)});
        row->arrange({origin.x, origin.y, size.width, size.height});
    }
    void step(double dt) {
        row->step(dt);
        relayout();
    }
    render::Rect bounds() const { return row->bounds(); }
    void paint(render::Canvas& c, const color::ColorScheme& s) const { row->paint(c, s); }
};

TEST_CASE("a segmented button row laid out again by its owner follows a style change") {
    auto make = [] {
        widgets::SegmentedButtonRowOptions o{{{"Day", std::nullopt, true}, {"Week"}, {"Month"}}};
        auto w = std::make_unique<RelaidSegments>(RelaidSegments{std::make_unique<widgets::SegmentedButtonRow>(std::move(o), fonts(), icon_font())});
        w->relayout();
        return w;
    };
    const RestyleResult r = restyle_round(make);
    save_round(r, "restyle_segmented_button");
    check_round(r);
    check_round(restyle_motion_round(make, [](RelaidSegments& w) { w.row->set_checked(1, true); }));
}

TEST_CASE("a chip lays out its label in the new style's type and animates with the new style's springs after a style change") {
    auto make = [] {
        widgets::ChipOptions o{"Vegetarian", tokens::ChipKind::filter};
        o.leading_icon = icon();
        return placed(std::make_unique<widgets::Chip>(o, fonts()));
    };
    const RestyleResult r = restyle_round(make);
    save_round(r, "restyle_chip");
    check_round(r);
    check_round(restyle_motion_round(make, [](widgets::Chip& w) { w.set_selected(true); }));
}

TEST_CASE("a chip group flows its chips again after a style change") {
    const RestyleResult r = restyle_round([] {
        widgets::ChipGroupOptions o{{{"Breakfast", tokens::ChipKind::filter}, {"Lunch", tokens::ChipKind::filter}, {"Dinner", tokens::ChipKind::filter},
                                     {"Snacks", tokens::ChipKind::filter}}};
        auto g = std::make_unique<widgets::ChipGroup>(std::move(o), fonts());
        const double w = restyle_canvas_width - 2.0 * origin.x;
        const layout::Size size = g->measure({0.0, w, 0.0, static_cast<double>(restyle_canvas_height)});
        g->arrange({origin.x, origin.y, w, size.height});
        return g;
    });
    save_round(r, "restyle_chip_group");
    check_round(r);
}

TEST_CASE("a clickable card animates its focus with the new style's springs after a style change") {
    auto make = [] {
        auto c = std::make_unique<widgets::Card>(widgets::CardOptions{tokens::CardStyle::outlined, true, true, nullptr, nullptr, "Trip"});
        c->arrange({origin.x, origin.y, 200.0, 100.0});
        return c;
    };
    const RestyleResult r = restyle_round(make);
    save_round(r, "restyle_card");
    check_round(r);
    check_round(restyle_motion_round(make, [](widgets::Card& w) { w.set_focused(true); }));
}

TEST_CASE("a tab row lays out its labels in the new style's type and moves its indicator to the new width after a style change") {
    auto make = [](tokens::TabStyle style) {
        widgets::TabRowOptions o{{{"Flights"}, {"Hotels"}, {"Explore"}}, style};
        o.selected = 1;
        auto row = std::make_unique<widgets::TabRow>(std::move(o), fonts());
        row->set_origin(origin);
        row->set_width(restyle_canvas_width - 2.0 * origin.x);
        return row;
    };
    const RestyleResult r = restyle_round([&] { return make(tokens::TabStyle::primary); });
    save_round(r, "restyle_tabs");
    check_round(r);
    check_round(restyle_motion_round([&] { return make(tokens::TabStyle::secondary); }, [](widgets::TabRow& w) { w.select(2); }));
}

TEST_CASE("a pager and a pull to refresh without their own tokens read the active style's") {
    Style wide = m3_expressive_style();
    wide.pager.page_spacing += restyle_length_add;
    wide.pull_to_refresh.container_size += restyle_length_add;
    widgets::Pager pager(widgets::PagerOptions{3});
    pager.set_bounds({0.0, 0.0, 200.0, 100.0});
    widgets::PullToRefresh pull;
    pull.set_area({0.0, 0.0, 200.0, 100.0});
    const double pitch = pager.page_pitch();
    const double size = pull.container_rect().width;
    {
        const StyleScope scope(wide);
        pager.step(frame_seconds);
        pull.step(frame_seconds);
        CHECK(pager.page_pitch() == doctest::Approx(pitch + restyle_length_add));
        CHECK(pull.container_rect().width == doctest::Approx(size + restyle_length_add));
    }
    CHECK(pager.page_pitch() == doctest::Approx(pitch));
}

TEST_CASE("the fling decay follows the active style and a fling given another decay goes on from where it is") {
    Style slippery = m3_expressive_style();
    slippery.gesture.fling_decay.friction *= restyle_corner_scale;
    const motion::SplineDecay& first = widgets::fling_decay();
    const motion::SplineDecay* other = nullptr;
    {
        const StyleScope scope(slippery);
        other = &widgets::fling_decay();
        CHECK(&widgets::fling_decay() == other);
    }
    CHECK(other != &first);
    CHECK(&widgets::fling_decay() == &first);
    CHECK(other->distance(1000.0) > first.distance(1000.0));

    motion::Fling fling(first);
    fling.start(0.0, 1000.0);
    fling.step(frame_seconds);
    const double value = fling.value();
    const double velocity = fling.velocity();
    fling.set_decay(*other);
    CHECK(fling.active());
    CHECK(fling.value() == value);
    CHECK(fling.velocity() == doctest::Approx(velocity).epsilon(restyle_velocity_tolerance));
}

namespace {

std::vector<widgets::NavigationItem> navigation_items() {
    std::vector<widgets::NavigationItem> items;
    for (auto [symbol, label] : {std::pair{res::icons::home, "Home"}, std::pair{res::icons::search, "Search"}, std::pair{res::icons::settings, "Settings"}}) {
        items.push_back({icon(symbol), label});
    }
    return items;
}

constexpr render::Rect restyle_area{0.0, 0.0, static_cast<double>(restyle_canvas_width), static_cast<double>(restyle_canvas_height)};

template <typename W>
struct InArea {
    std::unique_ptr<W> w;
    void step(double dt) { w->step(dt); }
    render::Rect bounds() const { return restyle_area; }
    void paint(render::Canvas& c, const color::ColorScheme& s) const { w->paint(c, s); }
};

template <typename W>
std::unique_ptr<InArea<W>> in_area(std::unique_ptr<W> w) {
    return std::make_unique<InArea<W>>(InArea<W>{std::move(w)});
}

}

TEST_CASE("a navigation bar lays out its labels in the new style's type after a style change") {
    const RestyleResult r = restyle_round([] {
        auto bar = std::make_unique<widgets::NavigationBar>(widgets::NavigationBarOptions{navigation_items(), 1}, fonts());
        bar->set_width(restyle_canvas_width);
        bar->set_origin({0.0, 20.0});
        return bar;
    });
    save_round(r, "restyle_navigation_bar");
    check_round(r);
    check_round(restyle_motion_round(
        [] {
            auto bar = std::make_unique<widgets::NavigationBar>(widgets::NavigationBarOptions{navigation_items(), 1}, fonts());
            bar->set_width(restyle_canvas_width);
            return bar;
        },
        [](widgets::NavigationBar& w) { w.select(2); }));
}

TEST_CASE("an expanded navigation rail sizes itself again after a style change") {
    const RestyleResult r = restyle_round([] {
        widgets::NavigationRailOptions o{navigation_items(), 1, true};
        auto rail = std::make_unique<widgets::NavigationRail>(std::move(o), fonts());
        rail->measure({0.0, static_cast<double>(restyle_canvas_width)});
        rail->arrange({0.0, 0.0, rail->width(), static_cast<double>(restyle_canvas_height)});
        return rail;
    });
    save_round(r, "restyle_navigation_rail");
    check_round(r);
}

TEST_CASE("an expanded side navigation sizes itself again after a style change") {
    const RestyleResult r = restyle_round([] {
        auto side = std::make_unique<widgets::SideNavigation>(widgets::SideNavigationOptions{navigation_items(), 1, true}, fonts());
        side->measure({0.0, static_cast<double>(restyle_canvas_width), 0.0, static_cast<double>(restyle_canvas_height)});
        side->arrange(restyle_area);
        return side;
    });
    save_round(r, "restyle_side_navigation");
    check_round(r);
    check_round(restyle_motion_round(
        [] {
            auto side = std::make_unique<widgets::SideNavigation>(widgets::SideNavigationOptions{navigation_items(), 1, true}, fonts());
            side->measure({0.0, static_cast<double>(restyle_canvas_width), 0.0, static_cast<double>(restyle_canvas_height)});
            side->arrange(restyle_area);
            return side;
        },
        [](widgets::SideNavigation& w) { w.set_expanded(false); }));
}

TEST_CASE("an open modal navigation drawer lays its rows out again after a style change") {
    const RestyleResult r = restyle_round([] {
        widgets::NavigationDrawerOptions o{{{widgets::drawer_headline("Mail"), {widgets::DrawerEntryKind::item, "Inbox"}, {widgets::DrawerEntryKind::item, "Sent"}}, 1}};
        o.initial = widgets::DrawerValue::open;
        auto drawer = std::make_unique<widgets::ModalNavigationDrawer>(std::move(o), fonts());
        drawer->set_area(restyle_area);
        return in_area(std::move(drawer));
    });
    save_round(r, "restyle_navigation_drawer");
    check_round(r);
}

TEST_CASE("a modal navigation rail and a navigation suite lay out their navigation again after a style change") {
    check_round(restyle_round([] {
        auto rail = std::make_unique<widgets::ModalNavigationRail>(widgets::ModalNavigationRailOptions{{navigation_items(), 1}}, fonts());
        rail->arrange(restyle_area);
        return in_area(std::move(rail));
    }));
    const RestyleResult r = restyle_round([] {
        widgets::NavigationSuiteScaffoldOptions o{navigation_items(), 1};
        o.type = widgets::NavigationSuiteType::wide_navigation_rail_expanded;
        auto suite = std::make_unique<widgets::NavigationSuiteScaffold>(std::move(o), fonts());
        suite->arrange(restyle_area);
        return suite;
    });
    save_round(r, "restyle_navigation_suite");
    check_round(r);
}
