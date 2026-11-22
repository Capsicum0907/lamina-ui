# Text and fonts

[日本語](text-and-fonts.ja.md) · [README](../README.md#documentation)

Fonts, shaping and line breaking, typefaces with variable instances, icons drawn from a font, and Japanese input.

## Text

`lamina/text/font.hpp`, `layout.hpp`: fonts, single lines and wrapped paragraphs of text. HarfBuzz shapes the text and supplies the glyph outlines, which are filled on the canvas like any other path, so no FreeType is needed. Variable fonts follow the weight of the type scale and, when they have the `opsz` axis, its size as the optical size (`text::Instance`), and characters a font lacks fall back to the next font in a `FontStack`. A `FontStack` holds its fonts by `shared_ptr`, so copying it is cheap; widgets keep a copy, so the stack they were built from does not need to outlive them, and widgets that hold fonts can be assigned. Paragraphs break at spaces in Latin text and between characters in Japanese, keeping punctuation such as 。 and 」 off the start of a line (`text/line_break.hpp`). A tab character draws nothing and advances to the next tab stop; the stops are `TextStyle::tab_width` spaces apart (the width of a space in the style, tracking included), counted from the start of the line, and `tokens::text_style` sets it to `tokens::tab_width_spaces` (4). The caret, hit-testing and wrapping measure a tab where it stands, and word moves treat it as a space. Words for moving and selecting by word are runs of one class of character (Latin letters and digits, kanji, hiragana, katakana, punctuation; `text/word_boundary.hpp`). Line breaking and word classes take their Unicode blocks from one table (`text/script.hpp`). The text field's editing model (the text, the selection, the IME composition, and moving by character, word and line) is `text::EditBuffer` (`text/edit_buffer.hpp`; `EditBufferOptions` says whether it keeps line breaks and tabs), which uses neither fonts nor the canvas: lines come in as a `text::LineMap` of index ranges (`text/line_map.hpp`), and horizontal positions through the `text::CaretGeometry` interface, which the text field implements with its laid-out lines. The buffer also keeps the undo history (`text::EditHistory`, `text/edit_history.hpp`): each step holds the text, the anchor and the cursor before and after it, and undo and redo put one of them back. `type` is typing, which may merge into the step before it; `insert`, `erase_selection` and an IME commit are steps of their own; `advance(dt)` gives the history its time, and `close_step()` ends the step being typed. The limits come in `EditBufferOptions::history`; without them the buffer keeps no history. `text::underline(fonts, style)` gives where an underline goes for a style: `offset` from the baseline down to the top of the line, and its `thickness`, both from the first font's own underline metrics (`Font::underline_em()`, which HarfBuzz derives from the font's extents when the font has none). `text::ellipsize_line(fonts, utf8, style, max_width, ellipsis)` lays out one line no wider than `max_width`: the whole text when it fits, otherwise the longest start of it that ends on a glyph cluster boundary, without trailing spaces, followed by `ellipsis` (`tokens::ellipsis`, U+2026); when not even the ellipsis fits, the ellipsis alone.

## Typefaces and embedded text fonts

A `text::Typeface` names a fallback stack of fonts with a variable instance for each type role (`text::typefaces` lists them: Google Sans Flex with Noto Sans JP, Roboto Flex with Noto Sans JP, and the OS fonts). `sdl3::typeface_stack(typeface, embedded_fonts)` builds the `text::FontStack` of a typeface from the fonts a program embeds with `lamina_add_resources(... TEXT_FONTS ...)`. The details, with the axes each role pins, are in [Resources: text fonts and typefaces](resources.md#text-fonts-and-typefaces).

## System fonts

`lamina_sdl3/system_fonts.hpp`: finds a font for Japanese UI text among the system fonts. The font folders come from environment variables (`WINDIR`, `SystemRoot`, `LOCALAPPDATA`), and the candidates are tried in order: Noto Sans JP, Yu Gothic UI, Meiryo UI, BIZ UDPGothic, MS UI Gothic. On Android the folder is `fonts` under `ANDROID_ROOT` (`/system/fonts`) and the candidate is the Japanese face of Noto Sans CJK (`NotoSansCJK-Regular.ttc`, face 0, as the system's `fonts.xml` lists it for `ja`). All the lists are data in the header, one set per platform. If none is found, it throws an error that lists every path it tried. `font_stack_from_arguments` uses the font files given on the command line instead, when there are any. `system_fallback_stack` returns an empty stack instead of throwing, and `typeface_stack(typeface, embedded)` builds the stack of a typeface from the embedded fonts with the system font as the last fallback ([docs/resources.md](resources.md#text-fonts-and-typefaces)).

## Icons

`lamina/icons/`: Material Symbols as a font. A `Symbol` is a codepoint; `IconFont` draws a symbol at any size and color from its HarfBuzz outline, with the font's variation axes (fill, weight, grade, optical size; the optical size follows the drawn size unless set). `text::Font` gained `glyph_for` and a `glyph_outline_em` overload that takes axis values.

## Japanese input

Text fields take committed text and IME composition as `input::TextEvent`s (`TextEventKind::commit` and `TextEventKind::compose`) through `handle(TextEvent)`. The program tells the window where the caret is with `sdl3::Window::set_text_input(caret_area)`, so the IME candidate window opens next to it; `FocusManager::caret_area()` gives the area of the focused field. Character keys (`+`, `*` and so on) arrive as key events without starting text input, so a kana IME does not take them. Line breaking follows UAX #14 with Japanese rules, described under [Text](#text).

## Sources

- Google Fonts (Google Sans Flex, Roboto Flex, Noto Sans JP): <https://github.com/google/fonts>
- Material Symbols: <https://github.com/google/material-design-icons>
- HarfBuzz: <https://github.com/harfbuzz/harfbuzz>
- Unicode line breaking algorithm (UAX #14): <https://www.unicode.org/reports/tr14/>
- Material 3, typography: <https://m3.material.io/styles/typography/overview>
- The source files and commits of every value: [design tokens](design-tokens.md)
