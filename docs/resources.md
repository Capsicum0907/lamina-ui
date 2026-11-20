# Resources

[日本語](resources.ja.md)

A program names the icons and files it uses in its CMake file. The build cuts the Material Symbols font down to those icons, embeds the fonts and files in the program as byte arrays, and writes a header with a typed constant for each, so the program runs as one executable with no folder beside it. This follows Android's `R` class: a misspelled icon name stops the build, in CMake when the font has no icon of that name, and in the compiler when the header has no constant of that name.

## CMake

```cmake
lamina_add_resources(<name>
    [NAMESPACE <namespace>]
    [HEADER <path>]
    [ICONS <symbol>...]
    [ICON_FONTS <id>[:<axis>=<value>[,<axis>=<value>...]]...]
    [FILES <id>=<path>...])
```

The function is defined by `cmake/LaminaResources.cmake`, which lamina-ui's `CMakeLists.txt` includes, so a project that adds lamina-ui with `add_subdirectory` can call it. It makes the static library `<name>` (an interface library when there are neither icons nor files); linking it gives the header.

- `NAMESPACE`: the C++ namespace of the constants. The default is `<name>`.
- `HEADER`: the include path of the generated header. The default is `<name>.hpp`.
- `ICONS`: Material Symbols names as the codepoints file spells them (`play_arrow`, `delete`). lamina's own icons, the global property `LAMINA_BUILTIN_ICONS` (today only `close`, for the title bar), are added after them.
- `ICON_FONTS`: the icon fonts cut with these icons. The default is one font, `symbols`. A font may pin variation axes, which removes them from it: `filled:FILL=1` is the font with the fill fixed at 1, for widgets that draw an icon with the default style. A font without pinned axes keeps fill, weight, grade and optical size, which `IconStyle` sets when drawing.
- `FILES`: other files embedded as they are (images, text fonts, data). A relative path is relative to the calling `CMakeLists.txt`.

The header declares, for `NAMESPACE app::res`:

| Name | Type | What |
|---|---|---|
| `app::res::icons::<symbol>` | `lamina::icons::Symbol` | one per icon, holding its codepoint |
| `app::res::all_icons` | `std::array<lamina::icons::Symbol, N>` | every icon in the listed order, lamina's own after them |
| `app::res::fonts::<id>` | `lamina::resources::Resource` | each icon font |
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

## When it runs and what it needs

- At configure time, the icon names are looked up in `LAMINA_SYMBOLS_CODEPOINTS` and the header is written (rewritten only when its text changes). An unknown name stops the configure step with `no icon named '<name>'`.
- At build time, `tools/icon-subset/subset.py` cuts the icon fonts from `LAMINA_SYMBOLS_FONT`, and `cmake/lamina_embed.cmake` writes each font and file as a C++ array. Each step runs again only when its inputs change: the icon list and font list, the source font, the script, or the embedded file. Nothing is downloaded during the build.
- Cutting the icon fonts needs Python 3 with fontTools and brotli (brotli reads the WOFF2 source): `python -m pip install -r tools/icon-subset/requirements.txt`. CMake looks for Python when a target lists icons (on Windows the interpreter on `PATH` before the registry; `Python3_EXECUTABLE` picks one) and stops with that command if it cannot import them. A target with only `FILES` needs no Python.
- `LAMINA_SYMBOLS_FONT` and `LAMINA_SYMBOLS_CODEPOINTS` are cache variables. They point at the vendored Material Symbols Rounded in `third_party/material-symbols/` and can point at another Material Symbols style with its codepoints file.
- Each cut takes about 2 seconds, most of it reading the 5.4 MB source. An embedded font adds about its own size to the executable: cadenza's 29 icons are 50.9 KB, and 45.9 KB with the fill pinned.
- The arrays are written as source code because MSVC (19.44, Visual Studio 2022) does not have C++ `#embed`.

## Tests

`tests/resources_test.cpp` checks the constants against the codepoints file (keywords and leading digits included), the order of `all_icons`, that every icon is in every cut, that the pinned cut draws as the variable font at that axis value (within 8 of 255 in any channel, the rounding of pinning), the embedded bytes of a file, and loading a text font from them. Two more CTest tests check the two build errors: `lamina_resources_unknown_icon` runs the lookup on a misspelled name, and `lamina_resources_misspelled_constant` builds `tests/resources/misspelled.cpp`, which uses a constant that does not exist, and expects the compiler's error.
