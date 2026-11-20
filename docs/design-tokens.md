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

At `1b31f4ddbd0d` the two sources disagreed on the three tracking values in bold: each C value was the W value rounded to one decimal place with ties to even (−0.25 → −0.2, 0.15 → 0.2, 0.25 → 0.2), so W was taken to be the unrounded value and lamina-ui used W. Read again on 2026-11-21 at androidx `6068a9492ba8`, C has −0.25, 0.15 and 0.25 too, so the sources now agree.

**Emphasized scale.** C `tokens/TypeScaleTokens.kt` at `6068a9492ba8` has `*EmphasizedSize`, `*EmphasizedLineHeight`, `*EmphasizedTracking` and `*EmphasizedWeight` for all 15 roles, read by `TypographyTokens.kt` into `displayLargeEmphasized` to `labelSmallEmphasized`: the same size, line height and tracking as the standard role; weight 500 (Medium) for Display, Headline, Title L and Body, and 700 (Bold) for Title M, Title S and Label. The values still sit under a "TODO update with the generated tokens once available" note, so they are provisional. lamina-ui has them as the 15 `*_emphasized` roles of `TypeRole`.

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

### Roles

`color::Role` has 53 roles: background, on-background, surface, surface-dim, surface-bright, surface-container-lowest / -low / (default) / -high / -highest, on-surface, surface-variant, on-surface-variant, inverse-surface, inverse-on-surface, outline, outline-variant, shadow, scrim, surface-tint; primary, on-primary, primary-container, on-primary-container, inverse-primary; secondary, tertiary and error with the same four roles each (inverse-primary has no counterpart); primary-, secondary- and tertiary-fixed, -fixed-dim, on-*-fixed, on-*-fixed-variant; and the 2025 roles primary-dim, secondary-dim, tertiary-dim, error-dim.

The 2021 spec has no dim roles (U `ColorSpec2021.primaryDim()` returns null). `spec_2021()` gives each dim role the color of its base role (primary-dim is primary, and so on), as U `ColorSpec2026` does when it maps the dim roles back to their base roles.

### 2025 spec

`lamina/color/spec_2025.cpp` ports U `java/dynamiccolor/ColorSpec2025.java` and the `DynamicScheme` rules it depends on, for the phone platform (U `DynamicScheme.Platform.PHONE`). The watch platform, the 2026 spec, the palette key colors and the Android-only roles (`controlActivated`, `textPrimaryInverse`, ...) are not ported. HCT, the HCT solver and the contrast functions are U's C++ code. Two helpers are taken from Java rather than from U's older C++: hue rotation by hue ranges (`DynamicScheme.getRotatedHue` / `getPiecewiseValue`) and tone 99 on a yellow hue (105° ≤ hue < 125°), which is the average of tones 98 and 100 (`TonalPalette.getHct`).

**Variants.** The 2025 rules cover Neutral, TonalSpot, Vibrant and Expressive. For Monochrome, Fidelity, Content, Rainbow and FruitSalad, U falls back to the 2021 spec and its palettes (`DynamicScheme.maybeFallbackSpecVersion`), so `spec_2025()` returns the colors of `spec_2021()` for them. Their dim roles are what Java computes in that case: the 2025 dim rule evaluated by the 2021 tone algorithm on the 2021 palettes.

**Palettes** (phone; hue "rotated" means the source hue plus a rotation looked up by hue range, from the tables in `spec_2025.cpp`):

| Variant | primary | secondary | tertiary | neutral | neutral-variant | error |
|---|---|---|---|---|---|---|
| Neutral | source, 8 (12 on a blue source, 250° ≤ hue < 270°) | source, 4 (blue 6) | rotated, 20 | source, 1.4 | source, 1.4 × 2.2 | 50 |
| TonalSpot | source, 32 (dark 26) | source, 16 | rotated, 28 | source, 5 | source, 5 × 1.7 | 60 |
| Vibrant | source, 74 | rotated, 56 | rotated, 56 | rotated, 28 | rotated, 28 × 1.29 | 80 |
| Expressive | source, 48 (dark 36) | rotated, 24 (dark 16) | rotated, 48 | rotated, 18 (dark 14, or 6 on a yellow neutral hue) | as neutral, × 2.3 (yellow × 1.6) | 64 |

The error hue is 12°, 22° or 32°, picked by the source hue's range. In 2021 it was always 25° with chroma 84.

**Surfaces** (neutral palette):

| Role | Light (other / Vibrant / yellow neutral hue) | Dark |
|---|---|---|
| surface | 98 / 97 / 99 | 4 |
| surface-dim | 87 / 85 / 90 | 4 |
| surface-bright | 98 / 97 / 99 | 18 |
| surface-container-lowest | 100 | 0 |
| surface-container-low | 96 / 95 / 98 | 6 |
| surface-container | 94 / 92 / 96 | 9 |
| surface-container-high | 92 / 90 / 94 | 12 |
| surface-container-highest | 90 / 88 / 92 | 15 |
| inverse-surface | 4 | 98 |

background is surface, surface-variant is surface-container-highest and surface-tint is primary. The containers, surface-dim (light only) and surface-bright (dark only) multiply the neutral chroma, for example by 1.25 / 1.4 / 1.5 / 1.7 for low / (default) / high / highest in TonalSpot.

**Text and lines on surfaces.** on-surface, on-surface-variant, outline and outline-variant use the neutral palette (no longer neutral-variant) with the chroma multiplied (TonalSpot 1.7, Neutral 2.2, Expressive 1.6), and take their tone from contrast against surface-dim in light mode and surface-bright in dark mode: 9 (dark 11), 4.5 (dark 6), 3 and 1.5. In Vibrant, on-surface starts from the tone of highest chroma at 1.1 × the neutral chroma.

**Accent tones.** Many accent tones are no longer fixed numbers. "Max-chroma tone" steps down from tone 100 while the highest chroma found so far is below the palette's chroma, and keeps the tone with the highest chroma; "min-chroma tone" steps up from 0. Each is then limited to a range. For TonalSpot:

