# Layout and adaptive layouts

[日本語](layout-and-adaptive.ja.md) · [README](../README.md#documentation)

Rows and columns, the window size classes, the scaffolds that change with them, panes and density.

## Rows and columns

`lamina/layout/layout.hpp`: rows and columns in the manner of Compose. A node is measured under constraints, then placed. Rows and columns take spacing, padding, main-axis alignment (start, center, end, space between/around/evenly), cross-axis alignment (start, center, end, stretch) and weights that share the leftover space. `layout::flow` places children in a row and starts a new line when the next one does not fit, from the start or, with `FlowDirection::end_to_start`, from the end. A stretched row is as tall as its tallest child unless `fill_cross` asks for all the space. `lamina/widgets/nodes.hpp` turns a button, icon button, FAB, switch, checkbox, radio button, text field or `Label` (`widgets/label.hpp`, wrapping text; like the other widgets it takes `LabelOptions`, with the text, the type role, the color role and whether it wraps, and the fonts) into a node. `build/Release/layout_demo.exe` reflows as the window is resized.

## Widgets as layout nodes

`lamina/widgets/nodes.hpp` also turns a slider, tab row or navigation bar into a node that takes the whole width it is given, and a navigation rail into a node as wide as the rail is at that moment and as tall as the space. `build/Release/navigation.exe` shows the tabs, two sliders, both menus, two dialogs, a navigation rail and a navigation bar. Tab goes through the rail's menu button, the rail, the dark-mode toggle, the three tab rows, the two sliders, the four buttons and the navigation bar, in that order; Space or Enter on a button opens its menu or dialog from the keyboard, so the first menu item or the dialog's action has the focus at once, and closing returns the focus to the button. The menus' items have access keys: in the Options menu, Edit, Share, Copy and Settings take their first letters, so Share and Settings share S and S moves between them, and the Japanese ダウンロード has D, shown as ダウンロード(D); in the View menu, Starred has R and Share has H, so each key there is unique.

## Screen patterns

`lamina/widgets/content_slot.hpp`, `content_host.hpp`, `scrim.hpp`, `pattern_strings.hpp`: what the screen patterns share. `ContentSlot` (a layout node, a `WidgetSet` and a `FocusManager`; `DialogContent` is now this type) is any content a program puts into a sheet or drawer; `ContentHost` lays it out at a width, scrolls it in a `ScrollArea` when it is taller than its viewport, gives it pointer events in its own coordinates, keys and text while it has the focus, keeps the focused widget in view and gathers its accessibility nodes. `chain_focus(parts, direction, wrap)` moves the focus through several `FocusManager`s in order, wrapping for the modal containers. `paint_scrim` draws the scrim role at 32 % (`tokens/scrim.hpp`). `PatternStrings` holds Compose's strings for these components in English (US and UK) and Japanese (`english_us_patterns()`, `english_gb_patterns()`, `japanese_patterns()`).

## Window size classes

`lamina/layout/window_size_class.hpp`: window size classes with window-core's breakpoints (width: compact, medium from 600 dp, expanded from 840, large from 1200, extra-large from 1600; height: compact, medium from 480, expanded from 900). `compute_window_size_class(width, height)` returns one, and `sdl3::Frame::size_class` holds the window's for every frame.

## Navigation suite scaffold

`lamina/widgets/navigation_suite.hpp`: `NavigationSuiteScaffold` shows one list of destinations as a navigation bar or as a `SideNavigation` (the icon-only rail, the wide rail or the drawer) and gives the rest to its content. `update(info)` picks the type from the window size class with `widening_navigation_suite`; `navigation_suite_type(info)` is Compose's chooser. The bar and the side list slide past each other on a spring; between the rail and the drawer the side list morphs without cross-fading, and a destination chosen on one is selected on all. `set_labels` (or `NavigationSuiteScaffoldOptions::labels`) shows or hides the bar's labels and collapses the side list to its icons whatever the type, and `snap_to_targets()` lands a type change and the labels without animating, for a first frame. `side()` returns the `SideNavigation`.

  - Breaking change: the suite no longer has `rail()` and `drawer()` (it used a `NavigationRail` and a `DrawerSheet` and cross-faded them); use `side()`. `wide_navigation_rail_collapsed` is now the 80 dp icon-only rail in every case: the 96 dp rail with labels under the icons is gone from the suite, since its taller rows would move the icons; `NavigationRail` itself still has it. `NavigationSuiteTokens::drawer_width` moved to `SideNavigationTokens`. The suite's drawer no longer scrolls or shows a text badge at the row's end; a badge is drawn on the icon in every form.

The side list the suite shows is `SideNavigation`, described in [Navigation](components/navigation.md#side-navigation).

## Pane scaffolds

`lamina/widgets/pane_navigator.hpp`, `pane_scaffold.hpp`: Compose's pane scaffold directive, value and navigator (`ThreePaneScaffoldNavigator`, with its back behaviors), and `ThreePaneScaffold`, which lays out a list-detail (`list_detail_pane_navigator`, `list_detail_pane_order`) or supporting-pane (`supporting_pane_navigator`, `supporting_pane_order`) scaffold in one, two or three partitions by window size class, moves panes in and out on springs and goes back on Escape. With `expansion_handle` it has the pane expansion drag handle with anchors: drag or fling it, Enter for the next anchor, Left and Right to nudge. Each pane is a `ContentSlot`; hidden panes take no input, focus or accessibility nodes.

## Split panes

`lamina/widgets/split_pane.hpp`: `SplitPane` divides its space between two layout nodes, side by side or stacked, nestable, with minimum sizes. Its divider shows the drag handle and asks for the resize cursor (`cursor()`) while hovered, follows a drag, and while focused takes the arrow keys, Page Up and Page Down, Home, End and Enter (collapse the first pane and restore it).

## Density

`lamina/tokens/density.hpp`: the density scale (`tokens::Density`: 0, -1, -2 and so on, 4 dp a step, stopping at each component's minimum). Buttons and icon buttons (extra small to medium), text fields, list items and list views, chips and chip groups, tab rows, segmented buttons, top app bars, navigation bars, rails and drawer sheets, and menus take `density` in their options and have `density()` and `set_density`; `WidgetSet::set_density` sets it on every member that has it (`Densifiable`). A dense control takes clicks only on its own bounds instead of a 48 dp area. Density 0 draws as before. The heights, minimums and their sources are in [docs/design-tokens.md](design-tokens.md#density).

## Sources

- Material 3, window size classes: <https://m3.material.io/foundations/layout/applying-layout/window-size-classes>
- Material 3, list-detail layout: <https://m3.material.io/foundations/layout/canonical-layouts/list-detail>
- Material 3, navigation rail: <https://m3.material.io/components/navigation-rail/overview>
- The source files and commits of every value: [design tokens](design-tokens.md)
