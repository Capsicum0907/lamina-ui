# lamina-ui

[English](README.md)

Material 3 Expressive を手本にした、SDL3 の上で動く C++ のデスクトップ向け UI ライブラリです。

lamina-ui は Google とは関係がなく、Google の承認も受けていません。Material Design は Google のデザインの体系で、lamina-ui はその公開されている指針とトークンの値に従っています。

## 現状

作り始めたところです。今あるのは、設計の値の層・2D の描画・文字・形・アイコン・SDL3 の窓・配置の仕組みと、部品（ボタン・アイコンボタン・FAB・スイッチ・チェックボックス・ラジオボタン・入力欄・スライダー・タブ・ナビゲーションバー・ナビゲーションレール・メニュー（従来のものと、まとまりに分けたもの）・ダイアログ）です。どの見本も、部品の位置を配置の仕組みで決めています。

- `lamina/tokens/motion.hpp`：standard と expressive の二つの動きの体系の、ばね・緩急・時間のトークン。どちらの体系も、Compose の `MotionScheme` と同じく6つのばねに役割の名前を付けています（`fast_spatial()`・`default_spatial()`・`slow_spatial()`・`fast_effects()`・`default_effects()`・`slow_effects()`）。部品はどれも `tokens::motion_scheme` からばねを取り、これは expressive の体系を指しています。
- `lamina/motion/spring.hpp`：減衰するばね。解を式で直接求めます。`advance` は値を目標に向けて好きな時間だけ進めます。時間をどうコマに分けても結果は変わりません。`is_settled` は動きが止まったかを判定します。

- `lamina/tokens/shape.hpp`・`typography.hpp`・`elevation.hpp`・`interaction.hpp`：角の丸めの段階、文字の段階（強調の太さを含む）、高さの段階、状態の重ねの不透明度と押せる範囲の最小の大きさ。これらの基礎のトークンはすべての部品で共有します。一つの部品のトークンは、その部品と同じ名前の `tokens/` のファイルにあります（`widgets/grouped_menu.hpp` には `tokens/grouped_menu.hpp`）。
- `lamina/color/scheme.hpp`：1 色から作る配色。`spec_2021().build(options)` と `spec_2025().build(options)` は、元の色・ライトかダークか・配色の種類（TonalSpot・Expressive・Vibrant など）・コントラストの段階から、2021 年または 2025 年の色の仕様で 53 個の色の役割（primary・primary-dim・surface-container・outline など）を返します。2021 年の仕様は Google の material-color-utilities（`third_party/`）が計算します。2025 年の仕様（`color/spec_2025.cpp`）は、同じライブラリの Java 版の `ColorSpec2025` をスマートフォン向けの部分だけ移したもので、C++ 版の HCT とコントラストの関数の上に作っています。Java 版との照合は `tools/color-dump/` で行います。

