# Theming and tokens

[日本語](theming.ja.md) · [README](../README.md#documentation)

The design values: color schemes, shapes, type, elevation, state layers and motion. Every value and its source is listed in [Design tokens](design-tokens.md); where lamina-ui chose its own values is in [Differences from Material 3](differences-from-m3.md). Density is in [Layout](layout-and-adaptive.md#density).

## Color

`lamina/color/scheme.hpp`: color schemes from one seed color. `spec_2021().build(options)` and `spec_2025().build(options)` return the 53 color roles (primary, primary-dim, surface-container, outline, ...) for a seed, light or dark, a variant (TonalSpot, Expressive, Vibrant, ...) and a contrast level, by the 2021 or the 2025 color spec. The 2021 spec is computed by Google's material-color-utilities (`third_party/`). The 2025 spec (`color/spec_2025.cpp`) is a port of that library's Java `ColorSpec2025` for phones, built on its C++ HCT and contrast functions; `tools/color-dump/` checks it against the Java code. Both specs start from the library's scheme for the variant, made in `color/mcu_scheme.cpp`.

The color spec is behind the `ColorSpec` interface, so code that uses a scheme does not change with the spec. The two specs share HCT; they differ in the palettes each variant builds and in the rules that pick each role's tone. `examples/buttons` has a switch that changes between them.

## Shape, type, elevation and state layers

`lamina/tokens/shape.hpp`, `typography.hpp`, `elevation.hpp`, `interaction.hpp`: the corner-radius scale (`corner_radii(corners, width, height)` resolves a `Corners` set to the four radii), the type scale (with the emphasized roles), elevation levels, and the state-layer opacities with the minimum touch target. These foundation tokens are shared by all components; the tokens of one component are in the `tokens/` file with its widget's name (`tokens/grouped_menu.hpp` for `widgets/grouped_menu.hpp`). `TypeRole` has the 15 roles and the 15 emphasized roles of the Expressive type scale (`display_large_emphasized` to `label_small_emphasized`, or `tokens::emphasized(role)`), with Compose's values: the same size, line height and tracking as the role, and weight 500, or 700 for Title M, Title S and the labels. Anything that takes a `TypeRole`, such as `LabelOptions::type`, takes an emphasized role as well.

## Shape library and morphing

`lamina/shapes/`: a C++ port of androidx `graphics-shapes` (rounded polygons with per-corner rounding and smoothing, and `Morph`, which pairs the corners of two outlines and interpolates their cubic curves) and the 35 Material 3 Expressive shapes (`material_shape`). Attribution: [third_party/NOTICE-androidx-graphics-shapes.md](../third_party/NOTICE-androidx-graphics-shapes.md). `build/Release/shapes_gallery.exe` shows every shape; clicking one morphs the large shape into it with the expressive spatial spring.

## Motion

`lamina/tokens/motion.hpp`: the spring, easing and duration tokens of the standard and expressive motion schemes. Each scheme names its six springs by role, as Compose's `MotionScheme` does (`fast_spatial()`, `default_spatial()`, `slow_spatial()`, `fast_effects()`, `default_effects()`, `slow_effects()`). Every widget takes its springs from `tokens::motion_scheme`, which refers to the expressive scheme.

`lamina/motion/easing.hpp`, `lamina/motion/tween.hpp`: cubic-bezier easing (`ease(curve, fraction)`, Compose's `CubicBezierEasing`) and `Tweened`, a value that moves to a target over a duration along a curve, for the few motions Compose runs as tweens rather than springs: the elevation of chips and cards and the title fade of the two-row app bars. `tokens::CubicBezier` is `motion::CubicBezier`.

`lamina/motion/spring.hpp`: a damped spring with a closed-form solution. `advance` moves a value toward its target by any time step, and the result does not depend on how the time is split into frames. `is_settled` tells when the motion has come to rest.

`lamina/motion/animated.hpp`, `lamina/motion/animated_radii.hpp`, `lamina/motion/lerp.hpp`, `lamina/color/mix.hpp`: a value driven by a spring, four corner radii each driven by a spring (`AnimatedRadii`, never below zero while a spring overshoots; the grouped menu's groups and items morph their corners with it), linear interpolation between two numbers, and color mixing and compositing. `color::source_over` lays one color over another, and `color::state_layer(container, content, opacity)` is a container with the state layer composited onto it: the button, icon button and FAB fill their container with it, since their state layer has the container's shape. A state layer with a shape of its own (the circle of a switch, a menu item, a rail pill) is filled with `with_alpha(content, opacity)` over what is already drawn, which the canvas composites the same way. `color::faded(scheme, opacity)` scales the alpha of every role; the dialog, the menu and the grouped menu paint with a faded scheme while they fade in and out, and the dialog passes it to the label and buttons inside it.

## Spring model

A spring has unit mass, stiffness k and damping ratio ζ. The displacement y from the target follows y'' + 2ζ√k y' + k y = 0. `advance` uses the exact solution for each case: ζ < 1 (oscillates and overshoots), ζ = 1 (fastest without overshoot), ζ > 1 (slower, no overshoot). Spatial springs (position, size, shape) in the expressive scheme have ζ < 1 and overshoot; effects springs (color, opacity) have ζ = 1.

## Sources

- Material 3, color: <https://m3.material.io/styles/color/overview>
- Material 3, shape: <https://m3.material.io/styles/shape/overview>
- Material 3, motion: <https://m3.material.io/styles/motion/overview>
- Material 3, elevation: <https://m3.material.io/styles/elevation/overview>
- material-color-utilities: <https://github.com/material-foundation/material-color-utilities>
- androidx graphics-shapes: <https://github.com/androidx/androidx/tree/androidx-main/graphics/graphics-shapes>
- The source files and commits of every value: [design tokens](design-tokens.md)