| Role | Light | Dark |
|---|---|---|
| primary | max-chroma tone | 80 |
| primary-container | max-chroma tone, at most 90 | min-chroma tone, 35 to 93 |
| secondary | max-chroma tone | 80 |
| secondary-container | 90 | 25 |
| tertiary | max-chroma tone | max-chroma tone, at most 98 |
| tertiary-container | max-chroma tone | max-chroma tone, at most 93 |
| error | max-chroma tone | min-chroma tone, at most 98 |
| error-container | max-chroma tone, at most 90 | min-chroma tone, 30 to 93 |
| primary-, secondary-, tertiary-dim | max-chroma tone, at most 90 | same |
| error-dim | min-chroma tone | same |

The other variants have their own ranges (and in Expressive and Vibrant, other limits on cyan hues, 170° ≤ hue < 207°, and for an Expressive yellow primary); they are the tables in `spec_2025.cpp`. Then:

- primary, secondary, tertiary and error need contrast 4.5 against surface-dim (light) or surface-bright (dark) and stay at least 5 tones beyond their container: darker in light mode, lighter in dark mode.
- Containers and *-fixed need contrast 1.5 only when the contrast level is above 0.
- Accent and dim roles, and containers and *-fixed when the contrast level is above 0, are kept out of the tones between 49 and 65: a tone of 57 or more is raised to at least 65, a lower one is lowered to at most 49. Surfaces and *-fixed-dim are not moved.
- Each dim role is at least 5 tones darker than its base role and needs contrast 4.5 against surface-container-high.
- *-fixed is the tone the container has in light mode at contrast 0; *-fixed-dim is exactly 5 tones darker.
- on-primary, on-secondary, on-tertiary, on-error, on-*-container and inverse-primary (against inverse-surface) need contrast 6; on-error-container 4.5, inverse-on-surface 7, on-*-fixed 7, on-*-fixed-variant 4.5.

Contrast curves (value at contrast −1 / 0 / 0.5 / 1, linear in between): 1.5 → 1.5 / 1.5 / 3 / 5.5, 3 → 3 / 3 / 4.5 / 7, 4.5 → 4.5 / 4.5 / 7 / 11, 6 → 6 / 6 / 7 / 11, 7 → 7 / 7 / 11 / 21, 9 → 9 / 9 / 11 / 21, 11 → 11 / 11 / 21 / 21. Below contrast 0, a role with a contrast requirement always takes the tone that `foregroundTone` gives for it, instead of keeping its own tone when that already meets the requirement.

`tools/color-dump/` compares every role with the Java code; the result is in its README.

### State of the C++ color library

- U has a C++ port (`cpp/`): HCT, tonal palettes, quantizers, scoring, DynamicScheme, and the scheme variants Monochrome, Neutral, TonalSpot, Vibrant, Expressive, Fidelity, Content, Rainbow, FruitSalad.
- **U's C++ port implements only the 2021 spec.** The Java port has `ColorSpec2021`, `ColorSpec2025` and `ColorSpec2026` (`SpecVersion`). U's C++ has no spec selection and none of the 2025 roles (`primary_dim`, etc.). The last change to `cpp/` was 2024-10-01. The Java port was changed as late as 2026-10.
- It has no build files (Google-internal include paths) and depends on abseil (`flat_hash_map`, `str_cat`) and gtest.

### Decision

lamina-ui started on U's C++ port and the 2021 spec, keeping the layer that decides each role's tone replaceable behind `ColorSpec`. The 2025 spec is now ported from the Java code as `spec_2025()` ("2025 spec" above). It is lamina-ui's own code on top of U's C++ HCT and contrast functions, so the vendored C++ is unchanged and `spec_2021()` still gives the same results. The 2025 spec changes the palettes as well as the tone rules, so the port covers both.

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

### Focus indicator

Read on 2026-11-19 from material-web at `9d2f79851cae` and androidx at `9b9d1fdd16f4`, newer than the commits in the table above.

- W `tokens/_md-comp-focus-ring.scss`: width 3 dp, outward offset 2 dp, inward offset 0, color secondary, and an active width of 8 dp over `duration-long4`.
- W `focus/internal/_focus-ring.scss` draws the ring as an outline outside a box grown by the outward offset, with each corner radius grown by the same offset.
- The ring follows the component's shape. W `button/internal/_shared.scss`, `iconbutton/internal/_shared.scss` and `fab/internal/_shared.scss` give it the container's corners, and W `switch/internal/_switch.scss` gives it the track's. W `checkbox/internal/_checkbox.scss` and `radio/internal/_radio.scss` make it a 44 dp circle: the 40 dp state layer plus the offset.
- Compose Material3 draws no ring around buttons. It shows focus with the state layer at `FocusStateLayerOpacity` 0.1 (C `tokens/StateTokens.kt`), and C `tokens/CheckboxTokens.kt` and `tokens/SwitchTokens.kt` name `FocusIndicatorColor` secondary.

lamina-ui draws both the ring (`lamina/tokens/focus.hpp`) and the focus state layer. The ring fades in and out on the fast effects spring. The 8 dp pulse of material-web is not implemented.

### Keys of the item controls

Read on 2026-11-20 from material-web at `9d2f79851cae` and from the Compose files above.

