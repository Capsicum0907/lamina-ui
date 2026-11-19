#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

#include "lamina/motion/animated.hpp"
#include "lamina/widgets/interaction.hpp"

using namespace lamina;
using input::PointerAction;

namespace {

struct Item {
    widgets::PressState press;
    motion::Animated state_layer{0.0, {1.0, 1600.0}};
    motion::Animated other{0.0, {1.0, 1600.0}};
};

render::Rect rect_of(std::size_t i) {
    return {static_cast<double>(i) * 10.0, 0.0, 10.0, 10.0};
}

}

TEST_CASE("update_presses reports the clicked item and moves every state layer") {
    std::vector<Item> items(3);
    CHECK_FALSE(widgets::update_presses(items, {PointerAction::down, 15.0, 5.0}, rect_of));
    CHECK(items[1].press.pressed);
    CHECK(items[1].state_layer.target() == tokens::state_layer_opacity::pressed);
    CHECK(items[0].state_layer.target() == 0.0);
    CHECK(widgets::update_presses(items, {PointerAction::up, 15.0, 5.0}, rect_of) == std::optional<std::size_t>{1});
    CHECK(items[1].state_layer.target() == tokens::state_layer_opacity::hover);
}

TEST_CASE("select_on_click selects only a clicked item that is not already selected") {
    std::vector<Item> items(3);
    std::size_t selected = 0;
    const auto select = [&selected](std::size_t i) { selected = i; };
    widgets::select_on_click(items, {PointerAction::down, 5.0, 5.0}, selected, rect_of, select);
    CHECK_FALSE(widgets::select_on_click(items, {PointerAction::up, 5.0, 5.0}, selected, rect_of, select));
    widgets::select_on_click(items, {PointerAction::down, 25.0, 5.0}, selected, rect_of, select);
    CHECK(widgets::select_on_click(items, {PointerAction::up, 25.0, 5.0}, selected, rect_of, select));
    CHECK(selected == 2);
}

TEST_CASE("any_unsettled looks only at the given members") {
    std::vector<Item> items(2);
    CHECK_FALSE(motion::any_unsettled(items, &Item::state_layer, &Item::other));
    items[1].other.animate_to(1.0);
    CHECK_FALSE(motion::any_unsettled(items, &Item::state_layer));
    CHECK(motion::any_unsettled(items, &Item::state_layer, &Item::other));
}