- `lamina/render/canvas.hpp`：曲線・角丸の四角・影・座標変換の積み重ねを持つ 2D の描画面。描くのは ThorVG です（`lamina::thorvg`）。ThorVG を選んだ理由は [docs/renderer-comparison.ja.md](docs/renderer-comparison.ja.md) にあります。
- `lamina_sdl3/window.hpp`：描画面を映す SDL3 の窓。ポインターの位置を dp に直し、何も動いていない間はイベントを待って止まります（`lamina::sdl3`）。
- `lamina_sdl3/app.hpp`：`run_app` がその窓でフレームの繰り返しを回します。毎フレーム、フレームの情報（入力・大きさ・経過時間）を渡して `update` を呼び、`update` が返す背景色で描画面を塗りつぶして dp の縮尺に合わせ、`paint` を呼んで描かせてから画面に出します。`paint` はまだ動いているものがあるかどうかを返し、何も動いていなければ次のフレームはイベントを待ちます。窓を `WindowOptions` から作る形と、作ってある `Window` を受け取る形があり、コールバックの中で窓を使うとき（クリップボード、`set_text_input`、`wake_after`）は後者を使います。`run_app` は例外を捕まえません。例外は呼び出し側に届き、報告のしかたは呼び出し側が決めます。フォントの読み込みのように繰り返しの前の準備で起きた例外も、同じ処理で扱えます。自分で繰り返しを書くときは、これまでどおり `Window` の関数をそのまま使えます。
- `lamina_sdl3/resources.hpp`：実行ファイルの隣の `assets/` フォルダーからファイルを探し、無ければ親のフォルダーを順にさかのぼって探します。そのため、ビルドしたその場所からでも、フォルダーごと別の場所に写しても動きます。`bundled_icon_font()` はそこからアイコンのフォントを読みます。
- `lamina_sdl3/system_fonts.hpp`：OS のフォントから、日本語の画面の文字に使うフォントを探します。フォントのフォルダーは環境変数（`WINDIR`・`SystemRoot`・`LOCALAPPDATA`）から決め、候補を Noto Sans JP・Yu Gothic UI・Meiryo UI・BIZ UDPゴシック・MS UI Gothic の順に試します。どちらの一覧もヘッダーに置いたデータです。一つも見つからなければ、試したパスを全部並べたエラーを投げます。`font_stack_from_arguments` は、コマンドラインにフォントのファイルが渡されていれば、そちらを使います。
- `lamina/text/font.hpp`・`layout.hpp`：フォントと、1 行の文字・折り返す段落。文字の並べ方と字形の輪郭はどちらも HarfBuzz から取り、輪郭はほかの図形と同じく描画面で塗るので、FreeType は使いません。可変フォントは文字の段階の太さに合わせて太さを変え、フォントに無い文字は `FontStack` の次のフォントで補います。段落は、欧文では空白で、日本語では文字の間で折り返し、。や 」などの約物は行頭に来ないようにします（`text/line_break.hpp`）。単語ごとの移動や選択で使う単語は、同じ種類の文字（英字と数字・漢字・ひらがな・カタカナ・約物）の連なりです（`text/word_boundary.hpp`）。折り返しと単語の種類は、Unicode の範囲を同じ一つの表から取ります（`text/script.hpp`）。入力欄の編集の中身（文字列・選択・IME の変換中の文字・文字や単語や行ごとの移動）は `text::EditBuffer`（`text/edit_buffer.hpp`）で、フォントも描画面も使いません。行は文字の位置の範囲を並べた `text::LineMap`（`text/line_map.hpp`）として受け取り、横の位置は `text::CaretGeometry` という口を通して尋ねます。この口は、入力欄が並べ終えた行を使って実装しています。
- `lamina/widgets/button.hpp`：Material 3 Expressive のボタン。5 種類（filled・tonal・outlined・elevated・text）・5 段階の大きさ・2 種類の形。押すと、Compose と同じく効果のばねで角が押したときの形に変わり、マウスを乗せたときと押したときには状態の重ねと高さが変わります。`ButtonOptions::leading_icon` でラベルの前にアイコンを置けます。大きさと間隔は大きさのトークンに従います。`cancel_interaction()`（`IconButton`・`Menu`・`GroupedMenu` にもあります）は、ポインターの出来事なしにマウスを乗せた状態と押した状態を取り消すので、押している途中のものはクリックになりません。ダイアログは閉じるときに自分のボタンにこれを使います。`build/Release/buttons.exe` で全部を並べて見られます。
- `lamina/icons/`：フォントとしての Material Symbols。`symbols.hpp` が 47 個の記号に名前を付け（`Symbol::add`・`Symbol::delete_` など）、`IconFont` は `assets/icons/` の切り出したフォントを読み、HarfBuzz から取った輪郭で記号を好きな大きさと色で描きます。フォントの可変軸（塗り・太さ・グレード・光学サイズ）も指定でき、光学サイズは指定しなければ描く大きさに合わせます。`text::Font` には `glyph_for` と、軸の値を受け取る `glyph_outline_em` を足しました。`tools/icon-subset/subset.py` は、一つの名前の一覧からフォントと `symbols.hpp` を作り直します。
- `lamina/widgets/icon_button.hpp`：アイコンボタン。4 種類（filled・tonal・outlined・standard）・5 段階の大きさ・2 種類の形・3 種類の幅（narrow・uniform・wide）。ボタンと同じく、押すと効果のばねで角が変わります。切り替えのアイコンボタンは、オンのときにも形と色が変わります。
- `lamina/widgets/fab.hpp`：フローティングアクションボタン。3 段階の大きさ（FAB・medium・large）で、地は primary-container か secondary-container です。高さは段階 3 で、マウスを乗せると段階 4 になります。
- `lamina/widgets/switch.hpp`：スイッチ。つまみは速い空間のばねで滑りながら大きさを変え（オフで 16 dp、オンかアイコンつきで 24 dp、押している間は 28 dp）、expressive では少し行き過ぎて戻ります。つまみには、オンとオフそれぞれのアイコンを載せられます。
- `lamina/widgets/checkbox.hpp`：チェックボックス。オフ・オン・中間の 3 状態です。チェックの印は空間のばねで線の長さに沿って描かれ、中間の状態では横線へ寄っていきます。オフにすると、Compose と同じく箱の色が薄れる間に 100 ms 遅れて印が消えます。
- `lamina/widgets/radio_button.hpp`：ラジオボタン。点は速い空間のばねで大きくなります。
- `lamina/widgets/slider.hpp`：Expressive のスライダー。4 × 44 dp のつまみが 16 dp の通り道を、動いた側（primary）と残りの側（secondary container）に分けます。つまみの両側には 6 dp の隙間があり、つまみの側の角は 2 dp です。終わりには 4 dp の点が付きます。スライダーのどこかを押すかドラッグすると、ポインターの位置から値が決まります。ドラッグの途中でポインターがスライダーの外に出ても追います。押している間、つまみは Compose と同じくすぐに 2 dp に細くなります。`SliderOptions::steps` を指定すると、値は等しい間隔の目盛りに寄ります。`handle(KeyEvent)` は左右の矢印（1 段、段が無ければ 1 %）・`Home`・`End` で値を動かします。どのスライダーにキーを渡すかはアプリが決めます。
- `lamina/widgets/tabs.hpp`：primary と secondary のタブの列（`TabRow`）。タブは幅を等しく分けます。ラベルの上（このとき列は 72 dp の高さ）か前（`TabIconPosition::leading`）にアイコンを置けます。印は標準の空間のばねで選んだタブへ滑ります。primary のタブではラベルと同じ幅（少なくとも 24 dp）、secondary のタブではタブの幅いっぱいです。ラベルの色は、Compose と同じく、現れるときは標準の効果のばね、消えるときは速い効果のばねで移ります。
- `lamina/widgets/navigation_bar.hpp`：Expressive の短いナビゲーションバー。高さ 64 dp で、項目は同じ幅に並びます。選んだ項目の印（56 × 32 dp）は、標準の空間のばねで中央から広がりながら現れます。マウスを乗せたときと押したときは、印の形に状態の重ねが出ます。`NavigationItem::selected_style`（`lamina/widgets/navigation_item.hpp`。レールと共通です）で、選んだときのアイコンに別の Material Symbols の軸の値（たとえば塗り 1）を与えられます。アイコンは色と一緒にその値へ移ります。
- `lamina/widgets/menu.hpp`：ドロップダウンメニュー。`open(anchor, window)` は元の部品の下に置き、収まらなければ上か窓の端に置きます。メニューは元の部品から広がるように、速い空間のばねで 0.8 倍から大きくなり、速い効果のばねで現れます。項目には前に置くアイコンを付けられ、マウスを乗せたときと押したときに状態の重ねが出ます。項目を選ぶか、メニューの外を押すか、`Escape` を押すと閉じます。`handle` は選んだ項目と、そのイベントを使ったかどうかを返すので、イベントは後ろの部品に届きません。
- `lamina/widgets/grouped_menu.hpp`：Expressive の、まとまりに分けたメニュー（`DropdownMenuPopup` と `DropdownMenuGroup`）。まとまりはそれぞれが一枚の面（色は surface-container-low、影は段階 2）で、次のまとまりと 2 dp 離れます。上に見出しと区切り線を置けます。最初と最後のまとまりは外側の角が 16 dp、内側の角が 8 dp です。マウスが一度乗ってから離れたまとまりは、角が 8 dp になります。項目は左右に 4 dp 引っ込み、高さは 44 dp 以上で、まとまりの最初と最後の項目は外側の角が 12 dp です。選んだ項目は色が tertiary-container になり、角が 12 dp に変わります。形は速い空間のばねで、色は速い効果のばねで動きます。押したときの動きは `MenuGroup::selection` で決めます。`none` は項目を知らせてメニューを閉じ、`single` はまとまりの中で一つを選び、`multiple` は項目の選択を切り替えます。後の二つではメニューは開いたままです。項目には後ろに置くアイコンと、選んだときの前のアイコンを付けられます。置き方は従来のメニューと同じですが、窓の左右の端からは 8 dp 離します。
- `lamina/widgets/dialog.hpp`：基本のダイアログ。窓全体を幕（色は scrim、不透明度 32 %）で覆い、面（幅 280 から 560 dp・角 28 dp）に見出し・本文・終わりの側にそろえたテキストボタンを `lamina::layout` で並べます。`DialogOptions::icon` を与えると見出しの上に 24 dp のアイコン（色は secondary）を置き、見出しは中央にそろえます。ボタンは終わりの側から並び、一行に収まらないときは、確定のボタンが一行目に残り、閉じるボタンが次の行に移ります。どちらも終わりの側にそろえます。0.8 倍から大きくなりながら現れます。確定か閉じるのボタン、幕を押す、`Escape` のどれかで閉じます。
- `lamina/widgets/navigation_rail.hpp`：Expressive の幅の広いナビゲーションレール（`WideNavigationRail`）。畳んだときは幅 96 dp で、アイコンの下に名前があります。`set_expanded(true)` で幅を 220 dp 以上（名前が長ければ 360 dp まで）に広げ、名前をアイコンの横に移し、印は両方を囲む高さ 56 dp に育ちます。幅・項目の高さと間隔・項目の中の配置は標準の空間のばねで動くので、少し行き過ぎます。半分まで進んだところで名前は Label M から Label L に替わり、いったん消えてから現れ直します。選んだ項目の印は始めの側の端から広がります。`NavigationRailOptions::header` には上端に置く配置の部品を渡せます。たとえばレールを開け閉めするメニューのボタンです。
- `lamina/widgets/nodes.hpp` は、スライダー・タブの列・ナビゲーションバーも、与えられた幅いっぱいを取る配置の部品に変えます。ナビゲーションレールは、その時点のレールの幅と、与えられた高さいっぱいを取る部品になります。`build/Release/navigation.exe` は、タブ・二つのスライダー・二種類のメニュー・二つのダイアログ・ナビゲーションレール・ナビゲーションバーを並べます。
- `build/Release/components.exe` は、アイコン・アイコンつきのボタン・アイコンボタン・FAB・スイッチ・チェックボックス・ラジオボタンを並べます。テーマの切り替えはボタンの例と同じです。
- `lamina/widgets/text_field.hpp`：枠線つきの入力欄。ほかの部品と同じく `TextFieldOptions`（ラベル・`multiline`・クリップボード）とフォントを受け取ります。幅は 280 dp で、`set_width` で広げられます。ほかの部品と同じく `width()` と `height()` を持ちます。`handle(PointerEvent)` は、押した位置が欄の中のときに true を返し、そのとき欄に焦点が移ります。焦点が当たるか文字が入ると、ラベルがばねで枠線の上に移ります。枠線の形はラベルの後ろだけ切れているので、入力欄は自分以外を塗らず、背景の色を受け取りません。カーソルの点滅は、点滅の間に描き直さずに済むよう時間で起こします（`Window::wake_after`）。日本語は OS の IME で入力し、変換中の文字はその場に下線つきで描き、候補ウィンドウはカーソルの位置に出します（`Window::set_text_input`）。選択は `Shift` を押しながら矢印・`Home`・`End` で広げるほか、ドラッグで範囲を、ダブルクリックで単語を（日本語では漢字・ひらがな・カタカナの同じ種類の連なりを）、トリプルクリックと `Ctrl+A` で全体を選びます。選択中に文字を打つ・IME で確定する・`Backspace` や `Delete` を押すと、選択した部分が置き換わります。`Ctrl+←` と `Ctrl+→` で単語ごとに動き、`Ctrl+Backspace` と `Ctrl+Delete` で単語ごとに消します。`Ctrl+C`・`Ctrl+X`・`Ctrl+V` は `TextFieldOptions` で渡したクリップボードを使います（OS のものは `Window::clipboard()`、テストでは任意の `input::Clipboard`）。1行の入力欄はカーソルが見えるよう横に送り、文字を欄の内側で切り抜きます（`Canvas::push_clip`）。`Enter` で送信します。`TextFieldOptions::multiline` にすると文字は欄の幅で折り返し、欄は 56 dp（文字の上下に 16 dp）から1行ずつ高くなります。`Enter` で改行し `Ctrl+Enter` で送信します。上下の矢印はカーソルの横位置を保ったまま行を移り、`Home` と `End` は行の頭と末尾へ、`Ctrl` を添えると全体の頭と末尾へ動きます。`build/Release/text_field.exe` は小さな入力の画面で、メッセージ欄が複数行です。
- `lamina/shapes/`：androidx の `graphics-shapes` を C++ に移したもの（角ごとに丸めと滑らかさを持つ多角形と、二つの輪郭の角を対応させて 3 次曲線を補間する `Morph`）と、Material 3 Expressive の 35 個の形（`material_shape`）。出典は [third_party/NOTICE-androidx-graphics-shapes.md](third_party/NOTICE-androidx-graphics-shapes.md)。`build/Release/shapes_gallery.exe` は全部の形を並べ、どれかを押すと大きな形が expressive の空間のばねでその形に変わります。
- `lamina/layout/layout.hpp`：Compose と同じ考え方の行と列。部品は、まず制約の中で大きさを測り、次に置かれます。行と列には、間隔・余白・並ぶ向きの寄せ方（始め・中央・終わり・均等割り）・交わる向きの寄せ方（始め・中央・終わり・引き伸ばし）と、余った幅を比で分ける重みがあります。`layout::flow` は子を一行に並べ、次の子が収まらなければ新しい行に移ります。並べ始めは始めの側で、`FlowDirection::end_to_start` なら終わりの側です。引き伸ばしの行は、`fill_cross` で全部の高さを求めない限り、いちばん高い子の高さになります。`lamina/widgets/nodes.hpp` は、ボタン・アイコンボタン・FAB・スイッチ・チェックボックス・ラジオボタン・入力欄・`Label`（`widgets/label.hpp`、折り返す文字）を配置の部品に変えます。`build/Release/layout_demo.exe` は、窓の大きさを変えるとその場で組み直します。
- `lamina/widgets/widget_set.hpp`：`WidgetSet` は `step(dt)`・`paint(canvas, scheme)`・`animating()` を持つ部品を種類を問わずまとめて持ち、まとめて進め、加えた順に描き、どれかが動いているかを返します。`add(widget)` はよそにある部品を参照し、`emplace<W>(args...)` は部品をセットの中に作って、ずっと有効な参照を返します。それ以外のことはしません。入力・配置・フォーカスはプログラムの側で扱います。セットを複数持てば、その間に別のものを描けます。ナビゲーションの例では、ページの部品とメニューの間にレールのボタンを描いています。
- `lamina/motion/animated.hpp`・`lamina/motion/lerp.hpp`・`lamina/color/mix.hpp`：ばねで動く値と、2つの数の間の線形補間と、色の混ぜ合わせ。

