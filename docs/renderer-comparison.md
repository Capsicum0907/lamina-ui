# Renderer comparison: Blend2D and ThorVG

[日本語](renderer-comparison.ja.md)

Both renderers implement the same `Canvas` interface (`lamina_blend2d`, `lamina_thorvg`). `tools/render-compare` draws the same scene on both, times it, writes PNGs and 8× zoomed crops, and compares the pixels.

Measured on 2026-11-19 with Blend2D (vcpkg 2025-03-08) and ThorVG 1.0.5, CPU engine only, one thread each, Release build.

## Scene

1600 × 1000 px at scale 1.25: 32 rounded rectangles (every corner size, a third of them with a shadow), 24 translucent overlapping circles, 12 star shapes made of cubic curves, and 40 hairlines 0.8 dp thick at fractional positions.

## Results

| | Blend2D | ThorVG |
|---|---|---|
| Fills only | 0.82 ms/frame | 1.77 ms/frame |
| With shadows | 14.5 ms/frame | 7.2 ms/frame |

Pixel difference between the two images, with shadows: mean 0.13 per channel, max 24, 0.05 % of pixels differ by more than 8. In the zoomed crops (corners, star curves, hairlines) the two cannot be told apart.

## Decision

ThorVG (2026-11-19). The quality is the same and the speed difference does not matter at these frame times, while ThorVG has blur and shadow built in and offers GPU engines behind the same API. The Blend2D backend and the comparison tool were removed after this measurement; they remain in the git history (commit `2583aaf`).

## Notes

- Blend2D has no blur. Its shadow is lamina-ui's own blur (three box blurs, `render/box_blur.cpp`), which is not optimized. The shadow time measures that code, not Blend2D.
- ThorVG has blur and drop shadow built in (`SceneEffect`). Its drop-shadow angle is measured clockwise from up, and the offset is rounded to whole pixels, so a shadow animated by a spring would move in 1 px steps. lamina-ui therefore does not use the drop-shadow effect: it draws the shadow shape itself at the (fractional) offset and blurs it with the Gaussian blur effect (`Canvas::fill_shadow`).
- ThorVG also has OpenGL and WebGPU engines with the same API. The vcpkg port builds only the CPU engine.
- Text was not compared. Either renderer needs HarfBuzz for shaping Japanese text.
