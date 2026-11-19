# Client-drawn title bar

[日本語](custom-title-bar.ja.md)

A lamina-ui app can draw its own window title bar on Windows while keeping the native window behaviour. It is opt-in: a window keeps the system frame unless it sets `WindowOptions::client_title_bar`. Material 3 has no desktop title bar; the nearest Google design is Android desktop windowing, where the app draws under a caption bar and the caption buttons sit on top. Here the whole bar, buttons included, is drawn by lamina.

## Trying it

```
components --title-bar
navigation --title-bar
calculator --title-bar
timer --title-bar
todo --title-bar
```

Without the flag the examples keep the system frame. The components example also puts an icon button inside the bar to show app content in the caption area. The calculator and the timer paint the bar in their own background color (TitleBarOptions::container), so each window reads as one surface. The to-do app does the same. In the to-do and navigation examples a dialog opens below the bar (`Dialog::open_in`), so the bar is never dimmed. The bar is window chrome, so a modal dialog does not block it, just as it does not block the system frame: the examples give each pointer event to the bar first, and a press on the bar goes nowhere else, so it neither closes the dialog nor reaches the content below.

## Layers

- `lamina/input/caption.hpp` (core, backend-independent): `CaptionRegions` holds the bar's drag area, the three caption button rects and the rects of app content inside the bar, all in dp. `hit_test` answers which of them a point is in; buttons and app content win over the drag area.
- `lamina/widgets/title_bar.hpp`: `TitleBar` lays out the title, an optional content node and minimize, maximize/restore and close buttons, paints them from the color scheme and reports `regions(interactive)`. `handle` returns a `TitleBarAction`; the app decides what to do with it. Sizes, colors and symbols are in `lamina/tokens/title_bar.hpp`. The close button draws the Material Symbols `close` glyph at `icon_size`. Minimize, maximize and restore are paths in a `glyph_box` square drawn with a `glyph_stroke` line, so their ink is as large as the close glyph's (8 × 8 dp at half coverage); the Material Symbols glyphs for them fill different shares of their grid and looked larger.
- `lamina_sdl3`: `WindowOptions::client_title_bar` asks for the client-drawn bar. `Window::client_title_bar()` says whether it is on; `set_caption_regions` takes the regions each frame; `minimize`, `toggle_maximize` and `close` carry out the actions. `Frame` gained `maximized` and `active`.
- `lamina_sdl3/caption_frame_win32.cpp` holds all Win32 code. CMake compiles it only on Windows with the `LAMINA_CLIENT_TITLE_BAR` option on (the default); otherwise it compiles `caption_frame_none.cpp`, which reports no support. Asking for `client_title_bar` then gives the system frame: the window prints one line to stderr, `client_title_bar()` returns false and `set_caption_regions` does nothing. `sdl3::client_title_bar_supported()` answers before a window exists, so the examples do not add the bar's height to a window that will not have the bar. Building on Windows with `-DLAMINA_CLIENT_TITLE_BAR=OFF` gives the same fallback.

## What Windows does and what lamina does

SDL 3.4 already gives a borderless resizable window the caption, system menu, thick frame and maximize box styles, sizes the client area to the whole window and to the work area when maximized, and repaints during a live resize. lamina adds a window subclass that:

- answers `WM_NCHITTEST`: resize edges (from the system frame metrics for the window's DPI, not while maximized), `HTMAXBUTTON` over the maximize button so Windows 11 shows Snap Layouts, `HTCAPTION` over the drag area, `HTCLIENT` elsewhere. Dragging, double-click to maximize, Aero snap and the shadow then come from Windows;
- turns non-client mouse messages over the maximize button into lamina pointer events, so it hovers and clicks like the other buttons;
- opens the system menu on a right click in the drag area, and lets `Alt+Space` through to it (SDL drops the menu key command otherwise);
- asks DWM for rounded corners.

## Known limits

- The resize band lies inside the client area (about 8 px at 100 %), so content at the very edge of the window cannot be clicked there.
- Tried on Windows 11 with two 1920 × 1080 monitors at 100 %, the second to the right of the primary, with the calculator and the to-do app: dragging the bar to the second monitor and back, maximizing there (the client area equals that monitor's work area), restoring with a double-click, snapping to its right edge, and the Snap Layouts flyout there.
- Not tried: moving between monitors with different DPI, Windows 10 (no rounded corners or Snap Layouts there), and a build on another platform. The fallback was checked on Windows with `LAMINA_CLIENT_TITLE_BAR=OFF`, which compiles the same `caption_frame_none.cpp`.
- `Alt+Space` was checked with a posted key message, because another app held that shortcut on the test machine. After a menu opened from the keyboard is closed with one Escape, the next click only leaves menu mode.
- The caption buttons do not take keyboard focus.
- The behaviour leans on SDL 3.4 details (the borderless window styles, the maximized client area, dropping the menu key command); an SDL update can change them.
