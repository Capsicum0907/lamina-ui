# Android

[English](android.md) · [README](../README.ja.md#資料)

部品の見本（components）は Android の APK として作れます。作るのに使うのは CMake と NDK と Android SDK のコマンドの道具だけで、Gradle のプロジェクトはありません。API 37 の x86_64 のエミュレーターで動かしました。arm64-v8a の APK も同じ手順で作れますが、まだ実機では動かしていません。

## 必要なもの

- Android SDK のうち、NDK 28.2.13676358（r28c）、プラットフォーム `android-36.1`、build-tools 36.1.0（aapt2・d8・zipalign・apksigner）、platform-tools（adb）。エミュレーターで動かすなら、エミュレーターとシステムイメージも要ります。
- `javac` と `jar` のある JDK 17 以降。Android Studio に入っている `jbr` で動きます。
- vcpkg（`VCPKG_ROOT`）と、その `x64-android`・`arm64-android` のトリプレット。マニフェストは Windows と同じで、vcpkg が HarfBuzz・SDL3・ThorVG・abseil を Android 向けにソースから作ります。
- vcpkg が作る版の SDL のソースのアーカイブ `$VCPKG_ROOT/downloads/libsdl-org-SDL-release-3.4.10.tar.gz`。vcpkg は `sdl3` を作るときにここへ置きます。APK は SDL の Java のソースをここから取ります。その版がリンクする SDL と違えば、構成の段階で止まります。
- CMake 3.25 以降と、`PATH` の通った Ninja。Visual Studio のジェネレーターでは Android 向けに作れません。
- アイコンのフォントを切り出すための、fontTools と brotli の入ったホストの Python 3（デスクトップと同じです）。
- デバッグ用のキーストア。既定では `ANDROID_USER_HOME` の、またはホームの `.android` の `debug.keystore` です（Android Studio が作ります）。

## 環境変数

プリセット `android-x64` と `android-arm64` は次の変数を読みます。CMake のファイルには道具のパスを書いていません。

| 変数 | 中身 |
|---|---|
| `ANDROID_HOME` | Android SDK |
| `ANDROID_NDK_HOME` | NDK（例：`$ANDROID_HOME/ndk/28.2.13676358`）。vcpkg の Android のツールチェーンもこれを読みます |
| `JAVA_HOME` | JDK。`d8` と `apksigner` もこれで動きます |
| `VCPKG_ROOT` | vcpkg |
| `LAMINA_HOST_PYTHON` | fontTools と brotli の入ったホストの `python.exe`。Android のツールチェーンの下では、CMake はホストのプログラムを自分では探しません |

`VCPKG_DISABLE_METRICS=1` で vcpkg の利用状況の送信を止められます。`X_VCPKG_ASSET_SOURCES=clear;x-block-origin` にすると、vcpkg はダウンロードせずに失敗するので、必要なものが `downloads` に揃っているかを確かめられます。

## 作り方

PowerShell で、SDK が Android Studio の既定の場所にある場合です。

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

それぞれのプリセットの最初の構成では、そのトリプレット向けに四つのポートを作ります（x64 で 5 分ほど）。できる APK は `build-android-x64/lamina-components-x86_64.apk` と `build-android-arm64/lamina-components-arm64-v8a.apk` です。

プリセットは、NDK のツールチェーンを vcpkg のものを通して使い（`VCPKG_CHAINLOAD_TOOLCHAIN_FILE`）、最低の API レベルを 28（標準のトリプレットがライブラリを作るレベル）、C++ の実行時ライブラリを `c++_shared`（トリプレットが動的にリンクするため）、Release にして、テストと道具は作りません。

## APK の組み立て

`cmake/LaminaAndroid.cmake` の `lamina_android_apk(target PACKAGE ... LABEL ... OUTPUT_NAME ...)` が次の順に組み立てます。

1. Android では `lamina_add_example` が見本を共有ライブラリ `libmain.so` として作ります。`examples/components/main.cpp` はそこで `SDL_main.h` を読み込むので、`main` が `SDL_main` になり、SDL のアクティビティがそれを呼びます。SDL は静的ライブラリなので、`JNI_OnLoad` がリンクで落ちないようにしています（`-u JNI_OnLoad`）。SDL はここから Java のネイティブ関数を登録します。ライブラリは 16 KB のページでリンクします（`-z max-page-size=16384`）。
2. SDL の Java のソースは構成の段階でアーカイブから取り出します。`javac` がそれを `LaminaActivity`（`android/LaminaActivity.java.in`）と一緒にプラットフォームの `android.jar` に対してコンパイルし、`d8` がクラスを `classes.dex` にします。
3. `aapt2` が `android/res` をコンパイルし、マニフェスト（`android/AndroidManifest.xml.in`）と合わせて元になる APK を作ります。
4. `cmake/LaminaAndroidAssemble.cmake` が `classes.dex`、シンボルを落とした `libmain.so`、NDK の `libc++_shared.so` を `lib/<abi>/` に加え、すべてを圧縮せずに書きます。マニフェストは `extractNativeLibs="false"` なので、ライブラリは APK の中からそのまま読み込まれます。
5. `zipalign -P 16` がライブラリを 16 KB のページ境界にそろえ、`apksigner` がデバッグ用のキーストアで署名します。

`LaminaActivity` は `c++_shared` と `main` を読み込み、SDL が窓の表示のしかたを変えるたびに、ステータスバーとナビゲーションバーを明るい背景向けの表示に戻します。アクティビティは SDL の `configChanges` の一覧をそのまま持つので、端末を回しても作り直されません。`enableOnBackInvokedCallback="false"` なので、戻る操作はキーとしてアプリに届きます。

| キャッシュ変数 | プリセットの値 | 中身 |
|---|---|---|
| `LAMINA_ANDROID_SDK` | `$env{ANDROID_HOME}` | SDK |
| `LAMINA_ANDROID_JDK` | `$env{JAVA_HOME}` | JDK |
| `LAMINA_ANDROID_PLATFORM` | `android-36.1` | Java の側をコンパイルするプラットフォーム |
| `LAMINA_ANDROID_BUILD_TOOLS` | `36.1.0` | build-tools の版 |
| `LAMINA_ANDROID_MIN_SDK` | `28` | `minSdkVersion` |
| `LAMINA_ANDROID_TARGET_SDK` | `35` | `targetSdkVersion` |
| `LAMINA_ANDROID_SDL_ARCHIVE` | vcpkg の `downloads` の SDL のアーカイブ | Java のソースの取り出し元 |
| `LAMINA_ANDROID_KEYSTORE` | （プリセットにはなし） | 署名に使うキーストア。既定はデバッグ用のもの |
| `LAMINA_ANDROID_PAGE_SIZE_KB` | （プリセットにはなし） | リンクと zipalign のページの大きさ。既定は 16 |

## 入れ方と動かし方

```
adb install -r build-android-x64/lamina-components-x86_64.apk
adb shell am start -n io.github.capsicum0907.lamina.components/.LaminaActivity
adb logcat -s SDL SDL/APP AndroidRuntime
```

デスクトップで標準エラー出力に出るエラーは、Android では logcat にタグ `SDL/APP` のエラーとして出ます（`lamina_sdl3/report.hpp`）。起動中の例外もここに出ます。

x86_64 の APK を動かすエミュレーターは、システムイメージ `android-37.0/google_apis_playstore_ps16k/x86_64` で 1080 × 2400・420 dpi の AVD（幅はおよそ 411 dp で、見本は小さい幅の配置になります）を作り、`emulator -avd <名前> -no-metrics` で起動します。スマートフォンでは USB デバッグかワイヤレスデバッグを有効にして、arm64-v8a の APK を入れます。

## 動くもの

API 37 のエミュレーター（16 KB ページ、GPU は SwiftShader）で確かめました。

- 見本は幅およそ 411 dp の小さい幅の配置で起動して描かれます。画面を横にすると広い幅の配置に変わります。
- タップが効きます。ページのタブ、ボタン、チップ、スイッチ、リストの項目、書体のメニュー、ダイアログで確かめました。指を離すとホバーの状態も消えます。
- 配置はステータスバーとジェスチャーのバーにかかりません（`Frame::safe_area`）。
- 戻る操作で、開いているメニューやダイアログが閉じ、文字の欄からフォーカスが外れます（見本には Escape として届きます）。アプリは閉じません。
- 文字の欄を押すとソフトウェアキーボードが出ます。Gboard で打った文字と `adb shell input text` の文字が入ります。
- マウスのホイールでページとタブがスクロールします（エミュレーターでは `adb shell input mouse scroll`）。
- 書体「OS fonts」は `/system/fonts` の Noto Sans CJK を読み込みます。
- ホームでアプリを離れて戻ると、見本がもう一度描かれます。
- エミュレーターでは、動き続ける Progress のページの 1 フレームがおよそ 19 ms（毎秒およそ 52 フレーム）でした。`update` の始めから `present` の終わりまでを測った値です。描画は ThorVG が CPU で 1080 × 2400 のテクスチャに描いています。

## まだできないこと

- 指でなぞってのスクロール。`ScrollArea` はホイールとスクロールバーでは動きますが、中身をなぞっても動きません。スマートフォンではこれがいちばん大きな欠けです。
- 何も開いていないときの戻る操作は何もせず、アプリを離れません。
- 見本を暗い配色にしても、ステータスバーとナビゲーションバーのアイコンは暗い色のままです。
- ダイアログとメニューは、安全な領域の中ではなく窓全体を基準に置かれます。
- 画面の下のほうの欄はソフトウェアキーボードに隠れることがあります。SDL が知らせるのはキーボードが出ているかどうかだけで、高さは分かりません。
- IME での日本語入力は試していません。SDL は変換中の文字を、変換の途中経過ではなく、確定した文字と削除として渡します。
- APK にはアイコンがありません。作るのは見本だけで、ほかの見本は Android では作りません。
- arm64-v8a の APK は実機で動かしていません。
