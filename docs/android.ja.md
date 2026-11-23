# Android

[English](android.md) · [README](../README.ja.md#資料)

部品の見本（components）は Android の APK として作れます。作るのに使うのは CMake と NDK と Android SDK のコマンドの道具だけで、Gradle のプロジェクトはありません。API 37 の x86_64 のエミュレーターで動かし、arm64-v8a の APK は実機で動かしました。

## 必要なもの

- Android SDK のうち、NDK 28.2.13676358（r28c）、プラットフォーム `android-36.1`、build-tools 36.1.0（aapt2・d8・zipalign・apksigner）、platform-tools（adb）。エミュレーターで動かすなら、エミュレーターとシステムイメージも要ります。
- `javac` と `jar` のある JDK 17 以降。Android Studio に入っている `jbr` で動きます。
- vcpkg（`VCPKG_ROOT`）と、その `x64-android`・`arm64-android` のトリプレット。マニフェストは Windows と同じで、vcpkg が HarfBuzz・SDL3・ThorVG・abseil を Android 向けにソースから作ります。
- vcpkg が作る版の SDL のソースのアーカイブ `$VCPKG_ROOT/downloads/libsdl-org-SDL-release-3.4.10.tar.gz`。vcpkg は `sdl3` を作るときにここへ置きます。APK は SDL の Java のソースをここから取ります。その版がリンクする SDL と違えば、構成の段階で止まります。
- CMake 4.3 以降と、`PATH` の通った Ninja。APK を組み立てる段階は `file(ARCHIVE_CREATE ... COMPRESSION None)` で zip の中身を圧縮せずに書きますが、CMake 4.1 は zip にこの指定を受け付けません。古い CMake では構成の段階で止まります。vcpkg が持っている CMake（`downloads/tools/cmake-4.3.2-windows`）なら足ります。SDK の `cmake/4.1.2/bin` に Ninja があります。Visual Studio のジェネレーターでは Android 向けに作れません。
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
$env:PATH = "$env:VCPKG_ROOT\downloads\tools\cmake-4.3.2-windows\cmake-4.3.2-windows-x86_64\bin;$env:ANDROID_HOME\cmake\4.1.2\bin;$env:PATH"
cmake --preset android-x64
cmake --build --preset android-x64
cmake --preset android-arm64
cmake --build --preset android-arm64
```

それぞれのプリセットの最初の構成では、そのトリプレット向けに四つのポートを作ります（x64 で 5 分ほど）。できる APK は `build-android-x64/lamina-components-x86_64.apk` と `build-android-arm64/lamina-components-arm64-v8a.apk` です。

プリセットは、NDK のツールチェーンを vcpkg のものを通して使い（`VCPKG_CHAINLOAD_TOOLCHAIN_FILE`）、最低の API レベルを 28（標準のトリプレットがライブラリを作るレベル）、C++ の実行時ライブラリを `c++_shared`（トリプレットが動的にリンクするため）、Release にして、テストと道具は作りません。

## APK の組み立て

`cmake/LaminaAndroid.cmake` の `lamina_android_apk(target PACKAGE ... LABEL ... OUTPUT_NAME ... [MANIFEST ...] [RES ...] [EXTRA_RES ...])` が次の順に組み立てます。

1. Android では `lamina_add_example` が見本を共有ライブラリ `libmain.so` として作ります。`examples/components/main.cpp` はそこで `SDL_main.h` を読み込むので、`main` が `SDL_main` になり、SDL のアクティビティがそれを呼びます。SDL は静的ライブラリなので、`JNI_OnLoad` がリンクで落ちないようにしています（`-u JNI_OnLoad`）。SDL はここから Java のネイティブ関数を登録します。ライブラリは 16 KB のページでリンクします（`-z max-page-size=16384`）。
2. SDL の Java のソースは構成の段階でアーカイブから取り出します。`javac` がそれを `LaminaActivity`（`android/LaminaActivity.java.in`）と一緒にプラットフォームの `android.jar` に対してコンパイルし、`d8` がクラスを `classes.dex` にします。
3. `aapt2` が `android/res`（と `EXTRA_RES` の各ディレクトリ。それぞれ別のアーカイブに）をコンパイルし、マニフェスト（`android/AndroidManifest.xml.in`）と合わせて元になる APK を作ります。
4. `cmake/LaminaAndroidAssemble.cmake` が `classes.dex`、シンボルを落とした `libmain.so`、NDK の `libc++_shared.so` を `lib/<abi>/` に加え、すべてを圧縮せずに書きます。マニフェストは `extractNativeLibs="false"` なので、ライブラリは APK の中からそのまま読み込まれます。
5. `zipalign -P 16` がライブラリを 16 KB のページ境界にそろえ、`apksigner` がデバッグ用のキーストアで署名します。

`LaminaActivity` は `c++_shared` と `main` を読み込み、ステータスバーとナビゲーションバーのアイコンをアプリの上で読める色に保ちます（[システムのバー](#システムのバー)）。アクティビティは SDL の `configChanges` の一覧をそのまま持つので、端末を回しても作り直されません。`enableOnBackInvokedCallback="false"` なので、戻る操作はキーとしてアプリに届きます。

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

## ほかのプロジェクトの APK

lamina-ui を `add_subdirectory` で取り込んだプロジェクトも、同じ関数で自分の APK を作れます。`lamina_android_apk` は、lamina のアクティビティのひな形、リソース、組み立てのスクリプトを、呼び出したプロジェクトからではなく自分のファイルの位置から探します。構成のときに求めるパス（`LAMINA_ANDROID_JAR`、`LAMINA_ANDROID_SDL_JAVA_FILES`、バイト単位のページの大きさ `LAMINA_ANDROID_PAGE_SIZE`）は内部用のキャッシュ変数なので、どのディレクトリからも見えます。`MANIFEST` にはアプリ自身のマニフェストのひな形を渡せます。既定のひな形にない権限などを足すためのもので、同じ `@LAMINA_APK_PACKAGE@` と `@LAMINA_APK_LABEL@` で埋められ、`.LaminaActivity` の項目は残す必要があります。`RES` には lamina の `android/res` の代わりのリソースのディレクトリを渡せます。その場合、マニフェストが名指しする `LaminaTheme` のスタイルを定義する必要があります。どちらも既定は lamina 自身のものです。`EXTRA_RES` には、lamina のリソース（または `RES`）に足すリソースのディレクトリを 1 つ以上渡せます。lamina のテーマを置き換えずにアプリが足すもの、たとえばランチャーのアイコンのためのものです。アプリはそこに `mipmap-*` のフォルダを置き、自分のマニフェストでアイコンを名指しします（`<application>` の `android:icon="@mipmap/..."`）。ディレクトリは 1 つずつコンパイルされ、まとめてリンクされます。同じリソースが 2 つのディレクトリにあると、上書きではなく `aapt2 link` のエラーになります。

アプリのネイティブのライブラリは、ギャラリーと同じ形で作ります。`SDL_main.h` を取り込む `main` という名前の共有ライブラリにし、`SDL3::SDL3` をリンクし、`JNI_OnLoad` を残し（`-Wl,-u,JNI_OnLoad`）、`-Wl,-z,max-page-size=${LAMINA_ANDROID_PAGE_SIZE}` でリンクします。

## システムのバー

`Window::set_dark_system_bars(dark)` は、ステータスバーとナビゲーションバーの下でアプリが暗い色かどうかをシステムに伝えます。Android では、JNI で `LaminaActivity.setDarkSystemBars(boolean)` を呼びます。アクティビティはその値を覚えておき、UI のスレッドで次のようにします。アプリが明るいときは `APPEARANCE_LIGHT_STATUS_BARS` と `APPEARANCE_LIGHT_NAVIGATION_BARS`（明るいバーに暗いアイコン）を立て、暗いときは下ろします。どちらの値にもできるように、二つの印をまとめて範囲に指定します（`WindowInsetsController.setSystemBarsAppearance(値, 範囲)`。Android 11 より前は `setSystemUiVisibility` の `SYSTEM_UI_FLAG_LIGHT_*`）。SDL は窓の表示のしかたを変えるときにシステムの表示の印を書き直すので、アクティビティは変わるたびに値をもう一度当てます。アプリが呼ぶまでは、値はシステムのダークモードに従います。ダークモードでは `android/res/values-night/styles.xml` が窓を暗いテーマと明るいアイコンにするので、最初の `update` より前のフレームもそろいます。ほかの OS では `set_dark_system_bars` は何もしません。`Window::system_dark_theme()` は、システムがダークモードかどうかを返します（`SDL_GetSystemTheme`）。

androidx の `6068a9492ba8` を読んで、同じ動きにしました。

- `activity/activity/src/main/java/androidx/activity/EdgeToEdge.kt`：`enableEdgeToEdge`（`:94`）は、`SystemBarStyle` ごとの `detectDarkMode` の結果を設定の処理に渡します。設定の処理は `isAppearanceLightStatusBars = !statusBarIsDark` と `isAppearanceLightNavigationBars = !navigationBarIsDark` にします（`:318-319`、API 29 以降）。`SystemBarStyle.auto`（`:160`）は、ダークモードかどうかを `uiMode & UI_MODE_NIGHT_MASK`、つまりシステムの設定から決めます。
- `core/core/src/main/java/androidx/core/view/WindowInsetsControllerCompat.java`：`setAppearanceLightStatusBars`（`:222`）と `setAppearanceLightNavigationBars`（`:256`）。API 30 以降の `setAppearanceLight`（`:689`）は、窓を持っていれば `systemUiVisibility` の印を、持っていなければ `setSystemBarsAppearance(印か 0, 印)` で、一つの印を立てるか下ろします。
- `compose/foundation/foundation/src/androidMain/kotlin/androidx/compose/foundation/DarkTheme.android.kt`：`isSystemInDarkTheme`（`:46`）も同じ `uiMode` の印を読みます。その説明は、システムの設定に従うことと、利用者が切り替えられるようにすることを勧めています。

lamina-ui は、システムの設定ではなくアプリ自身の配色に従います。見本の暗い配色のボタンのように、アプリはテーマを上書きできます。アイコンは、その下に描かれているものに合っていなければなりません。androidx で言えば `SystemBarStyle.auto(..., detectDarkMode = { appIsDark })` にあたります。

見本は、Android ではシステムのダークモードに合わせて起動します（`isSystemInDarkTheme` に従う形です）。暗い配色のボタンでそれを上書きできます。デスクトップでは、これまでどおり明るい配色で起動します。見本は配色が変わるたびに、また起動したときに一度、それをホストに伝えます（`GalleryHost::set_dark_theme`）。`examples/components/main.cpp` がそれを `set_dark_system_bars` に渡します。

## 余白（インセット）

SDL 3.4.10 が安全な領域（`SDL_GetWindowSafeArea`）として知らせるのは、`systemBars`・`systemGestures`・`mandatorySystemGestures`・`tappableElement`・`displayCutout` を合わせたものです（SDL の Java のソースの `SDLSurface.onApplyWindowInsets`、`android-project/app/src/main/java/org/libsdl/app/SDLSurface.java:205-213`）。ジェスチャーナビゲーションでは、システムのジェスチャーの余白が左右の端から 30 dp ほど内側まで来ます。そこを横になぞると戻る操作になる場所です。なので安全な領域の内側に配置すると、左右それぞれ 30 dp ほど狭くなっていました。エミュレーターでも実機でも、見本の中身は左端から自分の余白の 24 dp ではなく 54 dp の所から始まっていました。ジェスチャーの余白は、アプリが自分のドラッグを置くべきでない場所を表すもので、中身を止める場所ではありません。中身はその下にあってかまいません。Compose の `WindowInsets.safeDrawing` は、システムのバー・画面の切り欠き・IME を合わせたものです（記憶によるもので、ここでは読んでいません）。

`LaminaActivity` は自分の `SDLSurface` を作ります（`createSDLSurface`）。その `onApplyWindowInsets` は、`systemBars | displayCutout` と `systemGestures | mandatorySystemGestures` の二組の余白をピクセルで取っておき、そのあと余白をそのまま SDL の処理に渡します。`getWindowInsets()` は、それぞれの組の左・上・右・下の八つの整数を返します。`Window` は毎フレーム JNI でそれを読み、画面の倍率で割ります（`layout::insets_in_dp`）。一組目が `Frame::safe_area`、二組目が `Frame::gesture_insets` です。lamina は二組目を配置には使いません。端のドラッグをシステムのジェスチャーから離したいアプリのためのものです。端末を回したり、バーを出したり隠したりすると余白が変わり、次のフレームで新しい値になります。種類つきの余白がない Android 11（API 30）より前では、アクティビティは何も返さず、`Frame::safe_area` は SDL の安全な領域のまま、ジェスチャーの余白は 0 です。アクティビティに `getWindowInsets` がないときは、窓がそのことを logcat に一度だけ出し、やはり SDL の安全な領域を使います。ソフトウェアキーボードの余白は読みません。

## 戻る操作

lamina は `SDL_HINT_ANDROID_TRAP_BACK_BUTTON` を立てています。なので SDL は戻る操作をアクティビティに渡さず、キー `SDLK_AC_BACK` として知らせます。`Window` はそれを、`KeyEvent::back` を立てた `Key::escape` として渡します。Escape で閉じるものは、戻る操作でも閉じます。キーを使ったかどうかはアプリが決めます。何も使わなかったときは、アプリが `Window::system_back()` を呼びます。Android では、これが `SDL_SendAndroidBackButton()` を呼びます。SDL の Java 側は、UI のスレッドでプラットフォームの `Activity.onBackPressed()` を実行します（`SDLActivity.pressBackButton` と `superOnBackPressed`）。SDL 自身の捕まえる仕組みは通りません。ほかの OS では `system_back` は何もせず、`KeyEvent::back` は立ちません。なので、キーボードの Escape でアプリを離れることはありません。

そのあとプラットフォームがすることを、SDK のソース `sources/android-36.1/android/app/Activity.java` で読みました。`onBackPressed`（`:4331`）は、アクションビューがあればたたみ、フラグメントの戻る履歴があれば一つ戻します（見本にはどちらもありません）。どちらもなければ `onBackInvoked`（`:4344`）を呼びます。これは `ActivityClient.onBackPressed` で、押されたことをアクティビティのタスクの管理側に知らせます。そのとき、管理側に頼まれたらアクティビティを終える呼び戻しを渡します（`RequestFinishCallback`、`:4272`）。`onBackPressed` の説明（`:4288-4330`）には次のようにあります。Android 12（`VERSION_CODES.S`）からは、タスクの根元にあって `ACTION_MAIN` と `CATEGORY_LAUNCHER` を宣言しているアクティビティは、終わらずにタスクごと後ろへ回る。それ以外のアクティビティは終わる。この選択をする管理側のコードは SDK のソースになく、読んでいません。`LaminaActivity` はこの根元のランチャーのアクティビティです。API 37 のエミュレーターで、ランチャーのアイコンから開いた見本では、何も開いていないときの戻る操作でタスクが後ろへ回り（`wm_task_moved` で一番下へ、そのあと `wm_stop_activity`）、プロセスは残りました。ランチャーからもう一度開くと、同じプロセスのまま、同じスクロールの位置と色で出ました。`adb shell am start` で起動した見本では、`-a MAIN -c LAUNCHER` を付けても付けなくても、同じ戻る操作でアクティビティが終わりました（`wm_finish_activity ... app-request`。`RequestFinishCallback` を通した管理側の頼み）。管理側はマニフェストのほかにも、タスクをどこから起動したかなどを見ているようです。何を条件にしているかは読んでいません。

見本は、ページか下の部品がキーを使ったと答えたとき、戻る操作を使ったものとみなします。キーの前に文字の欄にフォーカスがあったときも、使ったものとみなします。キャプチャーの道具の確かめで見ているのは次のとおりです。Basics のメニューとダイアログ、モーダルのボトムシート、フォーカスのある文字の欄は、一回目の戻る操作を使います。何も開いていないときは、どのページでも一回目が渡ります。ページのほかの重なり（サイドシート・ドロワー・検索の画面・ピッカー）は、一つずつは確かめていません。文字の欄は Escape でフォーカスを外しますが、false を返すからです。`TextField::handle` が返すのは、キーを使ったかどうかではなく、文字を送ったかどうかです。押し続けて繰り返される戻る操作は、システムに渡しません。

## 入れ方と動かし方

```
adb install -r build-android-x64/lamina-components-x86_64.apk
adb shell am start -n io.github.capsicum0907.lamina.components/.LaminaActivity
adb logcat -s SDL SDL/APP AndroidRuntime
```

デスクトップで標準エラー出力に出るエラーは、Android では logcat にタグ `SDL/APP` のエラーとして出ます（`lamina_sdl3/report.hpp`）。起動中の例外もここに出ます。フレームの計時のログ（[フレームの計時](app-structure.ja.md#フレームの計時)）は、自分のタグで logcat に info の重さで出ます（`adb logcat -s lamina.frames`）。ギャラリーでは、Basics の Theme にある Frame timing のトグルで、ログと重ね表示が入ります。

x86_64 の APK を動かすエミュレーターは、システムイメージ `android-37.0/google_apis_playstore_ps16k/x86_64` で 1080 × 2400・420 dpi の AVD（幅はおよそ 411 dp で、見本は小さい幅の配置になります）を作り、`emulator -avd <名前> -no-metrics` で起動します。スマートフォンでは USB デバッグかワイヤレスデバッグを有効にして、arm64-v8a の APK を入れます。

## 動くもの

API 37 のエミュレーター（16 KB ページ、GPU は SwiftShader）で確かめました。

- 見本は幅およそ 411 dp の小さい幅の配置で起動して描かれます。画面を横にすると広い幅の配置に変わります。
- タップが効きます。ページのタブ、フィルターのチップ、スイッチ、書体のメニューとその項目、Menu と Dialog のボタン、文字の欄で確かめました。タップの後、ボタンにホバーの色は残りません。
- 配置はステータスバー・ナビゲーションバー・画面の切り欠きにかからず、左右のジェスチャーの余白の下には中身が来ます（[余白](#余白インセット)）。ジェスチャーナビゲーションでは、見本の中身は左端から自分の余白の 24 dp の所から始まります。SDL の安全な領域の内側では 54 dp でした。横向きにして戻すと、余白も変わります。
- 戻る操作で、開いているメニューやダイアログが閉じ、文字の欄からフォーカスが外れます（見本には Escape として届きます）。ランチャーから開いたアプリでは、何も開いていなければ、アプリは後ろへ回ってランチャーが出ます。もう一度開くと、同じプロセスのまま、同じページと配色で出ます（[戻る操作](#戻る操作)）。
- 文字の欄を押すとソフトウェアキーボードが出ます。Gboard で打った文字と `adb shell input text` の文字が入ります。
- マウスのホイールでページとタブがスクロールします（エミュレーターでは `adb shell input mouse scroll`）。
- 指でなぞると、ページは上下に、ページのタブと横に送る時刻ピッカーは左右に動きます。素早くなぞるとはじかれて Compose の減速の曲線で止まり、はじいている間に指を置くと止まります。ボタン・チップ・入力欄の上から始めたなぞりは、それらを押したことにも焦点を移すことにもならずにスクロールします。タップはこれまでどおり押したことになり、スライダーはその上から始めたなぞりについてきます。`adb shell input swipe` と `adb shell input tap` で確かめました（[指でのスクロール](design-tokens.ja.md#指でのスクロール)）。
- 書体「OS fonts」は `/system/fonts` の Noto Sans CJK を読み込みます。
- Viewer のページの二本の指：229 dp 離れた指を 350 dp まで広げると、模様は指の真ん中を中心に 141 % に拡大しました。一本の指でなぞると拡大した模様が動き、91 dp までつまむと収まる大きさより小さくなってから 100 % に戻り、二度たたくと 250 % に拡大し、もう一度で収まる大きさに戻りました。`adb shell input` には二本目の指がなく、このシステムイメージではシェルからの `sendevent` も断られる（`/dev/input` への書き込みが Permission denied）ので、指はエミュレーターのコンソールから送りました（`adb emu event send` で `ABS_MT_SLOT`・`ABS_MT_TRACKING_ID`・タッチ画面の 0〜32767 の範囲の `ABS_MT_POSITION_X/Y` を送り、`SYN_REPORT` は `EV_SYN:0:0` と書きます）。どのスロットにも 0 より大きい `ABS_MT_PRESSURE` が要ります。これがないと Android は二本目の指を触れていないものとみなし、指を一本だけ知らせました。
- ページの間のなぞり（[ページ送り](components/containers.ja.md#ページ送り)）：Basics で `adb shell input swipe` を左へ速く送ると Chips へ移り、右へ約 230 dp ゆっくりなぞると Chips に戻り、右へ速く送ると Basics へ移りました。左へ速く送り続けると一ページずつ進み、Drawers のページでは、モーダルのドロワーの枠をなぞってもページは動かず、枠の上をなぞると Viewer まで進みました。そこでなぞると写真 2 へ送り、エミュレーターのコンソールの二本の指でつまんで広げ（76 dp から 229 dp、もう一度 152 dp から 335 dp）、一本の指で右へなぞると左端まで動き、次に右へなぞるとページ送りへ渡って、写真 1 が収まる大きさで入ってきました。写真 1 で右へ速く送ると Desktop のページへ移り、そこで縦になぞるとページがスクロールしました。
- 48 dp のタッチの範囲：40 dp のボタンの上（Basics の Send）や下（Groups のボタンの組の Copy）を、縁から 3 dp ほど外れてタップしても押せます。4 dp 以上外れると押せません。2 つのフィルターチップの間の 8 dp のすき間をタップすると、近いほうだけが選ばれます。ボタンの組のオーバーフローのボタンのすぐ左をタップすると、メニューが開き、もう一度タップすると閉じます。`adb shell input tap` で確かめました（[タッチの範囲](design-tokens.ja.md#タッチの範囲)）。
- ステータスバーとナビゲーションバーのアイコンは、明るい配色では暗く、暗い配色では明るくなり、暗い配色のボタンに合わせて変わります。システムをダークモードにすると（`adb shell cmd uimode night yes`）、見本は暗い配色で起動します。最初のフレームより前の窓も、暗い背景に明るいアイコンです。
- ホームでアプリを離れて戻ると、見本がもう一度描かれます。
- 画面の端からの戻るのなぞりで、なぞり始めた部品がクリックされません。ジェスチャーナビゲーションでは、システムはなぞりが戻るだと決めるまでタッチをアプリに渡し、決めたところで取り消します。SDL はその取り消しをマウスの離し、つまりクリックにしていました（[アプリの構造](app-structure.ja.md#ターゲットと入力のイベント)）。Basics の最初のスイッチの上の x = 70 px から、テキスト欄にフォーカスがある状態で、ゆっくりの `adb shell input swipe` で確かめました。スイッチはオフのまま、見出しも最後の部品のままで、同じ点をタップするとスイッチはオンになります。
- エミュレーターでは、動き続ける Progress のページの 1 フレームがおよそ 19 ms（毎秒およそ 52 フレーム）でした。`update` の始めから `present` の終わりまでを測った値です。描画は ThorVG が CPU で 1080 × 2400 のテクスチャに描いています。

## まだできないこと

- 指でのスクロールには、端で伸びる動きと入れ子のスクロールがありません。ページの中のリストは、なぞりやはじきの残りをページに渡しません。スクロールの領域が追うのは一本の指だけで、拡大の表示は何本でも追います。指の操作は `adb shell input` とエミュレーターのコンソールで確かめたもので、実機の指では確かめていません。
- Drawers のページでは、モーダルのドロワーの枠が横のなぞりをすべて持ちます。ドロワーが閉じているときの左へのなぞりもです。そのためページは枠の外をなぞったときだけ送れます。
- 予測型の戻る操作はありません。マニフェストは `enableOnBackInvokedCallback="false"` のままです。なので戻る操作はキーとして届き、`OnBackInvokedCallback` はなく、シート・ドロワー・アプリが離れるときの先読みの動きもありません。
- 見本がシステムのダークモードを読むのは起動したときだけです。動いている間に設定を変えても追いません（`SDL_EVENT_SYSTEM_THEME_CHANGED` は読んでいません）。
- ダイアログとメニューは、安全な領域の中ではなく窓全体を基準に置かれます。
- 画面の下のほうの欄はソフトウェアキーボードに隠れることがあります。SDL が知らせるのはキーボードが出ているかどうかだけで、高さは分かりません。アクティビティも IME の余白は読んでいません。
- IME での日本語入力は試していません。SDL は変換中の文字を、変換の途中経過ではなく、確定した文字と削除として渡します。
- APK にはアイコンがありません。作るのは見本だけで、ほかの見本は Android では作りません。
