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
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/divider.hpp"
#include "lamina/widgets/extended_fab.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/fab_menu.hpp"
#include "lamina/widgets/grouped_menu.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/list_view.hpp"
#include "lamina/widgets/loading_indicator.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/menu_bar.hpp"
#include "lamina/widgets/modal_navigation_rail.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_drawer.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/navigation_suite.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/numeric_field.hpp"
#include "lamina/widgets/pager.hpp"
#include "lamina/widgets/pane_scaffold.hpp"
#include "lamina/widgets/progress_indicator.hpp"
#include "lamina/widgets/pull_to_refresh.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/segmented_button.hpp"
#include "lamina/widgets/side_navigation.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/snackbar.hpp"
#include "lamina/widgets/split_button.hpp"
#include "lamina/widgets/split_pane.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/title_bar.hpp"
#include "lamina/widgets/tooltip.hpp"
#include "lamina/widgets/top_app_bar.hpp"
#include "lamina/widgets/touch_scroll.hpp"
#include "lamina/widgets/wavy_progress.hpp"
#include "lamina/widgets/zoom_view.hpp"
#include "lamina/widgets/widget_set.hpp"
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
    const RestyleCanvas size = r.built_under_default.canvas;
    thorvg::Canvas canvas(size.width, size.height * 3);
    const RestyleShot* shots[] = {&r.built_under_default, &r.switched, &r.built_under_test};
    for (int i = 0; i < 3; ++i) {
        for (int y = 0; y < size.height; ++y) {
            for (int x = 0; x < size.width; ++x) {
                canvas.fill_rect({static_cast<double>(x), static_cast<double>(y + i * size.height), 1.0, 1.0},
                                 shots[i]->pixels[static_cast<std::size_t>(y * size.width + x)]);
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

TEST_CASE("a top app bar lays out its title again and places its buttons after a style change") {
    const RestyleResult r = restyle_round([] {
        widgets::TopAppBarOptions o{"Inbox"};
        o.navigation = widgets::IconButtonOptions{icon(res::icons::close), "Close"};
        o.actions = {widgets::IconButtonOptions{icon(res::icons::search), "Search"}};
        auto bar = std::make_unique<widgets::TopAppBar>(std::move(o), fonts());
        bar->arrange({0.0, 0.0, static_cast<double>(restyle_canvas_width), bar->height()});
        return bar;
    });
    save_round(r, "restyle_top_app_bar");
    check_round(r);
}

TEST_CASE("a snackbar and a dialog lay out their text and actions again after a style change") {
    const RestyleResult s = restyle_round([] {
        auto bar = std::make_unique<widgets::Snackbar>(widgets::SnackbarOptions{"Undo", 1000.0}, fonts());
        bar->set_area(restyle_area);
        bar->show("Message archived");
        return in_area(std::move(bar));
    });
    save_round(s, "restyle_snackbar");
    check_round(s);
    const RestyleResult d = restyle_round([] {
        auto dialog = std::make_unique<widgets::Dialog>(widgets::DialogOptions{"Discard draft?", "", "Discard", "Cancel"}, fonts());
        dialog->open_in(restyle_area);
        return in_area(std::move(dialog));
    });
    save_round(d, "restyle_dialog");
    check_round(d);
}

TEST_CASE("a menu and a grouped menu size their items again after a style change") {
    const RestyleResult m = restyle_round([] {
        auto menu = std::make_unique<widgets::Menu>(widgets::MenuOptions{{{"Cut"}, {"Copy"}, {"Paste"}}}, fonts());
        menu->open_at({20.0, 10.0}, {static_cast<double>(restyle_canvas_width), static_cast<double>(restyle_canvas_height)});
        return in_area(std::move(menu));
    });
    save_round(m, "restyle_menu");
    check_round(m);
    const RestyleResult g = restyle_round([] {
        widgets::GroupedMenuOptions o{{{{{"Bold"}, {"Italic", std::nullopt, std::nullopt, std::nullopt, true}}, "Style"}}};
        auto menu = std::make_unique<widgets::GroupedMenu>(std::move(o), fonts());
        menu->open({20.0, 0.0, 10.0, 10.0}, {static_cast<double>(restyle_canvas_width), static_cast<double>(restyle_canvas_height)});
        return in_area(std::move(menu));
    });
    save_round(g, "restyle_grouped_menu");
    check_round(g);
}

TEST_CASE("a slider animates its focus with the new style's springs after a style change") {
    check_round(restyle_motion_round(
        [] {
            auto s = std::make_unique<widgets::Slider>(widgets::SliderOptions{0.4});
            s->set_width(300.0);
            s->set_origin(origin);
            return s;
        },
        [](widgets::Slider& w) { w.set_focused(true); }));
}

TEST_CASE("a text field lays out its label and text in the new style's type and floats its label with the new style's springs") {
    auto make = [] {
        widgets::TextFieldOptions o;
        o.label = "Name";
        o.supporting_text = "As on your card";
        auto field = std::make_unique<widgets::TextField>(o, fonts());
        field->set_width(300.0);
        field->set_origin(origin);
        return field;
    };
    const RestyleResult r = restyle_round([&] {
        auto field = make();
        field->set_text("Ada");
        return field;
    });
    save_round(r, "restyle_text_field");
    check_round(r);
    check_round(restyle_motion_round(make, [](widgets::TextField& w) { w.set_focused(true); }));
}

template <typename W>
struct Arranged {
    std::unique_ptr<W> w;
    render::Rect area;
    void step(double dt) {
        w->step(dt);
        w->arrange(area);
    }
    render::Rect bounds() const { return w->bounds(); }
    void paint(render::Canvas& c, const color::ColorScheme& s) const { w->paint(c, s); }
};

template <typename W>
std::unique_ptr<Arranged<W>> arranged(std::unique_ptr<W> w, render::Rect area) {
    w->arrange(area);
    return std::make_unique<Arranged<W>>(Arranged<W>{std::move(w), area});
}

TEST_CASE("a split button arranged by its owner pads and shapes its halves again and turns its chevron with the new style's springs") {
    auto make = [] {
        widgets::SplitButtonOptions o{"Send", std::nullopt, ButtonStyle::filled, ButtonSize::medium, "More"};
        return arranged(std::make_unique<widgets::SplitButton>(std::move(o), fonts(), icon_font()),
                        {origin.x, origin.y, restyle_canvas_width - 2.0 * origin.x, restyle_canvas_height - 2.0 * origin.y});
    };
    const RestyleResult r = restyle_round(make);
    save_round(r, "restyle_split_button");
    check_round(r);
    check_round(restyle_motion_round(make, [](Arranged<widgets::SplitButton>& w) { w.w->set_checked(true); }));
}

TEST_CASE("a loading indicator takes the new style's shapes and morphs with the new style's springs after a style change") {
    auto make = [] { return placed(std::make_unique<widgets::LoadingIndicator>(widgets::LoadingIndicatorOptions{0.3})); };
    const RestyleResult r = restyle_round(make);
    save_round(r, "restyle_loading_indicator");
    check_round(r);
    check_round(restyle_motion_round(make, [](widgets::LoadingIndicator& w) { w.set_progress(std::nullopt); }));
}

TEST_CASE("wavy and circular progress indicators take the new style's size and shapes and ease with the new style's motion") {
    widgets::WavyProgressOptions still{0.5};
    still.wave_speed = 0.0;
    const RestyleResult linear = restyle_round([&] { return placed(std::make_unique<widgets::LinearWavyProgressIndicator>(still)); });
    save_round(linear, "restyle_linear_wavy_progress");
    check_round(linear);
    check_round(restyle_motion_round([&] { return placed(std::make_unique<widgets::LinearWavyProgressIndicator>(still)); },
                                     [](widgets::LinearWavyProgressIndicator& w) { w.ease_progress(0.9); }));
    const RestyleResult circular = restyle_round([&] { return placed(std::make_unique<widgets::CircularWavyProgressIndicator>(still)); });
    save_round(circular, "restyle_circular_wavy_progress");
    check_round(circular);
    widgets::CircularProgressOptions plain;
    plain.waves_per_second = 0.0;
    check_round(restyle_motion_round([&] { return placed(std::make_unique<widgets::CircularProgressIndicator>(plain)); },
                                     [](widgets::CircularProgressIndicator& w) { w.ease_progress(0.8); }));
}

TEST_CASE("a shown rich tooltip lays out its text and action again and stays by its anchor after a style change") {
    const RestyleResult r = restyle_round([] {
        widgets::TooltipOptions o{"Grant value is calculated using the closing stock price", widgets::TooltipKind::rich, "Add others", "Learn more"};
        o.position = widgets::TooltipPosition::below;
        auto tip = std::make_unique<widgets::Tooltip>(std::move(o), fonts());
        tip->set_window({static_cast<double>(restyle_canvas_width), 320.0});
        tip->set_anchor({40.0, 10.0, 40.0, 20.0});
        tip->show();
        return in_area(std::move(tip));
    }, {restyle_canvas_width, 320});
    save_round(r, "restyle_tooltip");
    check_round(r);
}

TEST_CASE("a title bar lays out its title again and fades its buttons with the new style's springs after a style change") {
    auto make = [] {
        auto bar = std::make_unique<widgets::TitleBar>(widgets::TitleBarOptions{"Notes", icon_font()}, fonts());
        bar->set_width(restyle_canvas_width);
        bar->set_origin({0.0, 20.0});
        return bar;
    };
    const RestyleResult r = restyle_round(make);
    save_round(r, "restyle_title_bar");
    check_round(r);
    check_round(restyle_motion_round(make, [](widgets::TitleBar& w) {
        const render::Rect b = w.button_rect(widgets::TitleBarAction::minimize);
        w.handle(input::PointerEvent{input::PointerAction::down, b.x + b.width / 2.0, b.y + b.height / 2.0});
    }));
}

TEST_CASE("a split pane places its divider again and shows its handle with the new style's springs after a style change") {
    auto make = [] {
        widgets::SplitPaneOptions o;
        o.first_size = 150.0;
        auto pane = std::make_unique<widgets::SplitPane>(std::move(o));
        pane->arrange(restyle_area);
        return pane;
    };
    const RestyleResult r = restyle_round([&] {
        auto pane = make();
        pane->set_focused(true);
        return pane;
    });
    save_round(r, "restyle_split_pane");
    check_round(r);
    check_round(restyle_motion_round(make, [](widgets::SplitPane& w) { w.set_focused(true); }));
}

TEST_CASE("a widget set restyles its widgets without stepping them") {
    const widgets::ButtonOptions o{"Send", ButtonStyle::filled, ButtonSize::medium};
    widgets::WidgetSet set;
    widgets::Button& button = set.emplace<widgets::Button>(o, fonts());
    set.emplace<widgets::Divider>();
    const StyleScope scope(restyle_test_style());
    const widgets::Button fresh(o, fonts());
    CHECK(button.height() != fresh.height());
    set.restyle();
    CHECK(button.width() == fresh.width());
    CHECK(button.height() == fresh.height());
}

namespace {

struct PaneRig {
    widgets::WidgetSet list;
    widgets::WidgetSet detail;
    layout::NodePtr list_node;
    layout::NodePtr detail_node;
    widgets::ThreePaneScaffoldNavigator navigator =
        widgets::list_detail_pane_navigator(widgets::calculate_pane_scaffold_directive({layout::compute_window_size_class(restyle_canvas_width, restyle_canvas_height)}));
    std::unique_ptr<widgets::ThreePaneScaffold> scaffold;

    PaneRig() {
        list_node = layout::column({widgets::node(list.emplace<widgets::Button>(widgets::ButtonOptions{"List"}, fonts()))});
        detail_node = layout::column({widgets::node(detail.emplace<widgets::Button>(widgets::ButtonOptions{"Detail"}, fonts()))});
        widgets::ThreePaneScaffoldOptions o;
        o.panes[static_cast<std::size_t>(widgets::list_detail_pane::list)] = widgets::ContentSlot{list_node, &list, nullptr};
        o.panes[static_cast<std::size_t>(widgets::list_detail_pane::detail)] = widgets::ContentSlot{detail_node, &detail, nullptr};
        scaffold = std::make_unique<widgets::ThreePaneScaffold>(std::move(o), navigator);
        scaffold->arrange(restyle_area);
    }
    void step(double dt) {
        scaffold->step(dt);
        scaffold->arrange(restyle_area);
    }
    render::Rect bounds() const { return scaffold->bounds(); }
    void paint(render::Canvas& c, const color::ColorScheme& s) const { scaffold->paint(c, s); }
};

}

TEST_CASE("a three pane scaffold restyles its panes' widgets and moves its panes with the new style's springs after a style change") {
    const RestyleResult r = restyle_round([] { return std::make_unique<PaneRig>(); });
    save_round(r, "restyle_pane_scaffold");
    check_round(r);
    check_round(restyle_motion_round([] { return std::make_unique<PaneRig>(); },
                                     [](PaneRig& w) { w.navigator.navigate_to(widgets::list_detail_pane::detail); }));
}

TEST_CASE("a zoom view without its own tokens reads the active style's and settles with its spring") {
    const widgets::ZoomViewOptions options{"Photo", 400.0, 300.0};
    widgets::ZoomView kept(options);
    widgets::ZoomView switched(options);
    for (widgets::ZoomView* v : {&kept, &switched}) {
        v->set_bounds({0.0, 0.0, 400.0, 300.0});
        v->zoom_to(2.0, 200.0, 150.0);
    }
    CHECK(&kept.tokens() == &m3_expressive_style().zoom);
    kept.step(frame_seconds);
    {
        const StyleScope scope(restyle_test_style());
        CHECK(&switched.tokens() == &restyle_test_style().zoom);
        switched.step(frame_seconds);
    }
    CHECK(switched.scale() != kept.scale());
    widgets::ZoomViewOptions own = options;
    own.tokens = &m3_expressive_style().zoom;
    const widgets::ZoomView given(own);
    const StyleScope scope(restyle_test_style());
    CHECK(&given.tokens() == &m3_expressive_style().zoom);
}

TEST_CASE("a numeric field lays out its label, value and unit again and outlines its focus with the new style's springs") {
    auto make = [] {
        widgets::NumericFieldOptions o{"Width", 42.0};
        o.unit = "px";
        return placed(std::make_unique<widgets::NumericField>(std::move(o), fonts()));
    };
    const RestyleResult r = restyle_round(make);
    save_round(r, "restyle_numeric_field");
    check_round(r);
    check_round(restyle_motion_round(make, [](widgets::NumericField& w) { w.set_focused(true); }));
}

TEST_CASE("an open submenu follows its item to its new place after a style change") {
    auto make = [] {
        widgets::MenuOptions o{.items = {{.label = "New"}, {.label = "Recent", .submenu = {{.label = "notes.txt"}, {.label = "plan.md"}}}},
                               .accessible_name = "File",
                               .icon_font = icon_font()};
        auto menu = std::make_unique<widgets::Menu>(std::move(o), fonts());
        menu->open({10.0, 0.0, 10.0, 10.0}, {static_cast<double>(restyle_canvas_width), 320.0}, input::InputSource::keyboard);
        menu->handle(input::KeyEvent{input::Key::down});
        menu->handle(input::KeyEvent{input::Key::right});
        return in_area(std::move(menu));
    };
    const RestyleResult r = restyle_round(make, {restyle_canvas_width, 320});
    save_round(r, "restyle_submenu");
    check_round(r);
    const auto opened = make();
    CHECK(opened->w->open_submenu() == 1u);
}

namespace {

struct BarWithMenus {
    std::unique_ptr<widgets::MenuBar> bar;
    void step(double dt) { bar->step(dt); }
    render::Rect bounds() const { return bar->bounds(); }
    void paint(render::Canvas& c, const color::ColorScheme& s) const {
        bar->paint(c, s);
        bar->paint_menus(c, s);
    }
};

}

TEST_CASE("a menu bar lays out its items again and keeps its open menu under its item after a style change") {
    const RestyleResult r = restyle_round([] {
        auto bar = std::make_unique<widgets::MenuBar>(
            widgets::MenuBarOptions{.menus = {{"File", {{.label = "New"}, {.label = "Quit"}}}, {"Edit", {{.label = "Undo"}, {.label = "Copy"}}}},
                                    .accessible_name = "Menu bar"},
            fonts());
        bar->set_window({static_cast<double>(restyle_canvas_width), 320.0});
        const render::Rect item = bar->item_rect(1);
        bar->handle(input::PointerEvent{input::PointerAction::down, item.x + item.width / 2.0, item.y + item.height / 2.0});
        bar->handle(input::PointerEvent{input::PointerAction::up, item.x + item.width / 2.0, item.y + item.height / 2.0});
        return std::make_unique<BarWithMenus>(BarWithMenus{std::move(bar)});
    }, {restyle_canvas_width, 320});
    save_round(r, "restyle_menu_bar");
    check_round(r);
}

namespace {

struct SplitWithMenu {
    std::unique_ptr<widgets::SplitButton> button;
    render::Rect area;
    void step(double dt) {
        button->step(dt);
        button->arrange(area);
    }
    render::Rect bounds() const { return button->bounds(); }
    void paint(render::Canvas& c, const color::ColorScheme& s) const {
        button->paint(c, s);
        button->paint_overlay(c, s);
    }
};

}

TEST_CASE("a split button keeps its open menu under its trailing button after a style change") {
    const RestyleResult r = restyle_round([] {
        widgets::SplitButtonOptions o{"Send", std::nullopt, ButtonStyle::filled, ButtonSize::small, "More", {{"Schedule"}, {"Save draft"}}};
        auto button = std::make_unique<widgets::SplitButton>(std::move(o), fonts(), icon_font());
        const render::Rect area{origin.x, origin.y, 200.0, 60.0};
        button->set_window({static_cast<double>(restyle_canvas_width), 320.0});
        button->arrange(area);
        button->set_checked(true);
        return std::make_unique<SplitWithMenu>(SplitWithMenu{std::move(button), area});
    }, {restyle_canvas_width, 320});
    save_round(r, "restyle_split_button_menu");
    check_round(r);
}

TEST_CASE("a FAB menu lays out its items again and opens with the new style's springs after a style change") {
    auto make = [](bool open) {
        widgets::FabMenuOptions o{{{"Reply", icon(res::icons::edit)}, {"Forward", icon()}}, {icon(res::icons::edit), icon(res::icons::close), "Menu"}};
        o.button.checked = open;
        return placed(std::make_unique<widgets::FabMenu>(std::move(o), fonts()));
    };
    const RestyleResult r = restyle_round([&] { return make(true); }, {restyle_canvas_width, 320});
    save_round(r, "restyle_fab_menu");
    check_round(r);
    check_round(restyle_motion_round([&] { return make(false); }, [](widgets::FabMenu& w) { w.set_expanded(true); }, {restyle_canvas_width, 320}));
}
