# リソース

[English](resources.md)

プログラムは、使うアイコン・文字のフォント・ファイルを CMake のファイルに書きます。ビルドは Material Symbols のフォントをそのアイコンだけに絞り、フォントとファイルをバイトの配列としてプログラムに埋め込み、それぞれに型の付いた定数を持つヘッダーを書き出します。そのためプログラムは、隣にフォルダーを置かない一つの実行ファイルとして動きます。Android の `R` クラスにならったもので、アイコンの名前を書き間違えるとビルドが止まります。フォントにその名前のアイコンが無ければ CMake で、ヘッダーにその名前の定数が無ければコンパイラーで止まります。

## CMake

```cmake
lamina_add_resources(<name>
    [NAMESPACE <namespace>]
    [HEADER <path>]
    [ICONS <symbol>...]
    [ICON_FONTS <id>[:<axis>=<value>[,<axis>=<value>...]]...]
    [TEXT_FONTS <font>...]
    [FILES <id>=<path>...])
```

この関数は `cmake/LaminaResources.cmake` にあり、lamina-ui の `CMakeLists.txt` がそれを読み込むので、lamina-ui を `add_subdirectory` で加えたプロジェクトから呼べます。静的ライブラリ `<name>` を作り（アイコンもファイルも無いときはインターフェースライブラリ）、それをリンクするとヘッダーが使えます。