ライブラリは CMake のターゲット三つに分かれていて、`src/` の下にターゲットごとのフォルダーがあります。バックエンドの別名 `lamina::thorvg` と `lamina::sdl3` はそのまま名前空間の名前で、本体の名前空間は `lamina` です。`src/lamina/` は本体（`lamina::lamina`：トークン・色・描画面の口・文字・図形・レイアウト・入力のイベント・部品）で、SDL3 も ThorVG も使いません。`src/lamina_thorvg/` は ThorVG で描く描画面（`lamina::thorvg`）、`src/lamina_sdl3/` は窓・素材の探索・OS のフォント（`lamina::sdl3`）です。部品はポインター・キー・文字の入力とクリップボードを `lamina/input/`（`lamina::input`）の形で受け取り、それを作るのは SDL3 の窓です。本体の中では、`tokens/` が値を入れる先のモジュール（`motion::SpringSpec`・`text::TextStyle`・`color::Role`）に依存し、`motion/`・`text/`・`color/` は `tokens/` に依存しません。

色の仕様は `ColorSpec` という口の向こうに置いているので、配色を使う側のコードは仕様によって変わりません。二つの仕様は HCT を共有し、配色の種類ごとに作る色の系列と、役割ごとに明度を選ぶ規則が違います。`examples/buttons` には二つを切り替えるスイッチがあります。

