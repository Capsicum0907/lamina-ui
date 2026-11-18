# lamina-ui

[日本語](README.ja.md)

A C++ desktop UI library built on SDL3, modeled on Material 3 Expressive.

lamina-ui is not affiliated with or endorsed by Google. "Material Design" is Google's design system; lamina-ui follows its published guidelines and token values.

## Status

Early. The design-value layer, a 2D canvas and an SDL3 window exist so far:

- `lamina/tokens/motion.hpp`: the spring, easing and duration tokens of the standard and expressive motion schemes.
- `lamina/motion/spring.hpp`: a damped spring with a closed-form solution. `advance` moves a value toward its target by any time step, and the result does not depend on how the time is split into frames. `is_settled` tells when the motion has come to rest.

- `lamina/tokens/shape.hpp`, `typography.hpp`, `elevation.hpp`: the corner-radius scale, the type scale (with the emphasized weights), elevation levels and state-layer opacities.
- `lamina/color/scheme.hpp`: color schemes from one seed color. `spec_2021().build(options)` returns the 49 color roles (primary, surface-container, outline, ...) for a seed, light or dark, a variant (TonalSpot, Expressive, Vibrant, ...) and a contrast level. It is computed by Google's material-color-utilities (`third_party/`), which implements the 2021 color spec.

- `lamina/render/canvas.hpp`: a 2D canvas with paths, rounded rectangles, drop shadows and a transform stack, drawn by ThorVG (`lamina::thorvg`). Why ThorVG: [docs/renderer-comparison.md](docs/renderer-comparison.md).
- `lamina/platform/window.hpp`: an SDL3 window that shows the canvas, converts pointer input to dp, and waits for events while nothing is animating (`lamina::sdl3`).
- `lamina/text/font.hpp`, `layout.hpp`: fonts, single lines and wrapped paragraphs of text. HarfBuzz shapes the text and supplies the glyph outlines, which are filled on the canvas like any other path, so no FreeType is needed. Variable fonts follow the weight of the type scale, and characters a font lacks fall back to the next font in a `FontStack`. Paragraphs break at spaces in Latin text and between characters in Japanese, keeping punctuation such as 。 and 」 off the start of a line (`text/line_break.hpp`).
- `lamina/widgets/button.hpp`: the Material 3 Expressive button in five styles (filled, tonal, outlined, elevated, text), five sizes and two shapes. Pressing morphs the corners to the pressed shape with the effects spring, as Compose does; hover and press add the state layer and change the elevation. `build/Release/buttons.exe` shows them all.
- `lamina/widgets/text_field.hpp`: the outlined text field. The label floats onto the outline with a spring when the field is focused or filled; the caret blinks without redrawing in between (`Window::wake_after`); Japanese input goes through the system IME: the composition is drawn underlined in place and the candidate window is placed at the caret (`Window::set_text_input`). `build/Release/text_field.exe` is a small form.
- `lamina/motion/animated.hpp`, `lamina/color/mix.hpp`: a value driven by a spring, and color mixing.

The color spec is behind the `ColorSpec` interface. HCT and the tonal palettes are the same in the 2021 and 2025 specs; only the rules that pick each role's tone differ. A 2025 spec can therefore be added as another `ColorSpec` without changing the code that uses the scheme.

The token values and their sources are listed in [docs/design-tokens.md](docs/design-tokens.md).

## Spring model

A spring has unit mass, stiffness k and damping ratio ζ. The displacement y from the target follows y'' + 2ζ√k y' + k y = 0. `advance` uses the exact solution for each case: ζ < 1 (oscillates and overshoots), ζ = 1 (fastest without overshoot), ζ > 1 (slower, no overshoot). Spatial springs (position, size, shape) in the expressive scheme have ζ < 1 and overshoot; effects springs (color, opacity) have ζ = 1.

## Build

Requires Visual Studio 2022, vcpkg (`VCPKG_ROOT` set) and CMake 3.25 or later.

```
cmake --preset default
cmake --build --preset release
ctest --preset release
```

`build/Release/playground.exe [font files...]` opens a window with six cards. The fonts default to `C:/Windows/Fonts/NotoSansJP-VF.ttf`. Clicking a card raises it, rounds its corners further and changes its color, each driven by a spring of the expressive motion scheme.

## Attribution

The test fonts in `tests/fixtures/fonts/` are subsets of Noto Sans JP and Oswald, renamed, under the SIL Open Font License 1.1 (license files next to them).


Token values are taken from Google's Apache-2.0 licensed sources: androidx (Jetpack Compose Material 3 and graphics-shapes), material-components-android, material-web and material-color-utilities. See [docs/design-tokens.md](docs/design-tokens.md) for the files and commits.
