#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "lamina/render/geometry.hpp"

namespace lamina::widgets {

enum class AccessibleRole {
    button,
    toggle_button,
    checkbox,
    radio_button,
    switch_control,
    slider,
    tab_list,
    tab,
    text_field,
    label,
    menu,
    menu_item,
    menu_group,
    dialog,
    navigation,
    navigation_item,
    progress_indicator,
    status,
    tooltip,
    title_bar,
    group,
    separator,
    list,
    list_item,
    toolbar,
    heading,
    spin_button,
};

enum class AccessibleToggle { off, on, mixed };

struct AccessibleRange {
    double value;
    double min;
    double max;
};

struct AccessibleState {
    bool enabled = true;
    bool focusable = false;
    bool focused = false;
    std::optional<AccessibleToggle> toggle = std::nullopt;
    std::optional<bool> selected = std::nullopt;
    std::optional<AccessibleRange> range = std::nullopt;
    bool modal = false;
    bool invalid = false;
};

struct AccessibleNode {
    AccessibleRole role;
    std::string name;
    render::Rect bounds{0.0, 0.0, 0.0, 0.0};
    AccessibleState state = {};
    std::string value = {};
    std::string description = {};
    std::vector<AccessibleNode> children = {};
};

template <typename W>
concept Accessible = requires(const W& w) {
    { w.accessible() } -> std::convertible_to<std::optional<AccessibleNode>>;
};

std::string_view role_name(AccessibleRole role);
std::string describe(const AccessibleNode& node);
std::string describe(const std::vector<AccessibleNode>& nodes);

void visit_accessible(const std::vector<AccessibleNode>& nodes, const std::function<void(const AccessibleNode&)>& f);
std::optional<AccessibleNode> find_accessible(const std::vector<AccessibleNode>& nodes,
                                              const std::function<bool(const AccessibleNode&)>& match);
std::optional<AccessibleNode> find_accessible(const std::vector<AccessibleNode>& nodes, std::string_view name);
std::optional<AccessibleNode> focused_accessible(const std::vector<AccessibleNode>& nodes);

inline std::string required_name(std::string name, std::string_view what) {
    if (name.empty()) {
        throw std::invalid_argument(std::string(what) + " has no visible text, so it needs an accessible name");
    }
    return name;
}

}
