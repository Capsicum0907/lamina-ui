# Android

[日本語](android.ja.md) · [README](../README.md#documentation)

The components gallery builds as an Android APK. The build uses CMake, the NDK and the command-line tools of the Android SDK; there is no Gradle project. It has been run on the API 37 x86_64 emulator; the arm64-v8a APK builds the same way but has not been run on a device yet.

## Requirements

- Android SDK parts: NDK 28.2.13676358 (r28c), the platform `android-36.1`, build-tools 36.1.0 (aapt2, d8, zipalign, apksigner), and platform-tools (adb). To run on an emulator, also the emulator and a system image.
- A JDK 17 or later with `javac` and `jar`. Android Studio's bundled `jbr` works.
- vcpkg (`VCPKG_ROOT`) with its `x64-android` and `arm64-android` triplets. The manifest is the same as on Windows; vcpkg builds HarfBuzz, SDL3, ThorVG and abseil for Android from source.
- The SDL source archive of the version vcpkg builds, `$VCPKG_ROOT/downloads/libsdl-org-SDL-release-3.4.10.tar.gz`. vcpkg leaves it there when it builds `sdl3`. The APK takes SDL's Java sources from it; configure stops if their version is not the linked SDL's.
- CMake 3.25 or later and Ninja on `PATH`. The Visual Studio generator cannot target Android.
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
cmake --preset android-x64
cmake --build --preset android-x64
cmake --preset android-arm64
cmake --build --preset android-arm64
```

The first configure of each preset builds the four ports for that triplet (about five minutes for x64). The APKs are `build-android-x64/lamina-components-x86_64.apk` and `build-android-arm64/lamina-components-arm64-v8a.apk`.

The presets use the NDK's toolchain through vcpkg's (`VCPKG_CHAINLOAD_TOOLCHAIN_FILE`), API level 28 as the minimum (the level the stock triplets build the libraries for), `c++_shared` (the triplets link the C++ runtime dynamically), Release, and turn the tests and tools off.

## How the APK is put together

`cmake/LaminaAndroid.cmake`, `lamina_android_apk(target PACKAGE ... LABEL ... OUTPUT_NAME ...)`:

1. On Android, `lamina_add_example` builds the gallery as a shared library, `libmain.so`. `examples/components/main.cpp` includes `SDL_main.h` there, so its `main` becomes `SDL_main`, which SDL's activity calls. SDL is a static library, so the link keeps `JNI_OnLoad` (`-u JNI_OnLoad`); SDL registers its Java natives from it. The library is linked with 16 KB pages (`-z max-page-size=16384`).
2. The SDL Java sources are extracted from the archive at configure time. `javac` compiles them with `LaminaActivity` (`android/LaminaActivity.java.in`) against the platform's `android.jar`, and `d8` turns the classes into `classes.dex`.
3. `aapt2` compiles `android/res` and links the manifest (`android/AndroidManifest.xml.in`) into a base APK.
4. `cmake/LaminaAndroidAssemble.cmake` adds `classes.dex`, the stripped `libmain.so` and the NDK's `libc++_shared.so` under `lib/<abi>/`, and writes every entry uncompressed. The manifest sets `extractNativeLibs="false"`, so the libraries load from the APK in place.
5. `zipalign -P 16` aligns the libraries to 16 KB pages, and `apksigner` signs with the debug keystore.

`LaminaActivity` loads `c++_shared` and `main`, and sets light status and navigation bars again each time SDL changes the window style. The activity keeps SDL's `configChanges` list, so turning the device does not restart it, and `enableOnBackInvokedCallback="false"`, so Back reaches the app as a key.

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
- Tapping works: the page tabs, buttons, chips, switches, list items, the typeface menu and the dialog. A touch release also ends the hover state.
- The layout stays out of the status bar and the gesture bar (`Frame::safe_area`).
- Back closes an open menu or dialog and takes the focus from a text field (it reaches the gallery as Escape), and the app stays open.
- Text fields show the soft keyboard; keys typed on Gboard and `adb shell input text` arrive.
- The mouse wheel scrolls the pages and the tabs (on the emulator, `adb shell input mouse scroll`).
- The "OS fonts" typeface loads Noto Sans CJK from `/system/fonts`.
- Leaving the app with Home and coming back draws the gallery again.
- On the emulator a frame of the animated Progress page takes about 19 ms (about 52 frames a second), measured from the start of `update` to the end of `present`. Drawing is ThorVG on the CPU into a 1080 × 2400 texture.

## Not yet

- Scrolling by dragging a finger. `ScrollArea` scrolls with the wheel and its scrollbar, but a drag on the content does not move it; this is the main gap on a phone.
- Back when nothing is open does nothing; it does not leave the app.
- The status and navigation bar icons stay dark in the gallery's dark theme.
- Dialogs and menus are placed in the whole window, not inside the safe area.
- The soft keyboard can cover a field low on the screen; SDL reports only whether the keyboard is shown, not its height.
- Japanese input through an IME has not been tried; SDL hands composing text over as committed text and deletions, not as composition events.
- The APK has no icon, and only the gallery is built; the other examples are skipped on Android.
- The arm64-v8a APK has not been run on a device.
