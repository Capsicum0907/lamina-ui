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
| A | androidx/androidx — `compose/material3/adaptive/` (adaptive, adaptive-layout, adaptive-navigation), `compose/material3/material3-adaptive-navigation-suite/` and `window/window-core/` | `6068a9492ba8` | Apache-2.0 |
| D | [material-components/material-components-web](https://github.com/material-components/material-components-web) — `packages/mdc-density/` and the components' `$density-config` | `6ceadb81df50` | MIT |
| S | The Android SDK as installed by Android Studio: SDK Platform `android-36.1` (revision 1), `platforms/android-36.1/data/res/values/config.xml`, and Sources for Android `android-36.1` (revision 1), `sources/android-36.1/android/view/ViewConfiguration.java`; for the zoom view also `ScaleGestureDetector.java` and `GestureDetector.java` in the same folder, and for the pager `sources/android-36.1/com/android/internal/widget/ViewPager.java` (all read on 2026-11-23) | read on 2026-11-22 | Apache-2.0 |
| J | [JetBrains/compose-multiplatform-core](https://github.com/JetBrains/compose-multiplatform-core) — `compose/foundation/foundation/src/desktopMain/` and `compose/ui/ui/src/desktopMain/` (Compose Multiplatform Desktop) | the `jb-main` branch as read on 2026-11-21 (no commit was recorded) | Apache-2.0 |

Reusing these values requires keeping the Apache-2.0 notice and attribution (the MIT notice for D).

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

The ThorVG canvas blurs each shadow shape once and draws it from a cache, at the nearest whole pixel ([Canvas](app-structure.md#canvas)); `lamina/tokens/raster.hpp`:

| Token | Value | Use |
|---|---|---|
| `shadow_quality` | 100 | ThorVG's Gaussian blur quality (0–100) |
| `shadow_margin_sigmas` | 3 | room around the shape in the cached image, and how far a shadow reaches when deciding whether it is outside the canvas, in standard deviations |
| `shadow_shape_steps` | 4 | the shape is rounded to 1/4 px for the cache key |
| `shadow_cache_bytes` | 16 MiB | the cached shadow images kept |
| `cull_glyph_outset_em` | 0.5 | how far a text line or an icon may reach outside its box, in its size |
| `worker_threads_max` | 3 | ThorVG's worker threads at most |
| `reserved_cores` | 1 | cores not given to ThorVG's workers |
| `layers_max` | 3 | canvas layers in a frame at most; a retained image that would need more is drawn by ThorVG ([Retained images on the GPU](app-structure.md#retained-images-on-the-gpu)) |
| `retained_min_scale` | 0.5 | a retained image drawn smaller than this in either direction is drawn by ThorVG |
| `image_texture_bytes` | 256 MiB | the default budget of the textures of retained images (`WindowOptions::image_texture_bytes`) |
| `image_texture_smooth` | true | bilinear filtering of those textures (false: nearest) |
| `damage_tracking` | true | repaint and upload only the tiles of layer 0 that changed ([Damage tracking](app-structure.md#damage-tracking)); `WindowOptions::damage_tracking` starts from it |
| `damage_tile` | 64 | the side of a tile, in px |
| `damage_rects_max` | 1 | rectangles repainted in a frame at most; more are merged. Each rectangle is a ThorVG pass that prepares every shape again, so one band from the first to the last dirty row costs less than several |
| `damage_merge_input_max` | 48 | more row runs than this are merged into their bounding box at once |
| `damage_full_fraction` | 0.6 | at this share of dirty tiles or more the whole canvas is repainted |
| `damage_outset` | 2 | px around a drawing's bounding box that count as touched, for antialiasing |
| `damage_pending_max` | 16 | repainted rectangles kept for the window before they are merged into one |
| `full_repaint_environment` | `LAMINA_FULL_REPAINT` | the environment variable that turns damage tracking off for every window |

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

### Touch target

Read on 2026-11-23 from androidx at `6068a9492ba8`: material3 `InteractiveComponentSize.kt` (`Modifier.minimumInteractiveComponentSize`, `LocalMinimumInteractiveComponentSize` = 48 dp) and `Surface.kt`, and ui `platform/ViewConfiguration.kt` (`minimumTouchTargetSize` = 48 × 48 dp, not overridden in `androidMain` `AndroidViewConfiguration.android.kt`), `node/NodeCoordinator.kt` (`hitTest`, `distanceInMinimumTouchTarget`) and `node/HitTestResult.kt` (`isHitInMinimumTouchTargetBetter`, `DistanceAndFlags.compareTo`). Into `lamina/tokens/interaction.hpp` (`minimum_interactive_size`) and `lamina/widgets/interaction.hpp` (`touch_target`, `hit_area`, `target_hit`, `TargetArbiter`).

- A control smaller than 48 dp takes the pointer in a 48 dp area centred on its bounds: buttons of every size (also in button groups, split buttons and toggle buttons), icon buttons, chips, checkboxes, radio buttons, switches, segmented buttons (the height only, so neighbouring segments do not overlap), sliders, range sliders and cards. The control is drawn and laid out at its own size.
- When the areas of several controls contain the point, the control whose bounds contain it wins; otherwise the one nearest to the point wins, by the squared distance from its edge, as in Compose. On a tie the control added later, which is drawn on top, wins; Compose keeps the first hit of its top-down walk, which is also the one on top. The winner is chosen on the press and kept until the release, so a finger that slides toward a neighbour does not hand the press over.
- The other controls still receive the event, with `PointerEvent::margin_claimed` set, and then hit-test their own bounds only (`hit_area`). They still react to a point inside their bounds, and a popup still closes on a press outside it.
- Where the choice is made: among the members of a `WidgetSet` (a nested set competes with its best member), and inside `ButtonGroup`, `ChipGroup`, `FloatingToolbar`, `TopAppBar`, `FlexibleBottomAppBar` and `SplitButton`. A widget takes part through `target_hit` (the `Targeted` concept). Widgets without it, such as list rows, text fields and menus, do not compete, and a control's margin can reach over them.
- Deviations: Compose's `minimumInteractiveComponentSize` also reserves the 48 dp in the layout ("Reserves at least 48.dp in size"), and `Surface(onClick)` starts its modifier chain with it, so a 40 dp Compose button takes a 48 dp slot. lamina-ui keeps the layout at the visual size, as for the dialog buttons ([Dialog](#dialog)). Compose widens the hit area for touch only (`PointerType.Touch`; a mouse or a stylus hits the bounds); lamina-ui widens it for the mouse and the pen too. Compose has no density and widens touches even with `LocalMinimumInteractiveComponentSize` at 0 dp; lamina-ui drops the area at a negative density, as MDC does ([Density](#density)). A disabled control still competes, so a tap in a margin it wins does nothing.

### Mouse echoes after a touch

lamina-ui's own rule, from what SDL 3.4.10 sends on Windows ([Testing](testing.md#touch-injection-on-windows)). It is `input::TouchEchoFilter` in `lamina/input/touch_echo.hpp`, with the tolerance `touch_echo_tolerance_pixels` (1 px) in `lamina/tokens/interaction.hpp`; the SDL window passes every pointer move, press and release from SDL through it before the widgets see it.

- While a finger is down, no mouse move reaches the widgets.
- After the finger lifts, a mouse move that lands less than one pixel, on each axis, from the last touch point on the screen does not reach the widgets. Any other mouse move does, at once: the hover comes back as soon as the mouse really moves.
- The point is kept in screen coordinates (the window's position plus the event's position), so a move at the same place on the screen after the window has moved is still held back.
- Mouse presses, releases and leaves, wheel turns, and touch and pen events always pass. A mouse press also ends a finger whose lift never arrived.

Why. On Windows a touch also moves the system cursor to the touch point. Windows then sends mouse moves of its own to the window under the cursor, without the touch marker that SDL uses to drop the mouse messages Windows makes from a touch (`GetMouseMessageSource`), so SDL reports them as the real mouse (`SDL_GLOBAL_MOUSE_ID`), and the control under the last touch shows hover. Traced on this machine: such a move came between the finger's down and up or right after the up, after the window was resized, after it was restored from minimized, after it was moved under the cursor, and, by timing only, after another process started or ended; during a drag one came at a point the finger had already left. Every one was at the cursor, which is the last touch point. SDL drops moves that do not change its position, but these pass: SDL's touch position has a fraction of a pixel and the cursor does not, and SDL forgets its position when the mouse enters the window again. An echo cannot be told from a real move by where it comes from; it can by where it lands, since a real mouse that moves leaves the touch point and an echo does not.

- The tolerance is one pixel because the echo is the cursor's whole-pixel position of a touch point that has a fraction: the two differ by less than one pixel on each axis (296.002 against 296 in the trace).
- No timer. A time window after a touch would hold back a real mouse moved right after a touch and still pass an echo that comes later, as the ones after a restore or a resize do.
- Cost: a real mouse that moves by less than a pixel from the touch point, or comes back exactly onto it, loses that one move; the next move passes. A real mouse moved while a finger is down is ignored until the finger lifts.
- Not covered: a tap on the title bar's system buttons (maximize) arrives as non-client mouse messages, not as a touch, so its point is not recorded.

Compared, from general knowledge and not read for this:

- Browsers fire compatibility mouse events at the touch point after a tap, and `:hover` stays on the tapped element until something else is touched; the media query `(hover: none)` exists partly for this. lamina-ui deliberately differs: it ends the hover when the finger lifts (`leaves_after_up`) and keeps it ended.
- Compose on Android gets hover only from a mouse or a stylus (hover `MotionEvent`s), so a touch never leaves hover there; this rule gives the same result on Windows. Compose for desktop on Windows was not looked at.
- Chromium on Windows is remembered to drop mouse messages for a short time after a touch; that was not checked here.

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
- Scrollable tab rows (C `TabRow.kt` on androidx-main, read 2026-11-22; `PrimaryScrollableTabRow`, `SecondaryScrollableTabRow`; `TabRowOptions::scrollable`): each tab is at least 90 dp wide (`TabRowDefaults.ScrollableTabRowMinTabWidth`), the row has 52 dp before the first tab and after the last (`ScrollableTabRowEdgeStartPadding`, applied at both ends by `ScrollableTabRow`'s layout), and tabs that fit are not stretched. Selecting a tab scrolls it toward the centre of the row, clamped to the ends (`ScrollableTabData.calculateTabOffset`), on the default spatial spring, as Compose's `scrollAnimationSpec` does. The wheel step (64 dp, `scroll_area_tokens.wheel_step`) is lamina-ui's own; Compose's row scrolls with its `horizontalScroll`. A touch drag scrolls the row and flings it as a scroll area does, and a drag past the slop cancels the press on the tab it started on ([Touch scrolling](#touch-scrolling)).

### Navigation bar

C `tokens/NavigationBarTokens.kt` (64 dp, surface-container, colors, Label M), `tokens/NavigationBarVerticalItemTokens.kt` (56 × 32 dp indicator, 24 dp icon, 6 dp item padding) and C `ShortNavigationBar.kt` and `NavigationItem.kt`, read into `lamina/tokens/navigation_bar.hpp`. lamina-ui has the short navigation bar with equal-width items and icons above labels.

- The indicator is 6 dp below the top of the item and the label 4 dp below the indicator (`TopIconIndicatorToLabelPadding`).
- The indicator's width is 56 dp times a progress that moves with the default spatial spring, so it grows from its center and overshoots a little; its opacity is the same progress, clamped to 1.
- The state layer is the whole 56 × 32 dp pill, shown while the pointer is anywhere on the item. Its color is on-surface (the bar's content color).
- The container has no elevation: `ShortNavigationBar` does not use `ContainerElevation` (Level2).
- Compose switches the icon and label colors at once; lamina-ui fades them with the default effects spring.
- Hiding the labels is lamina-ui's own addition: the bar shortens on the default spatial spring to the indicator and its padding, at least 48 dp (the minimum interactive size), and the labels fade as (2 (L − 0.5))² of the labels progress L.

### Navigation rail

C `tokens/NavigationRailCollapsedTokens.kt` (96 dp, 44 dp at the top, 4 dp between items, surface), C `tokens/NavigationRailExpandedTokens.kt` (220 to 360 dp), C `tokens/NavigationRailBaselineItemTokens.kt` (64 dp items, 24 dp icon, 40 dp under the header), C `tokens/NavigationRailVerticalItemTokens.kt` (56 × 32 dp pill, 4 dp to the label, Label M), C `tokens/NavigationRailHorizontalItemTokens.kt` (56 dp tall pill, 16 dp leading space, 8 dp to the label, Label L), C `tokens/NavigationRailColorTokens.kt`, C `WideNavigationRail.kt` and C `NavigationItem.kt` (`AnimatedNavigationItem`, `AnimatedMeasurePolicy`), read into `lamina/tokens/navigation_rail.hpp`. lamina-ui has the non-modal `WideNavigationRail` with the default top arrangement.

- Collapsed and expanded are two targets of one set of springs, all on the default spatial spring: the rail's minimum width (96 to 220 dp), its full-range width (96 to 360 dp), the gap between items (4 to 0 dp), the items' minimum height (64 to 48 dp, the minimum interactive size) and each item's icon-position progress. The width follows the measure policy of `WideNavigationRailLayout`: an expanded rail is as wide as the animated minimum width, or as its widest item or header (up to the animated full-range width) when that is wider than 220 dp.
- Items sit at the start of the rail with 20 dp before the pill (`WNRItemHorizontalPadding`, a constant in the .kt) and 20 dp after it while the icon is on top (0 dp once it moves to the start; this padding switches at once, as in Compose). Content smaller than the minimum height is centered, which puts the collapsed pill 6 dp from the item's top.
- The pill's padding around the icon moves from 16 × 4 dp to 16 × 16 dp with the progress, and its size from the icon (24 × 24 dp) to icon, 8 dp and label; it is 56 × 32 dp on top and 56 dp tall at the start, always fully rounded. When an item is selected, the pill's width grows from 0 on the default spatial spring from its start edge (`placeRelativeWithLayer(startPadding, …)`), not from its center as in the navigation bar; its opacity is the same progress.
- The label switches from Label M to Label L and from its top position to its start position when the progress passes 0.5; its opacity is 4 (p − 0.5)². Compose captures whether the icon is on top when the item first appears, so a rail that starts expanded keeps Label L when it collapses; lamina-ui always uses the progress alone.
- Colors: indicator secondary-container, selected icon on-secondary-container, selected label secondary on top and on-secondary-container at the start (the .kt reuses `ItemActiveIcon` there, with a TODO for a missing token), unselected on-surface-variant. Compose animates the label color on the default effects spring and switches the icon color at once; lamina-ui moves both on that spring.
- The state layer is the whole pill at its full width, shown while the pointer is anywhere on the item. Its color is on-surface: the ripple takes the rail's content color, not the `NavigationRailColorTokens` state-layer value (on-secondary-container).
- The header is placed at the top start of the content area, and the items start 40 dp below it (`HeaderSpaceMinimum`).
- Hiding the labels is lamina-ui's own addition, not in Compose: the collapsed rail narrows to 80 dp (the width of the narrow `NavigationRail`, with the pill centered) and its items to 56 dp, both lowered with the density like the 64 dp items, and the labels fade as (2 (L − 0.5))² of the labels progress L, so they are gone halfway.
- Not done: the center and bottom arrangements, items that never have labels, and the narrow 80 dp `NavigationRail` itself. The modal rail (`ModalWideNavigationRail`) is described under Modal wide navigation rail below.

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
- Settling: Compose settles in `onPostFling`, after a fling or a drag of the bar: below 1 % or at 100 % it stays, otherwise it goes to the nearer end with `snapAnimationSpec`, the default effects spring. lamina-ui settles when the scroll area comes to rest: no target left, no thumb or touch drag and no fling (`ScrollArea::scroll_in_progress`), and moves the content with the bar.
- Not taken over: the fling of the bar's own drag (lamina-ui's bar cannot be dragged), and the swallowing in `onPreScroll`, which hands the whole delta to the bar when it moved at all, even past its limit; lamina-ui hands the rest to the content.
- Semantics: Compose's bar is a traversal group with no role and its titles have no `heading`; lamina-ui reports a `toolbar` with a `label` for the title. A two-row bar exposes the top title from half collapsed on, as `hideTopRowSemantics` does.

### Segmented button

C `SegmentedButton.kt` and `tokens/OutlinedSegmentedButtonTokens.kt` at `6068a9492ba8`, into `lamina/tokens/segmented_button.hpp`.

- Which component: `SegmentedButton` carries no `@Deprecated` (only two binary-compatibility overloads without `contentPadding` are hidden) and its KDoc names no replacement. The connected button group (`ButtonGroup` with `ButtonGroupDefaults.connected*` shapes and `ToggleButton`) exists at this commit with the `@material3expressive` KDoc tag and no experimental annotation, but no KDoc says it supersedes segmented buttons. lamina-ui therefore implements `SegmentedButton`; the connected button group is not implemented in this stage.
- Size: the row is at least 40 dp tall (`ContainerHeight`); each item has `weight(1f)` and `ButtonDefaults.MinWidth` (58 dp) inside a row of `IntrinsicSize.Min`, so every segment is as wide as the widest. Padding 12 dp at the sides and 8 dp above and below (`ButtonDefaults.ContentPadding`'s vertical padding); icon 18 dp, 8 dp before the label (private `IconSpacing`), label Label L.
- Shape: `CornerFull`; the first item keeps its start corners, the last its end corners, the middle ones are rectangles. The row is `spacedBy(-1 dp)`, so the 1 dp borders overlap.
- Colors: checked container secondary-container, content on-secondary-container; unchecked container transparent, content on-surface; border outline. Disabled: content on-surface at 38 %, border outline at 12 % (not `DisabledOutlineColor`), checked container unchanged.
- Z order: checked + 5, plus one each for press and focus.
- Content: the icon slot is always measured (max(18 dp, icon)); a missing icon shifts the label by −(18 + 8)/2 = −13 dp, animated with the fast spatial spring after the first layout. The default icon is `Icons.Filled.Check`: without an inactive icon, `AnimatedVisibility` with `fadeIn(DefaultEffects) + scaleIn(0, bottom-start, FastSpatial)` and no exit transition; with one, `Crossfade(DefaultEffects)`.
- Semantics: single choice is `selectable` with `Role.RadioButton` in a `selectableGroup`; multiple choice is `toggleable` with no role.
- Width: both rows are `Row(modifier.defaultMinSize(minHeight = ContainerHeight).width(IntrinsicSize.Min), horizontalArrangement = Arrangement.spacedBy(-space))`, and each segment's `Surface` is `.weight(1f)` before `.defaultMinSize(minWidth = ButtonDefaults.MinWidth, minHeight = ButtonDefaults.MinHeight)`. A row narrower than its intrinsic width (for example with `fillMaxWidth()` in a narrow parent) measures each weighted segment at an exact share, and `defaultMinSize` only acts on a zero minimum, so Compose's segments shrink with no floor. `SegmentedButtonContentMeasurePolicy` measures the icon and the label with the same constraints and always counts max(18 dp, icon) + 8 dp before the label. The label is the caller's composable and the file sets no `maxLines` or `overflow`, so a `Text` with its defaults (soft wrap, no line limit) wraps onto more lines in a tight segment. lamina-ui keeps the label on one line and cuts it with an ellipsis (`tokens::ellipsis`), and stops the equal segments at `min_width()`: for the segment that needs most, 2 × 12 dp + 18 + 8 dp + the narrower of its label and an ellipsis (18 dp for an icon-only segment), and at least 58 dp. Without `fill_width` a row is never wider than its natural width; with it the row fills the width, as `fillMaxWidth()` does.
- Deviations: the colors cross-fade on the default effects spring where Compose switches them at once. The row is one Tab stop with arrow keys (selecting in a single-choice row, moving the focus in a multi-choice row); Compose makes every segment a Tab stop and has no arrow keys. The multi-choice segments report `toggle_button`. lamina-ui adds the focus ring inside each segment.

### Slider: vertical, centered, track corner and value indicator

C `Slider.kt` and `tokens/SliderTokens.kt` at `6068a9492ba8`.

- `VerticalSlider`: thumb `VerticalThumbSize` 44 × 4 dp, narrowing to half its height while pressed, dragged or focused; track 16 dp wide; `topToBottom` defaults to true (minimum at the top); keys Up and Down (`value − sign·delta` and `+`), Page Up and Page Down with the same sign, Home and End; Left and Right are not handled.
- `SliderDefaults.CenteredTrack`: the active part runs from the center to the handle; the gap is 6 dp on the side away from the handle and `thumb/2 + 6 dp` on its side; both active corners use the 2 dp inside corner; stop indicators at both ends only; ticks are skipped in both gaps and at both ends.
- `Track(sliderState, trackCornerSize)` and `CenteredTrack` enable corner shrinking: without steps the inactive and active parts are kept until their width reaches zero instead of the corner size. lamina-ui clamps each radius to half the part's shorter side, which is what makes the corners shrink. The vertical slider's default track also shrinks.
- Value indicator: Compose has only the tokens (`ValueIndicatorContainerColor` inverse-surface, `ValueIndicatorLabelTextColor` inverse-on-surface, `ValueIndicatorLabelTextFont` Label L, `ValueIndicatorActiveBottomSpace` 12 dp), used by nothing. The shape and motion come from material-web's slider at `9d2f79851cae` (`labeled`, `slider/internal/_slider.scss`): a full-corner container at least 28 dp (`label-container-height`), 4 dp padding, shown while focused or pressed, scaling from 0 at its bottom center over `duration-short2` (100 ms) with `easing-emphasized`. lamina-ui shows it while the handle is held or focused (material-web also on hover), draws no triangle under it, and shows it on horizontal sliders only.
- Not in Compose at this commit, so not implemented: an icon inside the track and the XS–XL slider sizes (only `SliderTokens.kt` exists, with one 16 dp track).

### Range slider

C `Slider.kt` (`RangeSlider`, `RangeSliderState`) at `6068a9492ba8`; the thumb names from androidx `compose/ui/ui/src/androidMain/res/values{,-ja}/strings.xml` (`range_start` "Range start" / 範囲の先頭, `range_end` "Range end" / 範囲の末尾).

- Track: start inactive part to `start − gap`, active part between `start + gap` and `end − gap` with 2 dp corners at both ends (drawn only while wider than 2 dp), end inactive part from `end + gap`; stop indicators at both ends while those parts are drawn; ticks skipped at both ends and in both gaps.
- Press: the nearer thumb; on a tie the start thumb when the press is before it (`rawOffsetStart > posX`); when both thumbs are within the touch slop of the press, the drag direction picks the thumb. The start value is coerced into `[min, end]` and the end into `[start, max]`.
- Keys: each thumb is focusable; Left and Right, Page Up and Page Down, Home (start: minimum; end: the start value) and End (start: the end value; end: maximum). lamina-ui also takes Up and Down, as its slider does.
- Deviations: lamina-ui moves the chosen thumb on the press, as its slider does, where Compose waits for the touch slop or the release; Compose's thumb snaps the value when the state is written back, lamina-ui at once.

### Date picker

C `DatePicker.kt`, `DateRangePicker.kt`, `DateInput.kt`, `DateRangeInput.kt`, `DatePickerDialog.kt` and `tokens/DatePickerModalTokens.kt` at `6068a9492ba8`, with the strings from `androidMain/res/values{,-ja}/strings.xml`, into `lamina/tokens/date_picker.hpp` and `lamina/calendar/locale.cpp`.

- Container 360 dp wide (`ContainerWidth`), dialog at most 568 dp tall (`ContainerHeight`), surface-container-high, dialog shape extra-large (28 dp). The bare picker paints a plain rectangle; only the dialog rounds it. `TonalElevation` is level 0, so the dialog has no shadow.
- Header: at least 120 dp (`HeaderContainerHeight`); title Label L at 24 dp start and 16 dp top; headline Headline L at 24 dp start and 12 dp bottom; the mode toggle 12 dp from the end and the bottom; then a 1 dp outline-variant divider. Title "Select date", headline the date in `yMMMd` or "Selected date" ("Entered date" in the input mode).
- Navigation row 56 dp, 12 dp at the sides: a circle-shaped text button with the month in `yMMMM` and `ArrowDropDown` (rotated 180° when open, without animation; lamina-ui draws `arrow_drop_up`), and the previous and next month icon buttons, which are removed while the year picker is open.
- Weekdays: narrow standalone names in Body L (on-surface), 48 dp cells. Days: 48 dp cells, 40 dp circles, Body L; selected primary with on-primary text; today on primary with a 1 dp primary outline when not selected; disabled at 38 %. Always six weeks (288 dp).
- Year picker: `GridCells.Fixed(3)` with 16 dp between rows, 72 × 36 dp items centered in their cells, `CornerFull`; the displayed year is primary with on-primary text, the current year has the 1 dp outline and primary text, others on-surface-variant. It covers the weekdays and the grid (48 × 7 − 1 dp, then a divider), opens with `expandVertically(DefaultEffects) + fadeIn(DefaultEffects, 0.6)` and closes with `shrinkVertically(DefaultEffects) + fadeOut(FastEffects)`; the first visible item is the displayed year minus three. Choosing a year keeps the month.
- Mode switch: `AnimatedContent`; to the input, the new content slides up from its full height (`DefaultSpatial`) and fades in (`DefaultEffects`) while the old one slides 48 dp up and fades out (`FastEffects`); back, the reverse; `SizeTransform(DefaultSpatial)`. The field asks for the focus after `DurationMedium2` (300 ms).
- Input: an outlined field 24 dp from the sides and 10 dp from the top, label "Date", placeholder the pattern in capitals; digits only, at most eight; the delimiters are shown as soon as each part is complete (`DateVisualTransformation`). The pattern is the locale's short date pattern reduced to `dd`, `MM` and `yyyy` with its delimiter: `MM/dd/yyyy` (en-US), `dd/MM/yyyy` (en-GB), `yyyy/MM/dd` (ja-JP), from CLDR. Errors, checked once eight digits are typed: "Date does not match expected pattern: %1$s", "Date out of expected year range %1$s - %2$s", "Date not allowed: %1$s" (and their Japanese strings).
- Range picker: header at least 68 dp (128 − 60), title and Title L headline at 64 dp start, "Start date - End date" (an ASCII hyphen with 4 dp spacing); a vertical list of months with a Title S month name (24 dp start, 20 dp top, 8 dp bottom) above each grid. Clicks: no start or a complete range → new start; on or after the start → end; before the start → new start. The band is secondary-container, 40 dp tall, from the center of the start circle to the center of the end circle; in-range text on-secondary-container. Modes cross-fade with `FastEffects`. Range input: two fields 8 dp apart; "Invalid date range input".
- Semantics: day names `yMMMMEEEEd`, prefixed by "Start date", "End date" or "In range" in a range and by "Today"; years "Navigate to year %1$s"; the headline's description "Current selection: %1$s" or "Entered date: %1$s" with "None".
- Deviations: the selected day's circle fades in on the default effects spring (Compose effectively switches it at once); the arrows and keys slide the month on the default effects spring (Compose's `animateScrollToItem` default spec is not in the material3 sources); there is no swipe between months. Keys: Compose handles only Left and Right at the month's first and last enabled day and leaves the rest to its default focus search; lamina-ui follows the WAI-ARIA date grid (arrows, Page Up and Down, Shift for a year, Home and End on the week, Space or Enter) and gives the year grid arrows, Home and End. The month and year button and the arrows are lamina-ui's own Tab stops. In the range input, a partial start in Compose calls `setSelection(null, end)`, which clears both dates; lamina-ui clears only the start.
- Docked: Compose has no docked date picker. lamina-ui's docked form puts the calendar 4 dp below an outlined date field in a popup on surface-container-high with 16 dp corners (corner-large) and a 6 dp shadow (level 3), from material-web's `tokens/versions/latest/sass/_md-comp-date-picker-docked.scss` at `9d2f79851cae` (`container-color`, `container-shape`, `container-elevation`, a 360 px `container-width`); the 4 dp gap and the scale-in from 0.8, as a menu does, are lamina-ui's own. Tooltips: Compose wraps the picker's icon buttons in `IconButtonWithTooltip`; lamina-ui's pickers expose their icon buttons (`mode_toggle()`, `previous_button()`, `next_button()`, `open_button()`, `PickerDialog::leading()`) and the program anchors name tooltips to them, as the components example does.

### Time picker

C `TimePicker.kt`, `TimePickerDialog.kt`, `tokens/TimePickerTokens.kt` and `tokens/TimeInputTokens.kt` at `6068a9492ba8`, into `lamina/tokens/time_picker.hpp`.

- Dial 256 dp (`ClockDialContainerSize`), surface-container-highest; numbers Body L on-surface on a circle of 101/256 of the dial (`OuterCircleToSizeRatio`), 24-hour hours 12–23 on 69/256 (`InnerCircleToSizeRatio`); the number cells are 48 dp. Selector: 48 dp primary handle, 2 dp track to the near edge of the handle, 8 dp center; the number under the handle is on-primary. In 24-hour mode a distance below 74 dp (`MaxDistance`) picks the inner ring.
- Time selectors 96 × 80 dp with 8 dp corners, Display L; selected primary-container / on-primary-container, otherwise surface-container-highest / on-surface; separator 24 dp, ":" in on-surface 4 dp up.
- Period toggle (the updated toggle, `isUpdatedTimepickerToggleEnabled` true): two toggle buttons 4 dp apart in 52 × 80 dp (vertical, 4 dp after the minutes) or 216 × 38 dp (horizontal, 16 dp below); unchecked surface-container-lowest / on-surface-variant and a full corner, checked primary-container / on-primary-container with 12 dp corners and a bold label; the shape morphs with `FastSpatial`.
- Layout: vertical: display, 36 dp, dial, 24 dp; horizontal: display column, 36 dp, dial. Compose picks horizontal when the screen is wider than tall; lamina-ui takes `TimePickerLayout`.
- Width: `TimePickerDefaults.layoutType()` returns `defaultTimePickerLayoutType`, which its KDoc says "uses the screen dimensions"; `TimePickerLayoutType.Horizontal` "Should be used in landscape mode". `ClockFaceSizeModifier` sizes the dial from the height it is given (at least `TimePickerMaxHeight` 384 dp: 256 dp, at least `TimePickerMidHeight` 330 dp: `ClockDialMidContainerSize` 238 dp, otherwise `ClockDialMinContainerSize` 200 dp) and never from the width, so the horizontal layout does not narrow. lamina-ui's horizontal picker is 508 dp wide at every width and keeps the 256 dp dial at every height. The components gallery scrolls it sideways inside its card when the window is narrower ([Examples](examples.md#components)).
- Motion: the handle moves with `DefaultSpatial` along the shortest way; a tap snaps minutes to 5 and hours to the nearest; after a tap on an hour it waits for the animation, then 100 ms, then shows the minutes; a drag follows the pointer and switches on release; the number sets cross-fade with `DefaultEffects`.
- Input: fields 96 × 72 dp, Display M, a 2 dp outline when focused (outline), primary-container when focused; supporting text 7 dp below, Body S, two lines: "Hour" / "Minute", or "Hour must be 1–12", "Hour must be 0–23", "Minute must be 0–59" in the error color. Typing into a full field keeps only the new digit; two hour digits, or one from 2 to 9 in 12-hour mode, advance to the minutes.
- Dialog: 24 dp padding, title Label M ("Select Time" / "Enter Time") 20 dp above the content, the actions row with the mode toggle (`Keyboard` / `Schedule`) at the start and Cancel and OK at the end, 8 dp apart.
- Semantics: selectors are radio buttons "Select hour" and "Select minutes"; dial numbers "%1$d o'clock", "%1$d hours" (24-hour) and "%1$d minutes"; the period toggle "Select AM or PM"; the fields "for hour" and "for minutes". AM and PM are string resources ("AM" and "PM" in Japanese too).
- Deviations: in 24-hour mode Compose's drag resets the selector to the quantized hour on every move, so the handle steps from hour to hour; lamina-ui's handle follows the pointer there too. Compose accepts 0 and 00 as an hour in 12-hour mode while its message says 1–12; lamina-ui treats them as invalid. The keyboard has the hour and minute boxes, the period toggle (one stop) and the dial (one stop with a focused number) as Tab stops, where Compose makes the dial's numbers focusable only after a selector is activated. The input fields are lamina-ui's own two-digit fields, not text fields; the dialog cross-fades between the modes on the fast effects spring. Compose has no disabled time picker; lamina-ui's dims to 38 %. Colors cross-fade where Compose switches them.

### Text field: supporting text and error

C `OutlinedTextField` defaults: supporting text Body S, 4 dp above it (`SupportingTopPadding`), 16 dp start; on-surface-variant, error when `isError`; outline, label and caret error when `isError`, the hovered error outline on-error-container. lamina-ui cross-fades them on the default effects spring.

### Drag gestures

Read on 2026-11-21 from androidx at `6068a9492ba8`: foundation `gestures/AnchoredDraggable.kt` and ui `input/pointer/util/VelocityTracker.kt` and `PlatformVelocityTracker.kt`, into `lamina/motion/velocity_tracker.hpp`, `lamina/motion/anchored_draggable.hpp` and `lamina/tokens/drag.hpp`.

- Velocity: the `Lsq2` strategy of `VelocityTracker1D`, which the desktop `PlatformVelocityTracker` uses: the last 20 samples (`HistorySize`), back to 100 ms before the newest (`HorizonMilliseconds`) and stopping at a gap of more than 40 ms (`AssumePointerMoveStoppedMilliseconds`), at least two samples, a quadratic least-squares fit of position against time whose slope at the newest sample is the velocity. A release more than 40 ms after the last move forgets the motion (the fix for b/238654963). `PointerEvent::time_seconds` carries the time; the SDL window fills it from the event timestamps.
- Target: `computeTarget` exactly: without velocity the closest anchor; at or above the velocity threshold the next anchor in the direction of the fling; below it, the next anchor once the offset has moved the positional threshold away from the anchor behind it, otherwise the anchor behind it. Offsets are clamped to the outer anchors while dragging.
- lamina-ui's own: a drag starts after more than 8 dp of movement along its axis (Android's touch slop, S `config_viewConfigurationTouchSlop`, see [Touch scrolling](#touch-scrolling); the desktop value of Compose was not read), and the part past the slop moves the offset at once, as Compose's `overSlop` does. Releases are clamped to 8000 dp/s, Android's maximum fling velocity (S `config_viewMaxFlingVelocity`). Compose's decay phase (`exponentialDecay`) is not used for these settles: every settle is one spring or one tween. The spline decay of [Touch scrolling](#touch-scrolling) is used only by scroll areas and scrollable tab rows.
- Child first: a widget inside a sheet or drawer gets the press first, and the container takes the gesture only once the slop is passed and the child did not report the move as used, after which the child's press is cancelled; Compose resolves the same order through pointer-event consumption.

### Touch scrolling

Read on 2026-11-22. From androidx at `6068a9492ba8` (C below means that commit in this section, outside `material3`): animation `SplineBasedDecay.kt` (`AndroidFlingSpline`), `FlingCalculator.kt`, `SplineBasedFloatDecayAnimationSpec.kt` and its `androidMain` `SplineBasedFloatDecayAnimationSpec.android.kt`; foundation `gestures/Draggable.kt`, `gestures/DragGestureDetector.kt`, `gestures/AbstractScrollableNode.kt`, `gestures/Scrollable.kt` and `androidMain` `gestures/Scrollable.android.kt`; ui `input/pointer/util/VelocityTracker.kt` and `androidMain` `platform/AndroidViewConfiguration.android.kt`. Android's own values from S. Into `lamina/tokens/drag.hpp` (`touch_scroll`, `fling_decay`), `lamina/motion/spline_decay.hpp` (`SplineDecay`, `Fling`), `lamina/widgets/touch_scroll.hpp` (`TouchScroll`) and `ScrollArea::route`.

| Value | lamina-ui | Source |
|---|---|---|
| Touch slop | 8 dp (`touch_slop`) | S `config.xml` `config_viewConfigurationTouchSlop` = 8dp, which `ViewConfiguration` reads into `getScaledTouchSlop()`; `ViewConfiguration.java` `TOUCH_SLOP = 8`. C `AndroidViewConfiguration.touchSlop` returns `scaledTouchSlop`. |
| Maximum fling velocity | 8000 dp/s (`maximum_fling_velocity`) | S `config_viewMaxFlingVelocity` = 8000dp, `MAXIMUM_FLING_VELOCITY = 8000`; C `AndroidViewConfiguration.maximumFlingVelocity` |
| Scroll friction | 0.015 (`fling_decay.friction`) | S `config_scrollFriction` = 0.015, `SCROLL_FRICTION = 0.015f`, returned by `getScrollFriction()`; C `platformFlingScrollFriction = ViewConfiguration.getScrollFriction()` |
| Spline | inflection 0.35, start tension 0.5, end tension 1.0, 100 samples, bisection to 1e-5 | C `SplineBasedDecay.kt`: `Inflection`, `StartTension`, `EndTension`, `P1 = StartTension * Inflection`, `P2 = 1 - EndTension * (1 - Inflection)`, `NbSamples = 100`, `computeSplineInfo` (`abs(tx - alpha) < 1E-5`), `flingPosition`, `deceleration = ln(Inflection * abs(velocity) / friction)` |
| Deceleration | 9.80665 m/s², 39.37 in/m, 160 dp per inch, 0.84; rate ln 0.78 / ln 0.9 | C `FlingCalculator.kt`: `GravityEarth`, `InchesPerMeter`, `computeDeceleration(0.84f, density)` = `GravityEarth * InchesPerMeter * density * 160f * friction`, `DecelerationRate`, `flingDuration`, `flingDistance`, `FlingInfo.position` and `velocity` |
| Fling start | above 1 dp/s (`touch_scroll.minimum_fling_velocity`) | C `AbstractScrollableNode.kt` `DefaultFlingBehavior.performFling`: `if (abs(initialVelocity) > 1f)`, in px/s; lamina-ui reads it as dp/s, the same at a density of 1 |
| Stop at the end | more than 0.5 dp past it (`touch_scroll.edge_tolerance`) | C same: `if (abs(delta - consumed) > 0.5f) this.cancelAnimation()`, in px; lamina-ui reads it as dp |
| Direction lock | horizontal at 30° or less, vertical above 30° up to 90° (`horizontal_angle_limit_degrees`, `vertical_angle_limit_degrees`) | C `Draggable.kt`: `HorizontalAngleUpperBounds = 30`, `VerticalAngleUpperBounds = 90`, `isDragAngleAlignedWithOrientation`, the angle `atan2(abs(y), abs(x))` in degrees |
| Velocity tracking | as in [Drag gestures](#drag-gestures) | C `VelocityTracker.kt`: `HistorySize = 20`, `HorizonMilliseconds = 100`, `AssumePointerMoveStoppedMilliseconds = 40`, `MinSampleSize = 2` |

- Pointer kind: only `PointerKind::touch` drags the content. The SDL window marks the mouse events SDL makes for a finger (`SDL_TOUCH_MOUSEID`) as touch, so finger events are not read separately. A mouse keeps the wheel, the bar and its own drags (text selection, sliders, handles); a touch never hovers or grabs the bar. When a finger lifts, the window follows the up with a leave on every platform (`input::leaves_after_up`), so a tapped control keeps no hover tint; the mouse and the pen keep their hover. The title bar's own events carry no kind and count as the mouse.
- Slop: a scroller that scrolls one way compares the movement along its axis only (`TouchSlopDetector.mainAxis()` when an orientation is set); one that scrolls both ways compares the distance (orientation `null`). The part past the slop moves the content at once (`overSlop`). lamina-ui needs more than 8 dp, as its `AnchoredDraggable` does; Compose's `hasCrossedSlop` is `>=`.
- Child first: a scroller gives each move to its content before it checks its own slop, as Compose's Main pass reaches the child first. A widget that holds the press keeps the gesture (`holds_pointer()`, gathered by `WidgetSet`): sliders and range sliders, the split pane and pane scaffold handles, the time picker's dial, a scrubbing numeric field, an inner scroller or tab row that took the drag, a dragged sheet, drawer or rail, and open menus, dialogs and modal sheets. Otherwise, once the slop is passed, the scroller takes the drag and the content gets a leave, which drops its press and hover, so the up is not a click.
- Nesting: a scroller waiting for its slop marks its axis in `PointerEvent::waiting_scrollers` for its content. An inner scroller past its slop leaves the gesture when its drag angle is not aligned with its own axis but is aligned with a waiting outer scroller's, as Compose's check over `traverseParentDraggableGestures` does. A drag at 40° that starts on a horizontal scroller inside a vertical page goes to the page; one at 20° goes to the inner scroller.
- Dragging follows the finger 1:1. On release the velocity of each axis comes from the tracker, clamped to 8000 dp/s, and the content flings on the spline decay. lamina-ui computes it in dp at density 1: the density appears in `computeDeceleration` and in a velocity in px, and they cancel, so the distance in px divided by the density is the distance in dp, and the duration is the same.
- A touch during a fling stops it and drags at once, without the slop, and the press does not reach the content, so it is not a tap. In Compose, `shouldScrollImmediately()` returns `isScrollInProgress`, `startDragImmediately` is then true, and `DragGestureNode.processInitialDownState` consumes the down (`firstDown.consume()`) and starts the drag.
- Ends: the content stops at the ends. Compose's overscroll stretch (`OverscrollEffect`) is not implemented.
- App bars: `ScrollArea::scroll_in_progress()` covers a touch drag and a fling, and the top and flexible bottom app bars settle only once it is over (Compose settles in `onPostFling`).
- Sheet content: a touch drag on a bottom sheet's content scrolls the content first when it can scroll that way as the drag starts: downward while the content is not at its top, upward while the sheet is at its Expanded anchor and the content is not at its end. Otherwise the sheet moves. Compose passes what is left of each delta between the content and the sheet through nested scroll during the gesture; lamina-ui decides once per gesture.
- Text fields, lamina-ui's own: a touch focuses a text field on the tap (the up), not on the press, so a scroll that starts on a field does not focus it, and a tap elsewhere takes the focus away on the up. The mouse still focuses and starts a selection on the press.
- Not implemented: the overscroll stretch; nested scrolling, so a list inside a page does not hand the rest of a drag or fling to the page at its end (a zoom view hands a whole drag it cannot follow to a scroller or pager at its slop, [Zoom view](#zoom-view)); Android's framework velocity tracker, which Compose uses when `AndroidComposeUiFlags.isFrameworkVelocityTrackerEnabled` is set (its default was not read; lamina-ui keeps Lsq2); more than one finger in a scroll area (SDL maps only the first finger to the mouse; the window reports every finger in `Frame::fingers` since the zoom view, but scroll areas do not read them); Android's minimum fling velocity (S `config_viewMinFlingVelocity` = 50dp), which Compose's scrollable does not use either. A data table selects a row on the press, so a touch drag that starts on a row selects it.

### Zoom view

Read on 2026-11-23 from S: `ScaleGestureDetector.java` (`onTouchEvent`, the constructor), `GestureDetector.java` (`onTouchEvent`, `isConsideredDoubleTap`, `cancelTaps`), `ViewConfiguration.java` and `config.xml`. No androidx (Compose `transformable`, `detectTransformGestures`) or material-components-android source was on this machine, so none was read; the values in the second table are lamina-ui's own. Into `lamina/tokens/zoom.hpp` (`zoom`), `lamina/motion/zoom.hpp` (the math) and `lamina/widgets/zoom_view.hpp` (`ZoomView`).

| Value | lamina-ui | Source |
|---|---|---|
| Span slop | 16 dp (`span_slop`) | S `ScaleGestureDetector(Context, ...)` passes `ViewConfiguration.getScaledTouchSlop() * 2` as `spanSlop`; the touch slop is 8 dp ([Touch scrolling](#touch-scrolling)) |
| Minimum span | 27 mm, 170.08 dp (`min_span`) | S `config.xml` `config_minScalingSpan` = 27mm, read by `getScaledMinimumScalingSpan()`. Android turns millimetres into pixels with the screen's real density; lamina-ui uses 160 dp per inch, so on a screen whose dp is not exactly 1/160 inch the span differs a little |
| Span | twice the mean distance of the fingers from their centroid on each axis, combined with `hypot` | S `onTouchEvent`: `devSumX / div`, `spanX = devX * 2`, `span = Math.hypot(spanX, spanY)` |
| Start and end of scaling | starts when the span is at least the minimum and has changed by more than the span slop since the fingers last changed; ends when the span falls below the minimum; a finger added or lifted during scaling restarts it at once if the span is still at least the minimum | S `onTouchEvent`: `span >= minSpan && (wasInProgress || Math.abs(span - mInitialSpan) > mSpanSlop)`, `span < mMinSpan`, `configChanged` |
| Scale step | each move multiplies the scale by the span over the previous span | S `getScaleFactor()` = `mCurrSpan / mPrevSpan` |
| Double tap | the second down 40 ms to 300 ms after the first up, less than 100 dp from the first down, and the first tap moved no more than 8 dp; a second finger cancels it; the zoom starts on the second down, about the first tap | S `config_doubleTapTimeoutMillis` = 300, `config_doubleTapMinTimeMillis` = 40, `ViewConfiguration.DOUBLE_TAP_SLOP = 100`, `DOUBLE_TAP_TOUCH_SLOP = TOUCH_SLOP`; `GestureDetector.isConsideredDoubleTap`, `mAlwaysInBiggerTapRegion`, `cancelTaps` on `ACTION_POINTER_DOWN`, `onDoubleTap(mCurrentDownEvent)` on the second `ACTION_DOWN` |
| Pan slop, fling, edge | 8 dp; Lsq2 velocity clamped to 8000 dp/s, spline decay, a fling above 1 dp/s, stop more than 0.5 dp past an edge | as in [Touch scrolling](#touch-scrolling) |
| Single tap | a tap that could be the first of a double tap is confirmed only when no second down came within the double tap timeout, 300 ms; lamina-ui counts the timeout from the up, as its double tap does, where Android counts it from the first down and confirms at the up if the finger is still down then | S `GestureDetector.onTouchEvent`: on `ACTION_DOWN` a first tap posts `TAP` delayed by `mDoubleTapTimeout` (`:666`), a second down removes it (`:651-652`); `TAP` calls `onSingleTapConfirmed`, or defers it to the up while the finger is down (`:333-341`, `:796-797`). Read on 2026-11-24 |

lamina-ui's own, with no source:

| Value | lamina-ui |
|---|---|
| Scale range | 1 (fitted) to 5 (`min_scale`, `max_scale`) |
| Double tap scale | 2.5 (`double_tap_scale`); a double tap on zoomed content fits it again |
| Zoomed | more than 0.001 above the minimum scale (`zoomed_epsilon`) |
| Ctrl and the wheel | 1.25 per notch about the cursor (`wheel_zoom_factor`) |
| Double click timeout | 500 ms (`double_click_timeout_seconds`), how long a single mouse click waits before it is confirmed; it is the default double click time of Windows, remembered and not read, because SDL counts a double click by the system's time and a click confirmed sooner could still become one |
| Pan rubber band | `extent × (1 − 1 / (overshoot × 0.55 / extent + 1))`, extent half the viewport (`pan_band_coefficient`, `pan_band_extent`); the form is the one UIKit is commonly described as using, remembered and not read |
| Scale rubber band | the same curve on the logarithm of the scale, at most ×1.5 past a limit (`scale_band_coefficient`, `scale_band_ratio`) |
| Settle | the default effects spring (damping 1.0), settled within 0.0001 of the way and 0.001 per second (`settle_spring`, `settle_threshold`); the scale and the offset move along the same fraction, which keeps the focus fixed because the focus-preserving offset is linear in the scale |

- Fitted means scaled to fit inside the viewport and centred (contain). On an axis where the scaled content is smaller than the viewport it stays centred; on an axis where it is larger, its edges cannot come inside the viewport.
- One finger on content at scale 1 does not pan and is not held, so a scroller or pager around the view keeps the drag. Two fingers always are held, also at scale 1.
- Handing a drag on: one finger on zoomed content is not held before the 8 dp slop either. At the slop, a drag within 30° of the horizontal (`handoff_angle_degrees`, the angle of Compose's `Draggable` that the scrollers use, [Touch scrolling](#touch-scrolling)) or within 30° of the vertical is given up when a scroller waits on that axis (`waiting_scrollers`) and the content cannot move that way (`can_pan_horizontally`, `can_pan_vertically`, within the 0.5 dp edge tolerance); a drag between the two always pans. Until the scroller or pager has taken the drag, a second finger takes the gesture back, so two fingers stay the view's. lamina-ui's own rule. Android's `ViewPager` decides at its own slop instead, by asking whether any view under the finger can scroll that way (`canScroll`, `canScrollHorizontally`, S `ViewPager.java`); the outcome is the same for a zoom view inside a pager, and lamina-ui asks the view rather than the pager because the pager does not know the views on its pages.
- The decision is made once per drag. A pan that reaches the edge and goes on keeps panning into the rubber band; the next drag goes to the pager. Compose passes what is left of each delta between nested scrollables during the drag (recalled, not read here).
- A leave while fingers are down gives the gesture up, except the leave that follows the lift of the touch pointer, which other fingers outlive. This includes a touch that an outer scroll area or pager takes at once because it stopped that scroller's fling or settle: the fingers reach the view before the pointer events, so the view had already accepted the finger. Before, only a leave after the view had seen the touch's own pointer down gave it up, and such content panned together with the outer scroller.
- A down for a finger the view still holds drops the old one first. On Windows a finger id comes back for the next touch, so a view that missed an up (its page did not pass it on) would otherwise ignore the next touch.
- The double tap fires on the second down, as Android's `GestureDetector` does; Android's quick scale (double tap and drag, `ANCHORED_SCALE_MODE_DOUBLE_TAP`) is not implemented. The mouse double click uses the system's click count, not these values.
- A single tap is confirmed in `step` once the timeout has passed, counting only the frames after the one that brought the up (that frame's time began before the tap), or at the next down if the event times show it came later than the timeout. A confirmed tap clears the double tap's first tap, so the two can never both happen.
- Not implemented: rotation, quick scale, keyboard zoom, a fling after a mouse drag, and the overscroll stretch.

### Pager

Read on 2026-11-23 from S: `com/android/internal/widget/ViewPager.java` (the platform's internal copy of the support library's `ViewPager`; `onInterceptTouchEvent`, `onTouchEvent`, `determineTargetPage`, `smoothScrollTo`, `canScroll`) and `ViewConfiguration.java`. No androidx foundation source (`HorizontalPager`, `PagerState`, `PagerDefaults`, the snap fling behavior) was on this machine (only Gradle metadata, no jars), so none of Compose's pager values were read. Into `lamina/tokens/pager.hpp` (`pager`), `lamina/motion/paging.hpp` (`snap_page`, `pages_in_view`, `pages_around`) and `lamina/widgets/pager.hpp` (`Pager`).

| Value | lamina-ui | Source |
|---|---|---|
| Paging slop | 16 dp (`touch_slop`) | S `ViewConfiguration.java` `PAGING_TOUCH_SLOP = TOUCH_SLOP * 2`, which `ViewPager` reads with `getScaledPagingTouchSlop()` |
| Vertical first | a drag that passes the slop up or down before it passes it sideways is left to the page for the whole gesture | `onInterceptTouchEvent`: `else if (yDiff > mTouchSlop)` sets `mIsUnableToDrag` |
| Fling | more than 25 dp from the press and faster than 400 dp/s: the page after the first visible one when the finger moves left, the first visible one when it moves right | `MIN_DISTANCE_FOR_FLING = 25`, `MIN_FLING_VELOCITY = 400`, `determineTargetPage`: `targetPage = currentPage - (velocity < 0 ? mLeftIncr : 0)`, `mLeftIncr` −1 left to right |
| Without a fling | 60 % of a page in either direction: `floor(position + 0.4)` at or ahead of the settled page, `floor(position + 0.6)` behind it (`snap.positional_threshold` 0.6) | `determineTargetPage`: `truncator = currentPage >= mCurItem ? 0.4f : 0.6f`, `targetPage = (int) (currentPage - mLeftIncr * (pageOffset + truncator))` |
| Catching a settle | a touch more than 2 dp from the page the pager is settling on stops it and drags at once; closer, the settle ends and the touch goes to the content (`catch_distance`) | `CLOSE_ENOUGH = 2`, ACTION_DOWN in `onInterceptTouchEvent` |
| Pages around the view | one page on each side (`beyond_viewport_pages`) | `DEFAULT_OFFSCREEN_PAGES = 1` |
| Nested content | content that can follow the drag keeps it | `canScroll`; in lamina-ui the content is asked first and holds the press, or a zoom view gives the drag up ([Zoom view](#zoom-view)) |
| Velocity | Lsq2, clamped to 8000 dp/s; the up is not a sample | as in [Touch scrolling](#touch-scrolling); ViewPager uses the platform tracker and `getScaledMaximumFlingVelocity()` |

Differences from ViewPager:

- Direction lock: ViewPager takes a drag when `xDiff * 0.5f > yDiff` (below about 26.6°). lamina-ui uses 30° (`horizontal_angle_limit_degrees`), the angle its scrollers take from Compose's `Draggable`, so a vertical scroller that waits on the pager and the pager agree on which drags are sideways.
- Settle: ViewPager scrolls with a `Scroller` and a quintic interpolator (`sInterpolator`) for up to 600 ms (`MAX_SETTLE_DURATION`, `smoothScrollTo`, `distanceInfluenceForSnapDuration`). lamina-ui settles on a spring, as its other drags do.
- ViewPager's edge effect, gutter drags (`DEFAULT_GUTTER_SIZE`), page margin drawable and page width factors are not implemented.

lamina-ui's own, with no source:

| Value | lamina-ui |
|---|---|
| Settle spring | damping 1.0, stiffness 400, carrying the release velocity in pages per second; it stops when it reaches the page (no swing past it), else when within 0.001 page and 0.01 page per second (`settle_spring`, `settle_threshold`) |
| `animate_to_page` | a page more than one page away is first jumped to its neighbour (`animate_jump_pages` 1), so only two pages show; Compose's rule for far pages was not read |
| Page spacing | 0 dp (`page_spacing`) |
| Focus ring | drawn inside the viewport with square corners (`focus_ring_corner`) |
| Mouse | drags pages like a finger by default (`PagerOptions::mouse_drag`); the gallery turns it off |
| Keys | Left and Right one page, Home and End to the ends, no wrapping, while the pager has the keyboard focus |

- Tabs: `TabRow::set_indicator_position` places the indicator at a fractional tab index, interpolating the centre and the width of the two tabs, and snaps it, so it follows the finger without lag; during a swipe the gallery also selects the pager's `current_page()` (the nearest page, which changes halfway), whose colors fade as on a click. A tapped tab keeps its own spring and the pager slides by `animate_to_page`, so the selection does not pass through the tabs in between. lamina-ui's own pairing; Compose's tabs-with-pager sample was not read.
- Lazy pages: `pages_to_build()` is the pages in view and one on each side. The Viewer page builds a `ZoomView` per photo there and drops the others; a photo that is not the settled one is fitted again when the pager settles.
- Not implemented: handing a drag over in the middle (a zoom view that reaches its edge during a drag keeps it), the overscroll stretch at the first and last page, wheel paging, right-to-left layouts, a vertical pager, and pages narrower than the viewport.

### Bottom sheets

C `SheetDefaults.kt`, `BottomSheet.kt`, `ModalBottomSheet.kt`, `BottomSheetScaffold.kt`, `Scrim.kt`, `ComposeMaterial3Flags.kt`, `tokens/SheetBottomTokens.kt` and `tokens/ScrimTokens.kt` at `6068a9492ba8`, with the strings from `androidMain/res/values{,-ja,-en-rGB}/strings.xml` and ui's `close_sheet`, into `lamina/tokens/bottom_sheet.hpp`, `lamina/tokens/scrim.hpp` and `lamina/widgets/pattern_strings.cpp`.

- Container surface-container-low, extra-large top corners (28 dp, `ExpandedShape`), at most 640 dp wide (`SheetMaxWidth`) and centered. The modal sheet has no shadow (`BottomSheet`'s `shadowElevation` 0 and `ModalBottomSheet`'s `tonalElevation` 0); the standard sheet of the scaffold has a level-1 shadow (`sheetShadowElevation = Elevation`). Scrim: the scrim role at 32 %.
- Drag handle: 32 × 4 dp, on-surface-variant, `shapes.extraLarge` (a pill at this size), 22 dp above and below; Compose wraps it in a plain tooltip above it (`DragHandleWithTooltip`) with "Drag handle". A click on it: modal: Expanded hides and dismisses, Partially expanded expands, otherwise shows; standard: Expanded hides when the sheet may hide and goes to Partially expanded otherwise.
- Anchors (modal): Hidden at the full height; Partially expanded, unless skipped, at the full height minus min(half the full height, half the sheet), the rule of `isBottomSheetPartiallyExpandedDeterministicEnabled`, which is true by default at this commit (the deprecated `rememberModalBottomSheetState` turns it off and uses half the full height only for sheets taller than half); Expanded at max(0, full height − sheet height). The sheet is as tall as the handle and its content, at most the full height, and the content scrolls beyond that. Standard (`BottomSheetScaffold`): Partially expanded at the full height minus the peek height (56 dp by default) unless the two are equal, Hidden only when hiding is allowed (`skip_hidden` false), Expanded at the full height minus the sheet height. When the anchors change, the target moves as Compose's `draggableAnchors` lambdas move it.
- Motion: show and expand on the default spatial spring (`showMotionSpec`); hide and partial expand on the fast effects spring (`hideMotionSpec`); a released drag settles on the default spatial spring with the release velocity (`AnchoredDraggableDefaults.flingBehavior`). Thresholds: 56 dp positional (`PositionalThreshold`, absolute, not a fraction), 125 dp/s velocity. The modal sheet damps a downward fling within 125 dp of Hidden (`BoundaryDampeningZone`): the velocity is multiplied by the square of the remaining distance over 125 dp, and a fling at or above the threshold keeps at least the threshold. The scrim alpha springs to 1 on the default effects spring whenever the target is not Hidden.
- Overshoot: the expressive default spatial spring overshoots the Expanded anchor. Compose stretches the container from its top (`verticalScaleUp`) and scales the content back (`verticalScaleDown`), so no gap opens at the bottom; lamina-ui draws the container from the sheet's top down to where the expanded sheet ends and leaves the content unscaled, which looks the same.
- Dismissing: Back (`settleToDismiss`) takes an Expanded sheet that has a Partially expanded state to Partially expanded first and hides it otherwise; a click on the scrim hides it directly (`animateToDismiss`); a fling or drag that settles on Hidden dismisses it. lamina-ui maps Back to Escape, on the key press, since it has no key-release events; `BottomSheetResponse::dismissed` is set when a dismissal starts, where Compose calls `onDismissRequest` after the hide animation ends.
- Semantics: the modal sheet is a modal `dialog` named "Bottom Sheet" (`BottomSheetPaneTitle`; "Bottom sheet" in en-GB, ボトムシート in Japanese), the standard sheet a `group`; the drag handle is a button "Drag handle" whose description is the action a click takes ("Expand bottom sheet", "Collapse bottom sheet", "Dismiss bottom sheet"), standing in for Compose's custom accessibility actions, which lamina-ui's tree has no place for; the scrim is a button "Close sheet".
- Focus: the modal sheet keeps Tab inside itself (handle, then the content's stops, wrapping), as Compose's dialog window does; opened from the keyboard it focuses the handle. The standard sheet's handle and content join the program's Tab order.
- Deviations: the handle's hover, focus and press layer is a pill around the 32 × 48 dp target and the focus ring is drawn around it, where Compose's ripple is a rectangle with no ring; the mouse wheel scrolls the content and never moves the sheet (Compose sends wheel deltas through nested scroll, which can move it), and a touch drag on the content scrolls it only when the content can scroll that way when the drag starts, and moves the sheet otherwise ([Touch scrolling](#touch-scrolling)); predictive back is not implemented.

### Side sheets

Compose has no side sheet at `6068a9492ba8` (no `SideSheet*.kt` and no `SheetSideTokens.kt`), and material-web has no side sheet component at `9d2f79851cae`, only tokens. The values come from W `tokens/versions/latest/sass/_md-comp-sheet-side.scss` (generated from Material 3 version 34.0.21) and M `lib/java/com/google/android/material/sidesheet/` at `6061a69915d0` (`SideSheetBehavior.java`, `RightSheetDelegate.java`, `res/values/{dimens,styles,tokens,strings}.xml`, `res/anim/m3_side_sheet_{enter_from_right,exit_to_right}.xml`, `res/values-ja/strings.xml`), into `lamina/tokens/side_sheet.hpp`. The M3 design site's side sheet specs could not be read (the page renders with script and returned no text).

- Width 256 dp (`docked-container-width`); standard container surface at level 0, modal container surface-container-low at level 1; docked modal shape corner-large on the start side (16 dp on the corners facing the content), docked standard shape no corners, detached shape corner-large on all corners with a 16 dp margin on every side (M `m3_side_sheet_margin_detached`). The headline is Title L in on-surface-variant; the divider above the actions is outline.
- Motion: MDC's modal side sheet slides in and out over 275 ms with the emphasized interpolator; lamina-ui uses that tween for opening, closing and settling after a drag, and fades the scrim with the slide (MDC dims the dialog window instead).
- Release (`calculateTargetStateOnViewReleased`): a fling outwards keeps it open; otherwise it hides when the swipe is faster than 500 (MDC's `SIGNIFICANT_VEL_THRESHOLD`, in pixels per second; lamina-ui uses dp per second) or when it was released past half its travel, and stays open otherwise.
- lamina-ui's own layout, since no source specifies it: a 72 dp header with an optional back icon button 12 dp from the start, the headline (24 dp from the start, or 4 dp after the back button) and a close icon button 12 dp from the end; content 24 dp from the sides; actions in a row after a 1 dp divider, 16 dp inside and 8 dp apart, with the buttons the program passes. MDC's catalog uses 16 dp padding and a baseline-aligned close button.
- Behavior: the modal sheet dismisses on Escape, a click on the scrim, the close button and a drag; it keeps Tab inside (header buttons, content, actions, wrapping) and reports a modal `dialog` named after its headline (or "Side Sheet", MDC's `side_sheet_accessibility_pane_title`; サイドシート in Japanese) with a "Close sheet" scrim button. The standard sheet is part of the layout: its width grows from 0 as it opens, so the content beside it narrows, as MDC's coplanar sheet; its buttons join the program's Tab order. Either edge (`SideSheetEdge`).

### Navigation drawer

C `NavigationDrawer.kt` and `tokens/NavigationDrawerTokens.kt` at `6068a9492ba8`, the samples `DrawerSamples.kt`, and M `lib/java/com/google/android/material/navigation/res/values/dimens.xml` at `6061a69915d0` for the headline and divider insets, into `lamina/tokens/navigation_drawer.hpp`.

- Sheet: at least 240 and at most 360 dp wide; the items fill the width, so it is 360 dp wide unless the window is narrower. Modal: surface-container-low with corner-large on the end side (`ContainerShape`), no shadow and no tonal elevation (`ModalDrawerElevation` is level 0, although the token file has a level-1 `ModalContainerElevation`). Dismissible and permanent: surface, no corners.
- Item (`NavigationDrawerItem`): at least 56 dp tall, a full-corner pill inset 12 dp at the sides (`ItemPadding`, as the samples place it), 16 dp before the content and 24 dp after it, a 24 dp icon and 12 dp before the label, Label L, and a badge in Label L 12 dp after the label. Selected: secondary-container with on-secondary-container icon, label and badge; unselected: transparent with on-surface-variant. The state layer is the content color (on-surface, or on-secondary-container while selected). The samples put 12 dp above the first item.
- Headline and divider: Compose has no drawer headline or divider. The headline uses the Title S and on-surface-variant tokens (`HeadlineFont`, `HeadlineColor`) with MDC's 28 dp start inset; lamina-ui makes it 56 dp tall. The divider is 1 dp of outline-variant inset 28 dp, 8 dp below the item above and 3 dp above the item below (MDC's `m3_navigation_menu_divider_horizontal_padding`, `m3_navigation_divider_top_margin`, `m3_navigation_divider_bottom_margin`).
- Motion: `open()` on the default spatial spring, `close()` on the fast effects spring. A released drag settles on a 256 ms tween with `FastOutSlowInEasing`: `DrawerState` builds its `AnchoredDraggableState` with `AnchoredDraggableDefaultAnimationSpec` before the `SideEffect` that sets `anchoredDraggableMotionSpec` runs, so the spring never reaches it. Thresholds: half the width positional, 400 dp/s velocity. The modal scrim's alpha is the open fraction of the offset (no animation of its own).
- Overshoot: the open spring overshoots; Compose scales the container from its end (`horizontalScaleUp`) and the content back (`horizontalScaleDown`); lamina-ui draws the container from the window edge to the sheet's end and leaves the content unscaled.
- Modal: the whole area drags (Compose puts `anchoredDraggable` on the box that holds the content), so a drag across closed content opens it; `DrawerResponse::took_over` tells the program to cancel the press it gave to its own widgets. A click on the scrim and Escape (on the key press) close it. Dismissible: the content is pushed by the sheet's width and keeps its own width; Escape closes it while the sheet has the focus. Permanent: `permanent_drawer(sheet, content)` is a row of the sheet and the content.
- Semantics: the sheet is a `navigation` named "Navigation menu" (ナビゲーションメニュー), items `navigation_item` (Compose's `Role.Tab`) with the selection and the badge as the description, headlines `heading`, dividers `separator`; the modal drawer's scrim is a button "Close navigation menu".
- Deviations: Compose's items have no disabled state; lamina-ui's are dimmed to 38 % and skipped. The colors cross-fade on the default effects spring where Compose switches them. Keys: the sheet is one Tab stop whose Up and Down move the focus between enabled items (skipping headlines and dividers), Home and End, and Space or Enter selects, as the navigation rail does; Compose makes every item a Tab stop. The modal drawer keeps Tab inside and takes the focus when opened from the keyboard; Compose's modal drawer asks for the focus but does not trap it. The sheet scrolls when its entries are taller than the window; Compose leaves that to the caller. The focus ring is drawn around the pill. Predictive back is not implemented.

### Modal wide navigation rail

C `WideNavigationRail.kt` and `tokens/NavigationRailExpandedTokens.kt` at `6068a9492ba8`, into `lamina/tokens/navigation_rail.hpp` (`modal_navigation_rail_tokens`) and `lamina/widgets/modal_navigation_rail.hpp`.

- `ModalWideNavigationRail` exists at this commit. Collapsed, it is the 96 dp rail in the layout; expanded, it opens over the content on a scrim as a 220–360 dp rail on surface-container with corner-large on every corner (`ModalContainerShape`; no shadow is applied, although `ModalContainerElevation` is level 2). The scrim follows a default effects animation of the expansion; lamina-ui fades it on the default effects spring while the target is expanded.
- `hideOnCollapse`: there is no collapsed rail; the expanded rail slides in from off the window on the default spatial spring and can be dragged away (positional threshold half the width, 125 dp/s).
- Escape (on the key press) and a click on the scrim collapse it; selecting an item in the hiding form collapses it in the components example. Semantics: the rail is a modal `navigation` with a "Close rail" scrim button; its pane title "Navigation rail" (ナビゲーション レール) names it when the program gave no name.
- Deviations: Compose removes the modal rail as soon as its effects animation reaches zero, before the width animation ends; lamina-ui keeps it until the rail is back at its collapsed width. The header (for example the menu button) is the program's widget; the components example paints it above the scrim.

### Search bar

C `SearchBar.kt`, `tokens/SearchBarTokens.kt`, `tokens/SearchViewTokens.kt`, `internal/TextFieldImpl.kt` and the samples `SearchBarSamples.kt` at `6068a9492ba8`, and `material3-window-size-class/.../WindowSizeClass.kt` for the 600 dp breakpoint, into `lamina/tokens/search.hpp`.

- Which API: `SearchBar(state, inputField)` with `ExpandedFullScreenSearchBar` or `ExpandedDockedSearchBar` for the results; the `SearchBar(inputField, expanded, onExpandedChange)` and `DockedSearchBar` overloads are deprecated. The Expressive variants at this commit, `AppBarWithSearch`, `ExpandedFullScreenContainedSearchBar` and `ExpandedDockedSearchBarWithGap`, are not implemented.
- Bar: 56 dp tall, at least 360 and at most 720 dp wide, full corners, surface-container-high, no elevation (tonal and shadow level 0). The input is a text field without a container or indicator: Body L on-surface, placeholder on-surface-variant, caret primary. The leading and trailing icons sit in 48 dp slots moved 4 dp inwards, so they are 16 dp from the ends and the text starts 52 dp in (`textFieldHorizontalIconPadding` with `SearchBarIconOffsetX`). As in the samples, the leading icon is a search icon while collapsed and a back icon button while expanded.
- Expanding: on the slow spatial spring, collapsing on the default spatial spring (`rememberSearchBarState`); the view is shown while the progress is above 0.02. A click on the bar, Down while it has the focus, or typing that lengthens the query expands it; inside, Down moves into the results, Escape collapses, and Enter in the field reports the query (`onSearch`).
- Full-screen view: the surface goes from the bar to the whole area (position and size lerp with the progress), the corner from 28 dp to 0, the field's width and center from the bar to the area with the unclamped progress, 8 dp of padding above and below the field fade in, then an outline divider and the results, faded in with the progress.
- Docked view: at the bar's place and width; its height grows to 2/3 of the window's height with the progress, never below 240 dp (or the limit when that is smaller), and fits the results; extra-large corners (28 dp); the divider and the results below the field. No scrim.
- Compact and wide: Compose does not switch between the two views; the samples use the full-screen view on phones and the docked view on large screens. lamina-ui's `SearchViewMode::automatic` uses the full-screen view below 600 dp, the start of the medium window width class, and the docked view from there.
- Semantics: the field is named "Search" (検索) and says "Suggestions below" (検索候補は次のとおりです) while expanded; the expanded view is a `dialog` named "Search", modal when full-screen.
- Deviations: Up on the first result goes back to the field; Tab stays inside the expanded view (back button, field, trailing button, results), as Compose's dialog and focusable popup do. A press outside the docked view collapses it. Predictive back and the scroll behavior of `AppBarWithSearch` are not implemented.

### Toggle buttons and per-corner shapes

C `ToggleButton.kt`, `internal/AnimatedShape.kt`, `HorizontalCenterOptically.kt` and `tokens/{Filled,Tonal,Outlined,Elevated}ButtonTokens.kt` at `6068a9492ba8`, into `lamina/tokens/button.hpp` (`toggle_button_styles`, `toggle_button_motion`) and `ButtonOptions::toggle` and `ButtonOptions::shapes` of `lamina/widgets/button.hpp`.

- Colors (`ToggleButtonDefaults.colors()` and the filled tonal, outlined and elevated defaults): filled surface-container / on-surface-variant, checked primary / on-primary; tonal secondary-container / on-secondary-container, checked secondary / on-secondary; outlined transparent with the outline-variant border, checked inverse-surface / inverse-on-surface without a border; elevated surface-container-low / primary, checked primary / on-primary. Disabled: container on-surface at 10 % (outlined: outline-variant at 10 %), content on-surface-variant at 38 %. A text button cannot toggle.
- Shapes (`ToggleButtonDefaults.shapesFor`): full at rest, the size's pressed corner while pressed, the size's square corner when checked; pressed wins over checked. The morph follows `AnimatedShapeState`: every corner goes from the start shape to the target shape with one progress on the fast spatial spring, and going back to the start shape runs the same progress backwards with its velocity negated. lamina-ui keeps each corner as dp plus a share of the shorter side, so a full corner follows the size during the morph.
- The outlined border's width goes to 0 on the fast spatial spring when checked (`animateBorderStrokeAsState`).
- `center_optically` shifts the content by 0.11 × (mean start corner − mean end corner), each corner clamped to half the height, and the shift to the paddings (`ShapeWithHorizontalCenterOptically`); the split button's trailing half uses it.
- Semantics: Compose gives a toggle button `Role.Checkbox`; lamina-ui reports `toggle_button` with on or off. A `radio` toggle reports `radio_button` with `selected` and is never unchecked by a click, as the single-select samples (`Role.RadioButton`).
- Deviations: the checked colors fade on the default effects spring where Compose switches them. A checked icon is drawn with FILL 1, standing in for the samples' switch from an outlined to a filled icon.

### Button groups

C `ButtonGroup.kt`, `tokens/ButtonGroupSmallTokens.kt` (`VERSION: v0_11_0`), `tokens/ConnectedButtonGroupSmallTokens.kt` (`VERSION: 14_1_0`) and the samples `ButtonGroupSamples.kt` at `6068a9492ba8`, into `lamina/tokens/button_group.hpp` and `lamina/widgets/button_group.hpp`.

- Layout: 12 dp between the buttons, 2 dp when connected. Each button takes its intrinsic width, or a share of the room left by its weight.
- Width on press (`ButtonGroupMeasurePolicy`, `EnlargeOnPressNode`): a pressed button grows into each neighbour by press × min(0.15 × its width / 2, the neighbours' compression limits); the first and the last grow by press × min(0.15 × width, the limit) into their only neighbour, and the neighbours shrink by as much, so the row keeps its width. The compression limit is the neighbour's end content padding (16 dp at the small size). The press runs on the fast spatial spring; on release it keeps going until it passes 0.75 and only then returns, so a short click still squeezes the neighbours fully. lamina-ui keeps the widths in fractional dp where Compose rounds them to pixels.
- Overflow: when the buttons do not fit, the ones that fit before the overflow indicator stay and the rest move into a menu. The indicator is a filled icon button with `more_vert` named "More options" (その他のオプション, `m3c_button_group_more_options`) at the end of the shown buttons; a menu item clicks or toggles its button and closes the menu.
- Connected shapes (`ButtonGroupDefaults.connected*ButtonShapes`): the leading button has full outer and small (8 dp) inner corners, the middle ones small corners, the trailing one mirrors the leading; pressed inner corners are extra-small (4 dp); a checked button is a pill. Compose's connected samples put the toggle buttons in a `FlowRow` without the width animation, and its custom-item sample puts connected shapes in a `ButtonGroup` with `animateWidth`; lamina-ui's connected group does both.
- Selection: none (`clickableItem`, buttons), multiple (`toggleableItem`, toggle buttons), single (toggle buttons with the radio role, as the samples).
- Deviations: a key press does not widen a button (Compose's keyboard presses do). Every button and the indicator are Tab stops, as in Compose; there are no arrow keys. The group reports a `group` node. Per-item vertical alignment and the vertical button group are not implemented. In a connected group the connected shapes also apply to clickable buttons.

### Split buttons

C `SplitButton.kt`, `tokens/SplitButton{XSmall,Small,Medium,Large,XLarge}Tokens.kt` (`VERSION: 18_0_18`) and the samples `SplitButtonSamples.kt` at `6068a9492ba8`, into `lamina/tokens/split_button.hpp` and `lamina/widgets/split_button.hpp`.

- Sizes 32 / 40 / 56 / 96 / 136 dp, 2 dp between the halves. Inner corners extra-small / extra-small / extra-small / small / medium, widening while pressed to small / medium / medium / large-increased / large-increased (the hovered tokens are not used by `SplitButton.kt`). Leading padding 12+10 / 16+12 / 24+24 / 48+48 / 64+64 dp, trailing padding 13 / 13 / 15 / 29 / 43 dp on both sides, trailing icon 22 / 22 / 26 / 38 / 50 dp; both halves are at least 48 dp wide. The label and leading icon follow the button sizes (`ButtonDefaults.textStyleFor`, `iconSizeFor`, `iconSpacingFor`), as the samples do.
- The shapes morph on the default effects spring. The checked trailing half becomes a circle, gets a content-colored overlay at 10 % (`TrailingButtonStateLayerAlpha`) and is centered optically.
- The chevron (`keyboard_arrow_down`) turns half a turn when checked, on `animateFloatAsState`'s default spring (damping 1, stiffness 1500), taken from the samples.
- Menu: as in `SplitButtonWithDropdownMenuSample`, checking the trailing half opens a menu under the whole split button, and choosing an item or dismissing the menu unchecks it. The trailing half says "Expanded" or "Collapsed" in its value, as the samples' `stateDescription`.
- Width: `SplitButtonLayout` estimates the trailing half's intrinsic width, subtracts it and the spacing from the maximum width for the leading half, measures the trailing half first ("to give it priority for width") and then the leading half in what is left. The halves' rows have `defaultMinSize(minWidth = LeadingButtonMinWidth / TrailingButtonMinWidth)` and no `widthIn`, and no label gets `maxLines` or `overflow`. lamina-ui gives the trailing half and the 2 dp gap their widths first too and shrinks the leading half down to its paddings, icon and an ellipsis (at least 48 dp), cutting its label to one line where Compose's `Text` would wrap.
- Deviations: the filled, tonal, outlined and elevated colors are the buttons' (`ButtonDefaults.*Colors`); lamina-ui's colors fade on the default effects spring.

### Extended FABs

C `FloatingActionButton.kt` and `tokens/ExtendedFab{Small,Medium,Large}Tokens.kt` at `6068a9492ba8`, into `lamina/tokens/fab.hpp` (`extended_fab_sizes`) and `lamina/widgets/extended_fab.hpp`.

- Small 56 dp (corner large, icon 24 dp, padding 16 dp, gap 8 dp, Title M), medium 80 dp (large-increased, 28 dp, 26 dp, 12 dp, Title L), large 96 dp (extra-large, 32 dp, 28 dp, 16 dp, Headline S). The medium and large gaps are the code's 12 and 16 dp; the token files say 16 and 20 ("IconLabelSpace is incorrect").
- Collapsing: the width goes from the expanded width to the height on the fast spatial spring and the label fades on the fast effects spring; the icon stays at the start padding.
- Elevation and colors are the FAB's. Compose's FABs have no disabled state, and neither do lamina-ui's.

### FAB menu

C `FloatingActionButtonMenu.kt` and `tokens/FabMenuBaselineTokens.kt` at `6068a9492ba8` and the samples `FloatingActionButtonMenuSamples.kt`, into `lamina/tokens/fab_menu.hpp` and `lamina/widgets/fab_menu.hpp`.

- Toggle FAB: the progress runs on the fast spatial spring; the size goes from 56 / 80 / 96 dp to 56 dp anchored at the top end, the corner from 16 / 20 / 28 dp to 28 dp, the container from primary-container to primary, the icon from on-primary-container to on-primary and from 24 / 28 / 36 dp to 20 dp. The icon switches at 0.5, as the sample switches Add for Close. Shadow level 3.
- Items: 56 dp pills on primary-container, 24 dp padding, 24 dp icons, 8 dp before the Title M label, 4 dp between items and 8 dp above the button, 16 dp padding at the sides and under the button, aligned to the end.
- Stagger: the number of shown items is an integer that follows the slow effects spring (threshold 1) and counts from the bottom, so the item nearest the button appears first and the top one leaves first. Each item's width grows from its end on the fast spatial spring and its alpha on the fast effects spring. The items have no shadow (the token says level 3, but the `Surface` sets none).
- Keyboard: Tab or Down on the open button moves into the items, Enter chooses, Escape closes and gives the focus back to the button (the sample's `BackHandler`). Choosing an item closes the menu, as the sample does.
- Deviations: the menu does not scroll when it is taller than its area. Like Compose's, the menu and its toggle FAB have no disabled state, so the gallery's Enabled switch leaves them as they are.

### Loading indicator

C `LoadingIndicator.kt`, `MaterialShapes.kt` and `tokens/LoadingIndicatorTokens.kt` at `6068a9492ba8`, into `lamina/tokens/loading_indicator.hpp` and `lamina/widgets/loading_indicator.hpp`, with lamina-ui's port of graphics-shapes.

- 48 dp container (full corners), 38 dp indicator: the shapes are scaled to the size × the polygons' smallest bounds-to-max-bounds ratio × 38 / 48 and centered on their control-point bounds every frame. Primary indicator; contained: on-primary-container on primary-container. The shape is clipped to the container.
- Indeterminate: SoftBurst, Cookie9Sided, Pentagon, Pill, Sunny, Cookie4Sided, Oval and back, every polygon normalized. Each morph runs on a spring with damping 0.6 and stiffness 200 that ends when Compose's duration estimate for a 0.1 threshold has passed (about 0.30 s); a new morph starts every 650 ms. The rotation is progress × 90° plus 90° per finished morph plus a linear turn every 4666 ms.
- Determinate: a circle turned by 18° morphs into SoftBurst as the progress goes from 0 to 1, and the shape turns back by progress × 180°.
- `LoadingIndicatorOptions::size` scales the whole indicator, like a larger size modifier in Compose.
- Compose's loading indicators have no disabled state; neither do lamina-ui's, nor its wavy progress indicators.

### Wavy progress indicators

C `WavyProgressIndicator.kt`, `internal/LinearWavyProgressModifiers.kt`, `internal/CircularWavyProgressModifiers.kt`, `ProgressIndicator.kt` (keyframes) and `tokens/{Linear,Circular}ProgressIndicatorTokens.kt` at `6068a9492ba8`, into `lamina/tokens/wavy_progress.hpp` and `lamina/widgets/wavy_progress.hpp`.

- Linear: 240 × 10 dp by default, 4 dp indicator and track, 4 dp gap, a 4 dp stop dot at the end of the track while determinate. The wave is Compose's path of quadratic curves with the control point `height − stroke` (6 dp) off the centre line, so the crest is 3 dp; the progress takes the arc length that matches its x (the path length over its width), and the amplitude scales the path around the centre line. Wavelength 40 dp determinate, 20 dp indeterminate; the wave moves one wavelength a second.
- Circular: 48 dp, 4 dp strokes, 4 dp gap. The track is a circle polygon and the indicator a morph from it to a star (inner radius 0.75, rounding 0.35 with smoothing 0.4, inner rounding 0.5) with max(5, round(2πr / 15 dp)) vertices, rotated so the path starts at 12 o'clock and runs clockwise, scaled to the size minus the stroke. The moving wave shifts the start along a doubled path and turns it back, so the ends stay put.
- Amplitude: `wavy_indicator_amplitude` (none up to 10 % and from 95 %) on 500 ms tweens, standard easing up and emphasized-accelerate easing down. `ease_progress` moves the progress on Compose's 500 ms linear `ProgressAnimationSpec`.
- Indeterminate linear: two lines on the 1750 ms keyframes (head and tail delays 0, 250, 650 and 900 ms, durations 1000, 1000, 850 and 850 ms, emphasized-accelerate easing). Indeterminate circular: the sweep and the rotation keyframes over 6000 ms plus 1080° linear and a 90° base. Compose's keyframe easings apply to the segment after each keyframe, so the sweep rises linearly to 0.87 and falls with standard easing, and the quarter turns are linear (the emphasized-decelerate easing falls on the holds); lamina-ui computes them the same way.
- Handoff: a linear indicator that turns determinate grows its bar and stop dot in over 500 ms; a circular one goes from the indeterminate sweep and rotation to the progress and 360° over 300 to 600 ms (emphasized decelerate), longer the further it has to turn.
- Deviations: the linear indicator does not first run its lines out at the accelerated playback speed when it turns determinate. Paths are drawn as flattened polylines (12 segments per curve) stroked with round caps. The older `CircularProgressIndicator` stays unchanged for programs that use it.

### Floating toolbars

C `FloatingToolbar.kt` and `tokens/FloatingToolbarTokens.kt` at `6068a9492ba8`, into `lamina/tokens/floating_toolbar.hpp` and `lamina/widgets/floating_toolbar.hpp`.

- 64 dp tall (or wide), full corners, 8 dp padding, actions in 48 dp slots (`IconButton` with its minimum touch size). Standard colors surface-container / on-surface with a primary-container FAB, vibrant primary-container / on-primary-container with a tertiary-container FAB. The FAB's corner stays 16 dp, its shadow level 2 (level 3 hovered).
- Without a FAB: the leading content expands from the start and shrinks toward the end, the trailing content the other way, on the fast spatial spring (`expandHorizontally`, `shrinkHorizontally`); no shadow.
- With a FAB: the bar's width (height when vertical) goes from its full size to 0 on the fast spatial spring, toward the FAB, while the FAB grows from 56 to 80 dp; the shadow goes from level 1 to level 0. The FAB is at the end (horizontal) or the bottom (vertical).
- Collapse on scroll (`floatingToolbarVerticalNestedScroll`): scrolling 40 dp toward the end collapses, 40 dp back expands; `ToolbarScrollExpansion` follows a scroll offset.
- Semantics: a `toolbar` node whose description names the custom action ("Collapse toolbar" or "Expand toolbar", ツールバーを閉じます / ツールバーを開きます). Collapsed actions are not Tab stops.
- Deviations: when the toolbar is disabled, its FAB stops reacting and its icon dims to 38 %, a lamina-ui addition (Compose's FABs have no disabled state). The exit-always scroll behavior and `minimumInteractiveBalancedPadding` are not implemented; the vibrant toggle-button colors are not used because the toolbar's actions do not toggle.

### Flexible bottom app bar

C `AppBar.kt` (`FlexibleBottomAppBar`, `BottomAppBarDefaults`, `settleAppBarBottom`) and `tokens/DockedToolbarTokens.kt` at `6068a9492ba8`, into `lamina/tokens/floating_toolbar.hpp` (`flexible_bottom_app_bar_tokens`) and `lamina/widgets/flexible_bottom_app_bar.hpp`.

- 64 dp, surface-container, no shadow, 16 dp padding, actions spaced between (or packed 32 dp apart and centered, `FlexibleFixedHorizontalArrangement`).
- Exit always: the height offset follows the scroll down to −64 dp, so the bar slides out of its bottom edge; when the scroll stops it settles on the fast spatial spring to whichever end is nearer (collapsed fraction 0.5, nothing to do below 0.01). Actions are not Tab stops while it is hidden.
- Deviation: the fling decay before the settle is not implemented.

### Density

Read on 2026-11-21. The material-web commit named for density (`9d2f79851cae`) has no density scale: its only density setting is the `touch-target` attribute (`wrapper` or `none`). Compose has none either, only `minimumInteractiveComponentSize` (48 dp, `LocalMinimumInteractiveComponentSize`). The scale comes from MDC Web (material-components-web, MIT) at `6ceadb81df50`, `packages/mdc-density/_density.scss`: each step adds `scale × 4 px` to a component's default height, a component lists its minimum and maximum (the default), and a non-zero scale resets the 48 px touch target. Values in `lamina/tokens/density.hpp` (`density_tokens`), read by each widget's `density` option and `set_density`; `WidgetSet::set_density` passes one to every member that has it (`Densifiable`), nested sets included.

| Component | Default | Minimum | Steps | Source of the minimum |
|---|---|---|---|---|
| Button, extra small / small / medium | 32 / 40 / 56 | 24 / 28 / 44 | −2 / −3 / −3 | MDC button (36 → 24) for small, lamina-ui for the others |
| Icon button, extra small / small / medium (width shrinks too, not below the icon) | 32 / 40 / 56 | 24 / 28 / 44 | −2 / −3 / −3 | MDC icon button (48 → 28), lamina-ui |
| Outlined text field (the multi-line padding shrinks by half a step on each side) | 56 | 36 | −5 | MDC text field |
| List item, one / two / three lines (the 10 dp top and bottom padding shrinks too) | 56 / 72 / 88 | 40 / 56 / 56 | −4 / −4 / −8 | MDC list (`tall` configurations) |
| Chip | 32 | 24 | −2 | MDC chips |
| Tab, text or icon beside / icon above | 48 / 72 | 30 / 56 | −4 / −4 | MDC tab bar |
| Segmented button | 40 | 28 | −3 | lamina-ui |
| Small and center-aligned top app bar, and the collapsed row of the others (their expanded height drops by the same amount) | 64 | 48 | −4 | MDC dense top app bar (48 px) |
| Navigation bar (the 6 dp item padding shrinks) | 64 | 52 | −3 | lamina-ui |
| Wide navigation rail item, collapsed / expanded | 64 / 56 | 52 / 40 | −3 / −4 | lamina-ui |
| Navigation drawer item | 56 | 40 | −4 | lamina-ui |
| Menu item | 48 | 32 | −4 | lamina-ui |
| Menu bar | 40 | 28 | −3 | lamina-ui |
| Table row / header | 52 / 56 | 36 / 40 | −4 / −4 | MDC data table |

- Large and extra-large buttons and icon buttons do not take density: removing 4 dp steps from 96 or 136 dp does not change how much fits.
- Deviations: MDC stops compilation for a scale below a component's minimum; lamina-ui clamps to the minimum, because one scale is set across many components. Positive scales clamp to the default. Segmented buttons, navigation items and menus have no density in MDC; their minimums are lamina-ui's own. The menu bar has no MDC counterpart either; the table follows MDC's data table rows (52 → 36), with the header 4 dp taller as in MDC.
- Touch target: at density 0 the controls keep their 48 dp hit area (`tokens::minimum_target`); at any negative density the hit area is the control's own bounds, for touch and pen too, as MDC resets its touch target. A 48 dp area around controls packed closer than 48 dp would overlap its neighbors' areas. Where the areas of controls at density 0 overlap, the nearest control takes the pointer ([Touch target](#touch-target)).
- Checkboxes, radio buttons and switches do not take density yet.

### Window size classes

Read on 2026-11-21 from androidx at `6068a9492ba8`: `window/window-core/src/commonMain/kotlin/androidx/window/core/layout/WindowSizeClass.kt` and `WindowSizeClassSelectors.kt` (`BREAKPOINTS_V2`, `computeWindowSizeClass`), into `lamina/layout/window_size_class.hpp`.

- Width: compact below 600 dp, medium from 600, expanded from 840, large from 1200, extra-large from 1600. Height: compact below 480 dp, medium from 480, expanded from 900.
- A size class is the pair of lower bounds (`min_width_dp`, `min_height_dp`), as in window-core. The window size is truncated to whole dp before it is compared, so 599.9 dp is compact.
- `compute_window_size_class(width, height)` is a pure function; `sdl3::Frame::size_class` holds the window's class for every frame, live resizing included. `WindowAdaptiveInfo` (in `lamina/widgets/pane_navigator.hpp`) pairs a class with a tabletop flag, as Compose's `WindowAdaptiveInfo` pairs it with a `Posture`; desktop windows have no posture, so the flag is false unless a program sets it.

### Navigation suite scaffold

C `compose/material3/material3-adaptive-navigation-suite/src/commonMain/kotlin/androidx/compose/material3/adaptive/navigationsuite/NavigationSuiteScaffold.kt` at `6068a9492ba8`, into `lamina/widgets/navigation_suite.hpp`.

- `navigation_suite_type(info)` is Compose's `NavigationSuiteScaffoldDefaults.navigationSuiteType`: a short navigation bar on compact widths, the medium short bar when the window is tabletop or its height is compact, and a collapsed wide rail otherwise. It never picks an expanded rail or a drawer.
- The bar and the side component show and hide on Compose's `AnimationSpec` (spring, damping 0.9, stiffness 700). The content gives up the bar's height or the side component's width times that progress.
- Deviations: `navigation_suite_type(info, choice)` with `widening_navigation_suite` (compact: bar, medium: collapsed rail, expanded: expanded rail, large and extra-large: drawer, compact height: medium bar) is lamina-ui's own chooser, after the M3 guidance that wider windows show more of the navigation. Compose swaps components without a transition when the type changes; lamina-ui slides the bar out and the side component in on the same spring, and shows the collapsed rail, the expanded rail and the drawer as one `SideNavigation` (see Side navigation), so changing between them morphs one list and moves no icon. The collapsed rail is the 80 dp icon-only rail: the 96 dp rail with labels under the icons has taller rows, which would move the icons. With the labels off, every side type collapses to the icon-only rail. The medium short bar (`ShortNavigationBarMedium`, icon beside the label) is drawn like the compact one, since lamina-ui's navigation bar has only the stacked arrangement. The primary action slot (`primaryActionContent`) is not implemented.

### Side navigation

lamina-ui's own component, built from the values above, in `lamina/tokens/side_navigation.hpp` (`side_navigation_tokens`) and `lamina/widgets/side_navigation.hpp`. Compose has no single list that is both a rail and a drawer.

- Values taken from M3: the collapsed width 80 dp (the narrow `NavigationRail`, as the icon-only rail above), the wide rail's 220 to 360 dp, the drawer's 360 dp, 12 dp at the top, 12 dp inset, 16 dp before the icon, 12 dp to the label and Label L (all `NavigationDrawerTokens`), the 56 × 32 dp pill (`NavigationRailVerticalItemTokens`), the row height (the drawer item, 56 dp, lowered with the density), and the rail's colors.
- The values line up so that one geometry serves every form: 80 = 12 + 56 + 12 and 56 = 16 + 24 + 16, so the icon sits at x = 28 dp both inside the collapsed pill and in the drawer row, and at y = 12 dp + n × row + (row − 24) / 2 in every form.
- Deviations from the wide rail: its items sit 12 + 16 dp from the start instead of 20 + 16 dp, with 12 dp instead of 8 dp to the label, and its rows have no 44 dp at the top or 4 dp between them; otherwise the icons would move when it collapses or turns into the drawer. Its indicator still hugs the icon and label.
- Motion (lamina-ui's own values): the expansion and the drawer amount move on a spring with damping 1.0 and the default spatial stiffness (380), so the width and the indicator never overshoot and change monotonically. Width = lerp(80, lerp(rail, drawer, D), E); the indicator is lerp(56, lerp(hug, row, D), E) wide and lerp(32, row, E) tall, centred on the row. Label opacity = (2 (E − 0.5))², clamped, which takes the labels away in the first half of collapsing and brings them in in the second half of expanding; it does not depend on D. Selection grows the indicator's width from its start edge on the default spatial spring, as on the rail.

### Pane scaffolds

From androidx at `6068a9492ba8`: `compose/material3/adaptive/adaptive-layout/.../layout/` (`PaneScaffoldDirective.kt`, `ThreePaneScaffoldValue.kt`, `ThreePaneScaffold.kt`, `PaneMotion.kt`, `PaneExpansionState.kt`, `PaneExpansionDraggableModifier.kt`, `ListDetailPaneScaffold.kt`, `SupportingPaneScaffold.kt`) and `adaptive-navigation/.../navigation/` (`ThreePaneScaffoldNavigator.kt`, `BackNavigationBehavior.kt`), into `lamina/widgets/pane_navigator.hpp` and `pane_scaffold.hpp`.

- Directive (`calculate_pane_scaffold_directive`): one horizontal partition on compact and medium widths, two from expanded (24 dp spacer), three from large with a 412 dp preferred pane width (360 dp below); two vertical partitions (24 dp spacer) when there is one horizontal partition and the height is expanded, or the window is tabletop. `..._with_two_panes_on_medium_width` gives medium widths two partitions.
- Value (`calculate_pane_scaffold_value`): the destinations from the latest back, then primary, secondary, tertiary, each take a partition while there is one; a pane whose strategy is reflow goes under its anchor pane when there is one horizontal partition and more than one vertical. List-detail: list = secondary, detail = primary, order list, detail, extra, every strategy hide. Supporting: main = primary, supporting = secondary (reflow under main), order main, supporting, extra.
- Navigator: history of destinations (pane and content key), `navigate_to`, `can_navigate_back` and `navigate_back` with the four `BackNavigationBehavior`s (pop latest, until the value changes (the default), until the destination changes, until the content changes), history-aware values. The scaffold calls `navigate_back` on Escape when no pane widget used the key.
- Layout: expanded panes take their preferred widths and the pane with the highest priority (primary 10, secondary 5, tertiary 1) takes what is left, or all shrink in proportion when there is not enough room. A reflowed pane gets the bottom of its anchor's partition, at most the 420 dp preferred height and at most half.
- Motion: Compose's `calculatePaneMotion` (shown panes animate their bounds, entering panes come from the side that nothing blocks, or expand from the middle, exiting panes leave the same way or shrink) on the spring with damping 0.8 and stiffness 380; the delayed entries wait 0.1 of the spring's estimated settle time.
- Focus: hidden panes take no pointer, keys or text and leave the accessibility tree; their focus is cleared. When a navigation changes the value while a pane holds the keyboard focus, the focus moves to the destination pane (Compose's `shouldAutoFocusCurrentDestination`). Programs put each pane's `FocusManager` in their own with `FocusManager::add_group`, gated by `pane_shown`.
- Deviations: Compose snaps the value when the directive changes (on resize) and animates only navigation; lamina-ui does the same by default and animates directive changes too with `animate_directive_changes`. Levitated panes, hinges and excluded bounds, predictive back and the saved state are not implemented; a reflowed pane's height is lamina-ui's simplification of Compose's partition measuring.

### Pane expansion drag handle

C `DragHandle.kt` and `tokens/DragHandleTokens.kt` (`VERSION: 7_0_1`) at `6068a9492ba8`, into `lamina/tokens/drag_handle.hpp`, with the expansion behavior of `PaneExpansionState.kt` in `PaneExpansionState`.

- Handle: 4 × 48 dp, full corners, outline color; pressed or dragged 12 × 52 dp, medium corners (12 dp), on-surface. Its touch target is 48 dp, centered in the spacer between two expanded panes, and it stays half a spacer inside the scaffold.
- Anchors: a proportion, an offset from the start or one from the end (`PaneExpansionAnchor`). Dragging moves the split freely between 0 and the scaffold's width; a split within half a spacer of an edge shows only the other pane. Releasing settles on the closest anchor, or, at 200 dp/s or faster, the closest one in the direction of the release (anchors behind count the whole width further), on the spring with damping 0.8 and stiffness 380 from the release velocity. `set_first_pane_width` and `set_first_pane_proportion` place the split without anchors.
- Semantics: a separator named "Pane expansion drag handle" whose value is "Current pane split, %s" and description "Change pane split to %s" with the anchor descriptions ("%d percent", "%d DPs from start", "%d DPs from end"), translated (`PatternStrings`, Japanese from androidx `values-ja`). Enter or Space moves to the next anchor, Compose's click action.
- Deviations: Compose's 200 is in px/s; lamina-ui reads it as dp/s, which is the same at a scale of 1. Compose runs the fling's decay before choosing the anchor; lamina-ui chooses from the release velocity directly. Left and Right move the split by 16 dp, a lamina-ui addition. The handle's size and color move on the fast spatial and fast effects springs where Compose switches at once.

### Split panes

`lamina/widgets/split_pane.hpp` and `lamina/tokens/split_pane.hpp`. Material 3 has no resizable split for desktop windows, so these values are lamina-ui's own: a 1 dp outline-variant divider (the divider tokens), an 8 dp hit zone with the resize cursor, the M3 drag handle shown on hover, drag and keyboard focus, and the keys of WAI-ARIA's window splitter pattern: arrows move the divider by 16 dp, Page Up and Page Down by 96 dp, Home and End go to the minimum and maximum, and Enter collapses the first pane to its minimum and restores it. Accessibility: a separator with the first pane's size as its value and range.

### Scroll area: horizontal

lamina-ui's own, beside the vertical scroll area above: `set_content_width` makes the content scroll sideways too. The wheel's horizontal axis (a touchpad, a tilting wheel) scrolls sideways; Shift with the vertical wheel scrolls sideways when the content is wider than the viewport and vertically otherwise (`WheelEvent::shift`, from `SDL_GetModState`). The horizontal bar runs along the bottom with the vertical bar's sizes, and each bar's track leaves the corner to the other. A touch drag scrolls both ways when the content is larger than the viewport both ways ([Touch scrolling](#touch-scrolling)).

### Numeric field

`lamina/widgets/numeric_field.hpp` and `lamina/tokens/numeric_field.hpp`, for inspectors and property panels; Material 3 has no compact numeric field, so the values are lamina-ui's own. 40 dp tall with density 0, 28 dp at least (4 dp a step), 96 dp wide by default, extra-small corners. The label is at the start (outside, outlined field) or inside (on surface-container-highest, before the value); the unit sits at the end. Dragging across the field or its start label by more than 3 dp scrubs the value one step every 4 dp; a click edits the text, which a bare `TextField` handles (selection, IME, clipboard, undo). Up and Down step the value (Shift for the large step), Page Up and Page Down take the large step, Enter commits, Escape restores the value, and losing the focus commits. Values are clamped to the range and rounded to `decimals`; text that is not a number keeps the field in the error color and marks it invalid. Accessibility: a spin button (`AccessibleRole::spin_button`) with the value and the unit, and the range.

### Menu: shortcuts, dividers and submenus

C `Menu.kt`, `MenuDefaults.kt` and `tokens/ListTokens.kt` at `6068a9492ba8`, and W `menu/internal/submenu/sub-menu.ts` at `9d2f79851cae`, into `menu_tokens` and `lamina/widgets/menu.hpp`.

- Trailing slot: in Compose's `DropdownMenuItem` with a `trailingIcon`, the text takes the rest of the row with 12 dp of padding at its end (`DropdownMenuItemHorizontalPadding`), the slot is at least 24 dp wide (`ListTokens.ItemTrailingIconSize`) and draws in on-surface-variant (`ItemTrailingIconColor`), on-surface at 38 % when disabled, and the row's text is Label L. lamina-ui puts a shortcut label (`MenuItem::shortcut`, written by `format_chord`) or the submenu arrow (`keyboard_arrow_right`, 24 dp) in that slot, at the item's end. Compose has no shortcut label; showing the chord in the trailing slot is lamina-ui's.
- Dividers: `divider_before` draws a 1 dp outline-variant line before the item, 12 dp from the sides with 2 dp above and below (`MenuDefaults.HorizontalDividerPadding`).
- Submenus follow material-web's `md-sub-menu`: hovering an item with a submenu opens it after 400 ms (`hoverOpenDelay`) and hovering another item closes it after 400 ms (`hoverCloseDelay`); Right, Enter or Space opens it with its first item focused, and Left or Escape closes it and returns to its item. The submenu's top start corner meets the item's top end corner (`anchorCorner` START_END, `menuCorner` START_START); it opens on the item's left when there is no room on the right, and moves up to stay 4 dp inside the window. The item whose submenu is open keeps the hover state layer. A submenu needs `MenuOptions::icon_font` for its arrow. Deviations: pressing an item opens its submenu at once, and Tab closes the whole menu. Compose has no submenus.
- A choice reports `MenuResponse::path` (indices from the top menu down) and the item's `command`; `chosen` is set only for an item of the menu itself, as before.
- `open_at(point)` is the position of Compose Desktop's `PopupPositionProviderAtPosition` (J `DesktopPopup.desktop.kt`, used by its context menus): the menu's top start corner at the point, moved left by its width when it would pass the window's right edge less 4 dp, up by its height when it would pass the bottom edge less 4 dp, and then kept at least 4 dp from the left and top (`windowMargin`, `context_window_margin`). It scales in around the point.

### Context menu area

J `ContextMenuProvider.desktop.kt` and `BasicContextMenuRepresentation.desktop.kt`: Compose Desktop's `ContextMenuArea` opens when the secondary button is pressed, at the pointer. `ContextMenuArea` (`lamina/widgets/context_menu.hpp`) takes an area and a `Menu`, and opens the menu with `open_at` on an `input::ContextClick` inside the area. The SDL3 window reports a right press as a `ContextClick` in `Frame::context`, not as a `PointerEvent`, so no widget takes it for a click. While the area's `focused` hook says so, Shift+F10 or the Menu key (`requests_context_menu`) opens the menu at `keyboard_point` (the area's top start corner by default) with its first item focused. `before_open` runs first with the point; the Files view uses it to select the row under the pointer (`Table::handle(ContextClick)` selects that row unless it is already selected, as file managers do). Deviation: the keyboard opening is lamina-ui's, after Windows; the Compose file has no key handling.

### Keyboard shortcuts

J `KeyShortcut.desktop.kt`: a `KeyShortcut` is a key with ctrl, meta, alt and shift, written as `Ctrl+`, `Meta+`, `Alt+`, `Shift+` and the key. In lamina-ui a `KeyChord` (`lamina/widgets/shortcuts.hpp`) is a key with Ctrl, Shift and Alt, and `format_chord` writes it with a `ShortcutFormat` from `lamina/tokens/shortcut.hpp`, the one place that holds the platform's spelling: `pc_shortcut_format` for Windows and Linux (Ctrl, Alt, Shift in that order joined by `+`, and Del, Ins, PgUp, PgDn, Esc, Enter, Space, Backspace, Left, Up, F1 to F12, Plus, Minus for named keys) and `mac_shortcut_format` (⌃ ⌥ ⇧ without a separator, and ⌫ ⌦ ↩ ⎋ ⇥ ← → ↑ ↓ ⇞ ⇟ ↖ ↘); `default_shortcut_format()` picks one by platform. lamina-ui's Ctrl is the Control key on every platform, so the mac format draws it as ⌃; the window does not map Command yet.

- `ShortcutRegistry` holds commands by id, each with an optional chord, an action and an enabled flag. `handle(KeyEvent)` runs the enabled command whose chord matches exactly and reports it consumed; a disabled command neither runs nor consumes its key. A second command with the same id or chord throws.
- The same definition drives the menus: `apply_shortcuts` copies the chords of the items' commands into the items, `sync_commands` copies the enabled flags, and `MenuBar` does both and runs the command of a chosen item.
- Focus: `handle(event, focus)` runs a command only when `FocusManager::claims_key` is false, so the focused widget keeps the keys it claims. A focused `TextField` or `NumericField` claims Ctrl+A, Ctrl+C, Ctrl+X and Ctrl+V, and Ctrl+Z and Ctrl+Y while it has something to undo or redo (`handles_history_key`, unchanged); a focused `Table` claims Ctrl+A. The routing is lamina-ui's: Compose Desktop leaves menu shortcuts to Swing.

### Menu bar

Compose Desktop's `MenuBar` is a native Swing menu bar (J `Menu.desktop.kt` builds `JMenu`s and hands shortcuts to Swing as accelerators), and Material 3 has no menu bar, so the look is lamina-ui's own, from M3 roles, in `lamina/tokens/menu_bar.hpp`: a surface-container-low bar 40 dp tall (28 dp at least, 4 dp a step); items with 12 dp of padding on each side of Label L text, inset 4 dp from the bar's top, bottom and start, with small (8 dp) corners; the open item on secondary-container with on-secondary-container text, fading on the default effects spring; hover and press state layers on on-surface and the inward focus ring. Each menu is the classic `Menu`, opened below its item with 4 dp window margins instead of Compose's 48 dp, which would push a menu bar's menus away from the bar.

- Keys, after Windows: Alt pressed and released alone (the window reports it as `Key::alt`) or F10 focuses the bar and shows the access-key underlines. Left and Right move along it (wrapping), Home and End go to the ends, Down, Up, Enter or Space open the menu, and a letter opens its menu; Alt with a letter opens a menu from anywhere. In an open menu, Left and Right open the previous or next menu unless a submenu takes the key; Escape closes the menu and leaves the bar focused, and a second Escape, Alt or F10 leaves the bar. While a menu is open the bar takes every key.
- Pointer: pressing an item opens its menu; while a menu is open, moving onto another item switches to its menu; pressing the open item closes it; pressing an item, dragging onto a menu item and releasing chooses it.
- Accessibility: a `menu_bar` whose children are `menu_item`s with the value Expanded or Collapsed, and the open menu inside its item.

### Table

Material 3 has no data table. The values below come from MDC Web's data table (D, `packages/mdc-data-table/_data-table-theme.scss` and the header cell styles at `6ceadb81df50`) where it has one, mapped onto M3 roles, and are lamina-ui's own otherwise; `lamina/tokens/table.hpp` and `lamina/widgets/table.hpp`.

| Value | lamina-ui | Source |
|---|---|---|
| Row height | 52 dp, 36 dp at least (density −4) | D `$row-height`, `$minimum-row-height` |
| Header height | the row height plus 4: 56 dp, 40 dp at least | D `get-header-row-height` |
| Cell padding | 16 dp at the start and end | D `$cell-leading-padding`, `$cell-trailing-padding` |
| Header text | Title S, on-surface | D `subtitle2`, mapped to M3 Title S |
| Cell text | Body M, on-surface | D `body2`, mapped to M3 Body M |
| Row divider | 1 dp outline-variant; none under the last row | D 1 px on-surface at 12 %, mapped to the M3 divider token |
| Selected row | secondary-container, on-secondary-container text | the M3 list's selected container (D: primary at 4 %) |
| Hover | on-surface state layer at 8 % | M3 state layer (D: 4 %) |
| Sort arrow | `arrow_upward` / `arrow_downward`, 18 dp, 4 dp after the label (before it for end-aligned columns); on-surface on the sorted column, on-surface-variant fading in on hover | D 4 px gap and hover reveal; the size is lamina-ui's |
| Header container | surface-container | lamina-ui |
| Header column dividers | 1 dp outline-variant, 16 dp from the top and bottom | lamina-ui |
| Resize zone | 8 dp centered on a column's end edge, with the resize cursor | lamina-ui (D has no column resizing) |
| Column width | 160 dp by default, 56 dp at least | lamina-ui |

- Sorting: clicking a sortable header sorts ascending, again descending; a click on another column sorts it ascending. The sort is stable and uses the column's `less` (on model rows), or the cells' text. The table keeps a permutation from view rows to model rows, so the selection and the focus stay on the same rows.
- Resizing: dragging a header divider changes the column's width, not below `min_width`, and the content scrolls sideways when the columns are wider than the table.
- Selection (`TableSelection::multiple`, after Windows list views): a click selects one row and sets the anchor, Ctrl+click toggles a row, Shift+click selects the range from the anchor, Ctrl+Shift+click adds the range; a click below the rows clears the selection; a double click or Enter activates. Arrows, Home, End, Page Up and Page Down move the focus and select it, with Shift they extend from the anchor and with Ctrl they move the focus only; Space selects the focused row, Ctrl+Space toggles it and Ctrl+A selects all. The table is one Tab stop and draws the inward focus ring on the focused row after a key.
- Scrolling: a `ScrollArea` below the header scrolls both ways (the wheel, Shift and the wheel, the bars); the header stays on top and scrolls sideways with the rows, and the wheel over the header scrolls the rows.
- Virtualization: only the rows in view and two more on each side are laid out and painted; their text is laid out once while they are in view and dropped when they leave. The accessibility tree has the header row with its column headers (the sorted one says Ascending or Descending) and the rows in view with their cells. The selected color and the hover move on springs.
- Not done: ellipses (cells clip their text), reordering columns, a checkbox column and pagination (D has both), fitting a column to its content, and editing cells.

## Open questions

- Components other than those above are not surveyed yet.
- Not implemented from the components of the second stage: the `shapes:` chip overloads (`ChipShapes`), `SegmentedListItem` with its 2 dp gaps and per-position corners, dragged chips, cards and list items, and dragging a top app bar itself.
- Not implemented from the components of the third stage: swiping between months, Compose's vibrant time picker (`TimeScroll` and the `shapes:` overloads), the time picker dial's 238 and 200 dp sizes for short heights, and the slider forms Compose does not have (an icon in the track, the XS–XL sizes).
- Not implemented from the screen patterns of the fourth stage: predictive back (sheets, drawers, rail, search), the decay phase of anchored flings, nested scrolling between sheet content and the sheet, `AppBarWithSearch`, `ExpandedFullScreenContainedSearchBar` and `ExpandedDockedSearchBarWithGap`, and the M3 design site's side sheet measurements (unreadable without script).
- Not implemented from the Expressive components of the fifth stage: the vertical button group and per-item alignment, a widening button on a key press, the exit-always floating toolbar and `minimumInteractiveBalancedPadding`, the linear wavy indicator's accelerated run-out before a handoff, a scrolling FAB menu, `Modifier.animateFloatingActionButton`, `AppBarWithSearch` and the other Expressive search bars, `AppBarRow` and `AppBarColumn` with their overflow, `CheckableDropdownMenuItem` and `SelectableDropdownMenuItem`, `ScrollField` and `Modifier.nonInteractiveScrollbar`.
- Not implemented from the adaptive and desktop work of the sixth stage: density for checkboxes, radio buttons and switches, the medium short navigation bar's arrangement, the navigation suite's primary action, levitated panes, hinges, predictive back and saved pane state, and the decay before a pane anchor is chosen.
- Not implemented for the pager: handing a drag over in the middle of it, the overscroll stretch, wheel paging, right-to-left layouts, a vertical pager, and Compose's own pager values, whose source was not on this machine ([Pager](#pager)).
- Not implemented from the desktop work of the seventh stage: menu typeahead, the Command key on macOS, checkable and radio menu items, ellipses in table cells, reordering and fitting table columns, a checkbox column, pagination and cell editing.
