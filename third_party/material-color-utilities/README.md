# material-color-utilities (C++)

[日本語](README.ja.md)

The `cpp/` directory of [material-foundation/material-color-utilities](https://github.com/material-foundation/material-color-utilities) at commit `f242efc74afc20b15753562d4afc6b6e32eb202b`, under the Apache License 2.0 (`LICENSE`). It implements the 2021 color spec.

lamina-ui's 2025 color spec (`src/lamina/color/spec_2025.cpp`) is a port of the Java `ColorSpec2025` from the same commit, written as lamina-ui code on top of this library's HCT and contrast functions.

Upstream has no build files. `CMakeLists.txt` builds it as the static library `mcu::mcu` with abseil from vcpkg, and builds the upstream tests as `mcu_tests` with GoogleTest. `shim/` maps the Google-internal test headers (`testing/base/public/gunit.h`, `gmock.h`) to GoogleTest.

## Changes from upstream

What MSVC needs to compile it, and one change in behavior: the default viewing conditions are computed in full, as Java does, instead of taken from a table rounded to 9 decimal places. With the rounded table, 9 of 733 200 role colors in a comparison with Java differed by one step in one channel ([tools/color-dump](../../tools/color-dump/README.md)). All 1045 upstream tests pass.

| File | Change |
|---|---|
| `cpp/cam/hct_solver.cc`, `cpp/utils/utils.cc` | `(Vec3){...}` → `Vec3{...}` (compound literals are a GCC/Clang extension) |
| `cpp/cam/viewing_conditions.h` | `kDefaultViewingConditions` is `DefaultWithBackgroundLstar(50.0)` instead of a literal rounded to 9 decimal places (the literal also used a compound literal) |
| `cpp/score/score.h` | `0xff4285f4` → `static_cast<int>(0xff4285f4)` (narrowing in a default member initializer) |
| `cpp/temperature/temperature_cache.h` | Add `#include <optional>` (was included only indirectly) |
