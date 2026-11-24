#include "lamina/style/style.hpp"
#include "lamina/widgets/pane_navigator.hpp"

#include <utility>

#include "lamina/tokens/pane_scaffold.hpp"

namespace lamina::widgets {

namespace {

const tokens::PaneScaffoldTokens& t() {
    return current_style().pane_scaffold;
}

std::size_t at(PaneRole role) {
    return static_cast<std::size_t>(role);
}

}

std::size_t PaneScaffoldValue::expanded_count() const {
    std::size_t n = 0;
    for (const PaneAdaptedValue& v : panes) {
        if (v.kind == PaneAdaptedValue::Kind::expanded) {
            ++n;
        }
    }
    return n;
}

PaneScaffoldDirective calculate_pane_scaffold_directive(const WindowAdaptiveInfo& info) {
    PaneScaffoldDirective d;
    switch (info.window_size_class.width_class()) {
    case layout::WindowWidthClass::compact:
    case layout::WindowWidthClass::medium:
        d.max_horizontal_partitions = 1;
        d.horizontal_partition_spacer = 0.0;
        d.default_pane_preferred_width = t().preferred_width;
        break;
    case layout::WindowWidthClass::expanded:
        d.max_horizontal_partitions = 2;
        d.horizontal_partition_spacer = t().partition_spacer;
        d.default_pane_preferred_width = t().preferred_width;
        break;
    default:
        d.max_horizontal_partitions = 3;
        d.horizontal_partition_spacer = t().partition_spacer;
        d.default_pane_preferred_width = t().preferred_width_extra_large;
        break;
    }
    const bool tall = info.window_size_class.height_class() == layout::WindowHeightClass::expanded;
    if (info.tabletop || (d.max_horizontal_partitions == 1 && tall)) {
        d.max_vertical_partitions = 2;
        d.vertical_partition_spacer = t().partition_spacer;
    } else {
        d.max_vertical_partitions = 1;
        d.vertical_partition_spacer = 0.0;
    }
    d.default_pane_preferred_height = t().preferred_height;
    return d;
}

PaneScaffoldDirective calculate_pane_scaffold_directive_with_two_panes_on_medium_width(const WindowAdaptiveInfo& info) {
    PaneScaffoldDirective d = calculate_pane_scaffold_directive(info);
    if (info.window_size_class.width_class() == layout::WindowWidthClass::medium) {
        d.max_horizontal_partitions = 2;
        d.horizontal_partition_spacer = t().partition_spacer;
        if (!info.tabletop) {
            d.max_vertical_partitions = 1;
            d.vertical_partition_spacer = 0.0;
        }
    }
    return d;
}

PaneScaffoldValue calculate_pane_scaffold_value(int max_horizontal_partitions, const AdaptStrategies& strategies,
                                                const std::vector<PaneDestination>& history, int max_vertical_partitions) {
    std::array<std::optional<PaneAdaptedValue>, pane_count> values{};
    int expanded = 0;
    bool check_reflow = max_horizontal_partitions == 1 && max_vertical_partitions > 1;
    if (check_reflow) {
        bool any = false;
        for (const AdaptStrategy& s : strategies) {
            any = any || s.kind == AdaptStrategy::Kind::reflow;
        }
        check_reflow = any;
    }
    std::vector<PaneRole> priority;
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
        priority.push_back(it->pane);
    }
    priority.insert(priority.end(), {PaneRole::primary, PaneRole::secondary, PaneRole::tertiary});
    for (PaneRole pane : priority) {
        const bool room = expanded < max_horizontal_partitions;
        if (!room && !check_reflow) {
            break;
        }
        if (values[at(pane)]) {
            continue;
        }
        std::optional<PaneRole> reflowed;
        PaneRole anchor = pane;
        std::optional<PaneAdaptedValue> anchor_value;
        if (check_reflow && strategies[at(pane)].kind == AdaptStrategy::Kind::reflow) {
            reflowed = pane;
            anchor = strategies[at(pane)].reflow_under;
            anchor_value = values[at(anchor)];
        }
        if (!anchor_value) {
            if (!room) {
                continue;
            }
            values[at(anchor)] = PaneAdaptedValue{PaneAdaptedValue::Kind::expanded};
            ++expanded;
        } else if (anchor_value->kind != PaneAdaptedValue::Kind::expanded) {
            continue;
        }
        if (reflowed) {
            values[at(*reflowed)] = PaneAdaptedValue{PaneAdaptedValue::Kind::reflowed, anchor};
            check_reflow = false;
        }
    }
    PaneScaffoldValue v;
    for (std::size_t i = 0; i < pane_count; ++i) {
        v.panes[i] = values[i].value_or(PaneAdaptedValue{});
    }
    if (!history.empty()) {
        v.current = history.back().pane;
    }
    return v;
}

