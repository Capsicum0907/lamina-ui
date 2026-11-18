# Design tokens (survey)

[日本語](design-tokens.ja.md)

The values lamina-ui takes from Material 3 Expressive, read from Google's open-source implementations. Every value below was read from source files, not from the design site. Spot checks were made against the raw files on 2026-11-18.

## Sources

| Key | Repository | Commit (2026-11-18) | License |
|---|---|---|---|
| C | [androidx/androidx](https://github.com/androidx/androidx) — `compose/material3/material3/src/commonMain/kotlin/androidx/compose/material3/` | `1b31f4ddbd0d` | Apache-2.0 |
| G | androidx/androidx — `graphics/graphics-shapes/src/commonMain/kotlin/androidx/graphics/shapes/` | same | Apache-2.0 |
| M | [material-components/material-components-android](https://github.com/material-components/material-components-android) — `lib/java/com/google/android/material/motion/res/values/tokens.xml` | `6061a69915d0` | Apache-2.0 |
| W | [material-components/material-web](https://github.com/material-components/material-web) — `tokens/versions/latest/sass/` | `c6e9a66360b1` | Apache-2.0 |
| U | [material-foundation/material-color-utilities](https://github.com/material-foundation/material-color-utilities) | `f242efc74afc` | Apache-2.0 |

Reusing these values requires keeping the Apache-2.0 notice and attribution.

## Motion

### Springs

From C `tokens/StandardMotionTokens.kt` and `tokens/ExpressiveMotionTokens.kt` (`VERSION: v0_14_0`). M `m3_sys_motion_{scheme}_spring_{speed}_{kind}_{damping|stiffness}` has the same values.

| Scheme | Speed | Spatial damping | Spatial stiffness | Effects damping | Effects stiffness |
|---|---|---|---|---|---|
| standard | fast | 0.9 | 1400 | 1.0 | 3800 |
| standard | default | 0.9 | 700 | 1.0 | 1600 |
| standard | slow | 0.9 | 300 | 1.0 | 800 |
| expressive | fast | 0.6 | 800 | 1.0 | 3800 |
| expressive | default | 0.8 | 380 | 1.0 | 1600 |
| expressive | slow | 0.8 | 200 | 1.0 | 800 |

- Damping is the damping ratio ζ (1.0 = critically damped, below 1 overshoots). Stiffness is k for a unit mass.
- Effects springs are the same in both schemes. Only spatial springs differ.
- Spatial: animations that change the shape or bounds of a component. Effects: animations that do not, such as color and alpha (C `MotionScheme.kt`, KDoc).

### Durations and easing

C `tokens/MotionTokens.kt` (`VERSION: v0_103`). Not cross-checked against M.

| Token | Value |
|---|---|
| Short1–4 | 50 / 100 / 150 / 200 ms |
| Medium1–4 | 250 / 300 / 350 / 400 ms |
| Long1–4 | 450 / 500 / 550 / 600 ms |
| ExtraLong1–4 | 700 / 800 / 900 / 1000 ms |

| Easing (cubic-bezier) | Value |
|---|---|
| Emphasized | (0.2, 0.0, 0.0, 1.0) |
| EmphasizedAccelerate | (0.3, 0.0, 0.8, 0.15) |
| EmphasizedDecelerate | (0.05, 0.7, 0.1, 1.0) |
| Standard | (0.2, 0.0, 0.0, 1.0) |
| StandardAccelerate | (0.3, 0.0, 1.0, 1.0) |
| StandardDecelerate | (0.0, 0.0, 0.0, 1.0) |
| Legacy | (0.4, 0.0, 0.2, 1.0) |
| LegacyAccelerate | (0.4, 0.0, 1.0, 1.0) |
| LegacyDecelerate | (0.0, 0.0, 0.2, 1.0) |
| Linear | (0.0, 0.0, 1.0, 1.0) |

Emphasized and Standard have the same value in the source.

## Shape

### Corner radius

C `tokens/ShapeTokens.kt` (`VERSION: 14_1_0`). W `_md-sys-shape.scss` agrees.

| Step | dp |
|---|---|
| none | 0 |
| extra-small | 4 |
| small | 8 |
| medium | 12 |
| large | 16 |
| large-increased | 20 |
| extra-large | 28 |
| extra-large-increased | 32 |
| extra-extra-large | 48 |
| full | half of the shorter side |

One-sided variants exist: ExtraSmallTop (4, 4, 0, 0), LargeTop, LargeStart, LargeEnd, ExtraLargeTop (28, 28, 0, 0).

### Shape library

C `MaterialShapes.kt` defines 35 shapes as normalized `RoundedPolygon`s (unit square, center (0.5, 0.5)):

Circle, Square, Slanted, Arch, Fan, Arrow, SemiCircle, Oval, Pill, Triangle, Diamond, ClamShell, Pentagon, Gem, Sunny, VerySunny, Cookie4Sided, Cookie6Sided, Cookie7Sided, Cookie9Sided, Cookie12Sided, Ghostish, Clover4Leaf, Clover8Leaf, Burst, SoftBurst, Boom, SoftBoom, Flower, Puffy, PuffyDiamond, PixelCircle, PixelTriangle, Bun, Heart.

How they are built:

- Shared roundings: `CornerRounding(radius)` with radius 0.15 / 0.2 / 0.3 / 0.5 / 1.0. G `CornerRounding(radius, smoothing ∈ [0, 1])`.
- Examples: Circle = `circle(numVertices = 10)`. Square = `rectangle(1, 1, rounding 0.3)`. Triangle = 3 vertices, rounding 0.2, rotated −90°. Sunny = `star(8 per radius, innerRadius 0.8, rounding 0.15)`.
- Most shapes are seed points, each with its own rounding, repeated `reps` times around the center, optionally mirrored.

### Morphing

G `Morph.kt`:

1. Measure both outlines by arc length.
2. Map each corner of one shape to the nearest corner of the other. Cut and shift the second outline so that progress 0 lines up. Split cubics until both outlines are lists of cubic Béziers paired one to one.
3. At progress t, interpolate each pair of cubics (8 floats) linearly. t slightly outside [0, 1] is allowed, so a spring can overshoot the shape.

## Typography

C `tokens/TypeScaleTokens.kt` (`VERSION: v0_103`) and W `_md-sys-typescale.scss`. Sizes in sp. Weight Regular = 400, Medium = 500, Bold = 700.

| Style | Size / line height | Weight | Tracking (C) | Tracking (W) |
|---|---|---|---|---|
| Display L | 57 / 64 | 400 | **−0.2** | −0.25 |
| Display M | 45 / 52 | 400 | 0 | 0 |
| Display S | 36 / 44 | 400 | 0 | 0 |
| Headline L | 32 / 40 | 400 | 0 | 0 |
| Headline M | 28 / 36 | 400 | 0 | 0 |
| Headline S | 24 / 32 | 400 | 0 | 0 |
| Title L | 22 / 28 | 400 | 0 | 0 |
| Title M | 16 / 24 | 500 | **0.2** | 0.15 |
| Title S | 14 / 20 | 500 | 0.1 | 0.1 |
| Body L | 16 / 24 | 400 | 0.5 | 0.5 |
| Body M | 14 / 20 | 400 | **0.2** | 0.25 |
| Body S | 12 / 16 | 400 | 0.4 | 0.4 |
| Label L | 14 / 20 | 500 | 0.1 | 0.1 |
| Label M | 12 / 16 | 500 | 0.5 | 0.5 |
| Label S | 11 / 16 | 500 | 0.5 | 0.5 |

The two sources disagree on the three tracking values in bold. Each C value is the W value rounded to one decimal place with ties to even (−0.25 → −0.2, 0.15 → 0.2, 0.25 → 0.2), so W is taken to be the unrounded value. lamina-ui uses W.

**Emphasized scale.** Same sizes and line heights. Weights: Display, Headline, Title L and Body are 500. Title M, Title S and Label are 700. W keeps the standard tracking. C has separate emphasized tracking values under a "TODO update with the generated tokens" note, so they are provisional.

## Color

### Roles and tones (2021 spec, TonalSpot)

U `cpp/dynamiccolor/material_dynamic_colors.cc`, at contrast level 0. At run time a role's tone can still move to meet its contrast curve against its background.

| Role | Light | Dark |
|---|---|---|
| primary / on-primary | 40 / 100 | 80 / 20 |
| primary-container / on-primary-container | 90 / 30 | 30 / 90 |
| inverse-primary | 80 | 40 |
| secondary, tertiary, error | same as primary | same as primary |
| surface / on-surface | 98 / 10 | 6 / 90 |
| surface-dim / surface-bright | 87 / 98 | 6 / 24 |
| surface-container lowest / low / (default) / high / highest | 100 / 96 / 94 / 92 / 90 | 4 / 10 / 12 / 17 / 22 |
| surface-variant / on-surface-variant | 90 / 30 | 30 / 80 |
| inverse-surface / inverse-on-surface | 20 / 95 | 90 / 20 |
| outline / outline-variant | 50 / 80 | 60 / 30 |
| surface-tint | 40 | 80 |
| shadow, scrim | 0 | 0 |
| *-fixed / *-fixed-dim | 90 / 80 | 90 / 80 |
| on-*-fixed / on-*-fixed-variant | 10 / 30 | 10 / 30 |

C `ColorLightTokens.kt` (`VERSION: v0_210`) uses 10 for on-*-container instead of 30. U changed this to 30 in "Update MCU to for expressive on-colors" (2024-06-11).

TonalSpot palettes (U `cpp/scheme/scheme_tonal_spot.cc`):

| Palette | Hue | Chroma |
|---|---|---|
| primary | source | 36 |
| secondary | source | 16 |
| tertiary | source + 60° | 24 |
| neutral | source | 6 |
| neutral-variant | source | 8 |

Baseline primary: #6750A4 (C `PaletteTokens.Primary40`).

### State of the C++ color library

- U has a C++ port (`cpp/`): HCT, tonal palettes, quantizers, scoring, DynamicScheme, and the scheme variants Monochrome, Neutral, TonalSpot, Vibrant, Expressive, Fidelity, Content, Rainbow, FruitSalad.
- **The C++ port implements only the 2021 spec.** The Java port has `ColorSpec2021`, `ColorSpec2025` and `ColorSpec2026` (`SpecVersion`). C++ has no spec selection and none of the 2025 roles (`primary_dim`, etc.). The last change to `cpp/` was 2024-10-01. The Java port was changed as late as 2026-10.
- It has no build files (Google-internal include paths) and depends on abseil (`flat_hash_map`, `str_cat`) and gtest.

### Decision

Start with the C++ port on the 2021 spec, and replace it with a C++ port of the 2025 spec later. HCT and tonal palettes are shared by both specs. Only the layer that decides each role's tone differs, so that layer is kept replaceable.

## Elevation and state layers

C `tokens/ElevationTokens.kt` (`VERSION: v0_210`): Level0–5 = 0 / 1 / 3 / 6 / 8 / 12 dp.

Shadows follow W `elevation/internal/_elevation.scss`: two box shadows whose offset, blur and spread are piecewise-linear in the level, so a level between two steps is well defined.

| Level | Key shadow (opacity 0.3): y / blur | Ambient shadow (opacity 0.15): y / blur / spread |
|---|---|---|
| 0 | 0 / 0 | 0 / 0 / 0 |
| 1 | 1 / 2 | 1 / 3 / 1 |
| 2 | 1 / 2 | 2 / 6 / 2 |
| 3 | 1 / 3 | 4 / 8 / 3 |
| 4 | 2 / 3 | 6 / 10 / 4 |
| 5 | 4 / 4 | 8 / 12 / 6 |

A CSS blur radius is twice the Gaussian standard deviation.

C `tokens/StateTokens.kt` (`VERSION: v0_103`), state layer opacity: hover 0.08, focus 0.10, pressed 0.10, dragged 0.16.

## Components

### Button

C `tokens/Button{XSmall,Small,Medium,Large,XLarge}Tokens.kt` (height, square and pressed corner, icon size and spacing, leading and trailing space, outline width) and `tokens/{Filled,Tonal,Outlined,Elevated,Text}ButtonTokens.kt` (colors and elevation per state), read into `lamina/tokens/button.hpp`. Choices taken from C `Button.kt` rather than the token files:

- The text button label is `primary`. The token file says `on-surface-variant`, but `Button.kt` overrides it with `Primary` ("replace with the token value once it's corrected").
- The label type follows the height, as `ButtonDefaults.textStyleFor` does: below medium Label L, below large Title M, below extra-large Headline S, otherwise Headline L.
- The extra-small button has 12 dp leading and trailing space, from `ButtonDefaults.ExtraSmallContentPadding` ("update with the value from ButtonXSmallTokens.kt once it's been corrected"); the token file says 16.
- The pressed shape animates with the default effects spring, which `Button.kt` uses "to prevent any bounce in this component". The minimum width is 58 dp (`ButtonDefaults.MinWidth`).

### Outlined text field

C `tokens/OutlinedTextFieldTokens.kt` (56 dp high, extra-small corners, outline 1 dp and 2 dp when focused, Body L input and label, colors per state) and C `internal/TextFieldImpl.kt` (16 dp padding, Body S for the floating label), read into `lamina/tokens/text_field.hpp`. The selection is `primary` at 0.4 opacity, from `TextSelectionBackgroundOpacity` in C `MaterialTheme.kt`.

### Button icons

C `Button.kt` (`ButtonDefaults.iconSizeFor`, `iconSpacingFor`, `contentPaddingFor`): the icon is 20 / 20 / 24 / 32 / 40 dp from extra-small to extra-large, and the leading and trailing space do not change when there is an icon. The extra-small icon spacing is 4 dp (`ExtraSmallIconSpacing`, "use the value from ButtonXSmallTokens.kt once it's been corrected"); the token file says 8.

### Icon buttons

C `tokens/{XSmall,Small,Medium,Large,XLarge}IconButtonTokens.kt` (height, icon size, narrow / default / wide side space, square, pressed and selected corners, outline width), `tokens/{Filled,FilledTonal,Outlined}IconButtonTokens.kt` and `tokens/IconButtonTokens.kt` (`StandardIconButtonTokens`, colors), read into `lamina/tokens/icon_button.hpp`. From C `IconButton.kt` and `IconButtonDefaults.kt`:

- The uniform width is the icon plus the default side space on both sides (`*ContainerSize(IconButtonWidthOption.Uniform)`), which makes it square at every size. Large has `UniformLeadingSpace` in place of `DefaultLeadingSpace`.
- The shape is the pressed shape while pressed, else the checked shape when checked, else the rest shape, animated with the default effects spring ("to prevent any bounce").
- The outlined and standard styles take `LocalContentColor` by default in Compose; lamina-ui has no content color, so they use the token colors (Compose's `*VibrantColors`).
- Compose's default outlined border is always `SmallIconButtonTokens.OutlinedOutlineWidth` (1 dp). lamina-ui uses each size's own `OutlinedOutlineWidth` (2 dp large, 3 dp extra-large), as the button does.
- Compose switches the colors of a toggled icon button at once; lamina-ui fades them with the default effects spring.

### FAB

C `tokens/Fab{Baseline,Medium,Large}Tokens.kt` (size, corner, icon size) and `tokens/Fab{Primary,Secondary}ContainerTokens.kt` (colors, elevation 3, hover 4, pressed 3), read into `lamina/tokens/fab.hpp`. From C `FloatingActionButton.kt`:

- The medium FAB corner is large-increased (20 dp, `ShapeDefaults.LargeIncreased`); the token is commented out with a TODO.
- The large FAB icon is 36 dp (`LargeIconSize`, "FabLargeTokens.IconSize is incorrect"); the token file says 32.
- A FAB does not change shape when pressed.

### Switch

C `tokens/SwitchTokens.kt` (track 52 × 32 dp with a 2 dp outline, thumb 16 / 24 / 28 dp, icon 16 dp, colors), read into `lamina/tokens/switch.hpp`. From C `Switch.kt`:

- The thumb position and size use the fast spatial spring (`MotionSchemeKeyTokens.FastSpatial`). The thumb's center travels from 16 dp to 36 dp (track width minus track height).
- The thumb is 24 dp when checked or when it carries an icon, and 28 dp while pressed.
- The colors depend only on checked: the hover and pressed handle colors of the token file (primary-container, on-surface-variant) are not used. The checked track has no outline (`Color.Transparent`); the unchecked outline is `UnselectedFocusTrackOutlineColor` (outline).
- Compose switches the colors at once; lamina-ui fades them with the default effects spring, so the change is continuous like the thumb.
- Dragging the thumb is not implemented; a click toggles.

### Checkbox

C `tokens/CheckboxTokens.kt` (colors, 2 dp outline, 40 dp state layer) and C `Checkbox.kt`, read into `lamina/tokens/checkbox.hpp`. `Checkbox.kt` draws the shipped geometry while `ComposeMaterial3Flags.isCheckboxStylingFixEnabled` is false (the default), so lamina-ui follows it rather than the token file:

- The box is 20 dp with 2 dp padding (`CheckboxSize`, `CheckboxDefaultPadding`), not the 18 dp `ContainerSize`. The corner is 2 dp and the stroke 2 dp with square caps.
- The check runs (0.2, 0.5) → (0.4, 0.7) → (0.8, 0.3) of the box. In the indeterminate state these points move toward y = 0.5 (and the corner toward x = 0.5), giving a dash.
- The draw fraction and the move to the dash use the default spatial spring (clamped to the whole mark). Turning off, both snap to 0 after 100 ms (`SnapAnimationDelay`).
- The box and check colors fade in with the default effects spring and out with the fast one.

### Radio button

C `tokens/RadioButtonTokens.kt` (20 dp icon, 40 dp state layer, colors) and C `RadioButton.kt` (2 dp padding, 12 dp dot, 2 dp stroke), read into `lamina/tokens/radio_button.hpp`. The dot radius animates on the fast spatial spring and the color on the default effects spring; the dot is drawn with its radius minus half the stroke, as in `RadioButton.kt`.

### State layers of the selection controls

Switch, checkbox and radio button draw an unbounded 40 dp circle as their state layer. Compose uses `LocalContentColor` for that ripple; lamina-ui uses on-surface, the content color on a surface. The touch target is at least 48 dp (`minimumInteractiveComponentSize`), also for small icon buttons.

### Slider

C `tokens/SliderTokens.kt` (16 dp track, 4 × 44 dp handle, 2 dp pressed handle, 6 dp handle gap, 4 dp stop indicator, colors) and C `Slider.kt`, read into `lamina/tokens/slider.hpp`. From `Slider.kt`:

- The track runs from half the handle width to the other end minus half the handle width. The gap on each side of the handle is half the 4 dp handle box plus 6 dp, so it does not change while the handle narrows. The corners next to the handle are 2 dp (`TrackInsideCornerSize`), the outer ones half the track height.
- The pointer sets the value linearly over that range, and the value then snaps to the nearest step. With steps, the handle and ticks are drawn inset by the track's corner radius except on the first and last step, as `SliderImpl` places them.
- The tick colors are crossed in `SliderDefaults.colors`: a tick on the active part is secondary-container, a tick on the inactive part is primary. The last tick, where the stop indicator sits, and ticks inside the handle gap are not drawn. The stop indicator is the active track color.
- Pressing or dragging narrows the handle to half its width (`ThumbContent`). Compose switches the width at once; lamina-ui moves it with the fast spatial spring. Focus, which also narrows the handle in Compose, is not implemented.
- Keys: Left/Right move by one step, or by 1 % of the range without steps; Home and End go to the ends. Page Up/Down are not implemented.

### Tabs

C `tokens/PrimaryNavigationTabTokens.kt` and `tokens/SecondaryNavigationTabTokens.kt` (48 dp row, 3 dp indicator, Title S label, colors), C `tokens/DividerTokens.kt`, and C `Tab.kt` and `TabRow.kt`, read into `lamina/tokens/tabs.hpp`. From the .kt files rather than the token files:

- A tab with an icon above its label is 72 dp high (`LargeTabHeight`); `IconAndLabelTextContainerHeight` says 64. Its label baseline is 14 dp above the indicator and the icon ends 20 dp above that baseline. A leading icon is 8 dp before the label (`LeadingIconTab`).
- The primary indicator is as wide as the tab's content (the label without its 16 dp side padding), at least 24 dp, centered on the tab, with 3 dp corners. The secondary indicator spans the tab and is 3 dp high, because `SecondaryIndicator` takes its height from the primary token.
- The divider is `HorizontalDivider` (1 dp outline-variant), not the secondary token's surface-variant.
- The indicator's offset and width move with the default spatial spring and are placed without animation the first time. Label and icon colors fade in with the default effects spring and out with the fast effects spring (`TabTransition`). The state layer covers the tab in the selected content color.
- Compose's `Tab` makes the unselected color equal to the selected one unless a color is passed. lamina-ui takes the inactive colors of the token files (on-surface-variant).

### Navigation bar

C `tokens/NavigationBarTokens.kt` (64 dp, surface-container, colors, Label M), `tokens/NavigationBarVerticalItemTokens.kt` (56 × 32 dp indicator, 24 dp icon, 6 dp item padding) and C `ShortNavigationBar.kt` and `NavigationItem.kt`, read into `lamina/tokens/navigation_bar.hpp`. lamina-ui has the short navigation bar with equal-width items and icons above labels.

- The indicator is 6 dp below the top of the item and the label 4 dp below the indicator (`TopIconIndicatorToLabelPadding`).
- The indicator's width is 56 dp times a progress that moves with the default spatial spring, so it grows from its center and overshoots a little; its opacity is the same progress, clamped to 1.
- The state layer is the whole 56 × 32 dp pill, shown while the pointer is anywhere on the item. Its color is on-surface (the bar's content color).
- The container has no elevation: `ShortNavigationBar` does not use `ContainerElevation` (Level2).
- Compose switches the icon and label colors at once; lamina-ui fades them with the default effects spring.

### Menu

C `tokens/MenuTokens.kt` (surface-container, extra-small corners, Level2 shadow), C `tokens/ListTokens.kt` (24 dp leading icon, on-surface label, on-surface-variant icon), C `Menu.kt`, C `internal/MenuPosition.kt` and `androidMain/.../AndroidMenu.android.kt`, read into `lamina/tokens/menu.hpp`. lamina-ui has the classic dropdown menu (`DropdownMenu`, `DropdownMenuItem`), not the Expressive grouped menu (`DropdownMenuPopup`, `StandardMenuTokens.kt`).

- 8 dp above and below the items, items 48 dp high and 112 to 280 dp wide, 12 dp padding at the sides and between the icon and the label (`DropdownMenuItemHorizontalPadding`), Label L text.
- Position (`MenuAnchorPosition.Below`): vertically the first of below the anchor, above it, centered on its top, and against the window's top or bottom edge that keeps 48 dp from the top and bottom of the window; horizontally the first of start-aligned, end-aligned and against the window's left or right edge, with no horizontal margin (`AndroidMenu` passes 0).
- The menu scales from 0.8 around the point where it meets the anchor (`calculateTransformOrigin`) with the fast spatial spring and fades in with the fast effects spring, and reverses when closed.
- The canvas has no group opacity: lamina-ui fades the menu by fading each color it draws, so overlapping layers composite slightly differently in the middle of the fade.

### Dialog

C `tokens/DialogTokens.kt` (surface-container-high, extra-large corners, Headline S, Body M, colors), C `tokens/ScrimTokens.kt` (scrim at 0.32) and C `AlertDialog.kt` (24 dp padding, 16 dp under the headline, 24 dp under the text, 280 to 560 dp wide, 8 dp between the buttons), read into `lamina/tokens/dialog.hpp`.

- The dialog has no shadow: `AlertDialog` uses no shadow and a tonal elevation of 0, so `ContainerElevation` (Level3) is not used.
- The buttons are text buttons at the end, dismiss before confirm. They do not wrap to a second line as `AlertDialogFlowRow` would.
- Compose leaves the enter and exit animation to the platform window. lamina-ui's own animation scales the surface from 0.8 with the default spatial spring and fades it and the scrim with the default effects spring.

## Open questions

- Components other than those above are not surveyed yet.
