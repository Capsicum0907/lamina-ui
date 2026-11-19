# Client-drawn title bar (experiment)

[日本語](custom-title-bar.ja.md)

This branch (`experiment/custom-title-bar`) lets a lamina-ui app draw its own window title bar on Windows while keeping the native window behaviour. Material 3 has no desktop title bar; the nearest Google design is Android desktop windowing, where the app draws under a caption bar and the caption buttons sit on top. Here the whole bar, buttons included, is drawn by lamina.

## Trying it

```
components --title-bar
navigation --title-bar
```

Without the flag the examples keep the system frame. The components example also puts an icon button inside the bar to show app content in the caption area.

## Layers

- `lamina/input/caption.hpp` (core, backend-independent): `CaptionRegions` holds the bar's drag area, the three caption button rects and the rects of app content inside the bar, all in dp. `hit_test` answers which of them a point is in; buttons and app content win over the drag area.
- `lamina/widgets/title_bar.hpp`: `TitleBar` lays out the title, an optional content node and minimize, maximize/restore and close buttons, paints them from the color scheme and reports `regions(interactive)`. `handle` returns a `TitleBarAction`; the app decides what to do with it. Sizes, colors and symbols are in `lamina/tokens/title_bar.hpp`.
- `lamina_sdl3`: `WindowOptions::client_title_bar` asks for the client-drawn bar. `Window::client_title_bar()` says whether it is on; `set_caption_regions` takes the regions each frame; `minimize`, `toggle_maximize` and `close` carry out the actions. `Frame` gained `maximized` and `active`.
- `lamina_sdl3/caption_frame_win32.cpp` holds all Win32 code. On other platforms `caption_frame_none.cpp` reports no support and the window keeps its system frame.

## What Windows does and what lamina does

SDL 3.4 already gives a borderless resizable window the caption, system menu, thick frame and maximize box styles, sizes the client area to the whole window and to the work area when maximized, and repaints during a live resize. The branch adds a window subclass that:

- answers `WM_NCHITTEST`: resize edges (from the system frame metrics for the window's DPI, not while maximized), `HTMAXBUTTON` over the maximize button so Windows 11 shows Snap Layouts, `HTCAPTION` over the drag area, `HTCLIENT` elsewhere. Dragging, double-click to maximize, Aero snap and the shadow then come from Windows;
- turns non-client mouse messages over the maximize button into lamina pointer events, so it hovers and clicks like the other buttons;
- opens the system menu on a right click in the drag area, and lets `Alt+Space` through to it (SDL drops the menu key command otherwise);
- asks DWM for rounded corners.

## Known limits

- The resize band lies inside the client area (about 8 px at 100 %), so content at the very edge of the window cannot be clicked there.
- Only Windows 11 was tried. Windows 10 has no rounded corners or Snap Layouts.
- The caption buttons do not take keyboard focus.
