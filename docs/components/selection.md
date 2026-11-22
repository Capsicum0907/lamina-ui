# Selection

[日本語](selection.ja.md) · [README](../../README.md#documentation)

## Switch

`lamina/widgets/switch.hpp`: the switch. The thumb slides and grows (16 dp off, 24 dp on or with an icon, 28 dp while pressed) on the fast spatial spring, so it overshoots a little in the expressive scheme; the thumb can carry an icon for each state.

## Checkbox

`lamina/widgets/checkbox.hpp`: the checkbox with off, on and indeterminate states. The check mark is drawn in along its length on the spatial spring and leans into the indeterminate dash; when unchecked it vanishes after 100 ms while the box fades, as in Compose.

## Radio button

`lamina/widgets/radio_button.hpp`: the radio button. The dot grows on the fast spatial spring.

## Slider

`lamina/widgets/slider.hpp`: the Expressive slider. A 4 × 44 dp handle splits the 16 dp track into an active part (primary) and an inactive part (secondary container), with a 6 dp gap on each side of the handle and 2 dp corners toward it; a 4 dp dot marks the end. Pressing anywhere on the slider or dragging sets the value from the pointer, also when the pointer leaves the slider while dragging, and the handle narrows to 2 dp at once while held, as in Compose. With `SliderOptions::steps` the value snaps to evenly spaced ticks. The slider is `Focusable`, and `handle(KeyEvent)` moves it only while it has the keyboard focus: Right or Up and Left or Down by one step (1 % without steps), Page Up and Page Down by a tenth of the steps (one to ten steps, so 10 % without steps), Home and End to the ends, returning true when the value changed. Focus narrows the handle as a press does and draws the focus ring around the handle; a press takes the focus away, so a click alone gives the slider no keys. `SliderOptions::orientation` makes it vertical, as Compose's `VerticalSlider`: a 44 × 4 dp handle on a 16 dp wide track, the minimum at the top unless `top_to_bottom` is false, and only Up, Down, Page Up, Page Down, Home and End, in the direction the slider is drawn. `centered` draws Compose's `CenteredTrack`: the active part runs from the middle to the handle, with a 6 dp gap on the side away from the handle and stop dots at both ends. `track_corner` sets the outer corner of the track (half its thickness by default) and, as Compose's `Track(trackCornerSize)` does, keeps the inactive part down to zero width when there are no steps. `value_label` (a function from the value to its text, with the constructor that takes the fonts) shows a value indicator above the handle while it is held or focused, and the text becomes the accessible value: a pill of inverse-surface with the label in Label L, 12 dp above the handle, that scales in from its bottom over 100 ms on the emphasized curve, on horizontal sliders only. Compose has only the tokens for it (`SliderTokens.ValueIndicator*`), so the size and the motion come from material-web's `labeled` slider. A slider is `Measurable`: a horizontal one takes the width it is given and a vertical one the height, so `widgets::node` places it as before. `lamina/widgets/slider_track.hpp` holds the track drawing, the snapping and the key steps that the slider and the range slider share.

## Range slider

`lamina/widgets/range_slider.hpp`: Compose's `RangeSlider`. Two handles on one track, with the active part between them, inactive parts outside them and stop dots at both ends; `steps` snaps both to the same ticks. A press moves the nearer handle (on a tie, the start handle when the press is before them and the end handle otherwise), and when the handles meet, the direction of the drag picks the one that moves; they meet but never cross. Each handle is its own Tab stop: `thumb(RangeThumb::start)` and `thumb(RangeThumb::end)` are `Focusable` and `FocusGated` objects that a program adds to its `FocusManager`, and the keys are the slider's, with Home and End stopping at the other handle. The range slider is an accessible `group` with a `slider` for each handle, named by `RangeSliderOptions::names` (Compose's "Range start" and "Range end" by default), whose range ends at the other handle. `value_label` shows an indicator on each handle, as on the slider.

## Chips

`lamina/widgets/chip.hpp`, `chip_group.hpp`: assist, filter, input and suggestion chips (`ChipKind`), flat or elevated (`ChipOptions::elevated`; input chips have no elevated variant), 32 dp tall with 8 dp corners and Label L, 16 dp around the label or 8 dp before an 18 dp icon, as Compose's `ChipArrangement` places them; an input chip pads 4 dp at a bare or avatar edge and 8 dp at an icon. The colors, outlines and elevations are those of Compose's `*ChipDefaults` (`tokens/chip.hpp`). Filter and input chips toggle on a click, Space or Enter and report themselves as checkboxes with their selection, as Compose's `Role.Checkbox` does; assist and suggestion chips are buttons. A filter chip's `selected_leading_icon` (a checkmark) replaces its leading icon while selected: a chip without a leading icon grows the slot on the fast spatial spring and fades the icon in on the slow effects spring, and shrinks it on the default effects spring while fading it out on the fast one, as Compose's `AnimatingChipContent` does; a chip that has a leading icon swaps it at once. The selection colors fade on the default effects spring, where Compose switches them at once. An input chip takes an `avatar` (a `render::Image` clipped to a 24 dp circle, at 38 % while disabled) and, with `removable` and a trailing icon, a remove target: a click on the trailing icon, or Delete or Backspace while the chip has the focus, reports `ChipResponse::removed` instead of toggling. The elevation moves with Compose's elevation tweens (120 ms `FastOutSlowInEasing` toward a state; 150 ms, or 120 ms from hover, back) and snaps when the chip is disabled, as Compose's `animateElevation` does (`widgets/interaction_elevation.hpp`, also used by cards). `ChipGroup` holds chips and wraps them onto lines 8 dp apart in both directions, as material-web's chip set does; `single_selection` keeps one selected. The group is one Tab stop: Left and Right move the focus between its enabled chips, wrapping, Home and End go to the ends, and Space, Enter, Delete and Backspace reach the focused chip. A removed chip shrinks to nothing and fades on the default effects spring while the chips after it close up, and a chip that moves to another line glides there on the default spatial spring; `ChipGroupResponse` reports the clicked or removed index and the removed label.

## Sources

- Material 3, switch: <https://m3.material.io/components/switch/overview>
- Material 3, checkbox: <https://m3.material.io/components/checkbox/overview>
- Material 3, radio button: <https://m3.material.io/components/radio-button/overview>
- Material 3, sliders: <https://m3.material.io/components/sliders/overview>
- Material 3, chips: <https://m3.material.io/components/chips/overview>
- The source files and commits of every value: [design tokens](../design-tokens.md)
