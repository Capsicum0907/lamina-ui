# Resources

[日本語](resources.ja.md)

A program names the icons, text fonts and files it uses in its CMake file. The build cuts the Material Symbols font down to those icons, embeds the fonts and files in the program as byte arrays, and writes a header with a typed constant for each, so the program runs as one executable with no folder beside it. This follows Android's `R` class: a misspelled icon name stops the build, in CMake when the font has no icon of that name, and in the compiler when the header has no constant of that name.

## CMake

```cmake
lamina_add_resources(<name>
    [NAMESPACE <namespace>]
    [HEADER <path>]
    [ICONS <symbol>...]
    [ICON_FONTS <id>[:<axis>=<value>[,<axis>=<value>...]]...]
    [TEXT_FONTS <font>...]
    [FILES <id>=<path>...])
```

The function is defined by `cmake/LaminaResources.cmake`, which lamina-ui's `CMakeLists.txt` includes, so a project that adds lamina-ui with `add_subdirectory` can call it. It makes the static library `<name>` (an interface library when there are neither icons nor files); linking it gives the header.

- `NAMESPACE`: the C++ namespace of the constants. The default is `<name>`.
- `HEADER`: the include path of the generated header. The default is `<name>.hpp`.
- `ICONS`: Material Symbols names as the codepoints file spells them (`play_arrow`, `delete`). lamina's own icons, the global property `LAMINA_BUILTIN_ICONS` (the 15 icons its widgets draw: `close`, `check`, `edit`, `date_range`, `arrow_drop_down`, `arrow_drop_up`, `keyboard_arrow_left`, `keyboard_arrow_right`, `calendar_today`, `keyboard`, `schedule`, `more_vert`, `keyboard_arrow_down`, `arrow_upward` and `arrow_downward`, set in lamina-ui's `CMakeLists.txt`), are added after them.
- `ICON_FONTS`: the icon fonts cut with these icons. The default is one font, `symbols`. A font may pin variation axes, which removes them from it: `filled:FILL=1` is the font with the fill fixed at 1, for widgets that draw an icon with the default style. A font without pinned axes keeps fill, weight, grade and optical size, which `IconStyle` sets when drawing.
- `TEXT_FONTS`: the bundled text fonts to embed, unchanged: `google_sans_flex`, `roboto_flex`, `noto_sans_jp` ([Text fonts and typefaces](#text-fonts-and-typefaces)). Each font adds about its own size to the executable, so a program lists only the ones it uses. Another name stops the configure step with `no text font named '<name>'`.
- `FILES`: other files embedded as they are (images, text fonts, data). A relative path is relative to the calling `CMakeLists.txt`.

The header declares, for `NAMESPACE app::res`:

| Name | Type | What |
|---|---|---|
| `app::res::icons::<symbol>` | `lamina::icons::Symbol` | one per icon, holding its codepoint |
| `app::res::all_icons` | `std::array<lamina::icons::Symbol, N>` | every icon in the listed order, lamina's own after them |
| `app::res::fonts::<id>` | `lamina::resources::Resource` | each icon font |
| `app::res::text_fonts::<font>` | `lamina::resources::Resource` | each text font |
| `app::res::embedded_fonts` | `std::array<lamina::text::EmbeddedFont, N>` | each text font with its `lamina::text::BundledFont`, for `typeface_stack` |
| `app::res::files::<id>` | `lamina::resources::Resource` | each file |

A symbol name that is a C++ keyword gets a trailing `_` (`delete_`, `class_`), and one that starts with a digit gets a leading `_` (`_10k`). The ids of fonts and files must be C++ identifiers.

## Loading

- `lamina::resources::Resource` (`lamina/resources/resource.hpp`) has the id as `name`, the bytes as `data` (`std::span<const unsigned char>`), and `bytes()` (`std::span<const std::byte>`). The bytes of a generated resource live as long as the program.
- `lamina::icons::IconFont::from_resource(resource)` makes an icon font. It throws `std::runtime_error` when the bytes are not a font, and `std::invalid_argument` when the font lacks one of lamina's own icons (`lamina::builtin::all_icons`), that is, when it was not cut by `lamina_add_resources`.
- `lamina::text::Font::from_resource(resource, face_index)` makes a text font from an embedded file. HarfBuzz reads the embedded bytes without copying them, so a resource made by hand must outlive its fonts. `Font::from_file` still loads a font file; the system fonts and the font arguments of the examples use it.
- `lamina::builtin` (`lamina/builtin_resources.hpp`, generated at configure time from `LAMINA_BUILTIN_ICONS`) holds lamina's own icons: `builtin::icons::close` is the title bar's close icon.

```cmake
lamina_add_resources(player_resources NAMESPACE player::res HEADER player/res.hpp
    ICONS play_arrow pause skip_next
    ICON_FONTS outlined filled:FILL=1
    FILES logo=art/logo.png)
target_link_libraries(player PRIVATE player_resources)
```

```cpp
#include "player/res.hpp"

const lamina::icons::IconFont icons = lamina::icons::IconFont::from_resource(player::res::fonts::filled);
icons.draw(canvas, player::res::icons::play_arrow, {0.0, 0.0}, 24.0, color);
const std::span<const std::byte> logo = player::res::files::logo.bytes();
```

## Text fonts and typefaces

lamina-ui bundles three variable fonts in `third_party/`, unchanged from google/fonts (their READMEs give the commit and the licenses):

| `TEXT_FONTS` | Font | Axes lamina-ui sets | Size added to an executable |
|---|---|---|---|
| `google_sans_flex` | Google Sans Flex | `wght`, `opsz` (1 to 144) | 3 910 784 bytes (3.7 MiB) |
| `roboto_flex` | Roboto Flex | `wght`, `opsz` (8 to 144) | 1 787 292 bytes (1.7 MiB) |
| `noto_sans_jp` | Noto Sans JP | `wght` | 9 589 900 bytes (9.1 MiB) |

"Google Sans Flex" is a trademark of Google LLC; lamina-ui is not affiliated with or endorsed by Google, and it embeds the font only unchanged, as `third_party/google-sans-flex/TRADEMARKS.md` asks.

A typeface (`lamina::text::Typeface`, `lamina/text/typeface.hpp`) is the one place a program chooses the fonts of all its text. The table `text::typefaces` lists them:

| `Typeface` | Fonts, in fallback order | Key |
|---|---|---|
| `google_sans_flex` (`default_typeface`) | Google Sans Flex, Noto Sans JP, the system font | `google-sans-flex` |
| `roboto_flex` | Roboto Flex, Noto Sans JP, the system font | `roboto-flex` |
| `system` | the system font only, as before bundled fonts existed | `os` |

Each character takes the first font in the stack that has it (`FontStack::font_for`): Latin text the Latin font, kana and kanji Noto Sans JP (neither Latin font has them), and anything else the system font (`lamina_sdl3/system_fonts.hpp`). Of the three, the default is the closest to Material's own look.

- `text::typeface_stack(typeface, embedded, system_fonts)` builds the `FontStack` from the embedded fonts (`app::res::embedded_fonts`) and a stack of system fonts, which a bundled typeface appends as its last fallback and may be empty. It throws `std::invalid_argument` when a font of the typeface is not embedded, naming it, rather than drawing with another font; `typeface_available(typeface, embedded)` says beforehand.
- `sdl3::typeface_stack(typeface, embedded)` does the same with the system font found by `system_fallback_stack()`. For `Typeface::system` it is exactly `system_font_stack()`, which throws when there is no system font; a bundled typeface works without one.
- `text::typeface_from_key` reads a key, as the components example's `--font <key>` does.

The type scale reaches the axes without any setting. `wght` is the role's weight, so the Expressive emphasized roles are heavier by weight alone (500, or 700 for Title M, Title S and the labels). `opsz` is the text size in dp, rounded to a whole number, in a font that has the axis, as CSS's `font-optical-sizing: auto` does: Display L draws at `opsz` 57 and Label S at 11. Fonts without an axis, such as the system fonts, are drawn as before. `Font::with_variations({{text::axes::roundness, 100.0f}})` returns the same font with other axes pinned (`ROND`, `slnt`, `wdth`, `GRAD` and the like); a pinned `wght` or `opsz` replaces the one from the style. `Font::variations(instance)` returns the axis values a style draws with.

```cmake
lamina_add_resources(app_resources NAMESPACE app::res HEADER app/res.hpp
    TEXT_FONTS google_sans_flex noto_sans_jp)
```

```cpp
const lamina::text::FontStack fonts = lamina::sdl3::typeface_stack(lamina::text::Typeface::google_sans_flex, app::res::embedded_fonts);
```

## When it runs and what it needs

- At configure time, the icon names are looked up in `LAMINA_SYMBOLS_CODEPOINTS` and the header is written (rewritten only when its text changes). An unknown name stops the configure step with `no icon named '<name>'`.
- At build time, `tools/icon-subset/subset.py` cuts the icon fonts from `LAMINA_SYMBOLS_FONT`, and `cmake/lamina_embed.cmake` writes each font and file as a C++ array. Each step runs again only when its inputs change: the icon list and font list, the source font, the script, or the embedded file. Text fonts are embedded as they are, without Python; Noto Sans JP takes about 12 seconds to write as a 48 MB source file and about 7 seconds to compile. Nothing is downloaded during the build.
- Cutting the icon fonts needs Python 3 with fontTools and brotli (brotli reads the WOFF2 source): `python -m pip install -r tools/icon-subset/requirements.txt`. CMake looks for Python when a target lists icons (on Windows the interpreter on `PATH` before the registry; `Python3_EXECUTABLE` picks one) and stops with that command if it cannot import them. A target with only `FILES` needs no Python.
- `LAMINA_SYMBOLS_FONT` and `LAMINA_SYMBOLS_CODEPOINTS` are cache variables. They point at the vendored Material Symbols Rounded in `third_party/material-symbols/` and can point at another Material Symbols style with its codepoints file.
- Each cut takes about 2 seconds, most of it reading the 5.4 MB source. An embedded font adds about its own size to the executable: cadenza's 29 icons are 50.9 KB, and 45.9 KB with the fill pinned.
- The arrays are written as source code because MSVC (19.44, Visual Studio 2022) does not have C++ `#embed`.

## Tests

`tests/resources_test.cpp` checks the constants against the codepoints file (keywords and leading digits included), the order of `all_icons`, that every icon is in every cut, that the pinned cut draws as the variable font at that axis value (within 8 of 255 in any channel, the rounding of pinning), the embedded bytes of a file, and loading a text font from them. `tests/typeface_test.cpp` checks the typeface table, the fallback order of each typeface, that the OS fonts typeface is exactly the system stack, that a missing font is reported, that `TEXT_FONTS` embeds Roboto Flex byte for byte and lists it in `embedded_fonts`, and that every role of the type scale draws at its weight and size. Three more CTest tests check the build errors: `lamina_resources_unknown_icon` runs the lookup on a misspelled name, `lamina_resources_unknown_text_font` on a misspelled text font, and `lamina_resources_misspelled_constant` builds `tests/resources/misspelled.cpp`, which uses a constant that does not exist, and expects the compiler's error.
