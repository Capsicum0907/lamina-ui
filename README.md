# lamina-ui

[日本語](README.ja.md)

A C++ desktop UI library built on SDL3, modeled on Material 3 Expressive.

lamina-ui is not affiliated with or endorsed by Google. "Material Design" is Google's design system; lamina-ui follows its published guidelines and token values.

## Status

Early. So far there are the design-value layer, a 2D canvas, text, shapes, icons, an SDL3 window, a layout system, and these components: button, icon button, FAB, switch, checkbox, radio button and text field. Some older examples still place components by hand.

- `lamina/tokens/motion.hpp`: the spring, easing and duration tokens of the standard and expressive motion schemes.
- `lamina/motion/spring.hpp`: a damped spring with a closed-form solution. `advance` moves a value toward its target by any time step, and the result does not depend on how the time is split into frames. `is_settled` tells when the motion has come to rest.

- `lamina/tokens/shape.hpp`, `typography.hpp`, `elevation.hpp`: the corner-radius scale, the type scale (with the emphasized weights), elevation levels and state-layer opacities.
- `lamina/color/scheme.hpp`: color schemes from one seed color. `spec_2021().build(options)` returns the 49 color roles (primary, surface-container, outline, ...) for a seed, light or dark, a variant (TonalSpot, Expressive, Vibrant, ...) and a contrast level. It is computed by Google's material-color-utilities (`third_party/`), which implements the 2021 color spec.

