#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>

#include "lamina/input/events.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/interaction.hpp"

namespace lamina::widgets {

enum class ItemAxis { horizontal, vertical };

struct AllItemsEnabled {
    constexpr bool operator()(std::size_t) const { return true; }
};

template <typename Enabled>
constexpr std::optional<std::size_t> scan_items(std::size_t start, std::size_t count, bool forward, Enabled enabled) {
    for (std::size_t k = 0; k < count; ++k) {
        const std::size_t i = forward ? (start + k) % count : (start + count - k) % count;
        if (enabled(i)) {
            return i;
        }
    }
    return std::nullopt;
}

template <typename Enabled>
constexpr std::optional<std::size_t> first_enabled_item(std::size_t count, Enabled enabled) {
    return count == 0 ? std::nullopt : scan_items(0, count, true, enabled);
}

template <typename Enabled = AllItemsEnabled>
constexpr std::optional<std::size_t> item_key_target(const input::KeyEvent& e, std::size_t current, std::size_t count, ItemAxis axis,
                                                     Enabled enabled = {}) {
    if (count == 0 || e.ctrl) {
        return std::nullopt;
    }
    const input::Key back = axis == ItemAxis::horizontal ? input::Key::left : input::Key::up;
    const input::Key forth = axis == ItemAxis::horizontal ? input::Key::right : input::Key::down;
    if (e.key == input::Key::home) {
        return scan_items(0, count, true, enabled);
    }
    if (e.key == input::Key::end) {
        return scan_items(count - 1, count, false, enabled);
    }
    if (e.key == forth) {
        return scan_items((current + 1) % count, count, true, enabled);
    }
    if (e.key == back) {
        return scan_items((current + count - 1) % count, count, false, enabled);
    }
    return std::nullopt;
}

constexpr bool closes_menu(const input::KeyEvent& e) {
    return e.key == input::Key::escape || e.key == input::Key::tab;
}

template <typename Enabled = AllItemsEnabled>
constexpr std::optional<std::size_t> menu_key_target(const input::KeyEvent& e, std::optional<std::size_t> active, std::size_t count,
                                                     Enabled enabled = {}) {
    if (active) {
        return item_key_target(e, *active, count, ItemAxis::vertical, enabled);
    }
    if (count == 0 || e.ctrl) {
        return std::nullopt;
    }
    if (e.key == input::Key::down || e.key == input::Key::home) {
        return scan_items(0, count, true, enabled);
    }
    if (e.key == input::Key::up || e.key == input::Key::end) {
        return scan_items(count - 1, count, false, enabled);
    }
    return std::nullopt;
}

template <typename Items>
void retarget_state_layers(Items& items, std::optional<std::size_t> focused) {
    for (std::size_t i = 0; i < items.size(); ++i) {
        const double pointer = items[i].press.state_layer_opacity();
        items[i].state_layer.animate_to(focused == i ? std::max(pointer, current_style().state_layer.focus) : pointer);
    }
}

template <typename Items>
bool any_item_enabled(const Items& items) {
    return std::any_of(items.begin(), items.end(), [](const auto& item) { return item.enabled.enabled(); });
}

template <typename Items>
void disable_item(Items& items, std::size_t index) {
    items[index].press = {};
    items[index].state_layer.animate_to(0.0);
}

template <typename Items>
bool refresh_item(Items& items, std::size_t index, bool row_enabled) {
    const bool on = row_enabled && items[index].allowed;
    if (!items[index].enabled.set(on)) {
        return false;
    }
    if (!on) {
        disable_item(items, index);
    }
    return true;
}

}
