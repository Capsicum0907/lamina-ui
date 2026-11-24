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

`lamina/tokens/motion.hpp`: the spring, easing and duration tokens of the standard and expressive motion schemes. Each scheme names its six springs by role, as Compose's `MotionScheme` does (`fast_spatial()`, `default_spatial()`, `slow_spatial()`, `fast_effects()`, `default_effects()`, `slow_effects()`). Every widget takes its springs from the active style's `motion` ([Styles](#styles)), which is the expressive scheme (`tokens::motion_scheme`) in the M3 Expressive style.

`lamina/motion/easing.hpp`, `lamina/motion/tween.hpp`: cubic-bezier easing (`ease(curve, fraction)`, Compose's `CubicBezierEasing`) and `Tweened`, a value that moves to a target over a duration along a curve, for the few motions Compose runs as tweens rather than springs: the elevation of chips and cards and the title fade of the two-row app bars. `tokens::CubicBezier` is `motion::CubicBezier`.

`lamina/motion/spring.hpp`: a damped spring with a closed-form solution. `advance` moves a value toward its target by any time step, and the result does not depend on how the time is split into frames. `is_settled` tells when the motion has come to rest.

`lamina/motion/animated.hpp`, `lamina/motion/animated_radii.hpp`, `lamina/motion/lerp.hpp`, `lamina/color/mix.hpp`: a value driven by a spring, four corner radii each driven by a spring (`AnimatedRadii`, never below zero while a spring overshoots; the grouped menu's groups and items morph their corners with it), linear interpolation between two numbers, and color mixing and compositing. `color::source_over` lays one color over another, and `color::state_layer(container, content, opacity)` is a container with the state layer composited onto it: the button, icon button and FAB fill their container with it, since their state layer has the container's shape. A state layer with a shape of its own (the circle of a switch, a menu item, a rail pill) is filled with `with_alpha(content, opacity)` over what is already drawn, which the canvas composites the same way. `color::faded(scheme, opacity)` scales the alpha of every role; the dialog, the menu and the grouped menu paint with a faded scheme while they fade in and out, and the dialog passes it to the label and buttons inside it. An icon button scales its state layer's opacity by the alpha of its content color, so a pressed or focused icon button painted with a faded scheme fades its state layer too, instead of keeping it at full strength.

## Spring model

A spring has unit mass, stiffness k and damping ratio ζ. The displacement y from the target follows y'' + 2ζ√k y' + k y = 0. `advance` uses the exact solution for each case: ζ < 1 (oscillates and overshoots), ζ = 1 (fastest without overshoot), ζ > 1 (slower, no overshoot). Spatial springs (position, size, shape) in the expressive scheme have ζ < 1 and overshoot; effects springs (color, opacity) have ζ = 1.

## Styles

`lamina/style/style.hpp`: `lamina::Style` holds every design value a widget reads, as runtime values. It has the color spec (`Style::color`, a `const color::ColorSpec*`) and one member for each component's tokens (`button`, `icon_button`, `navigation_bar`, `dialog`, `scrim`, `menu`, `list`, `wavy_progress`, ...), plus the foundation tokens shared by all of them: `shape` (the corner-radius scale), `type` (the type scale), `motion` (the motion scheme), `elevation` (the levels, the shadow layers and the elevation tweens), `state_layer` (the state-layer opacities), `interaction` (the minimum touch target and the disabled alpha), `density` (the dense lengths), `gesture` (touch slop, velocity tracking, fling decay and touch scrolling) and `zoom`. A member is either the component's token struct (`tokens::ScrimTokens scrim`) or, where a component has several tables, a set that holds them with the lookups that used to be free functions (`tokens::ButtonSet` with `size(ButtonSize)`, `style(ButtonStyle)` and `toggle_style(ButtonStyle)`; `tokens::ShapeTokens` with `radius(CornerSize, width, height)` and `radii(Corners, width, height)`; `tokens::TypeTokens` with `style(TypeRole)` and `text(TypeRole)`). `Style::minimum_target(density)` is the touch target for a density.

`m3_expressive_style()` is the Material 3 Expressive style. A default-constructed `Style` is the same: every member's default is the matching `constexpr` table in `tokens/`, so the values are written once, in the token headers, and the tables stay the source of the defaults. The free functions in `tokens/` (`tokens::button_size`, `tokens::corner_radius`, `tokens::type_style`, ...) still return those default values; they do not follow the active style.

### How widgets get their tokens

`current_style()` is the active style of the calling thread: `m3_expressive_style()` unless another one is installed. Every widget reads its tokens through it (`current_style().button.size(options_.size)`, `current_style().scrim.opacity`); no widget reads a `tokens::` table directly, except the shortcut text formats listed below, and no widget asks which style is active.

The active style is a thread-wide setting rather than an argument because widgets read tokens in every phase, not only while painting. A button takes its height, its label's type and its springs when it is constructed, its minimum width when it is measured, its touch target when it handles a press, and its colors and outline when it paints. The color scheme reaches only `paint(canvas, scheme)`, so passing the style alongside the scheme would have left the other phases on the constant tables, and adding a style to every constructor would have changed the signature of every widget. With the active style, no public signature changed. The color scheme stays on the paint path: it is built per app (seed, light or dark, contrast) and is changed per paint, since the dialog and the menus paint their contents with a faded scheme while they fade.

The style holds the color spec rather than a built scheme for the same reason: the spec is the rule that turns an app's `SchemeOptions` into roles, and it belongs to the style, while the options and the fading belong to the app. An app that follows the style builds its scheme with `style.color->build(options)`. The M3 Expressive style's spec is `spec_2025()`. The examples still name their spec themselves.

### Supplying a style

- `StyleScope scope(style)` installs a style until the end of the scope and then restores the one before it; scopes nest.
- `set_current_style(style)` installs a style for the rest of the thread, for an app with one style.
- `sdl3::Window::set_style(style)` gives a window its own style; `Window::style()` returns it, or the active style when none was set. `sdl3::run_app` installs the window's style around each frame's update and paint. An app with its own frame loop puts a `StyleScope` around its update and paint. A per-window style needs the `run_app(Window&, update, paint)` overload, since the overload that takes `WindowOptions` creates its window itself.
- Widgets built before the frame loop, as in the [minimal program](../README.md#a-minimal-program), read their construction-time values from the style active where they are built, not from the window's style. Build them under the same style too: inside a `StyleScope`, or after `set_current_style`.

The style is held by reference: the caller keeps it alive while it is installed. A style is copied and changed like any struct:

```cpp
lamina::Style quiet = lamina::m3_expressive_style();
quiet.scrim.opacity = 0.5;
quiet.button.styles[static_cast<std::size_t>(lamina::tokens::ButtonStyle::outlined)].outline = lamina::color::Role::outline;
window.set_style(quiet);
```

A widget reads each value when it uses it. Values used while painting (colors, outlines, the scrim, state layers, corner radii computed from the size) follow a style installed later at once. Values taken at construction (heights, the laid-out labels, springs) belong to the style that was active when the widget was built, so after switching styles a program rebuilds the widgets whose sizes or motion should change.

Some tables are derived from others in `tokens/`: the dense lengths start from the components' heights, and many components' springs come from the motion scheme. A `Style` holds the derived values as they are, so a style that changes a height or the motion scheme changes the derived members too.

Not in the style: the raster settings of the window (`tokens::raster`), the frame statistics overlay (`tokens::frame_stats`), the icon font's axes (`tokens::symbol_axes`), the video tokens and the shortcut text formats (`tokens::default_shortcut_format()`, chosen by the platform). The canvas and the text layout do not read the style.

### Breaking changes in the next release

- `lamina::Style` is a new name in `namespace lamina`. A file that has `using namespace lamina;` and declares its own `Style` at namespace scope gets an ambiguous name; rename one of them or qualify it. The layout example's `Style` is now `LayoutStyle`.
- The option structs whose defaults come from tokens (`ButtonGroupOptions::expanded_ratio`, `ChipGroupOptions::spacing`, `PagerOptions::tokens`, `PullToRefreshOptions::tokens`, ...) take those defaults from the style active when the options object is constructed, and default arguments such as the `max` of `badge_count` from the style active at the call. `PagerOptions::tokens` and `PullToRefreshOptions::tokens` point into that style.
- Code that reads `tokens::` tables directly keeps the M3 Expressive values when another style is active. To follow the style, read `current_style()` instead.
- `paint(render::Canvas&, const color::ColorScheme&)`, `Paintable` and every widget constructor are unchanged.

## Sources

- Material 3, color: <https://m3.material.io/styles/color/overview>
- Material 3, shape: <https://m3.material.io/styles/shape/overview>
- Material 3, motion: <https://m3.material.io/styles/motion/overview>
- Material 3, elevation: <https://m3.material.io/styles/elevation/overview>
- material-color-utilities: <https://github.com/material-foundation/material-color-utilities>
- androidx graphics-shapes: <https://github.com/androidx/androidx/tree/androidx-main/graphics/graphics-shapes>
- The source files and commits of every value: [design tokens](design-tokens.md)