トークンの値と出典は [docs/design-tokens.ja.md](docs/design-tokens.ja.md) にあります。

## ばねの模型

ばねは質量 1、硬さ k、減衰比 ζ とします。目標からのずれ y は y'' + 2ζ√k y' + k y = 0 に従います。`advance` は場合ごとの厳密な解を使います：ζ < 1（振動して行き過ぎる）、ζ = 1（行き過ぎない中でいちばん速い）、ζ > 1（遅く、行き過ぎない）。expressive の空間のばね（位置・大きさ・形）は ζ < 1 で行き過ぎ、効果のばね（色・透明度）は ζ = 1 です。

## ビルド

Visual Studio 2022、vcpkg（`VCPKG_ROOT` を設定）、CMake 3.25 以上が要ります。

```
cmake --preset default
cmake --build --preset release
ctest --preset release
```

CMake のオプション `LAMINA_BUILD_THORVG` と `LAMINA_BUILD_SDL3`（どちらも既定で有効）で二つのバックエンドを、`LAMINA_BUILD_EXAMPLES`・`LAMINA_BUILD_TOOLS`（どちらも有効）と `LAMINA_BUILD_TESTS`（プリセットで有効）でそれ以外をビルドします。SDL3 の窓と見本には両方のバックエンドが、テストには ThorVG が要ります。これに合わない組み合わせでは、構成の段階でエラーになって止まります。両方のバックエンドを外しても、本体のライブラリと `tools/` の道具はビルドできます。

