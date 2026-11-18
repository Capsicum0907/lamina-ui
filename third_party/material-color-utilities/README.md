# material-color-utilities (C++)

[日本語](README.ja.md)

The `cpp/` directory of [material-foundation/material-color-utilities](https://github.com/material-foundation/material-color-utilities) at commit `f242efc74afc20b15753562d4afc6b6e32eb202b`, under the Apache License 2.0 (`LICENSE`). It implements the 2021 color spec.

Upstream has no build files. `CMakeLists.txt` builds it as the static library `mcu::mcu` with abseil from vcpkg, and builds the upstream tests as `mcu_tests` with GoogleTest. `shim/` maps the Google-internal test headers (`testing/base/public/gunit.h`, `gmock.h`) to GoogleTest.

## Changes from upstream

Only what MSVC needs to compile it. The behavior is unchanged, and all 1045 upstream tests pass.

| File | Change |
|---|---|
| `cpp/cam/hct_solver.cc`, `cpp/utils/utils.cc` | `(Vec3){...}` → `Vec3{...}` (compound literals are a GCC/Clang extension) |
| `cpp/cam/viewing_conditions.h` | `(ViewingConditions){...}` → `ViewingConditions{...}` (same) |
| `cpp/score/score.h` | `0xff4285f4` → `static_cast<int>(0xff4285f4)` (narrowing in a default member initializer) |
| `cpp/temperature/temperature_cache.h` | Add `#include <optional>` (was included only indirectly) |