- Tabs: W `tabs/internal/tabs.ts` makes the row one Tab stop (roving `tabIndex`); ArrowLeft and ArrowRight move to the previous and next tab and wrap, Home and End go to the ends, and with `autoActivate` the moved-to tab is selected. lamina-ui's tab row behaves as `autoActivate` does. W `tabs/internal/tab.ts` draws the focus ring `inward` around the whole tab; the inward offset is 0 (W `tokens/_md-comp-focus-ring.scss`). Compose makes each `Tab` its own `selectable` stop (C `Tab.kt`) with no arrow keys.
- Navigation bar and rail: W `labs/navigationbar/internal/navigation-bar.ts` (a `role="tablist"`) moves the focus with ArrowLeft and ArrowRight, wrapping, and Home and End, without selecting; Enter or Space selects the focused item. lamina-ui follows it, with one Tab stop per bar or rail as in the tabs, Up and Down on the vertical rail, and the ring around the item's pill. Compose makes each item a `selectable` stop (C `NavigationItem.kt`). Whether material-web's bar is one Tab stop was not read (its `navigationtab` file).
- Menu: W `menu/internal/menu.ts` defaults `defaultFocus` to `FIRST_ITEM` (W `menu/internal/controllers/shared.ts`), W `list/internal/list-controller.ts` moves with ArrowUp and ArrowDown, wrapping by default, and Home and End, and W `menu/internal/controllers/menuItemController.ts` closes the menu on Enter, Space or Escape; the focus returns to the element focused before the menu opened. Focus leaving the menu (Tab) closes it without returning the focus; lamina-ui closes on Tab and returns the focus to the opener, as for Escape. W `menu/internal/menuitem/menu-item.ts` draws the item's ring `inward`. Typeahead (W `typeaheadController.ts`) is not implemented.
- Snackbar: holding the time while the action has the focus is lamina-ui's own choice, from WCAG 2.1 success criterion 2.2.1 (Timing Adjustable); it was not read from Compose's `SnackbarHost`.
- Snackbar: the action's focus ring uses inverse-primary, the action's own color, instead of secondary, which has too little contrast on the inverse-surface container. This is lamina-ui's own choice; `ButtonColors::focus_ring` sets the ring's role for any button.
- A text field that accepts tabs leaves on Ctrl+Tab, as `AcceptsTab` text boxes do in WinForms and WPF, so it is no keyboard trap (WCAG 2.1 success criterion 2.1.2).

## Disabled state

Read on 2026-11-21 from androidx at `6068a9492ba8` (C below means that commit in this section): the disabled colors of each component's Compose defaults, which sometimes differ from the token file.

| Component | Container | Content | Other |
|---|---|---|---|
| Filled, elevated button | on-surface 10 % | on-surface-variant 38 % | elevation Level 0 |
| Tonal button | on-surface 12 % | on-surface 38 % | elevation 0 (`FilledTonalButtonTokens` is still `v0_103`) |
| Outlined button | none | on-surface-variant 38 % | outline: outline-variant at 10 % (`outlinedButtonBorder(enabled)` uses `DisabledContainerOpacity`) |
| Text button | none | on-surface-variant 38 % | |
| Filled, tonal icon button | on-surface 10 % | on-surface 38 % | toggles use the same colors whatever the checked state |
| Outlined icon button | none | on-surface 38 % | outline: outline-variant 38 % while unchecked, none while checked (`outlinedIconButtonVibrantBorder`) |
| Standard icon button | none | on-surface 38 % | |
| Switch | track: on-surface 12 % (checked), surface-container-highest 12 % (unchecked) | thumb: surface (checked), on-surface 38 % (unchecked); icon: on-surface 38 %, surface-container-highest 38 % | outline on-surface 12 % while unchecked; every color is composited over surface (`compositeOver(surface)`) |
| Checkbox | box: on-surface 38 % when checked or indeterminate | check mark: surface | outline on-surface 38 % while unchecked |
| Radio button | | on-surface 38 % | |
| Slider | | handle on-surface 38 % composited over surface | active track and stop on-surface 38 %, inactive track on-surface 12 %; ticks crossed as when enabled (active ticks on-surface 12 %, inactive 38 %) |
| Outlined text field | none | text, label, placeholder on-surface 38 % | outline on-surface 12 %, 1 dp |
| Navigation bar and rail item | | icon and label: the unselected color at `DisabledAlpha` (0.38) | |
| Menu item, grouped menu item | the group's container (no selected highlight) | text and icons on-surface 38 % | |

- Compose switches every color at once when `enabled` changes; lamina-ui fades between the enabled and disabled colors on the default effects spring.
- Compose snaps a button's elevation to its disabled level ("No transition when moving to a disabled state" in `Button.kt`); lamina-ui moves it on the elevation spring, so the transition stays continuous.
- `Tab.kt` passes `enabled` only to `selectable` and has no disabled colors, and the tab tokens have none, so a disabled Compose tab looks enabled. lamina-ui draws a disabled tab's icon and label at `DisabledAlpha` (0.38), as the navigation items do; this is lamina-ui's own choice.
- `FloatingActionButton` has no `enabled` parameter and the FAB tokens have no disabled colors, so lamina-ui's FAB has no disabled state.

## Components

### Button

C `tokens/Button{XSmall,Small,Medium,Large,XLarge}Tokens.kt` (height, square and pressed corner, icon size and spacing, leading and trailing space, outline width) and `tokens/{Filled,Tonal,Outlined,Elevated,Text}ButtonTokens.kt` (colors and elevation per state), read into `lamina/tokens/button.hpp`. Choices taken from C `Button.kt` rather than the token files:

- The text button label is `primary`. The token file says `on-surface-variant`, but `Button.kt` overrides it with `Primary` ("replace with the token value once it's corrected").
- The label type follows the height, as `ButtonDefaults.textStyleFor` does: below medium Label L, below large Title M, below extra-large Headline S, otherwise Headline L.
- The extra-small button has 12 dp leading and trailing space, from `ButtonDefaults.ExtraSmallContentPadding` ("update with the value from ButtonXSmallTokens.kt once it's been corrected"); the token file says 16.
- The pressed shape animates with the default effects spring, which `Button.kt` uses "to prevent any bounce in this component". The minimum width is 58 dp (`ButtonDefaults.MinWidth`).

### Outlined text field