- `lamina/render/canvas.hpp`: a 2D canvas with paths, rounded rectangles, drop shadows and a transform stack, drawn by ThorVG (`lamina::thorvg`). Why ThorVG: [docs/renderer-comparison.md](docs/renderer-comparison.md).
- `lamina/platform/window.hpp`: an SDL3 window that shows the canvas, converts pointer input to dp, and waits for events while nothing is animating (`lamina::sdl3`).
- `lamina/text/font.hpp`, `layout.hpp`: fonts, single lines and wrapped paragraphs of text. HarfBuzz shapes the text and supplies the glyph outlines, which are filled on the canvas like any other path, so no FreeType is needed. Variable fonts follow the weight of the type scale, and characters a font lacks fall back to the next font in a `FontStack`. Paragraphs break at spaces in Latin text and between characters in Japanese, keeping punctuation such as 。 and 」 off the start of a line (`text/line_break.hpp`).
- `lamina/widgets/button.hpp`: the Material 3 Expressive button in five styles (filled, tonal, outlined, elevated, text), five sizes and two shapes. Pressing morphs the corners to the pressed shape with the effects spring, as Compose does; hover and press add the state layer and change the elevation. `ButtonOptions::leading_icon` puts an icon before the label, sized and spaced by the size tokens. `build/Release/buttons.exe` shows them all.
- `lamina/icons/`: Material Symbols as a font. `symbols.hpp` names 44 symbols (`Symbol::add`, `Symbol::delete_`, ...); `IconFont` loads the subset font in `assets/icons/` and draws a symbol at any size and color from its HarfBuzz outline, with the font's variation axes (fill, weight, grade, optical size; the optical size follows the drawn size unless set). `text::Font` gained `glyph_for` and a `glyph_outline_em` overload that takes axis values. `tools/icon-subset/subset.py` regenerates the font and `symbols.hpp` from one list of names.
- `lamina/widgets/icon_button.hpp`: icon buttons in four styles (filled, tonal, outlined, standard), five sizes, two shapes and three widths (narrow, uniform, wide). Like the button, pressing morphs the corners with the effects spring; a toggleable one also changes shape and colors when checked.
- `lamina/widgets/fab.hpp`: the floating action button in three sizes (FAB, medium, large) on the primary or secondary container, raised to level 3 and to level 4 on hover.
- `lamina/widgets/switch.hpp`: the switch. The thumb slides and grows (16 dp off, 24 dp on or with an icon, 28 dp while pressed) on the fast spatial spring, so it overshoots a little in the expressive scheme; the thumb can carry an icon for each state.
- `lamina/widgets/checkbox.hpp`: the checkbox with off, on and indeterminate states. The check mark is drawn in along its length on the spatial spring and leans into the indeterminate dash; when unchecked it vanishes after 100 ms while the box fades, as in Compose.
- `lamina/widgets/radio_button.hpp`: the radio button. The dot grows on the fast spatial spring.
- `build/Release/components.exe` shows the icons, buttons with icons, icon buttons, FABs, switches, checkboxes and radio buttons, with the theme chips of the buttons example.
- `lamina/widgets/text_field.hpp`: the outlined text field. The label floats onto the outline with a spring when the field is focused or filled; the caret blinks without redrawing in between (`Window::wake_after`); Japanese input goes through the system IME: the composition is drawn underlined in place and the candidate window is placed at the caret (`Window::set_text_input`). Shift with the arrow keys, Home or End extends a selection; dragging selects a range, a double click selects a word (in Japanese, a run of kanji, hiragana or katakana), and a triple click or Ctrl+A selects everything. Typing, an IME commit, Backspace and Delete replace the selection. Ctrl+Left/Right move by word and Ctrl+Backspace/Delete delete by word. Ctrl+C, Ctrl+X and Ctrl+V use the clipboard given in `TextFieldOptions` (`Window::clipboard()` for the system clipboard, any `platform::Clipboard` in tests). A single-line field scrolls sideways to keep the caret in view and clips its text to the field (`Canvas::push_clip`); Enter submits. With `TextFieldOptions::multiline` the text wraps to the field's width and the field grows by one line at a time from 56 dp (16 dp above and below the text). Enter inserts a newline and Ctrl+Enter submits; Up and Down move between lines keeping the caret's horizontal position; Home and End go to the start and end of the line, and with Ctrl to the start and end of the text. `build/Release/text_field.exe` is a small form whose message field is multi-line.
- `lamina/shapes/`: a C++ port of androidx `graphics-shapes` (rounded polygons with per-corner rounding and smoothing, and `Morph`, which pairs the corners of two outlines and interpolates their cubic curves) and the 35 Material 3 Expressive shapes (`material_shape`). Attribution: [third_party/NOTICE-androidx-graphics-shapes.md](third_party/NOTICE-androidx-graphics-shapes.md). `build/Release/shapes_gallery.exe` shows every shape; clicking one morphs the large shape into it with the expressive spatial spring.
- `lamina/layout/layout.hpp`: rows and columns in the manner of Compose. A node is measured under constraints, then placed. Rows and columns take spacing, padding, main-axis alignment (start, center, end, space between/around/evenly), cross-axis alignment (start, center, end, stretch) and weights that share the leftover space. A stretched row is as tall as its tallest child unless `fill_cross` asks for all the space. `lamina/widgets/nodes.hpp` turns a button, text field or `Label` (`widgets/label.hpp`, wrapping text) into a node. `build/Release/layout_demo.exe` reflows as the window is resized.
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

`assets/icons/MaterialSymbolsRounded-subset.ttf` is a modified version of Material Symbols Rounded (variable font) by Google, from [google/material-design-icons](https://github.com/google/material-design-icons) at commit `10ecc14790612ca8fd164e3f4ffe9c16fc50d5b7`, under the Apache License 2.0 (`assets/icons/LICENSE`). It was cut down to the 44 symbols listed in `src/lamina/icons/symbols.hpp`, without the ligature table and hinting, by `tools/icon-subset/subset.py`; the variation axes and the name table are kept.


Token values are taken from Google's Apache-2.0 licensed sources: androidx (Jetpack Compose Material 3 and graphics-shapes), material-components-android, material-web and material-color-utilities. See [docs/design-tokens.md](docs/design-tokens.md) for the files and commits.
