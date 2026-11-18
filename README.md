# lamina-ui

[日本語](README.ja.md)

A C++ desktop UI library built on SDL3, modeled on Material 3 Expressive.

lamina-ui is not affiliated with or endorsed by Google. "Material Design" is Google's design system; lamina-ui follows its published guidelines and token values.

## Status

Early. Only the design-value layer exists so far, without rendering:

- `lamina/tokens/motion.hpp`: the spring, easing and duration tokens of the standard and expressive motion schemes.
- `lamina/motion/spring.hpp`: a damped spring with a closed-form solution. `advance` moves a value toward its target by any time step, and the result does not depend on how the time is split into frames. `is_settled` tells when the motion has come to rest.

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

## Attribution

Token values are taken from Google's Apache-2.0 licensed sources: androidx (Jetpack Compose Material 3 and graphics-shapes), material-components-android, material-web and material-color-utilities. See [docs/design-tokens.md](docs/design-tokens.md) for the files and commits.
