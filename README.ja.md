# lamina-ui

[English](README.md)

Material 3 Expressive を手本にした、C++20 のデスクトップ向け UI ライブラリです。描画は ThorVG、字形の並べ方は HarfBuzz で行い、SDL3 の窓で動きます。部品・トークンの値・動きのばねは、Google が公開している Material 3 の実装から読み取っています。

lamina-ui は Google とは関係がなく、Google の承認も受けていません。Material Design は Google のデザインの体系で、lamina-ui はその公開されている指針とトークンの値に従っています。

<img src="docs/images/gallery-basics.png" alt="部品の一覧の Basics のページ：ボタン・アイコンボタン・FAB・選ぶ部品・入力欄・タブ・文字・アイコン" height="300"> <img src="docs/images/gallery-adaptive-dark.png" alt="ダークテーマの Adaptive のページ：横のナビゲーションとリストと詳細の足場を持つメールのアプリ" height="300">

## 状況

バージョン 0.2.0 です。API はまだ固まっておらず、0.x のどの版でも変わることがあります。ビルドして動かしたのは Windows 11 と Visual Studio 2022 だけです。ほかの OS 向けのコードも書いてありますが、ビルドしたことはありません。拡大率の違うモニターと Windows 10 でも確かめていません。

## すぐに試す

Visual Studio 2022、vcpkg（`VCPKG_ROOT` を設定）、CMake 3.25 以上が要ります。アイコンのフォントを切り出す見本とテストには、fontTools と brotli の入った Python 3 も要ります（`python -m pip install -r tools/icon-subset/requirements.txt`）。

```
cmake --preset default
cmake --build --preset release
ctest --preset release
build/Release/components.exe
```

`components.exe` は、すべての部品を並べた一覧です。見出しの下のタブで十五のページを選びます。詳しくは[はじめに](docs/getting-started.ja.md)と[見本](docs/examples.ja.md)にあります。

## いちばん小さなプログラム

ボタンが一つある窓で、クリックするとコンソールに書き出します。`lamina::lamina`・`lamina::thorvg`・`lamina::sdl3` をリンクします。フォントのファイルをコマンドラインで渡すか、渡さなければ OS のフォントから日本語の画面向けのフォントを探します。

```cpp
#include <cstdio>
#include <memory>

#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/system_fonts.hpp"
#include "lamina_thorvg/canvas.hpp"

using namespace lamina;

int main(int argc, char** argv) {
    const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
    const color::ColorScheme scheme = color::spec_2025().build(color::default_scheme_options);

    widgets::WidgetSet set;
    auto& hello = set.emplace_with<widgets::Button>(
        [](bool clicked) {
            if (clicked) {
                std::puts("Hello");
            }
        },
        widgets::ButtonOptions{.label = "Say hello"}, fonts);

    auto page = layout::column({widgets::node(hello)});
    page->padding(layout::Insets::all(24.0));

    auto update = [&](const sdl3::Frame& frame) {
        layout::run(*page, {0.0, 0.0, frame.width_dp, frame.height_dp});
        for (const auto& e : frame.pointer) {
            set.handle(e);
        }
        return scheme[color::Role::surface];
    };
    auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
        set.step(frame.dt_seconds);
        set.paint(canvas, scheme);
        return set.animating();
    };
    sdl3::run_app({"hello", 480, 320}, std::make_unique<thorvg::Canvas>(1, 1), update, paint);
}
```

繰り返し・入力・部品の関係は[アプリの組み立て](docs/app-structure.ja.md)にあります。

## 資料

| ページ | 内容 |
| --- | --- |
| [はじめに](docs/getting-started.ja.md) | 要るもの・ビルド・CMake のオプション・ほかのプロジェクトから使う方法・アイコンとフォントの埋め込み |
| [見本](docs/examples.ja.md) | 部品の一覧とほかの見本、その動かし方 |
| [アプリの組み立て](docs/app-structure.ja.md) | ターゲット・繰り返し・入力のイベント・窓と描画面・`WidgetSet`・重なるもの・フォーカス・読み上げ・ショートカット |
| [レイアウトと幅への対応](docs/layout-and-adaptive.ja.md) | 行と列・窓の大きさの区分・ナビゲーションスイート・ペインの足場・分割ペイン・密度 |
| [部品](docs/components.ja.md) | 操作・選択・入力・入れ物・ナビゲーション・知らせ・メニュー・データの表・日付と時刻 |
| [文字とフォント](docs/text-and-fonts.ja.md) | 字形の並べ方と行の折り返し・書体・OS のフォント・アイコン・日本語の入力 |
| [テーマとトークン](docs/theming.ja.md) | 配色・形・文字・高さ・動きとばねの模型 |
| [リソース](docs/resources.ja.md) | `lamina_add_resources`・生成されるヘッダー・アイコンとファイルと文字のフォントの埋め込み |
| [テスト](docs/testing.ja.md) | 単体テストと、部品の一覧の窓なしの記録 |
| [Material 3 との違い](docs/differences-from-m3.ja.md) | lamina-ui が自分の値やふるまいを使うところ |
| [デザイントークン](docs/design-tokens.ja.md) | すべての値と、そのソースのファイルとコミット |
| [自前で描くタイトルバー](docs/custom-title-bar.ja.md) | lamina が自分で描く Windows のタイトルバー |
| [描画の比較](docs/renderer-comparison.ja.md) | Blend2D ではなく ThorVG を選んだ理由 |

## 出典

`tests/fixtures/fonts/` のテスト用フォントは、Noto Sans JP と Oswald の一部を取り出して名前を変えたもので、SIL Open Font License 1.1 に従います（ライセンスの文書を同じ場所に置いています）。

`third_party/google-sans-flex/`・`third_party/roboto-flex/`・`third_party/noto-sans-jp/` には、[google/fonts](https://github.com/google/fonts) の Google Sans Flex・Roboto Flex・Noto Sans JP を変えずに置いています。SIL Open Font License 1.1 に従い（ライセンスのファイルはそれぞれの中にあります）、プログラムにも変えずに埋め込みます。「Google Sans Flex」は Google LLC の商標です（`third_party/google-sans-flex/TRADEMARKS.md`）。lamina-ui は Google と関係がありません。

`third_party/material-symbols/` には、Google の Material Symbols Rounded（可変フォント）を変えずに置いています。[google/material-design-icons](https://github.com/google/material-design-icons) のコミット `10ecc14790612ca8fd164e3f4ffe9c16fc50d5b7` から取り、Apache License 2.0 に従います（`third_party/material-symbols/LICENSE`）。プログラムに埋め込むアイコンのフォントはこれを変更したものです。`lamina_add_resources` が書いた記号だけに絞り、合字の表とヒンティングを除き、求められた軸を固定します。ほかの可変軸と名前の表は残しています。

トークンの値は、Google が Apache-2.0 で公開しているソースから取っています：androidx（Jetpack Compose の Material 3 と graphics-shapes）、material-components-android、material-web、material-color-utilities。密度の高さと最小は MDC Web（material-components-web、MIT ライセンス）から取っています。ファイルとコミットは [docs/design-tokens.ja.md](docs/design-tokens.ja.md) にあります。

## ライセンス

lamina-ui は [Apache License 2.0](LICENSE) で公開しています。含めているよそのコード・フォント・データは [NOTICE](NOTICE) にまとめています。