`build/Release/playground.exe [フォントのファイル...]` を開くと、カードが 6 枚並んだ窓が出ます。カードをクリックすると、浮き上がり・角の丸みの増加・色の変化が、それぞれ expressive の体系のばねで動きます。

ビルドは、見本ごとにその隣へ `assets/` を写します。見本は `build/Release/` から動くほか、`assets/` フォルダーを一緒に写せば、ほかのどのフォルダーからでも動きます。写さなかったときは親のフォルダーをさかのぼって `assets/` を探すので、ソースの中のものが見つかります。

`shapes_gallery.exe` 以外の見本は、コマンドラインでフォントのファイルを受け取ります（例：`buttons.exe C:/path/to/font.ttf`）。後ろのファイルは、前のファイルに無い文字を補います。指定しないときは、OS のフォントから最初に見つかった日本語の画面向けのフォントを使い（`lamina_sdl3/system_fonts.hpp`）、一つも無ければ、試したパスを並べたエラーを出して終わります。

## 出典

`tests/fixtures/fonts/` のテスト用フォントは、Noto Sans JP と Oswald の一部を取り出して名前を変えたもので、SIL Open Font License 1.1 に従います（ライセンスの文書を同じ場所に置いています）。

`assets/icons/MaterialSymbolsRounded-subset.ttf` は、Google の Material Symbols Rounded（可変フォント）を変更したものです。[google/material-design-icons](https://github.com/google/material-design-icons) のコミット `10ecc14790612ca8fd164e3f4ffe9c16fc50d5b7` から取り、Apache License 2.0 に従います（`assets/icons/LICENSE`）。`tools/icon-subset/subset.py` で `src/lamina/icons/symbols.hpp` にある 47 個の記号だけに絞り、合字の表とヒンティングを除きました。可変軸と名前の表は残しています。


トークンの値は、Google が Apache-2.0 で公開しているソースから取っています：androidx（Jetpack Compose の Material 3 と graphics-shapes）、material-components-android、material-web、material-color-utilities。ファイルとコミットは [docs/design-tokens.ja.md](docs/design-tokens.ja.md) にあります。
