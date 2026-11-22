# Actions

[日本語](actions.ja.md) · [README](../../README.md#documentation)

Buttons and the components built from them.

## Buttons

`lamina/widgets/button.hpp`: the Material 3 Expressive button in five styles (filled, tonal, outlined, elevated, text), five sizes and two shapes. Pressing morphs the corners to the pressed shape with the effects spring, as Compose does; hover and press add the state layer and change the elevation. `ButtonOptions::leading_icon` puts an icon before the label, sized and spaced by the size tokens; with an empty label the icon alone is centered. A button is as tall as its size token and as wide as its content; `set_size(width, height)` gives it any other size (the calculator's keys fill their grid cells this way), while the size token still sets the label, the icon and the corners, and a round button stays round at the new height. `ButtonOptions::colors` replaces the style's container and content roles, as Compose's `ButtonDefaults.buttonColors` does; the style still decides the elevation and the outline. A container of `std::nullopt` leaves the button without one, for a text button with its own content color on another surface, such as a snackbar's action. `set_label` changes the label (a button that has not been given a size refits its width), and `set_content_scale(label, icon)` draws the label and the icon larger or smaller than their tokens, so the calculator's glyphs grow with its keys. `set_shape(shape)` morphs the corners between round and square on the fast spatial spring, so a toggle such as the timer's pause/resume key overshoots a little, and `set_icon` cross-fades from the old icon to the new one on the fast effects spring, and `ButtonOptions::icon_style` draws the icon with other Material Symbols axes (a fill of 1 for the timer's play and pause). The press morph is mixed on top of the shape, so both can move at once. `cancel_interaction()` drops the hover and press without a pointer event, so a press in progress no longer clicks; every widget that takes pointer input has it (on a slider it ends the drag, on a text field the drag selection, on a dialog the press on its buttons), a dialog uses it on its buttons when it closes, and `WidgetSet::cancel_interaction()` passes it to the members that have it. `build/Release/buttons.exe` shows them all.

## Toggle buttons

`ButtonOptions::toggle` (`ButtonToggle`) makes a button a toggle button: `checked` is its state, and it reports `toggle_button` with on or off, or `radio_button` when it is a radio toggle, which a click never unchecks. At rest it is a pill; while pressed it takes the size's pressed corner and when checked the size's square corner, all corners morphing with one progress on the fast spatial spring. The checked colors fade on the default effects spring. `toggle_button_shapes(size)` returns the shapes of the M3 toggle button for a size. Values: [Toggle buttons and per-corner shapes](../design-tokens.md#toggle-buttons-and-per-corner-shapes).

## Icon buttons

`lamina/widgets/icon_button.hpp`: icon buttons in four styles (filled, tonal, outlined, standard), five sizes, two shapes and three widths (narrow, uniform, wide). Like the button, pressing morphs the corners with the effects spring; a toggleable one also changes shape and colors when checked. `set_icon` cross-fades to another icon on the fast effects spring, as the button's does.

## FAB

`lamina/widgets/fab.hpp`: the floating action button in three sizes (FAB, medium, large) on the primary or secondary container, raised to level 3 and to level 4 on hover.

## Extended FAB

`ExtendedFab` (`lamina/widgets/extended_fab.hpp`) is a FAB with a label, in three sizes (`tokens::ExtendedFabSize`): small 56 dp, medium 80 dp and large 96 dp. `set_expanded(false)` collapses it to a square FAB: the width goes to the height on the fast spatial spring and the label fades on the fast effects spring, while the icon stays in place. Like Compose's, it has no disabled state. Values: [Extended FABs](../design-tokens.md#extended-fabs).

## FAB menu

`FabMenu` (`lamina/widgets/fab_menu.hpp`) is a toggle FAB (`ToggleFabOptions`) that opens a column of labelled items (`FabMenuItem`) above it, aligned to the start or the end (`FabMenuAlignment`). Opening shrinks the button to 56 dp with a 28 dp corner and changes it from primary-container to primary; the items appear one by one from the one nearest the button. `handle` returns a `FabMenuResponse` with `toggled` and `chosen`. Tab or Down moves from the open button into the items, Enter chooses, Escape closes. Values: [FAB menu](../design-tokens.md#fab-menu).

## Segmented buttons

`lamina/widgets/segmented_button.hpp`: Compose's `SingleChoiceSegmentedButtonRow` and `MultiChoiceSegmentedButtonRow` (`SegmentSelection`). Segments are 40 dp tall, as wide as the widest one (at least 58 dp), padded 12 dp at the sides, with an 18 dp icon slot and 8 dp before the Label L label, and their 1 dp outline overlaps the next segment's; the first and last segments round their outer corners fully. A checked segment fills with secondary-container. A segment without an icon reserves the icon slot, shifting its label 13 dp back while unchecked: checking it scales the checkmark in from the bottom start on the fast spatial spring while the label slides over, and unchecking removes it at once while the label slides back, as Compose's `SegmentedButtonDefaults.Icon` does; a segment with an icon cross-fades it with the checkmark on the default effects spring. A segment may show only an icon, with an `accessible_name`. The row is one Tab stop: in a single-choice row Left, Right, Home and End select, as a radio group does; in a multi-choice row they move the focus and Space or Enter toggles. Segments report themselves as radio buttons or toggle buttons inside a `group`.

## Button groups

`ButtonGroup` (`lamina/widgets/button_group.hpp`) lays out `ButtonGroupItem`s 12 dp apart, or 2 dp apart with connected shapes (`ButtonGroupOptions::connected`). `selection` is `none` (plain buttons), `single` (radio toggles) or `multiple` (toggle buttons). A pressed button widens into its neighbours by up to 15 % of its width while they shrink, so the row keeps its width (`animate_width`). Buttons that do not fit move into an overflow menu behind a "More options" icon button. `handle` returns a `ButtonGroupResponse` with `clicked`. Values: [Button groups](../design-tokens.md#button-groups).

## Split buttons

`SplitButton` (`lamina/widgets/split_button.hpp`) is a leading button and a trailing chevron 2 dp apart, in the five button sizes (32, 40, 56, 96 and 136 dp) and the filled, tonal, outlined and elevated styles. Checking the trailing half turns the chevron half a turn and opens `SplitButtonOptions::menu` under the whole button; choosing an item or dismissing the menu unchecks it. `handle` returns a `SplitButtonResponse` with `leading_clicked`, `trailing_toggled` and `chosen`. Values: [Split buttons](../design-tokens.md#split-buttons).

## Sources

- Material 3, buttons: <https://m3.material.io/components/buttons/overview>
- Material 3, icon buttons: <https://m3.material.io/components/icon-buttons/overview>
- Material 3, FAB: <https://m3.material.io/components/floating-action-button/overview>
- Material 3, extended FAB: <https://m3.material.io/components/extended-fab/overview>
- Material 3, segmented buttons: <https://m3.material.io/components/segmented-buttons/overview>
- Material 3, button groups: <https://m3.material.io/components/button-groups/overview>
- Material 3, split button: <https://m3.material.io/components/split-button/overview>
- Material 3, FAB menu: <https://m3.material.io/components/fab-menu/overview>
- The source files and commits of every value: [design tokens](../design-tokens.md)