- `NAMESPACE`：定数の C++ の名前空間。既定は `<name>` です。
- `HEADER`：書き出すヘッダーをインクルードするときのパス。既定は `<name>.hpp` です。
- `ICONS`：codepoints のファイルにある綴りのままの Material Symbols の名前（`play_arrow`・`delete`）。その後ろに lamina 自身のアイコン（グローバルなプロパティ `LAMINA_BUILTIN_ICONS`。部品が描く 15 個で、`close`・`check`・`edit`・`date_range`・`arrow_drop_down`・`arrow_drop_up`・`keyboard_arrow_left`・`keyboard_arrow_right`・`calendar_today`・`keyboard`・`schedule`・`more_vert`・`keyboard_arrow_down`・`arrow_upward`・`arrow_downward`。lamina-ui の `CMakeLists.txt` で決めています）が加わります。
- `ICON_FONTS`：これらのアイコンで切り出すアイコンのフォント。既定は `symbols` という一つのフォントです。フォントごとに可変軸を固定でき、固定した軸はそのフォントから無くなります。`filled:FILL=1` は塗りを 1 に固定したフォントで、既定の描き方でアイコンを描く部品に使います。軸を固定しないフォントは塗り・太さ・グレード・光学サイズを残し、描くときに `IconStyle` で決めます。
- `TEXT_FONTS`：変えずに埋め込む、同梱の文字のフォント。`google_sans_flex`・`roboto_flex`・`noto_sans_jp` です（[文字のフォントと書体](#文字のフォントと書体)）。フォントはそれぞれほぼその大きさの分だけ実行ファイルを大きくするので、プログラムは使うものだけを書きます。ほかの名前を書くと、`no text font named '<name>'` と出して構成の段階で止まります。
- `FILES`：そのまま埋め込むほかのファイル（画像・文字のフォント・データ）。相対パスは、呼んだ `CMakeLists.txt` からの相対です。

`NAMESPACE app::res` のとき、ヘッダーには次のものがあります。

| 名前 | 型 | 中身 |
|---|---|---|
| `app::res::icons::<symbol>` | `lamina::icons::Symbol` | アイコンごとに一つ。コードポイントを持ちます |
| `app::res::all_icons` | `std::array<lamina::icons::Symbol, N>` | 書いた順のすべてのアイコンと、その後ろの lamina 自身のアイコン |
| `app::res::fonts::<id>` | `lamina::resources::Resource` | アイコンのフォントごとに一つ |
| `app::res::text_fonts::<font>` | `lamina::resources::Resource` | 文字のフォントごとに一つ |
| `app::res::embedded_fonts` | `std::array<lamina::text::EmbeddedFont, N>` | 文字のフォントごとに、その `lamina::text::BundledFont` と組にしたもの。`typeface_stack` に渡します |
| `app::res::files::<id>` | `lamina::resources::Resource` | ファイルごとに一つ |

C++ の予約語と同じ記号の名前には後ろに `_` を（`delete_`・`class_`）、数字で始まる名前には前に `_` を付けます（`_10k`）。フォントとファイルの id は C++ の識別子でなければなりません。

## 読み込み

- `lamina::resources::Resource`（`lamina/resources/resource.hpp`）は、id を `name` に、バイトを `data`（`std::span<const unsigned char>`）に持ち、`bytes()`（`std::span<const std::byte>`）でも返します。生成されたリソースのバイトは、プログラムが終わるまで残ります。
- `lamina::icons::IconFont::from_resource(resource)` はアイコンのフォントを作ります。バイトがフォントでなければ `std::runtime_error` を、lamina 自身のアイコン（`lamina::builtin::all_icons`）が欠けていれば、つまり `lamina_add_resources` で切り出したものでなければ `std::invalid_argument` を投げます。
- `lamina::text::Font::from_resource(resource, face_index)` は、埋め込んだファイルから文字のフォントを作ります。HarfBuzz は埋め込んだバイトを写さずに読むので、手で作ったリソースはフォントより長く残してください。フォントのファイルはこれまでどおり `Font::from_file` で読めます。OS のフォントと、見本のコマンドラインのフォントはこちらを使います。
- `lamina::builtin`（`lamina/builtin_resources.hpp`。構成の段階で `LAMINA_BUILTIN_ICONS` から書き出します）は lamina 自身のアイコンを持ちます。`builtin::icons::close` はタイトルバーの閉じるアイコンです。

```cmake
lamina_add_resources(player_resources NAMESPACE player::res HEADER player/res.hpp
    ICONS play_arrow pause skip_next
    ICON_FONTS outlined filled:FILL=1
    FILES logo=art/logo.png)
target_link_libraries(player PRIVATE player_resources)
```

```cpp
#include "player/res.hpp"

const lamina::icons::IconFont icons = lamina::icons::IconFont::from_resource(player::res::fonts::filled);
icons.draw(canvas, player::res::icons::play_arrow, {0.0, 0.0}, 24.0, color);
const std::span<const std::byte> logo = player::res::files::logo.bytes();
```

## 文字のフォントと書体

lamina-ui は、三つの可変フォントを google/fonts から変えずに `third_party/` に置いています（コミットとライセンスはそれぞれの README にあります）。

| `TEXT_FONTS` | フォント | lamina-ui が動かす軸 | 実行ファイルが大きくなる分 |
|---|---|---|---|
| `google_sans_flex` | Google Sans Flex | `wght`・`opsz`（1〜144） | 3 910 784 バイト（3.7 MiB） |
| `roboto_flex` | Roboto Flex | `wght`・`opsz`（8〜144） | 1 787 292 バイト（1.7 MiB） |
| `noto_sans_jp` | Noto Sans JP | `wght` | 9 589 900 バイト（9.1 MiB） |

「Google Sans Flex」は Google LLC の商標です。lamina-ui は Google と関係がなく、推奨も受けていません。`third_party/google-sans-flex/TRADEMARKS.md` に従い、このフォントは変えずにだけ埋め込みます。

書体（`lamina::text::Typeface`、`lamina/text/typeface.hpp`）は、プログラムがすべての文字のフォントを選ぶ一か所です。表 `text::typefaces` にあります。

| `Typeface` | フォント（代わりを探す順） | キー |
|---|---|---|
| `google_sans_flex`（`default_typeface`） | Google Sans Flex、Noto Sans JP、OS のフォント | `google-sans-flex` |
| `roboto_flex` | Roboto Flex、Noto Sans JP、OS のフォント | `roboto-flex` |
| `system` | OS のフォントだけ。同梱のフォントが無かったころと同じです | `os` |

文字ごとに、スタックの中でその文字を持つ最初のフォントを使います（`FontStack::font_for`）。ラテン文字はラテン文字のフォント、かなと漢字は Noto Sans JP（どちらのラテン文字のフォントも持っていません）、それ以外は OS のフォント（`lamina_sdl3/system_fonts.hpp`）です。三つのうち Material 自身の見た目にいちばん近いものを既定にしています。

- `text::typeface_stack(typeface, embedded, system_fonts)` は、埋め込んだフォント（`app::res::embedded_fonts`）と OS のフォントのスタックから `FontStack` を作ります。同梱の書体は OS のフォントを最後の代わりとして後ろに足し、OS のフォントは空でもかまいません。書体のフォントが埋め込まれていなければ、別のフォントで描くのではなく、そのフォントの名前を出して `std::invalid_argument` を投げます。`typeface_available(typeface, embedded)` で前もって分かります。
- `sdl3::typeface_stack(typeface, embedded)` は、`system_fallback_stack()` が見つけた OS のフォントで同じことをします。`Typeface::system` のときは `system_font_stack()` そのもので、OS のフォントが無ければ例外を投げます。同梱の書体は OS のフォントが無くても動きます。
- `text::typeface_from_key` はキーを読みます。components の見本の `--font <key>` がこれを使います。

型のスケールは、何も設定しなくても軸に届きます。`wght` は役割の太さなので、Expressive の強調の役割は太さだけで太くなります（500、Title M・Title S・ラベルは 700）。`opsz` は、その軸を持つフォントでは文字の大きさ（dp、整数に丸めたもの）で、CSS の `font-optical-sizing: auto` と同じです。Display L は `opsz` 57、Label S は 11 で描きます。OS のフォントのように軸の無いフォントは、これまでどおりに描きます。`Font::with_variations({{text::axes::roundness, 100.0f}})` は、ほかの軸（`ROND`・`slnt`・`wdth`・`GRAD` など）を固定した同じフォントを返します。`wght` や `opsz` を固定すると、スタイルの値の代わりにそれを使います。`Font::variations(instance)` は、あるスタイルで描くときの軸の値を返します。

```cmake
lamina_add_resources(app_resources NAMESPACE app::res HEADER app/res.hpp
    TEXT_FONTS google_sans_flex noto_sans_jp)
```

```cpp
const lamina::text::FontStack fonts = lamina::sdl3::typeface_stack(lamina::text::Typeface::google_sans_flex, app::res::embedded_fonts);
```

## いつ動き、何が要るか

- 構成の段階で、アイコンの名前を `LAMINA_SYMBOLS_CODEPOINTS` から引き、ヘッダーを書き出します（中身が変わったときだけ書き直します）。知らない名前があると、`no icon named '<name>'` と出して構成の段階で止まります。
- ビルドの段階で、`tools/icon-subset/subset.py` が `LAMINA_SYMBOLS_FONT` からアイコンのフォントを切り出し、`cmake/lamina_embed.cmake` がフォントとファイルを C++ の配列として書き出します。それぞれ、入力（アイコンとフォントの一覧・元のフォント・スクリプト・埋め込むファイル）が変わったときだけやり直します。文字のフォントは Python を使わずそのまま埋め込みます。Noto Sans JP は、48 MB のソースファイルに書き出すのに約 12 秒、コンパイルに約 7 秒かかります。ビルドの途中で何かをダウンロードすることはありません。
- アイコンのフォントを切り出すには、fontTools と brotli の入った Python 3 が要ります（brotli は WOFF2 の元のフォントを読むのに使います）：`python -m pip install -r tools/icon-subset/requirements.txt`。CMake はアイコンを書いたターゲットがあるときに Python を探し（Windows ではレジストリより先に `PATH` にあるもの。`Python3_EXECUTABLE` で選べます）、二つを読み込めなければこのコマンドを示して止まります。`FILES` だけのターゲットには Python は要りません。
- `LAMINA_SYMBOLS_FONT` と `LAMINA_SYMBOLS_CODEPOINTS` はキャッシュ変数です。既定では `third_party/material-symbols/` に置いた Material Symbols Rounded を指し、codepoints のファイルと組にすれば別の書体の Material Symbols も指せます。
- 一つ切り出すのに約 2 秒かかり、そのほとんどは 5.4 MB の元のフォントを読む時間です。埋め込んだフォントは、ほぼその大きさの分だけ実行ファイルを大きくします。cadenza の 29 個のアイコンは 50.9 KB、塗りを固定したものは 45.9 KB です。
- 配列をソースコードとして書き出すのは、MSVC（19.44、Visual Studio 2022）に C++ の `#embed` が無いからです。

## テスト

`tests/resources_test.cpp` は、定数が codepoints のファイルと合うこと（予約語と数字で始まる名前を含む）、`all_icons` の順番、どのアイコンもどの切り出しにも入っていること、軸を固定した切り出しが、可変フォントをその軸の値で描いたものと同じに描けること（固定するときの丸めのため、どの色の成分も 255 のうち 8 までの差は許します）、ファイルを埋め込んだバイト、そこから文字のフォントを読めることを確かめます。`tests/typeface_test.cpp` は、書体の表、書体ごとの代わりを探す順、OS のフォントの書体が OS のスタックそのものであること、無いフォントを知らせること、`TEXT_FONTS` が Roboto Flex をバイト単位でそのまま埋め込み `embedded_fonts` に並べること、型のスケールのどの役割もその太さと大きさで描くことを確かめます。ほかに CTest の三つのテストが、ビルドのエラーを確かめます。`lamina_resources_unknown_icon` は書き間違えた名前を引かせ、`lamina_resources_unknown_text_font` は書き間違えた文字のフォントを引かせ、`lamina_resources_misspelled_constant` は無い定数を使う `tests/resources/misspelled.cpp` をビルドして、コンパイラーのエラーを待ちます。