C `tokens/OutlinedTextFieldTokens.kt` (56 dp high, extra-small corners, outline 1 dp and 2 dp when focused, Body L input and label, colors per state) and C `internal/TextFieldImpl.kt` (16 dp padding, Body S for the floating label), read into `lamina/tokens/text_field.hpp`. The placeholder, read on 2026-11-21 from C `internal/TextFieldImpl.kt` and `TextFieldDefaults.kt` at `6068a9492ba8`, is Body L in `InputPlaceholderColor` (on-surface-variant), on-surface at 38 % when disabled. Its opacity is 1 while the field has the focus, 0 when the field is unfocused and empty with a label (1 without a label), and 0 when it holds text; it moves on the slow effects spring from unfocused-empty to focused and from unfocused-not-empty to unfocused-empty, and on the fast effects spring otherwise (`placeholderOpacity`). It is drawn only while the text, including an IME composition, is empty, so the first character removes it at once. The selection is `primary` at 0.4 opacity, from `TextSelectionBackgroundOpacity` in C `MaterialTheme.kt`. The undo history keeps 100 steps (`undo_depth`) and ends a typing step after a pause of more than 1 s (`undo_pause_seconds`); Material 3 has no tokens for these, so they are lamina-ui's own values.

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
- Pressing or dragging narrows the handle to half its width (`ThumbContent`). Compose switches the width at once, and so does lamina-ui. Keyboard focus narrows it too, as in Compose (any focus, press or drag interaction halves `thumbSize`).
- Keys, from `slideOnKeyEvents` in C `Slider.kt`: one step, or 1 % of the range without steps (`actualSteps` 100); Page Up/Down move `(actualSteps / 10).coerceIn(1, 10)` steps; Home and End go to the ends (`slider_tokens.keyboard_*`). A horizontal Compose slider takes only Left and Right; lamina-ui also takes Up and Down, as material-web's slider does through the native `<input type=range>` (W `slider/internal/slider.ts`, read at `9d2f79851cae`, adds no key handling of its own). material-web draws the focus ring around the handle (`md-focus-ring` inside the handle element), and lamina-ui does too.

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

### Navigation rail

C `tokens/NavigationRailCollapsedTokens.kt` (96 dp, 44 dp at the top, 4 dp between items, surface), C `tokens/NavigationRailExpandedTokens.kt` (220 to 360 dp), C `tokens/NavigationRailBaselineItemTokens.kt` (64 dp items, 24 dp icon, 40 dp under the header), C `tokens/NavigationRailVerticalItemTokens.kt` (56 × 32 dp pill, 4 dp to the label, Label M), C `tokens/NavigationRailHorizontalItemTokens.kt` (56 dp tall pill, 16 dp leading space, 8 dp to the label, Label L), C `tokens/NavigationRailColorTokens.kt`, C `WideNavigationRail.kt` and C `NavigationItem.kt` (`AnimatedNavigationItem`, `AnimatedMeasurePolicy`), read into `lamina/tokens/navigation_rail.hpp`. lamina-ui has the non-modal `WideNavigationRail` with the default top arrangement.

- Collapsed and expanded are two targets of one set of springs, all on the default spatial spring: the rail's minimum width (96 to 220 dp), its full-range width (96 to 360 dp), the gap between items (4 to 0 dp), the items' minimum height (64 to 48 dp, the minimum interactive size) and each item's icon-position progress. The width follows the measure policy of `WideNavigationRailLayout`: an expanded rail is as wide as the animated minimum width, or as its widest item or header (up to the animated full-range width) when that is wider than 220 dp.
- Items sit at the start of the rail with 20 dp before the pill (`WNRItemHorizontalPadding`, a constant in the .kt) and 20 dp after it while the icon is on top (0 dp once it moves to the start; this padding switches at once, as in Compose). Content smaller than the minimum height is centered, which puts the collapsed pill 6 dp from the item's top.
- The pill's padding around the icon moves from 16 × 4 dp to 16 × 16 dp with the progress, and its size from the icon (24 × 24 dp) to icon, 8 dp and label; it is 56 × 32 dp on top and 56 dp tall at the start, always fully rounded. When an item is selected, the pill's width grows from 0 on the default spatial spring from its start edge (`placeRelativeWithLayer(startPadding, …)`), not from its center as in the navigation bar; its opacity is the same progress.
- The label switches from Label M to Label L and from its top position to its start position when the progress passes 0.5; its opacity is 4 (p − 0.5)². Compose captures whether the icon is on top when the item first appears, so a rail that starts expanded keeps Label L when it collapses; lamina-ui always uses the progress alone.
- Colors: indicator secondary-container, selected icon on-secondary-container, selected label secondary on top and on-secondary-container at the start (the .kt reuses `ItemActiveIcon` there, with a TODO for a missing token), unselected on-surface-variant. Compose animates the label color on the default effects spring and switches the icon color at once; lamina-ui moves both on that spring.
- The state layer is the whole pill at its full width, shown while the pointer is anywhere on the item. Its color is on-surface: the ripple takes the rail's content color, not the `NavigationRailColorTokens` state-layer value (on-secondary-container).
- The header is placed at the top start of the content area, and the items start 40 dp below it (`HeaderSpaceMinimum`).
- Not done: the modal rail (`ModalWideNavigationRail`), the center and bottom arrangements, items without labels, badges, and the narrow 80 dp `NavigationRail`.

### Menu

C `tokens/MenuTokens.kt` (surface-container, extra-small corners, Level2 shadow), C `tokens/ListTokens.kt` (24 dp leading icon, on-surface label, on-surface-variant icon), C `Menu.kt`, C `internal/MenuPosition.kt` and `androidMain/.../AndroidMenu.android.kt`, read into `lamina/tokens/menu.hpp`. This section is the classic dropdown menu (`DropdownMenu`, `DropdownMenuItem`); the Expressive grouped menu has its own section below.

- 8 dp above and below the items, items 48 dp high and 112 to 280 dp wide, 12 dp padding at the sides and between the icon and the label (`DropdownMenuItemHorizontalPadding`), Label L text.
- Position (`MenuAnchorPosition.Below`): vertically the first of below the anchor, above it, centered on its top, and against the window's top or bottom edge that keeps 48 dp from the top and bottom of the window; horizontally the first of start-aligned, end-aligned and against the window's left or right edge, with no horizontal margin (`AndroidMenu` passes 0).
- The menu scales from 0.8 around the point where it meets the anchor (`calculateTransformOrigin`) with the fast spatial spring and fades in with the fast effects spring, and reverses when closed.
- The canvas has no group opacity: lamina-ui fades the menu by fading each color it draws, so overlapping layers composite slightly differently in the middle of the fade.

### Grouped menu

C `tokens/StandardMenuTokens.kt` (surface-container-low, tertiary-container and on-tertiary-container when selected, on-surface label, on-surface-variant icons), C `tokens/SegmentedMenuTokens.kt` (44 dp items, 20 dp icons, corner sizes, 2 dp between groups), C `tokens/MenuTokens.kt` (Level2 shadow), C `tokens/DividerTokens.kt` (1 dp, outline-variant), C `MenuDefaults.kt` and C `Menu.kt` (`DropdownMenuPopup`, `DropdownMenuGroup`, `SelectableDropdownMenuItem`, `CheckableDropdownMenuItem`), read into `grouped_menu_tokens` in `lamina/tokens/grouped_menu.hpp`.

