#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include "lamina/layout/window_size_class.hpp"

namespace lamina::widgets {

enum class PaneRole : std::size_t { primary, secondary, tertiary, count };

inline constexpr std::size_t pane_count = static_cast<std::size_t>(PaneRole::count);

namespace list_detail_pane {
inline constexpr PaneRole list = PaneRole::secondary;
inline constexpr PaneRole detail = PaneRole::primary;
inline constexpr PaneRole extra = PaneRole::tertiary;
}

namespace supporting_pane {
inline constexpr PaneRole main = PaneRole::primary;
inline constexpr PaneRole supporting = PaneRole::secondary;
inline constexpr PaneRole extra = PaneRole::tertiary;
}

struct AdaptStrategy {
    enum class Kind { hide, reflow };
    Kind kind = Kind::hide;
    PaneRole reflow_under = PaneRole::primary;

    static constexpr AdaptStrategy hide() { return {}; }
    static constexpr AdaptStrategy reflow(PaneRole under) { return {Kind::reflow, under}; }
    constexpr bool operator==(const AdaptStrategy&) const = default;
};

using AdaptStrategies = std::array<AdaptStrategy, pane_count>;

struct PaneAdaptedValue {
    enum class Kind { hidden, expanded, reflowed };
    Kind kind = Kind::hidden;
    PaneRole reflow_under = PaneRole::primary;

    constexpr bool shown() const { return kind != Kind::hidden; }
    constexpr bool operator==(const PaneAdaptedValue&) const = default;
};

struct PaneScaffoldValue {
    std::array<PaneAdaptedValue, pane_count> panes{};
    std::optional<PaneRole> current = std::nullopt;

    const PaneAdaptedValue& operator[](PaneRole role) const { return panes[static_cast<std::size_t>(role)]; }
    std::size_t expanded_count() const;
    bool operator==(const PaneScaffoldValue& other) const { return panes == other.panes; }
};

struct PaneDestination {
    PaneRole pane;
    std::optional<std::size_t> content = std::nullopt;
    constexpr bool operator==(const PaneDestination&) const = default;
};

struct PaneScaffoldDirective {
    int max_horizontal_partitions = 1;
    double horizontal_partition_spacer = 0.0;
    int max_vertical_partitions = 1;
    double vertical_partition_spacer = 0.0;
    double default_pane_preferred_width = 0.0;
    double default_pane_preferred_height = 0.0;
    bool operator==(const PaneScaffoldDirective&) const = default;
};

struct WindowAdaptiveInfo {
    layout::WindowSizeClass window_size_class = {};
    bool tabletop = false;
};

PaneScaffoldDirective calculate_pane_scaffold_directive(const WindowAdaptiveInfo& info);
PaneScaffoldDirective calculate_pane_scaffold_directive_with_two_panes_on_medium_width(const WindowAdaptiveInfo& info);

PaneScaffoldValue calculate_pane_scaffold_value(int max_horizontal_partitions, const AdaptStrategies& strategies,
                                                const std::vector<PaneDestination>& history, int max_vertical_partitions = 1);

enum class BackNavigationBehavior { pop_latest, pop_until_scaffold_value_change, pop_until_current_destination_change, pop_until_content_change };

struct PaneOrder {
    std::array<PaneRole, pane_count> roles;
};

inline constexpr PaneOrder list_detail_pane_order{{PaneRole::secondary, PaneRole::primary, PaneRole::tertiary}};
inline constexpr PaneOrder supporting_pane_order{{PaneRole::primary, PaneRole::secondary, PaneRole::tertiary}};

inline constexpr AdaptStrategies list_detail_adapt_strategies{AdaptStrategy::hide(), AdaptStrategy::hide(), AdaptStrategy::hide()};
inline constexpr AdaptStrategies supporting_pane_adapt_strategies{AdaptStrategy::hide(), AdaptStrategy::reflow(PaneRole::primary),
                                                                  AdaptStrategy::hide()};

class ThreePaneScaffoldNavigator {
public:
    ThreePaneScaffoldNavigator(PaneScaffoldDirective directive, AdaptStrategies strategies, std::vector<PaneDestination> history,
                               bool history_aware = true);

    const PaneScaffoldDirective& directive() const { return directive_; }
    void set_directive(const PaneScaffoldDirective& directive);
    const AdaptStrategies& strategies() const { return strategies_; }
    const std::vector<PaneDestination>& history() const { return history_; }
    std::optional<PaneDestination> current_destination() const;
    const PaneScaffoldValue& value() const { return value_; }
    PaneScaffoldValue peek_previous_value(BackNavigationBehavior behavior = BackNavigationBehavior::pop_until_scaffold_value_change) const;

    void navigate_to(PaneRole pane, std::optional<std::size_t> content = std::nullopt);
    bool can_navigate_back(BackNavigationBehavior behavior = BackNavigationBehavior::pop_until_scaffold_value_change) const;
    bool navigate_back(BackNavigationBehavior behavior = BackNavigationBehavior::pop_until_scaffold_value_change);

private:
    PaneScaffoldValue value_at(std::ptrdiff_t index) const;
    std::ptrdiff_t previous_index(BackNavigationBehavior behavior) const;

    PaneScaffoldDirective directive_;
    AdaptStrategies strategies_;
    std::vector<PaneDestination> history_;
    bool history_aware_;
    PaneScaffoldValue value_;
};

ThreePaneScaffoldNavigator list_detail_pane_navigator(const PaneScaffoldDirective& directive);
ThreePaneScaffoldNavigator supporting_pane_navigator(const PaneScaffoldDirective& directive);

}
