# Getting started

[日本語](getting-started.ja.md) · [README](../README.md#documentation)

How to build lamina-ui and its examples, and how to use it from another CMake project.

## Requirements and build

Requires Visual Studio 2022, vcpkg (`VCPKG_ROOT` set), CMake 3.25 or later, and for the examples and tests, which cut icon fonts, Python 3 with fontTools and brotli (`python -m pip install -r tools/icon-subset/requirements.txt`).

```
cmake --preset default
cmake --build --preset release
ctest --preset release
```

The `default` preset uses the Visual Studio 17 2022 generator, the vcpkg toolchain from `VCPKG_ROOT`, the `x64-windows-static` triplet and the static MSVC runtime, and turns `LAMINA_BUILD_TESTS` on. The build directory is `build/`. vcpkg installs HarfBuzz, SDL3, ThorVG and abseil from `vcpkg.json`, and doctest and GoogleTest for the tests.

The components gallery also builds as an Android APK with the `android-x64` and `android-arm64` presets; see [Android](android.md).

## CMake options

The CMake options `LAMINA_BUILD_THORVG` and `LAMINA_BUILD_SDL3` (both on by default) build the two backends, and `LAMINA_BUILD_EXAMPLES`, `LAMINA_BUILD_TOOLS` (both on) and `LAMINA_BUILD_TESTS` (on in the preset) build the rest. The examples need both backends and the tests need ThorVG; a combination that breaks this stops the configure step with an error. With both backends off, the core library and the tools in `tools/` still build.

`LAMINA_CLIENT_TITLE_BAR` (on by default) builds the title bar that lamina draws itself on Windows; see [Client-drawn title bar](custom-title-bar.md).

## Running the gallery

`build/Release/components.exe` shows every component on sixteen pages. The pages are chosen with the scrollable tabs under the header; the wheel over the tabs scrolls them, and the arrow keys move along them when they have the focus. `--font roboto-flex` or `--font os` starts it in another typeface, and `--title-bar` uses the client-drawn title bar. The pages are described in [Examples](examples.md#components).

The other examples are listed in [Examples](examples.md).

## Using lamina-ui from another CMake project

lamina-ui has no install rules. Add its directory with `add_subdirectory` from a project that uses the same vcpkg toolchain, and link the targets you need:

```cmake
set(LAMINA_BUILD_EXAMPLES OFF)
set(LAMINA_BUILD_TOOLS OFF)
add_subdirectory(lamina-ui)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE lamina::lamina lamina::thorvg lamina::sdl3)
```

`lamina::lamina` is the core (tokens, color, the canvas interface, text, shapes, layout, input events and widgets) and uses neither SDL3 nor ThorVG. `lamina::thorvg` is the canvas drawn with ThorVG and `lamina::sdl3` the window, the preference folder and the OS fonts. How the three fit together is in [App structure](app-structure.md#targets-and-input-events).

## Embedded icons and fonts

`cmake/LaminaResources.cmake`, `lamina/resources/resource.hpp`: resources embedded in the executable, as Android's `R` class does. `lamina_add_resources(<name> ICONS ... ICON_FONTS ... TEXT_FONTS ... FILES ...)` cuts an icon font to the listed symbols at build time, embeds it, the bundled text fonts listed (Google Sans Flex, Roboto Flex, Noto Sans JP) and any other file as byte arrays, and generates a header of typed constants (`app::res::icons::play_arrow`, `app::res::fonts::symbols`, `app::res::files::logo`), so a misspelled icon stops the build. `IconFont::from_resource` and `text::Font::from_resource` load them without the file system. The examples and the tests use it, and the built programs run with no folder beside them. See [docs/resources.md](resources.md).

The full reference, with the generated header, the loading functions and what each step needs, is [Resources](resources.md). Text fonts and the typefaces built from them are in [Text and fonts](text-and-fonts.md).
