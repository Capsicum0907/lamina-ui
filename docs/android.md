# Android

[日本語](android.ja.md) · [README](../README.md#documentation)

The components gallery builds as an Android APK. The build uses CMake, the NDK and the command-line tools of the Android SDK; there is no Gradle project. It has been run on the API 37 x86_64 emulator, and the arm64-v8a APK on a phone.

## Requirements

- Android SDK parts: NDK 28.2.13676358 (r28c), the platform `android-36.1`, build-tools 36.1.0 (aapt2, d8, zipalign, apksigner), and platform-tools (adb). To run on an emulator, also the emulator and a system image.
- A JDK 17 or later with `javac` and `jar`. Android Studio's bundled `jbr` works.
- vcpkg (`VCPKG_ROOT`) with its `x64-android` and `arm64-android` triplets. The manifest is the same as on Windows; vcpkg builds HarfBuzz, SDL3, ThorVG and abseil for Android from source.
- The SDL source archive of the version vcpkg builds, `$VCPKG_ROOT/downloads/libsdl-org-SDL-release-3.4.10.tar.gz`. vcpkg leaves it there when it builds `sdl3`. The APK takes SDL's Java sources from it; configure stops if their version is not the linked SDL's.
- CMake 4.3 or later and Ninja on `PATH`. The APK step writes its zip entries uncompressed with `file(ARCHIVE_CREATE ... COMPRESSION None)`, which CMake 4.1 rejects for zip; configure stops on an older CMake. vcpkg's own CMake (`downloads/tools/cmake-4.3.2-windows`) is new enough, and the SDK's `cmake/4.1.2/bin` has a Ninja. The Visual Studio generator cannot target Android.
- Python 3 with fontTools and brotli on the host, as for the desktop build, for the icon fonts.
- A debug keystore. By default it is `debug.keystore` in `ANDROID_USER_HOME`, or in `.android` under the home folder (Android Studio creates it).

## Environment

The `android-x64` and `android-arm64` presets read these variables; nothing in the CMake files names a tool path.

| Variable | What |
|---|---|
| `ANDROID_HOME` | the Android SDK |
| `ANDROID_NDK_HOME` | the NDK, e.g. `$ANDROID_HOME/ndk/28.2.13676358`; vcpkg's Android toolchain reads it too |
| `JAVA_HOME` | the JDK; `d8` and `apksigner` also run with it |
| `VCPKG_ROOT` | vcpkg |
| `LAMINA_HOST_PYTHON` | the host `python.exe` with fontTools and brotli; under the Android toolchain CMake does not look for host programs by itself |

`VCPKG_DISABLE_METRICS=1` turns off vcpkg's telemetry, and `X_VCPKG_ASSET_SOURCES=clear;x-block-origin` makes vcpkg fail instead of downloading, which checks that everything it needs is already in `downloads`.

## Build

PowerShell, with the SDK in the default place of Android Studio:

```
$env:ANDROID_HOME = "$env:LOCALAPPDATA\Android\Sdk"
$env:ANDROID_NDK_HOME = "$env:ANDROID_HOME\ndk\28.2.13676358"
$env:JAVA_HOME = "$env:ProgramFiles\Android\Android Studio\jbr"
$env:LAMINA_HOST_PYTHON = (Get-Command python).Source
$env:PATH = "$env:VCPKG_ROOT\downloads\tools\cmake-4.3.2-windows\cmake-4.3.2-windows-x86_64\bin;$env:ANDROID_HOME\cmake\4.1.2\bin;$env:PATH"
cmake --preset android-x64
cmake --build --preset android-x64
cmake --preset android-arm64
cmake --build --preset android-arm64
```

The first configure of each preset builds the four ports for that triplet (about five minutes for x64). The APKs are `build-android-x64/lamina-components-x86_64.apk` and `build-android-arm64/lamina-components-arm64-v8a.apk`.

The presets use the NDK's toolchain through vcpkg's (`VCPKG_CHAINLOAD_TOOLCHAIN_FILE`), API level 28 as the minimum (the level the stock triplets build the libraries for), `c++_shared` (the triplets link the C++ runtime dynamically), Release, and turn the tests and tools off.

## How the APK is put together

`cmake/LaminaAndroid.cmake`, `lamina_android_apk(target PACKAGE ... LABEL ... OUTPUT_NAME ... [MANIFEST ...] [RES ...])`:

1. On Android, `lamina_add_example` builds the gallery as a shared library, `libmain.so`. `examples/components/main.cpp` includes `SDL_main.h` there, so its `main` becomes `SDL_main`, which SDL's activity calls. SDL is a static library, so the link keeps `JNI_OnLoad` (`-u JNI_OnLoad`); SDL registers its Java natives from it. The library is linked with 16 KB pages (`-z max-page-size=16384`).
2. The SDL Java sources are extracted from the archive at configure time. `javac` compiles them with `LaminaActivity` (`android/LaminaActivity.java.in`) against the platform's `android.jar`, and `d8` turns the classes into `classes.dex`.
3. `aapt2` compiles `android/res` and links the manifest (`android/AndroidManifest.xml.in`) into a base APK.
4. `cmake/LaminaAndroidAssemble.cmake` adds `classes.dex`, the stripped `libmain.so` and the NDK's `libc++_shared.so` under `lib/<abi>/`, and writes every entry uncompressed. The manifest sets `extractNativeLibs="false"`, so the libraries load from the APK in place.
5. `zipalign -P 16` aligns the libraries to 16 KB pages, and `apksigner` signs with the debug keystore.

`LaminaActivity` loads `c++_shared` and `main`, and keeps the status and navigation bar icons readable over the app ([System bars](#system-bars)). The activity keeps SDL's `configChanges` list, so turning the device does not restart it, and `enableOnBackInvokedCallback="false"`, so Back reaches the app as a key.

| Cache variable | Preset value | What |
|---|---|---|
| `LAMINA_ANDROID_SDK` | `$env{ANDROID_HOME}` | the SDK |
| `LAMINA_ANDROID_JDK` | `$env{JAVA_HOME}` | the JDK |
| `LAMINA_ANDROID_PLATFORM` | `android-36.1` | the platform the Java side compiles against |
| `LAMINA_ANDROID_BUILD_TOOLS` | `36.1.0` | the build-tools version |
| `LAMINA_ANDROID_MIN_SDK` | `28` | `minSdkVersion` |
| `LAMINA_ANDROID_TARGET_SDK` | `35` | `targetSdkVersion` |
| `LAMINA_ANDROID_SDL_ARCHIVE` | the SDL archive in vcpkg's `downloads` | where the Java sources come from |
| `LAMINA_ANDROID_KEYSTORE` | (not in the preset) | the signing keystore; defaults to the debug keystore |
| `LAMINA_ANDROID_PAGE_SIZE_KB` | (not in the preset) | page size for the link and for zipalign, 16 by default |

## An APK of another project

A project that adds lamina-ui with `add_subdirectory` builds its own APK with the same function. `lamina_android_apk` finds lamina's activity template, resources and assembly script from its own file, not from the calling project, and the paths it computes at configure time (`LAMINA_ANDROID_JAR`, `LAMINA_ANDROID_SDL_JAVA_FILES`, and the page size in bytes, `LAMINA_ANDROID_PAGE_SIZE`) are internal cache variables, so they are seen from any directory. `MANIFEST` takes a manifest template of the app's own, for permissions and other entries the default one does not have; it is configured with the same `@LAMINA_APK_PACKAGE@` and `@LAMINA_APK_LABEL@`, and must keep the `.LaminaActivity` entry. `RES` takes a resource directory instead of lamina's `android/res`, which must then define the `LaminaTheme` style the manifest names. Both default to lamina's own.

The app's native library is built as the gallery's is: a shared library named `main` that includes `SDL_main.h`, links `SDL3::SDL3`, keeps `JNI_OnLoad` (`-Wl,-u,JNI_OnLoad`) and links with `-Wl,-z,max-page-size=${LAMINA_ANDROID_PAGE_SIZE}`.

## System bars

`Window::set_dark_system_bars(dark)` tells the system whether the app is dark behind the status and navigation bars. On Android it calls `LaminaActivity.setDarkSystemBars(boolean)` through JNI. The activity keeps the value and, on the UI thread, sets `APPEARANCE_LIGHT_STATUS_BARS` and `APPEARANCE_LIGHT_NAVIGATION_BARS` (light bars, dark icons) when the app is light and clears them when it is dark, with both flags as the mask so that either value can be set (`WindowInsetsController.setSystemBarsAppearance(value, mask)`; below Android 11 the `SYSTEM_UI_FLAG_LIGHT_*` flags of `setSystemUiVisibility`). SDL rewrites the system UI flags when it changes the window style, so the activity applies the value again after each change. Until the app calls it, the value follows the system dark mode, and `android/res/values-night/styles.xml` gives the window a dark theme with light icons in dark mode, so the frames before the first `update` match. On other platforms `set_dark_system_bars` does nothing. `Window::system_dark_theme()` says whether the system is in dark mode (`SDL_GetSystemTheme`).

This is what androidx does, read at `6068a9492ba8`:

- `activity/activity/src/main/java/androidx/activity/EdgeToEdge.kt`: `enableEdgeToEdge` (`:94`) passes `detectDarkMode` of each `SystemBarStyle` to the setup, which sets `isAppearanceLightStatusBars = !statusBarIsDark` and `isAppearanceLightNavigationBars = !navigationBarIsDark` (`:318-319`, API 29 and later). `SystemBarStyle.auto` (`:160`) detects dark mode from `uiMode & UI_MODE_NIGHT_MASK`, the system setting.
- `core/core/src/main/java/androidx/core/view/WindowInsetsControllerCompat.java`: `setAppearanceLightStatusBars` (`:222`) and `setAppearanceLightNavigationBars` (`:256`); from API 30 `setAppearanceLight` (`:689`) sets or clears one flag, either in `systemUiVisibility` when it has the window or with `setSystemBarsAppearance(flag or 0, flag)`.
- `compose/foundation/foundation/src/androidMain/kotlin/androidx/compose/foundation/DarkTheme.android.kt`: `isSystemInDarkTheme` (`:46`) reads the same `uiMode` bit. Its documentation recommends following the system setting and offering a user override.

lamina-ui follows the app's own scheme, not the system setting: an app can override its theme, as the gallery's dark theme button does, and the icons must match what is drawn under them. In androidx terms this is `SystemBarStyle.auto(..., detectDarkMode = { appIsDark })`.

The gallery starts in the system's dark mode on Android, following `isSystemInDarkTheme`, and the dark theme button overrides it. On the desktop it starts light, as before. It tells its host each time its theme changes (`GalleryHost::set_dark_theme`, also once at start), and `examples/components/main.cpp` passes that to `set_dark_system_bars`.

## Insets

SDL 3.4.10 reports as its safe area (`SDL_GetWindowSafeArea`) the union of `systemBars`, `systemGestures`, `mandatorySystemGestures`, `tappableElement` and `displayCutout` (`SDLSurface.onApplyWindowInsets` in SDL's Java sources, `android-project/app/src/main/java/org/libsdl/app/SDLSurface.java:205-213`). With gesture navigation the system gesture insets reach about 30 dp in from the left and right edges, where a swipe means Back, so a layout inside SDL's safe area lost about 30 dp on each side: on the emulator and on a phone the gallery's content started 54 dp from the left edge instead of its own 24 dp. The gesture insets say where an app should not put drags of its own, not where its content must stop; content may lie under them. Compose's `WindowInsets.safeDrawing` is the system bars, the display cutout and the IME (recalled, not read here).

`LaminaActivity` makes its own `SDLSurface` (`createSDLSurface`). Its `onApplyWindowInsets` keeps two sets of insets in pixels, `systemBars | displayCutout` and `systemGestures | mandatorySystemGestures`, and then passes the insets on to SDL's handler unchanged. `getWindowInsets()` returns them as eight ints, left, top, right and bottom of each set. `Window` reads them through JNI every frame and divides them by the display scale (`layout::insets_in_dp`): the first set is `Frame::safe_area`, the second `Frame::gesture_insets`, which lamina does not use for layout; it is there for an app that wants to keep edge drags away from the system gestures. Turning the device or showing and hiding the bars changes the insets, and the next frame has the new values. Below Android 11 (API 30), which has no typed insets, the activity returns nothing and `Frame::safe_area` stays SDL's safe area, with no gesture insets. If the activity has no `getWindowInsets`, the window says so once in logcat and uses SDL's safe area as well. The soft keyboard's inset is not read.

## Back

lamina sets `SDL_HINT_ANDROID_TRAP_BACK_BUTTON`, so SDL keeps Back from the activity and reports it as the key `SDLK_AC_BACK`. `Window` hands it over as `Key::escape` with `KeyEvent::back` set, so whatever closes on Escape closes on Back. The app decides whether it used the key; when nothing did, it calls `Window::system_back()`. On Android that calls `SDL_SendAndroidBackButton()`, and SDL's Java side runs the platform's `Activity.onBackPressed()` on the UI thread (`SDLActivity.pressBackButton` and `superOnBackPressed`), past its own trap. On other platforms `system_back` does nothing and `KeyEvent::back` stays false, so Escape from a keyboard never leaves an app.

What the platform then does, read in the SDK sources `sources/android-36.1/android/app/Activity.java`: `onBackPressed` (`:4331`) collapses an action view or pops the fragment back stack if there is one (the gallery has neither) and otherwise calls `onBackInvoked` (`:4344`), which reports the press to the activity task manager with `ActivityClient.onBackPressed`, passing a callback that finishes the activity if the server asks for it (`RequestFinishCallback`, `:4272`). The documentation of `onBackPressed` (`:4288-4330`) says that from Android 12 (`VERSION_CODES.S`) an activity that is the root of its task and declares `ACTION_MAIN` with `CATEGORY_LAUNCHER` is moved to the back together with its task instead of being finished, and that other activities are finished. The server side that makes this choice is not in the SDK sources and was not read. `LaminaActivity` is such a root launcher activity. On the API 37 emulator, with the gallery opened from its launcher icon, Back with nothing open moved the task to the back (`wm_task_moved` to the bottom, then `wm_stop_activity`) and kept the process: opening it again from the launcher showed the same scroll position and colors in the same process. With the gallery started by `adb shell am start`, with or without `-a MAIN -c LAUNCHER`, the same Back finished the activity instead (`wm_finish_activity ... app-request`, the server's request through `RequestFinishCallback`), so the server looks at more than the manifest, such as where the task was launched from; which condition it checks was not read.

The gallery counts Back as used when a page or the base widgets say they used the key, and when a text field had the focus before the key. The capture check covers the menu and the dialog on Basics, the modal bottom sheet and a focused text field, which each take the first Back, and every page with nothing open, where the first Back is handed on; the other overlays of the pages (side sheets, drawers, search views, pickers) were not checked one by one. A text field drops its focus on Escape but returns false, because `TextField::handle` returns whether the text was submitted, not whether it used the key. An auto-repeated Back is never handed on.

## Install and run

```
adb install -r build-android-x64/lamina-components-x86_64.apk
adb shell am start -n io.github.capsicum0907.lamina.components/.LaminaActivity
adb logcat -s SDL SDL/APP AndroidRuntime
```

On Android, the errors that go to stderr on the desktop go to logcat as errors under the tag `SDL/APP` (`lamina_sdl3/report.hpp`), so an exception during startup shows there.

An emulator for the x86_64 APK: an AVD of the system image `android-37.0/google_apis_playstore_ps16k/x86_64` at 1080 × 2400 and 420 dpi (about 411 dp wide, the gallery's compact layout), started with `emulator -avd <name> -no-metrics`. For a phone, enable USB or wireless debugging and install the arm64-v8a APK.

## What works

Checked on the API 37 emulator (16 KB pages, SwiftShader GPU):

- The gallery starts and draws at about 411 dp wide, in the compact layout. Turning the screen to landscape switches it to the expanded layout.
- Tapping works: the page tabs, a filter chip, a switch, the typeface menu and its items, the Menu and Dialog buttons and a text field. After a tap the button keeps no hover tint.
- The layout stays out of the status bar, the navigation bar and display cutouts and runs under the side gesture areas ([Insets](#insets)): with gesture navigation the gallery's content starts 24 dp from the left edge, its own margin, where it started at 54 dp inside SDL's safe area. Turning to landscape and back updates the insets.
- Back closes an open menu or dialog and takes the focus from a text field (it reaches the gallery as Escape). With nothing open it sends the app to the back when it was opened from the launcher: the launcher shows, and opening the app again shows the same page and theme in the same process ([Back](#back)).
- Text fields show the soft keyboard; keys typed on Gboard and `adb shell input text` arrive.
- The mouse wheel scrolls the pages and the tabs (on the emulator, `adb shell input mouse scroll`).
- A finger drag scrolls the pages up and down, and the page tabs and the side-scrolled time picker sideways; a quick swipe flings and slows down on Compose's spline decay, and a touch during a fling stops it. A swipe that starts on a button, a chip or a text field scrolls without clicking or focusing it, a tap still clicks, and the slider follows a drag that starts on it. Checked with `adb shell input swipe` and `adb shell input tap` ([Touch scrolling](design-tokens.md#touch-scrolling)).
- The "OS fonts" typeface loads Noto Sans CJK from `/system/fonts`.
- Two fingers on the Viewer page: a pinch out from 229 dp to 350 dp apart zoomed the pattern to 141 % about the fingers' centre, a one-finger drag moved the zoomed pattern, a pinch in to 91 dp shrank it below the fitted size and it sprang back to 100 %, a double tap zoomed in to 250 % and a second one fitted it again. `adb shell input` has no second finger, and `sendevent` from the shell is refused on this image (permission denied on `/dev/input`), so the fingers were sent through the emulator console (`adb emu event send` with `ABS_MT_SLOT`, `ABS_MT_TRACKING_ID`, `ABS_MT_POSITION_X/Y` in the touchscreen's 0 to 32767 range, and `SYN_REPORT` as `EV_SYN:0:0`). Each slot needs `ABS_MT_PRESSURE` above 0: without it Android took the second finger as not touching and reported one pointer.
- Swiping between pages ([Pager](components/containers.md#pager)): a quick `adb shell input swipe` to the left on Basics went to Chips, a slow one of about 230 dp to the right came back to Chips, and a quick one to the right went to Basics. Quick swipes to the left went on page by page until the Drawers page, where a swipe across the modal drawer's frame left the page where it was and one above the frame went on to the Viewer. There a swipe went to photo 2; two console fingers pinched it, from 76 to 229 dp apart and again from 152 to 335 dp; a one-finger drag to the right panned it to its left edge; the next drag to the right went to the pager, and photo 1 came in fitted. A quick swipe to the right on photo 1 went on to the Desktop page, where a vertical swipe scrolled the page.
- The 48 dp touch target: a tap up to about 3 dp above a 40 dp button (Send on Basics) or below one in a button group (Copy on Groups) clicks it, and a tap 4 dp or more away does not. A tap in the 8 dp gap between two filter chips selects only the nearer one. Taps just left of the overflow button of a button group open its menu and close it again. Checked with `adb shell input tap` ([Touch target](design-tokens.md#touch-target)).
- The status and navigation bar icons are dark over the light theme and light over the dark theme, and follow the dark theme button. With the system in dark mode (`adb shell cmd uimode night yes`) the gallery starts dark, and the window is dark with light icons before the first frame.
- Leaving the app with Home and coming back draws the gallery again.
- On the emulator a frame of the animated Progress page takes about 19 ms (about 52 frames a second), measured from the start of `update` to the end of `present`. Drawing is ThorVG on the CPU into a 1080 × 2400 texture.

## Not yet

- Touch scrolling has no overscroll stretch at the ends and no nested scrolling: a list inside a page does not hand the rest of a drag or fling to the page. A scroll area follows only one finger; the zoom view follows several. The touch checks were made with `adb shell input` and the emulator console, not with a finger on a device.
- On the Drawers page the modal drawer's frame keeps every horizontal drag, also one to the left while the drawer is closed, so the page turns only by a swipe outside the frame.
- Predictive back is not done. The manifest keeps `enableOnBackInvokedCallback="false"`, so Back arrives as a key, there is no `OnBackInvokedCallback` and no preview animation of the sheets, drawers or the app leaving.
- The gallery reads the system dark mode only at start; it does not follow a change of the setting while it runs (`SDL_EVENT_SYSTEM_THEME_CHANGED` is not read).
- Dialogs and menus are placed in the whole window, not inside the safe area.
- The soft keyboard can cover a field low on the screen; SDL reports only whether the keyboard is shown, not its height, and the activity does not read the IME inset.
- Japanese input through an IME has not been tried; SDL hands composing text over as committed text and deletions, not as composition events.
- The APK has no icon, and only the gallery is built; the other examples are skipped on Android.
