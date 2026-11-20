# リソース

[English](resources.md)

プログラムは、使うアイコンとファイルを CMake のファイルに書きます。ビルドは Material Symbols のフォントをそのアイコンだけに絞り、フォントとファイルをバイトの配列としてプログラムに埋め込み、それぞれに型の付いた定数を持つヘッダーを書き出します。そのためプログラムは、隣にフォルダーを置かない一つの実行ファイルとして動きます。Android の `R` クラスにならったもので、アイコンの名前を書き間違えるとビルドが止まります。フォントにその名前のアイコンが無ければ CMake で、ヘッダーにその名前の定数が無ければコンパイラーで止まります。

## CMake

```cmake
lamina_add_resources(<name>
    [NAMESPACE <namespace>]
    [HEADER <path>]
    [ICONS <symbol>...]
    [ICON_FONTS <id>[:<axis>=<value>[,<axis>=<value>...]]...]
    [FILES <id>=<path>...])
```

この関数は `cmake/LaminaResources.cmake` にあり、lamina-ui の `CMakeLists.txt` がそれを読み込むので、lamina-ui を `add_subdirectory` で加えたプロジェクトから呼べます。静的ライブラリ `<name>` を作り（アイコンもファイルも無いときはインターフェースライブラリ）、それをリンクするとヘッダーが使えます。

- `NAMESPACE`：定数の C++ の名前空間。既定は `<name>` です。
- `HEADER`：書き出すヘッダーをインクルードするときのパス。既定は `<name>.hpp` です。
- `ICONS`：codepoints のファイルにある綴りのままの Material Symbols の名前（`play_arrow`・`delete`）。その後ろに lamina 自身のアイコン（グローバルなプロパティ `LAMINA_BUILTIN_ICONS`。今はタイトルバーの `close` だけ）が加わります。
- `ICON_FONTS`：これらのアイコンで切り出すアイコンのフォント。既定は `symbols` という一つのフォントです。フォントごとに可変軸を固定でき、固定した軸はそのフォントから無くなります。`filled:FILL=1` は塗りを 1 に固定したフォントで、既定の描き方でアイコンを描く部品に使います。軸を固定しないフォントは塗り・太さ・グレード・光学サイズを残し、描くときに `IconStyle` で決めます。
- `FILES`：そのまま埋め込むほかのファイル（画像・文字のフォント・データ）。相対パスは、呼んだ `CMakeLists.txt` からの相対です。

`NAMESPACE app::res` のとき、ヘッダーには次のものがあります。

| 名前 | 型 | 中身 |
|---|---|---|
| `app::res::icons::<symbol>` | `lamina::icons::Symbol` | アイコンごとに一つ。コードポイントを持ちます |
| `app::res::all_icons` | `std::array<lamina::icons::Symbol, N>` | 書いた順のすべてのアイコンと、その後ろの lamina 自身のアイコン |
| `app::res::fonts::<id>` | `lamina::resources::Resource` | アイコンのフォントごとに一つ |
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

## いつ動き、何が要るか

- 構成の段階で、アイコンの名前を `LAMINA_SYMBOLS_CODEPOINTS` から引き、ヘッダーを書き出します（中身が変わったときだけ書き直します）。知らない名前があると、`no icon named '<name>'` と出して構成の段階で止まります。
- ビルドの段階で、`tools/icon-subset/subset.py` が `LAMINA_SYMBOLS_FONT` からアイコンのフォントを切り出し、`cmake/lamina_embed.cmake` がフォントとファイルを C++ の配列として書き出します。それぞれ、入力（アイコンとフォントの一覧・元のフォント・スクリプト・埋め込むファイル）が変わったときだけやり直します。ビルドの途中で何かをダウンロードすることはありません。
- アイコンのフォントを切り出すには、fontTools と brotli の入った Python 3 が要ります（brotli は WOFF2 の元のフォントを読むのに使います）：`python -m pip install -r tools/icon-subset/requirements.txt`。CMake はアイコンを書いたターゲットがあるときに Python を探し（Windows ではレジストリより先に `PATH` にあるもの。`Python3_EXECUTABLE` で選べます）、二つを読み込めなければこのコマンドを示して止まります。`FILES` だけのターゲットには Python は要りません。
- `LAMINA_SYMBOLS_FONT` と `LAMINA_SYMBOLS_CODEPOINTS` はキャッシュ変数です。既定では `third_party/material-symbols/` に置いた Material Symbols Rounded を指し、codepoints のファイルと組にすれば別の書体の Material Symbols も指せます。
- 一つ切り出すのに約 2 秒かかり、そのほとんどは 5.4 MB の元のフォントを読む時間です。埋め込んだフォントは、ほぼその大きさの分だけ実行ファイルを大きくします。cadenza の 29 個のアイコンは 50.9 KB、塗りを固定したものは 45.9 KB です。
- 配列をソースコードとして書き出すのは、MSVC（19.44、Visual Studio 2022）に C++ の `#embed` が無いからです。

## テスト

`tests/resources_test.cpp` は、定数が codepoints のファイルと合うこと（予約語と数字で始まる名前を含む）、`all_icons` の順番、どのアイコンもどの切り出しにも入っていること、軸を固定した切り出しが、可変フォントをその軸の値で描いたものと同じに描けること（固定するときの丸めのため、どの色の成分も 255 のうち 8 までの差は許します）、ファイルを埋め込んだバイト、そこから文字のフォントを読めることを確かめます。ほかに CTest の二つのテストが、二つのビルドのエラーを確かめます。`lamina_resources_unknown_icon` は書き間違えた名前を引かせ、`lamina_resources_misspelled_constant` は無い定数を使う `tests/resources/misspelled.cpp` をビルドして、コンパイラーのエラーを待ちます。
