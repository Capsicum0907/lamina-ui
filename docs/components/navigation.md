# Navigation

[日本語](navigation.ja.md) · [README](../../README.md#documentation)

## Tabs

`lamina/widgets/tabs.hpp`: primary and secondary tab rows (`TabRow`). The tabs share the width equally; a label can have an icon above it (the row is then 72 dp tall) or before it (`TabIconPosition::leading`). The indicator slides to the selected tab on the default spatial spring. Under a primary tab it is as wide as the label (at least 24 dp); under a secondary tab it spans the tab. Label colors fade in on the default effects spring and out on the fast one, as in Compose. The row is one stop in the Tab order (`Focusable`): while it has the focus, Left and Right select the previous and next tab, wrapping at the ends, and Home and End the first and last, with `handle(KeyEvent)` returning true when the selection changed, as a click does; the selected tab shows the focus state layer and the focus ring drawn inside its edges, as material-web's tabs do (`widgets/item_focus.hpp` holds the key rule the item rows share).

### Scrollable tabs

With `TabRowOptions::scrollable`, the tabs no longer share the width. Each tab is as wide as its content plus 16 dp on each side, at least 90 dp, and the row starts and ends with `edge_padding` (52 dp by default, Compose's `ScrollableTabRowEdgeStartPadding`). When the tabs fit they stay at the start and are not stretched. When they do not fit, the row scrolls:

- Selecting a tab by a click, a key or `select()` scrolls it toward the centre of the row, clamped to the ends, on the default spatial spring (Compose uses the same spring). The indicator moves on its own spring at the same time, so neither jumps.
- The wheel over the row scrolls it by 64 dp a notch (`scroll_area_tokens.wheel_step`); a vertical wheel scrolls it sideways too. `handle(WheelEvent)` returns true when it scrolled.
- Tabs outside the row cannot be clicked, and their accessible bounds are the part that is visible (empty when hidden).
- `scroll_extent()`, `max_scroll()` and `scroll_offset()` give the content width, the range and the current offset. Changing the width keeps the offset in range without a jump.

Drag scrolling with a finger or the mouse is not implemented yet. The gallery uses a scrollable secondary tab row for its fifteen pages, with `edge_padding` set so that the first label lines up with the title.

## Navigation bar

`lamina/widgets/navigation_bar.hpp`: the Expressive short navigation bar, 64 dp tall with items of equal width. The selected item's indicator pill (56 × 32 dp) grows from its center and fades in on the default spatial spring; hover and press show the state layer on the pill. `NavigationItem::selected_style` (`lamina/widgets/navigation_item.hpp`, shared with the rail) gives the selected icon other Material Symbols axes, for example a fill of 1, and the icon moves to them as its color changes. `set_labels(false)` (or `NavigationBarOptions::labels`) hides the labels: on the default spatial spring the bar shortens to 48 dp with the pill centered, and the labels fade out in the first half of the move.

## Navigation rail

`lamina/widgets/navigation_rail.hpp`: the Expressive wide navigation rail (`WideNavigationRail`). Collapsed, it is 96 dp wide with the icon above the label; `set_expanded(true)` widens it to at least 220 dp (up to 360 dp for long labels) and moves each label beside its icon, with the pill growing to 56 dp tall around both. The width, the item heights and gaps and the item layout move on the default spatial spring, so they overshoot a little; at the halfway point the label switches from Label M to Label L and fades out and back in. The selected pill grows from its start edge. `NavigationRailOptions::header` takes a layout node placed at the top, for example a menu button that toggles the rail. The bar and the rail are each one stop in the Tab order (`Focusable`), like the tabs, but their arrow keys move the focus, not the selection, as material-web's navigation bar does: Left and Right on the bar, Up and Down on the rail, wrapping at the ends, and Home and End; Space or Enter then selects the focused item (`handle(KeyEvent)` returns true when the selection changed), since choosing a destination changes the page. Taking the focus starts it on the selected item (`focused_item()`); the focused item shows the focus state layer on its pill and the focus ring around the pill. `set_labels(false)` (or `NavigationRailOptions::labels`) makes the collapsed rail icon-only: on the default spatial spring it narrows to 80 dp with 12 dp beside the 56 × 32 dp pill and 56 dp items, and the labels fade out in the first half of the move; collapsing an expanded rail and hiding its labels together never brings the top label back. `snap_to_targets()` ends the width, label and item springs at once.

## Side navigation

`lamina/widgets/side_navigation.hpp`: `SideNavigation`, one vertical list of destinations that is the icon-only rail (80 dp), the wide rail (220 to 360 dp, as wide as its longest label) and the standard drawer (360 dp), all from the same items. Two values move it: the expansion (`set_expanded`) and the drawer amount (`set_drawer`), each on a critically damped spring. Every icon stays at the same x and y in every form and at every frame: the rows are the drawer's (12 dp from the top, a row as tall as a drawer item), and the icon sits 12 + 16 dp from the start. The selected indicator keeps its start edge at 12 dp and morphs from the 56 × 32 dp pill around the icon to a pill around the icon and label (the wide rail) or to the whole row (the drawer); the container width follows. The labels fade out in the first half of collapsing and in during the second half of expanding, and do not fade between the wide rail and the drawer. `icon_rect`, `label_rect`, `shape_rect`, `indicator_rect`, `item_rect`, `expansion()`, `drawer_amount()` and `label_opacity()` expose the geometry.

## Navigation drawers

`lamina/widgets/navigation_drawer.hpp`: Compose's navigation drawers. `DrawerSheet` lists `DrawerEntry`s: items (an optional icon, a label and a text badge such as an unread count, on 56 dp pills inset 12 dp, the selected one on secondary-container), headlines in Title S, and dividers, with 12 dp above the first; it is 360 dp wide (less in a narrower window), scrolls when its entries are taller than the window, and is one Tab stop whose Up and Down move the focus between enabled items and whose Space or Enter selects. `ModalNavigationDrawer` slides it over a scrim whose alpha follows the open fraction: `open` on the default spatial spring, `close` on the fast effects spring, and a drag anywhere in its area, also across the closed content (`DrawerResponse::took_over` tells the program to cancel its own press), settles on Compose's 256 ms tween; Escape and a click on the scrim close it and its container stretches when the spring overshoots. `DismissibleNavigationDrawer` is a layout node that pushes its content aside by the sheet's width; `permanent_drawer(sheet, content)` puts a standard sheet beside the content. The sheet reports a `navigation` named "Navigation menu" with `navigation_item`, `heading` and `separator` children.

## Modal wide navigation rail

`lamina/widgets/modal_navigation_rail.hpp`: the Expressive `ModalWideNavigationRail`. It keeps the collapsed rail's 96 dp in the layout and expands a `NavigationRail` over the content on a scrim, on surface-container with 16 dp corners; Escape and a click on the scrim collapse it. With `hide_on_collapse` it has no collapsed form: the expanded rail slides in from off the window and can be dragged away. `NavigationRail::paint_items` paints a rail without its container, for this.

## Top app bars

`lamina/widgets/top_app_bar.hpp`, `top_app_bar_state.hpp`: top app bars from Compose's `AppBar.kt` (`TopAppBarKind`, `tokens/top_app_bar.hpp`): small and center-aligned (64 dp, Title L), medium (112 dp, Headline S), large (152 dp, Headline M), and the Expressive medium flexible (112 dp, or 136 dp with a subtitle, Headline M) and large flexible (120 dp, or 152 dp with a subtitle, Display S); the two-row bars collapse to 64 dp. The navigation icon and the actions are icon buttons (on-surface and on-surface-variant, `IconButtonOptions::content`), the title starts 16 dp in or 56 dp in after the navigation icon, and a centered title moves aside only when it would meet the icons. The single-row and flexible bars take a subtitle (Label M in one row, Label L or Title M below a flexible title). `TopAppBarState` holds Compose's `heightOffset`, `heightOffsetLimit`, `contentOffset`, `collapsedFraction` and `overlappedFraction` and applies the three scroll behaviors to the offset of a scroll area whose content starts below the expanded bar (`ListView::top_inset`): `pinned` keeps the bar, `enter_always` hides it as the content scrolls on and shows it on any scroll back, and `exit_until_collapsed` collapses it first and expands it only once the content is back at the top. `TopAppBar::follow(scroll_area)` runs it each frame. A small bar takes the scrolled container (surface-container) on the default effects spring once content is under it; a two-row bar mixes the colors with the collapsed fraction (`FastOutLinearInEasing`), shrinks its bottom row, fades its large title out linearly and the small title in along Compose's `TopTitleAlphaEasing`. When the scroll comes to rest with a bar between 1 % and 100 % collapsed, the bar settles to the nearer end on the default effects spring and the content follows it. The bar is an accessible `toolbar` holding the title, which for a two-row bar is the row more than half shown, and its icon buttons.

## Floating toolbars

`FloatingToolbar` (`lamina/widgets/floating_toolbar.hpp`) is a 64 dp pill of icon actions (`ToolbarAction`), horizontal or vertical (`ToolbarOrientation`), in standard or vibrant colors (`ToolbarColorStyle`), with optional leading and trailing content and an optional FAB (`ToolbarFab`). `set_expanded(false)` folds the bar toward its FAB on the fast spatial spring while the FAB grows from 56 to 80 dp; without a FAB the leading and trailing content shrink away. Scrolling 40 dp toward the end collapses it and 40 dp back expands it. `handle` returns a `FloatingToolbarResponse` with `clicked` (a `ToolbarSlot` and an index) and `fab_clicked`. Values: [Floating toolbars](../design-tokens.md#floating-toolbars).

## Flexible bottom app bar

`FlexibleBottomAppBar` (`lamina/widgets/flexible_bottom_app_bar.hpp`) is a 64 dp bar of actions on surface-container, spaced between or packed 32 dp apart (`BottomBarArrangement`). With `exit_always` it slides out of the bottom edge as the content scrolls down and, when the scroll stops, settles on the fast spatial spring to the nearer end. Values: [Flexible bottom app bar](../design-tokens.md#flexible-bottom-app-bar).

## Sources

- Material 3, tabs: <https://m3.material.io/components/tabs/overview>
- Material 3, navigation bar: <https://m3.material.io/components/navigation-bar/overview>
- Material 3, navigation rail: <https://m3.material.io/components/navigation-rail/overview>
- Material 3, navigation drawer: <https://m3.material.io/components/navigation-drawer/overview>
- Material 3, app bars: <https://m3.material.io/components/app-bars/overview>
- Material 3, toolbars: <https://m3.material.io/components/toolbars/overview>
- Compose Material 3, `TabRow.kt` (scrollable tab row values): <https://github.com/androidx/androidx/blob/androidx-main/compose/material3/material3/src/commonMain/kotlin/androidx/compose/material3/TabRow.kt>
- The source files and commits of every value: [design tokens](../design-tokens.md)
