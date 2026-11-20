#include "lamina/widgets/accessibility.hpp"

#include <array>
#include <sstream>

namespace lamina::widgets {

namespace {

constexpr std::array<std::string_view, static_cast<std::size_t>(AccessibleRole::group) + 1> role_names{
    "button",     "toggle button", "checkbox",   "radio button", "switch",  "slider",     "tab list",
    "tab",        "text field",    "label",      "menu",         "menu item", "menu group", "dialog",
    "navigation", "navigation item", "progress indicator", "status", "tooltip", "title bar", "group",
};

std::string_view toggle_name(AccessibleToggle t) {
    switch (t) {
    case AccessibleToggle::on:
        return "on";
    case AccessibleToggle::mixed:
        return "mixed";
    case AccessibleToggle::off:
        break;
    }
    return "off";
}

void write(std::ostringstream& out, const AccessibleNode& node, int depth) {
    out << std::string(static_cast<std::size_t>(depth) * 2, ' ') << role_name(node.role) << " \"" << node.name << "\"";
    const AccessibleState& s = node.state;
    if (!s.enabled) {
        out << " disabled";
    }
    if (s.focusable) {
        out << " focusable";
    }
    if (s.focused) {
        out << " focused";
    }
    if (s.toggle) {
        out << " " << toggle_name(*s.toggle);
    }
    if (s.selected) {
        out << (*s.selected ? " selected" : " unselected");
    }
    if (s.range) {
        out << " " << s.range->value << " in [" << s.range->min << ", " << s.range->max << "]";
    }
    if (s.modal) {
        out << " modal";
    }
    if (!node.value.empty()) {
        out << " value=\"" << node.value << "\"";
    }
    out << "\n";
    for (const AccessibleNode& child : node.children) {
        write(out, child, depth + 1);
    }
}

}

std::string_view role_name(AccessibleRole role) {
    return role_names[static_cast<std::size_t>(role)];
}

std::string describe(const AccessibleNode& node) {
    std::ostringstream out;
    write(out, node, 0);
    return out.str();
}

std::string describe(const std::vector<AccessibleNode>& nodes) {
    std::ostringstream out;
    for (const AccessibleNode& node : nodes) {
        write(out, node, 0);
    }
    return out.str();
}

void visit_accessible(const std::vector<AccessibleNode>& nodes, const std::function<void(const AccessibleNode&)>& f) {
    for (const AccessibleNode& node : nodes) {
        f(node);
        visit_accessible(node.children, f);
    }
}

std::optional<AccessibleNode> find_accessible(const std::vector<AccessibleNode>& nodes,
                                              const std::function<bool(const AccessibleNode&)>& match) {
    for (const AccessibleNode& node : nodes) {
        if (match(node)) {
            return node;
        }
        if (std::optional<AccessibleNode> found = find_accessible(node.children, match)) {
            return found;
        }
    }
    return std::nullopt;
}

std::optional<AccessibleNode> find_accessible(const std::vector<AccessibleNode>& nodes, std::string_view name) {
    return find_accessible(nodes, [name](const AccessibleNode& n) { return n.name == name; });
}

std::optional<AccessibleNode> focused_accessible(const std::vector<AccessibleNode>& nodes) {
    return find_accessible(nodes, [](const AccessibleNode& n) { return n.state.focused; });
}

}
