#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>

#include "lamina/input/events.hpp"
#include "lamina/tokens/interaction.hpp"

namespace lamina::widgets {

enum class ItemAxis { horizontal, vertical };

constexpr std::optional<std::size_t> item_key_target(const input::KeyEvent& e, std::size_t current, std::size_t count, ItemAxis axis) {
    if (count == 0 || e.ctrl) {
        return std::nullopt;
    }
    const input::Key back = axis == ItemAxis::horizontal ? input::Key::left : input::Key::up;
    const input::Key forth = axis == ItemAxis::horizontal ? input::Key::right : input::Key::down;
    if (e.key == input::Key::home) {
        return 0;
    }
    if (e.key == input::Key::end) {
        return count - 1;
    }
    if (e.key == forth) {
        return (current + 1) % count;
    }
    if (e.key == back) {
        return (current + count - 1) % count;
    }
    return std::nullopt;
}

template <typename Items>
void retarget_state_layers(Items& items, std::optional<std::size_t> focused) {
    for (std::size_t i = 0; i < items.size(); ++i) {
        const double pointer = items[i].press.state_layer_opacity();
        items[i].state_layer.animate_to(focused == i ? std::max(pointer, tokens::state_layer_opacity::focus) : pointer);
    }
}

}