- Groups: each is a surface with a Level2 shadow, 2 dp of padding above and below its content, and 2 dp from the next group (`GroupSpacing`, as the sample puts it). The whole menu is as wide as its widest item or label (`IntrinsicSize.Max`), so every group has the same width. There is no container around the groups.
- Group corners by position: an only group 16 dp; the first 16 dp at the top and 8 dp at the bottom; middle groups 8 dp; the last 8 dp at the top and 16 dp at the bottom. A group that has been hovered and is not hovered now takes 8 dp corners (`inactiveShape`); the corners move on the fast spatial spring. Each group has its own hover state, the default of `DropdownMenuGroup`; the sample shares one interaction source among all groups, which lamina-ui does not do.
- Items: inset 4 dp from the group's sides, 112 to 280 dp wide, at least 44 dp tall, 12 dp padding on every side, a 20 dp leading icon 8 dp before the text and a 20 dp trailing icon at the end. These are the values without `isPrecisionPointerComponentSizingEnabled`, which is off by default. The text is Label L: `Menu.kt` sets `labelLarge` (with a TODO) instead of `SegmentedMenuTokens.ItemLabelTextFont` (Body L).
- Item corners: an only item 4 dp; the first 12 dp at the top and 4 dp at the bottom; middle items 4 dp; the last 4 dp at the top and 12 dp at the bottom. A selected item has 12 dp corners. The corners morph on the fast spatial spring and the container color on the fast effects spring. Compose switches the text and icon colors at once; lamina-ui moves them on the same effects spring.
- The optional group label is Label L in on-surface, at least 32 dp tall, with 12 dp before and 4 dp after it. The optional divider below it has 12 dp at the sides and 2 dp above and below.
- Position: the candidates of the classic menu, with 48 dp from the top and bottom of the window and 8 dp from its sides (`MenuHorizontalMargin`, the default of `DropdownMenuPositionProvider`; the classic menu gets 0 from `AndroidMenu`). The open and close animation is the classic one.
- What a click does is the app's choice in Compose; lamina-ui offers it per group as `MenuSelection`: `none` reports the item and closes the menu, `single` selects one item, `multiple` toggles (like `CheckableDropdownMenuItem`), and the menu stays open for the last two.
- Not done: supporting text, the vibrant colors (`VibrantMenuTokens.kt`), submenus, and a selected leading icon on an item without a leading icon (Compose expands it horizontally); `selected_leading_icon` only replaces an existing leading icon.

### Dialog

C `tokens/DialogTokens.kt` (surface-container-high, extra-large corners, Headline S, Body M, colors), C `tokens/ScrimTokens.kt` (scrim at 0.32) and C `AlertDialog.kt` (24 dp padding, 16 dp under the headline, 24 dp under the text, 280 to 560 dp wide, 8 dp between the buttons), read into `lamina/tokens/dialog.hpp`.

- The dialog has no shadow: `AlertDialog` uses no shadow and a tonal elevation of 0, so `ContainerElevation` (Level3) is not used.
- The optional icon (`DialogTokens.IconSize` 24 dp, `IconColor` secondary) is centered above the headline with 16 dp below it (`IconPadding`), and the headline is then centered (`AlertDialogContent`).
- The buttons are text buttons in an `AlertDialogFlowRow`: a flow row in the flipped layout direction with 8 dp between buttons, so on one line dismiss comes before confirm at the end, and when they do not fit, confirm stays on the first line and dismiss moves to the next, both at the end. Compose spaces the lines by 8 dp minus the 8 dp by which a button's 48 dp touch target exceeds its 40 dp height, that is 0 dp between 48 dp slots. lamina-ui lays the buttons out at their 40 dp height, so it puts that 8 dp back and the lines are 8 dp apart, as in Compose. The 4 dp of touch target below the last line is not added, so the last button is 24 dp from the bottom edge where Compose has 28 dp.
- A headline that wraps to several lines is centered as a block as wide as its longest line. A wrapping Compose `Text` most likely takes the full maximum width, so with an icon its lines would start at the start edge (not verified). Single-line headlines are the same.
- Compose leaves the enter and exit animation to the platform window. lamina-ui's own animation scales the surface from 0.8 with the default spatial spring and fades it and the scrim with the default effects spring.
- Content (read at `6068a9492ba8`): `AlertDialogContent` puts the `text` composable in a `Box` with `weight(1f, fill = false)` and 24 dp below it, so arbitrary content fits there and shrinks when the dialog is too tall. Compose does not scroll it; the caller adds `verticalScroll`. lamina-ui puts the supporting text and `DialogOptions::content` in that slot and scrolls it in a `ScrollArea` itself. The height limit (the area less `window_margin`, 48 dp, above and below) and the 16 dp between the supporting text and the content (`supporting_text_content_space`) are lamina-ui's own: Compose leaves the height to the platform dialog window and has a single text slot. The dividers that the M3 guidelines show at the edges of scrolling dialog content are not drawn; Compose draws none either.

### Snackbar

C `tokens/SnackbarTokens.kt` (inverse-surface container, extra-small corners, Level 3, Body M in inverse-on-surface, Label L action in inverse-primary, 48 dp for one line and 68 dp for two), C `Snackbar.kt` (600 dp at most, 12 dp around it, 16 dp before the text, 8 dp after the action, 8 dp between the text and the action, 14 dp above and below the text) and C `SnackbarHost.kt` (4 s short and 10 s long durations; the snackbar scales from 0.8 on the fast spatial spring and fades on the fast effects spring), read into `lamina/tokens/snackbar.hpp`.

- The height counts the lines of the message at the Body M line height (20 dp), so one line is 48 dp and two are 68 dp whatever the font's own ascent and descent.
- Compose gives a snackbar with an action an indefinite duration by default; lamina-ui uses the short duration unless told otherwise, and holds the time while the pointer is over the snackbar, which Compose does not.

### Tooltip

Read on 2026-11-21 from C `Tooltip.kt`, `internal/BasicTooltip.kt`, `tokens/PlainTooltipTokens.kt` and `tokens/RichTooltipTokens.kt` (`VERSION: v0_210`) at `6068a9492ba8`, into `lamina/tokens/tooltip.hpp`.

