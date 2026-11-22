# Differences from Material 3

[日本語](differences-from-m3.ja.md) · [README](../README.md#documentation)

Where lamina-ui does not follow Material 3 and its Compose implementation: values it chose itself, components Material 3 does not have, and behaviour that differs. Each item links to the section of [Design tokens](design-tokens.md) that gives the full values and the reason.

## Components Material 3 does not have

- Scroll area: Material 3 defines no scroll bar, so every value in `lamina/tokens/scroll_area.hpp` is lamina-ui's own, including horizontal scrolling. Touch scrolling follows Compose and Android, except that there is no overscroll stretch and no nested scrolling, and a text field takes the focus on a touch tap rather than on the press. [Scroll area](design-tokens.md#scroll-area), [Scroll area: horizontal](design-tokens.md#scroll-area-horizontal), [Touch scrolling](design-tokens.md#touch-scrolling)
- Side navigation: one list that is the icon-only rail, the wide rail and the drawer. Its spring (damping 1.0, stiffness 380) and the label fade are lamina-ui's own. [Side navigation](design-tokens.md#side-navigation)
- Split panes, the numeric field and the menu bar: built from M3 roles with lamina-ui's own sizes. [Split panes](design-tokens.md#split-panes), [Numeric field](design-tokens.md#numeric-field), [Menu bar](design-tokens.md#menu-bar)
- Data table: the values come from MDC Web's data table where it has one, and are lamina-ui's own otherwise. [Table](design-tokens.md#table)
- Docked date picker: Compose has none; the popup values come from material-web, and the 4 dp gap and the scale-in from 0.8 are lamina-ui's own. [Date picker](design-tokens.md#date-picker)
- Side sheet header and actions: no source specifies them; the 72 dp header and the action row are lamina-ui's own. [Side sheets](design-tokens.md#side-sheets)

## Values lamina-ui chose

- Navigation suite: `widening_navigation_suite` (compact: bar, medium: collapsed rail, expanded: expanded rail, large and wider: drawer) is lamina-ui's own chooser, and the bar and the side list change on a spring instead of being swapped. [Navigation suite scaffold](design-tokens.md#navigation-suite-scaffold)
- Hiding the labels of the navigation bar and the rail is lamina-ui's own addition. [Navigation bar](design-tokens.md#navigation-bar), [Navigation rail](design-tokens.md#navigation-rail)
- Dialog: Compose leaves the enter and exit animation to the platform; lamina-ui scales the surface from 0.8 on the default spatial spring and fades it and the scrim on the default effects spring. [Dialog](design-tokens.md#dialog)
- Snackbar: a snackbar with an action gets the short duration unless told otherwise; the time is held while the pointer is over it or its action has the focus (WCAG 2.1, 2.2.1); the action's focus ring is inverse-primary. [Snackbar](design-tokens.md#snackbar), [Keys of the item controls](design-tokens.md#keys-of-the-item-controls)
- Tooltip: the long-press time is 0.5 s, and the hover delay is a token. [Tooltip](design-tokens.md#tooltip)
- Drag gestures: a drag starts after 8 dp, releases are clamped to 8000 dp/s, and there is no decay phase. [Drag gestures](design-tokens.md#drag-gestures)
- Density: a scale below a component's minimum is clamped instead of rejected, and the minimums of segmented buttons, navigation items and menus are lamina-ui's own. [Density](design-tokens.md#density)
- Circular progress indicator: the arcs are sampled every 1 dp (`sample_length`). [Circular progress indicator](design-tokens.md#circular-progress-indicator)
- Scrollable tabs: the 90 dp minimum width, the 52 dp edge padding and the default spatial spring are Compose's; the wheel step (64 dp, the scroll area's) is lamina-ui's own. [Tabs](design-tokens.md#tabs)

## Behaviour that differs

- Colors that Compose switches at once cross-fade on the default effects spring: segmented buttons, toggle buttons, split buttons and the date picker's selected day. [Segmented button](design-tokens.md#segmented-button), [Toggle buttons and per-corner shapes](design-tokens.md#toggle-buttons-and-per-corner-shapes)
- Disabled tabs are drawn at 38 %; Compose's look enabled. [Disabled state](design-tokens.md#disabled-state)
- Outlined icon buttons use each size's border width (2 dp large, 3 dp extra-large); Compose always uses 1 dp. [Icon buttons](design-tokens.md#icon-buttons)
- The state layer of switches, checkboxes and radio buttons is on-surface instead of the local content color. [State layers of the selection controls](design-tokens.md#state-layers-of-the-selection-controls)
- A removable input chip has its own remove target and is removed with Delete or Backspace. [Chips](design-tokens.md#chips)
- A segmented button row is one Tab stop with arrow keys; the date picker follows the WAI-ARIA date grid keys. [Segmented button](design-tokens.md#segmented-button), [Date picker](design-tokens.md#date-picker)
- A segment label or a split button's leading label that does not fit is cut to one line with an ellipsis, where a Compose `Text` with its defaults wraps; a segmented row stops shrinking at a minimum that keeps each segment's paddings, icon slot and an ellipsis, where Compose's weighted segments have none. [Segmented button](design-tokens.md#segmented-button), [Split buttons](design-tokens.md#split-buttons)
- In 24-hour mode the time picker's handle follows the pointer, and 0 and 00 are invalid hours in 12-hour mode. [Time picker](design-tokens.md#time-picker)
- A disabled floating toolbar dims its FAB and stops it reacting; Compose's FABs have no disabled state. [Floating toolbars](design-tokens.md#floating-toolbars)
- The 48 dp touch target widens the hit area only: it takes no room in the layout, where Compose's `minimumInteractiveComponentSize` reserves 48 dp. It works for the mouse and the pen as well as for touch, where Compose widens touches only, and a negative density turns it off. [Touch target](design-tokens.md#touch-target)

The design tokens page also lists the smaller deviations of each component under "Deviations".
