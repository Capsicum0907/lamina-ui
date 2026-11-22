# はじめに

[English](getting-started.md) · [README](../README.ja.md#資料)

lamina-ui と見本のビルドのしかたと、ほかの CMake のプロジェクトから使う方法です。

## 要るものとビルド

Visual Studio 2022、vcpkg（`VCPKG_ROOT` を設定）、CMake 3.25 以上が要ります。アイコンのフォントを切り出す見本とテストには、fontTools と brotli の入った Python 3 も要ります（`python -m pip install -r tools/icon-subset/requirements.txt`）。

```
cmake --preset default
cmake --build --preset release
ctest --preset release
```

プリセット `default` は、Visual Studio 17 2022 のジェネレーター、`VCPKG_ROOT` の vcpkg のツールチェーン、トリプレット `x64-windows-static`、静的な MSVC のランタイムを使い、`LAMINA_BUILD_TESTS` を有効にします。ビルドの先は `build/` です。vcpkg は `vcpkg.json` に従って HarfBuzz・SDL3・ThorVG・abseil を入れ、テストのために doctest と GoogleTest を入れます。

## CMake のオプション

CMake のオプション `LAMINA_BUILD_THORVG` と `LAMINA_BUILD_SDL3`（どちらも既定で有効）で二つのバックエンドを、`LAMINA_BUILD_EXAMPLES`・`LAMINA_BUILD_TOOLS`（どちらも有効）と `LAMINA_BUILD_TESTS`（プリセットで有効）でそれ以外をビルドします。見本には両方のバックエンドが、テストには ThorVG が要ります。これに合わない組み合わせでは、構成の段階でエラーになって止まります。両方のバックエンドを外しても、本体のライブラリと `tools/` の道具はビルドできます。

`LAMINA_CLIENT_TITLE_BAR`（既定で有効）は、Windows で lamina が自分で描くタイトルバーをビルドします。[自前で描くタイトルバー](custom-title-bar.ja.md)を見てください。

## 部品の一覧を動かす

`build/Release/components.exe` は、すべての部品を十五のページに並べます。ページは見出しの下のタブで選びます。タブの上でホイールを回すとタブが横に送られ、タブにフォーカスがあるときは左右の矢印で移れます。`--font roboto-flex` か `--font os` でほかの書体で始まり、`--title-bar` で自前で描くタイトルバーを使います。各ページの中身は[見本](examples.ja.md#components部品の一覧)にあります。

ほかの見本は[見本](examples.ja.md)にあります。

## ほかの CMake のプロジェクトから使う

lamina-ui にはインストールの規則がありません。同じ vcpkg のツールチェーンを使うプロジェクトから `add_subdirectory` で取り込み、要るターゲットをリンクします。

```cmake
set(LAMINA_BUILD_EXAMPLES OFF)
set(LAMINA_BUILD_TOOLS OFF)
add_subdirectory(lamina-ui)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE lamina::lamina lamina::thorvg lamina::sdl3)
```

`lamina::lamina` は本体（トークン・色・描画面の口・文字・図形・レイアウト・入力のイベント・部品）で、SDL3 も ThorVG も使いません。`lamina::thorvg` は ThorVG で描く描画面、`lamina::sdl3` は窓・設定のフォルダー・OS のフォントです。三つの関係は[アプリの組み立て](app-structure.ja.md#ターゲットと入力のイベント)にあります。

## 埋め込むアイコンとフォント

`cmake/LaminaResources.cmake`・`lamina/resources/resource.hpp`：Android の `R` クラスのように実行ファイルに埋め込むリソース。`lamina_add_resources(<name> ICONS ... ICON_FONTS ... TEXT_FONTS ... FILES ...)` は、ビルドの段階でアイコンのフォントを書いた記号だけに切り出し、それと、書いた同梱の文字のフォント（Google Sans Flex・Roboto Flex・Noto Sans JP）と、ほかのファイルをバイトの配列として埋め込み、型の付いた定数のヘッダー（`app::res::icons::play_arrow`・`app::res::fonts::symbols`・`app::res::files::logo`）を書き出します。アイコンの名前を書き間違えるとビルドが止まります。`IconFont::from_resource` と `text::Font::from_resource` は、ファイルを使わずにそこから読みます。見本とテストもこれを使い、ビルドしたプログラムは隣にフォルダーを置かずに動きます。詳しくは [docs/resources.ja.md](resources.ja.md)。

生成されるヘッダー、読み込みの関数、各段階に要るものまで含めた説明は[リソース](resources.ja.md)にあります。文字のフォントと、それから作る書体は[文字とフォント](text-and-fonts.ja.md)にあります。