- Plain: inverse-surface container, inverse-on-surface Body S, extra-small (4 dp) corners, 8 × 4 dp padding (`PlainTooltipContentPadding`), at least 40 × 24 dp (`TooltipMinWidth`, `TooltipMinHeight`), at most 200 dp wide (`plainTooltipMaxWidth`), no shadow.
- Rich: surface-container container, medium (12 dp) corners, Level 2 shadow (`ContainerElevation`), at most 320 dp wide, 16 dp at the sides. The title (Title S, on-surface-variant) has its first baseline 28 dp below the top (`HeightToSubheadFirstLine`); the text (Body M, on-surface-variant) has its first baseline 24 dp below the title when there is a title or an action (`HeightFromSubheadToTextFirstLine`) and 16 dp below it, or 4 dp above and below otherwise; the action (Label L, primary) is at least 36 dp tall with 8 dp below (`ActionLabelMinHeight`, `ActionLabelBottomPadding`). lamina-ui draws the action as a small text button.
- Position (`TooltipPositionProviderImpl`): 4 dp from the anchor (`SpacingBetweenTooltipAndAnchor`); above: centered horizontally, below when it would leave the top, clamped to the window; below, left and right in the same way. The caret is drawn only when a caret shape is passed (none by default); `caretSize` is 16 × 8 dp and it points at the middle of the anchor (`caretX`). Compose draws the caret outside the tooltip with the same 4 dp spacing, so its tip reaches 4 dp into the anchor; lamina-ui adds the caret's height to the spacing, so the tip stays 4 dp from the anchor.
- Motion: scale from 0.8 on the fast spatial spring and alpha on the fast effects spring (`MotionSchemeKeyTokens.FastSpatial`, `FastEffects`).
- Showing (`BasicTooltip.kt`): a mouse entering the anchor shows the tooltip at once, with no delay, and leaving dismisses it unless it is persistent; a touch or stylus held for the long-press timeout shows it until it is lifted; keyboard focus on the anchor shows it, losing the focus dismisses it, and Escape dismisses it. A tooltip shown by focus or long press and not persistent dismisses itself after 1500 ms (`TooltipDuration`); one shown by hover stays while the pointer is there.
- lamina-ui's choices: the hover delay is a token, `hover_show_delay_seconds`, set to Compose's 0, and `TooltipOptions::hover_delay_seconds` changes it per tooltip. The long-press time is 0.5 s (`long_press_seconds`), lamina-ui's own value; Compose uses the platform's `ViewConfiguration`. Compose shows one tooltip at a time through a shared `MutatorMutex`; lamina-ui does not enforce that. A press outside the anchor and the tooltip dismisses any shown tooltip. The heights count the lines of text at the type's line height, as the snackbar does. The rich action takes the keyboard focus through `FocusGated`, as the snackbar's action does, instead of Compose's forced focus into the popup on Tab.

### Circular progress indicator

C `tokens/CircularProgressIndicatorTokens.kt` (40 dp, 48 dp for the wavy one, 4 dp indicator and track, 4 dp between them, a 1.6 dp wave amplitude, a 15 dp wavelength), C `tokens/ProgressIndicatorTokens.kt` (primary indicator, secondary-container track) and C `WavyProgressIndicator.kt` with `internal/CircularWavyProgressModifiers.kt`, read into `lamina/tokens/progress_indicator.hpp`.