ThreePaneScaffoldNavigator::ThreePaneScaffoldNavigator(PaneScaffoldDirective directive, AdaptStrategies strategies,
                                                       std::vector<PaneDestination> history, bool history_aware)
    : directive_(directive), strategies_(strategies), history_(std::move(history)), history_aware_(history_aware) {
    value_ = value_at(static_cast<std::ptrdiff_t>(history_.size()) - 1);
}

void ThreePaneScaffoldNavigator::set_directive(const PaneScaffoldDirective& directive) {
    directive_ = directive;
    value_ = value_at(static_cast<std::ptrdiff_t>(history_.size()) - 1);
}

std::optional<PaneDestination> ThreePaneScaffoldNavigator::current_destination() const {
    if (history_.empty()) {
        return std::nullopt;
    }
    return history_.back();
}

PaneScaffoldValue ThreePaneScaffoldNavigator::value_at(std::ptrdiff_t index) const {
    if (index < 0) {
        return calculate_pane_scaffold_value(directive_.max_horizontal_partitions, strategies_, {}, directive_.max_vertical_partitions);
    }
    if (history_aware_) {
        const std::vector<PaneDestination> part(history_.begin(), history_.begin() + index + 1);
        return calculate_pane_scaffold_value(directive_.max_horizontal_partitions, strategies_, part, directive_.max_vertical_partitions);
    }
    return calculate_pane_scaffold_value(directive_.max_horizontal_partitions, strategies_, {history_[static_cast<std::size_t>(index)]},
                                         directive_.max_vertical_partitions);
}

std::ptrdiff_t ThreePaneScaffoldNavigator::previous_index(BackNavigationBehavior behavior) const {
    if (history_.size() <= 1) {
        return -1;
    }
    const auto last = static_cast<std::ptrdiff_t>(history_.size()) - 1;
    const PaneDestination& current = history_.back();
    switch (behavior) {
    case BackNavigationBehavior::pop_latest:
        return last - 1;
    case BackNavigationBehavior::pop_until_scaffold_value_change:
        for (std::ptrdiff_t i = last - 1; i >= 0; --i) {
            if (!(value_at(i) == value_)) {
                return i;
            }
        }
        break;
    case BackNavigationBehavior::pop_until_current_destination_change:
        for (std::ptrdiff_t i = last - 1; i >= 0; --i) {
            if (history_[static_cast<std::size_t>(i)].pane != current.pane) {
                return i;
            }
        }
        break;
    case BackNavigationBehavior::pop_until_content_change:
        for (std::ptrdiff_t i = last - 1; i >= 0; --i) {
            if (history_[static_cast<std::size_t>(i)].content != current.content || !(value_at(i) == value_)) {
                return i;
            }
        }
        break;
    }
    return -1;
}

PaneScaffoldValue ThreePaneScaffoldNavigator::peek_previous_value(BackNavigationBehavior behavior) const {
    const std::ptrdiff_t i = previous_index(behavior);
    return i < 0 ? value_ : value_at(i);
}

void ThreePaneScaffoldNavigator::navigate_to(PaneRole pane, std::optional<std::size_t> content) {
    history_.push_back({pane, content});
    value_ = value_at(static_cast<std::ptrdiff_t>(history_.size()) - 1);
}

bool ThreePaneScaffoldNavigator::can_navigate_back(BackNavigationBehavior behavior) const {
    return previous_index(behavior) >= 0;
}

bool ThreePaneScaffoldNavigator::navigate_back(BackNavigationBehavior behavior) {
    const std::ptrdiff_t i = previous_index(behavior);
    if (i < 0) {
        history_.clear();
        value_ = value_at(-1);
        return false;
    }
    history_.resize(static_cast<std::size_t>(i) + 1);
    value_ = value_at(i);
    return true;
}

ThreePaneScaffoldNavigator list_detail_pane_navigator(const PaneScaffoldDirective& directive) {
    return {directive, list_detail_adapt_strategies, {{list_detail_pane::list}}};
}

ThreePaneScaffoldNavigator supporting_pane_navigator(const PaneScaffoldDirective& directive) {
    return {directive, supporting_pane_adapt_strategies, {{supporting_pane::main}}};
}

}