- The indicator starts at the top and runs clockwise. The track runs from the indicator's end to the top, leaving the gap plus both round caps between them at each end; at zero progress the gaps close and the track is a full circle, as in `CircularProgressDrawingCache.updateDrawPaths`.
- `WavyProgressIndicatorDefaults.indicatorAmplitude` gives the full wave between 10 % and 95 % progress and none outside; the wave moves one wavelength per second (`waveSpeed = wavelength`).
- Compose draws the wave by morphing a circle into a star with an inner radius of 0.75, normalised and scaled to the size, so the depth of the wave is a fixed share of the diameter and grows with it (lamina-ui did not render Compose's version for comparison). lamina-ui draws the wave as a sine of the arc length around the circle instead, with the amplitude and the wavelength in dp, so it looks the same at any size, and the wavelength does not have to divide the circumference (the ends never meet while the wave is on, so a resize does not make the wave jump).
- Compose changes the amplitude with 500 ms tweens (standard easing up, emphasized-accelerate down). lamina-ui moves it on the slow effects spring, and a widget that is not wavy (`set_wavy(false)`) flattens its wave, so a paused timer stops animating.
- Compose's stroke has round caps and a zero-length segment draws nothing. lamina-ui narrows an arc shorter than its thickness in proportion, so a growing indicator or a shrinking track does not appear or vanish as a whole dot.
- `sample_length` (1 dp) is lamina-ui's own: the arcs are filled as outlines (`render::stroke_path`), sampled every 1 dp along the arc.

### Scroll area

Material 3 defines no scroll bar, and Compose Material 3 has none: every value in `lamina/tokens/scroll_area.hpp` is lamina-ui's own, none was read from a source.

- The offset moves on the default effects spring because it is critically damped: a spatial spring of the expressive scheme overshoots, which would scroll past the ends of the content.
- 64 dp per wheel notch, 48 dp kept on screen by Page Up and Page Down, a 4 dp thumb that widens to 8 dp under the pointer, 2 dp from the edge, at least 16 dp long, `on_surface` at 38 % (50 % under the pointer), a 12 dp strip at the end edge that counts as over the bar, and hiding 1 s after scrolling stops.

### Elevation tweens

Read on 2026-11-21 from C `internal/Elevation.kt` at `6068a9492ba8`, into `lamina/tokens/elevation.hpp` (`elevation_tweens`) and `widgets/interaction_elevation.hpp`.

- Compose moves a chip's or a card's elevation with tweens, not springs: toward a hover, focus, press or drag in 120 ms with `FastOutSlowInEasing` (0.4, 0, 0.2, 1); back to rest in 150 ms with (0.4, 0, 0.6, 1), or in 120 ms after a hover. The interaction that started last decides the target (`interactions.lastOrNull()`), and a disabled component snaps to its disabled elevation. lamina-ui does the same with `motion::Tweened`. The buttons of the first stage still move their elevation on a spring.

### Divider

C `Divider.kt` and `tokens/DividerTokens.kt` (`VERSION: v0_117`) at `6068a9492ba8`: 1 dp of outline-variant, as wide as the space (horizontal) or as tall (vertical), into `lamina/tokens/divider.hpp`.

- Compose has no inset: a caller pads the divider. C `tokens/ListTokens.kt` has `DividerLeadingSpace` and `DividerTrailingSpace` of 16 dp that nothing in Compose uses, and material-web's divider (W at `9d2f79851cae`, `divider/internal/_divider.scss`) pads 16 px for `inset`, `inset-start` and `inset-end`. lamina-ui's `DividerInset` takes those 16 dp.
- The accessible role `separator` is lamina-ui's; Compose's divider has no semantics.

### Badge

C `Badge.kt` and `tokens/BadgeTokens.kt` at `6068a9492ba8`, into `lamina/tokens/badge.hpp`.

- Small badge: 6 dp (`Size`), full corners. Large badge: at least 16 dp (`LargeSize`), full corners, 4 dp inside the ends (`Badge.kt`), Label S. Both error with on-error content.
- `BadgedBox`: a badge wider than 6 dp has content; its start is 12 dp before the anchor's end edge (`BadgeWithContentHorizontalOffset`) and its bottom 14 dp below the anchor's top (`BadgeWithContentVerticalOffset`), so a 16 dp badge rises 2 dp above the anchor. A dot uses 6 dp for both (`BadgeOffset`), which puts it inside the top end corner. The `BadgeEndRuler` and `BadgeTopRuler` set by `Modifier.badgeBounds()` on the navigation items keep it inside the item; without them nothing is clamped. lamina-ui applies the item's bounds on the navigation bar and rail and no bounds on icon buttons.
- In Compose the badge goes on the icon (`IconButton { BadgedBox { Icon } }`, the icon slot of a navigation item); lamina-ui anchors it to the icon's 24 dp square, also while the rail moves its icons.
- Compose does not shorten a count. `"999+"` appears only in the samples (`samples/BadgeSamples.kt`); `badge_count` with its maximum of 999 is lamina-ui's helper for that style.
- Compose gives the badge no semantics and `NavigationBarItem` clears the icon's semantics while a label is shown; lamina-ui puts the badge's description in the item's or button's accessible description.

### Chips

C `Chip.kt` and `tokens/{Assist,Filter,Input,Suggestion}ChipTokens.kt` at `6068a9492ba8`, into `lamina/tokens/chip.hpp`.

- 32 dp minimum height, small (8 dp) corners, Label L, 18 dp icons, 24 dp avatar (full corners, 38 % alpha while disabled). Content padding 8 dp at both ends and `ChipArrangement(8 dp)`, which puts 8 dp between the leading slot, the label and the trailing slot even when a slot is empty, so a bare label has 16 dp at each side. Input chips pad 4 dp at the start with an avatar or without a leading icon and 8 dp with an icon, and 8 dp at the end with a trailing icon, 4 dp without (`InputChipDefaults.contentPadding`).
- Colors (`*ChipDefaults`): assist flat: no container, on-surface label, primary icons, 1 dp outline-variant outline; elevated: surface-container-low, no outline. Suggestion: like assist with an on-surface-variant label. Filter: on-surface-variant label, primary leading and on-surface-variant trailing icon; selected: secondary-container with on-secondary-container content and no outline (0 dp). Input: like filter, with an on-surface-variant leading icon that turns primary when selected. Disabled: content on-surface at 38 %, outline on-surface at 12 %, elevated and selected containers on-surface at 12 %; a disabled chip uses the disabled colors whatever its selection.
- Elevation: flat 0 (the flat filter chip rises to 1 dp on hover); elevated 1 dp, 3 dp on hover, 0 when disabled; 8 dp while dragged, which lamina-ui has no use for yet.
- Filter and input chips are `Role.Checkbox` with the selection; assist and suggestion chips `Role.Button`. The touch target is 48 dp.
- `AnimatingChipContent`: the leading slot appears with `expandHorizontally` on the fast spatial spring and a fade-in on the slow effects spring, and leaves with `shrinkHorizontally` on the default effects spring and a fade-out on the fast effects spring; replacing one icon with another changes it at once. Colors and the outline switch at once in Compose; lamina-ui fades them on the default effects spring.
- Not taken over: the `shapes:` overloads (`ChipShapes`, a morph between 12 dp, full and 8 dp corners on the fast spatial spring) and the tonal colors that go with them; dragging.
- lamina-ui's own: Compose's trailing icon is part of the chip and has no action. lamina-ui gives a `removable` input chip a remove target on its trailing icon (an accessible button named "Remove …" inside the chip, with a state layer of its own), as material-web's input chip has a remove button, and removes it on Delete or Backspace, which material-web does not. `ChipGroup` follows material-web's chip set (W `chips/internal/chip-set.ts`, `_chip-set.scss`): 8 px gaps, wrapping, one Tab stop with ArrowLeft and ArrowRight wrapping past disabled chips, Home and End. Compose has no chip group (its samples put chips in a `FlowRow` with 4 dp padding around each) and makes every chip its own Tab stop. The fade and shrink of a removed chip and the glide of the chips that move are lamina-ui's.

### Card

C `Card.kt`, `Surface.kt` and `tokens/{Elevated,Filled,Outlined}CardTokens.kt` at `6068a9492ba8`, into `lamina/tokens/card.hpp`.

- Medium (12 dp) corners for all three. Containers: elevated surface-container-low, filled surface-container-highest, outlined surface with a 1 dp outline-variant border; content on-surface.
- Disabled (`CardDefaults.*cardColors`): filled surface-variant at 38 % over surface-container-highest; elevated surface at 38 % over surface, which is surface; outlined surface; content on-surface at 38 %. The outlined border is outline at 12 % composited over the elevated card's container, surface-container-low (`outlinedCardBorder`), not over surface; lamina-ui follows it.
- Elevation: filled 0, 1 dp on hover; elevated 1 dp, 3 dp on hover, 1 dp when disabled; outlined 0 in every state (`Card.kt` sets its pressed, focused and hovered elevations to the default, so its hover token is unused); dragged 6 or 8 dp, unused in lamina-ui. A card without `onClick` has a fixed elevation; a clickable one moves with the elevation tweens.
- `Surface` draws the border over the content (`Modifier.border`) and clips the content to the shape; lamina-ui does both.
- A clickable card is a `clickable` `Surface` with no role and a 48 dp minimum; a plain one is a container (`isContainer`). lamina-ui reports `button` and `group`. Compose leaves a disabled card's content to `LocalContentColor`; lamina-ui paints the whole content at 38 %, since its labels carry their own color roles.

### List

C `ListItem.kt`, `ListItemDefaults.kt` and `tokens/ListTokens.kt` at `6068a9492ba8`, into `lamina/tokens/list.hpp`. lamina-ui follows the interactive `ListItem` overloads (the classic one, with 8 and 12 dp vertical padding, is deprecated).

- Padding 16 dp at the start and end and 10 dp above and below (`ListItemDefaults.ContentPadding`), 12 dp between the slots and the text (`ItemBetweenSpace`). Minimum heights 56, 72 and 88 dp for one, two and three lines; three lines with both an overline and supporting text or with supporting text over more than one line. The slots and the text are centered while the inside is under 60 dp, (88 + 72) / 2 − 10 − 10, and aligned to the top above it.
- Headline Body L in on-surface; overline Label S, supporting text Body M and the slots in on-surface-variant; trailing text Label S. Selected or checked: secondary-container with on-secondary-container content. Disabled: content on-surface at 38 % on surface, whatever the selection.
- Shapes: extra-small (4 dp) at rest, medium (12 dp) on hover, large (16 dp) when focused, pressed or selected (`shapeForInteraction`), on the fast spatial spring; colors on the default effects spring (`updateTransition`); state layers from the content color.
- A selectable item is `Role.RadioButton` with `selected`, a toggleable one `Role.Checkbox` with its state, which also gives it the selected colors; a clickable one has no role (lamina-ui: `list_item`).
- Compose defines but does not enforce slot sizes; lamina-ui sizes the slots by the tokens: icons 24 dp, avatars 40 dp (primary-container with on-primary-container Title M initials), images 56 dp with small corners (`ItemLeadingImageExpressiveShape`), and a checkbox or radio button 24 dp and a switch 52 × 32 dp, the sizes Compose's controls take inside an item. A control in a slot follows the item, as in Compose's samples where it has no `onCheckedChange`.
- Keyboard: Compose makes each item its own focus stop. `ListView` follows material-web's list (W `list/internal/list-controller.ts`): one stop, ArrowUp and ArrowDown wrapping past disabled rows, Home and End; Page Up and Page Down and the inward focus ring of the row are lamina-ui's. Compose shows the pressed shape while Space or Enter is held; lamina-ui has no key release, so a key acts at once without the pressed shape.
- `ListView`, the row height estimates and `overscan_rows` (2) are lamina-ui's; Compose's lazy lists are a separate library.

### Top app bar

C `AppBar.kt` and `tokens/AppBar{,Small,Medium,Large,MediumFlexible,LargeFlexible}Tokens.kt` at `6068a9492ba8`, into `lamina/tokens/top_app_bar.hpp`.

- Heights: small and center-aligned 64 dp; medium 112; large 152; medium flexible 112, or 136 with a subtitle; large flexible 120, or 152 with a subtitle; the two-row bars collapse to 64. Titles: Title L in one row; Headline S (medium), Headline M (large, medium flexible), Display S (large flexible) in the bottom row; the top row of a two-row bar uses Title L, with a Label M subtitle. Subtitles: Label M in one row, Label L (medium flexible) and Title M (large flexible) in the bottom row. The flexible bars use the plain type roles, not the emphasized ones.
- Colors: surface, surface-container when scrolled; navigation icon and title on-surface, actions and subtitle on-surface-variant. No shadow.
- 4 dp at both ends (`TopAppBarHorizontalPadding`); the title starts at max(12 dp, the navigation slot) plus 4 dp, so 16 dp without and 56 dp with a 48 dp navigation slot; a centered title is centered on the whole bar and pushed in only when it would cross the icons. The bottom title sits 24 dp (medium) or 28 dp (large) from the bottom, measured from its last baseline.
- `TopAppBarState`: `heightOffsetLimit` is minus the bar's height (single row) or minus the bottom row's (two rows); `collapsedFraction` is `heightOffset / limit`; `overlappedFraction` is `|contentOffset| / |limit|`, capped at 1. `pinned` only counts the content; `enter_always` moves the bar by every scroll first; `exit_until_collapsed` takes scrolls toward the end first and gives the bar back only what the content cannot use at its top.
- Small bars: the container takes the scrolled color when `overlappedFraction` passes 0.01, on the default effects spring. Two-row bars: `lerp(container, scrolled, FastOutLinearInEasing(f))` with no animation; the bottom title fades as 1 − f and the top title as `TopTitleAlphaEasing` (0.8, 0, 0.8, 0.15); the bottom row shrinks and clips its title.
- Settling: Compose settles in `onPostFling`, after a fling or a drag of the bar: below 1 % or at 100 % it stays, otherwise it goes to the nearer end with `snapAnimationSpec`, the default effects spring. lamina-ui has no fling; it settles when the scroll area comes to rest (no target left and no thumb drag) and moves the content with the bar.
- Not taken over: the fling of the bar's own drag (lamina-ui's bar cannot be dragged), and the swallowing in `onPreScroll`, which hands the whole delta to the bar when it moved at all, even past its limit; lamina-ui hands the rest to the content.
- Semantics: Compose's bar is a traversal group with no role and its titles have no `heading`; lamina-ui reports a `toolbar` with a `label` for the title. A two-row bar exposes the top title from half collapsed on, as `hideTopRowSemantics` does.

## Open questions

- Components other than those above are not surveyed yet.
- Not implemented from the components of the second stage: the `shapes:` chip overloads (`ChipShapes`), `SegmentedListItem` with its 2 dp gaps and per-position corners, dragged chips, cards and list items, and dragging a top app bar itself.
