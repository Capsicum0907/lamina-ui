# レイアウトと幅への対応

[English](layout-and-adaptive.md) · [README](../README.ja.md#資料)

行と列、窓の大きさの区分、それに合わせて形を変える足場、ペイン、密度について書きます。

## 行と列

`lamina/layout/layout.hpp`：Compose と同じ考え方の行と列。部品は、まず制約の中で大きさを測り、次に置かれます。行と列には、間隔・余白・並ぶ向きの寄せ方（始め・中央・終わり・均等割り）・交わる向きの寄せ方（始め・中央・終わり・引き伸ばし）と、余った幅を比で分ける重みがあります。`layout::flow` は子を一行に並べ、次の子が収まらなければ新しい行に移ります。並べ始めは始めの側で、`FlowDirection::end_to_start` なら終わりの側です。`cross_align` は、それぞれの子をその子がいる行の高さの中でそろえます（指定がなければ上端です。子の `self_align` があればそちらが優先です）。`line_count()` と `line_start(line)` で、どこで行が折り返されたかが分かります。引き伸ばしの行は、`fill_cross` で全部の高さを求めない限り、いちばん高い子の高さになります。`lamina/widgets/nodes.hpp` は、ボタン・アイコンボタン・FAB・スイッチ・チェックボックス・ラジオボタン・入力欄・`Label`（`widgets/label.hpp`、折り返す文字。ほかの部品と同じく、文字・文字の役割・色の役割・折り返すかどうかを持つ `LabelOptions` と、フォントを受け取ります）を配置の部品に変えます。`build/Release/layout_demo.exe` は、窓の大きさを変えるとその場で組み直します。

## 部品をレイアウトの節にする

`lamina/widgets/nodes.hpp` は、スライダー・タブの列・ナビゲーションバーも、与えられた幅いっぱいを取る配置の部品に変えます。ナビゲーションレールは、その時点のレールの幅と、与えられた高さいっぱいを取る部品になります。`build/Release/navigation.exe` は、タブ・二つのスライダー・二種類のメニュー・二つのダイアログ・ナビゲーションレール・ナビゲーションバーを並べます。Tab は、レールのメニューボタン・レール・ダークモードの切り替え・三つのタブの列・二つのスライダー・四つのボタン・ナビゲーションバーの順にたどります。ボタンで Space か Enter を押すと、メニューやダイアログをキーボードから開くので、最初の項目やダイアログの操作にすぐフォーカスがあり、閉じるとフォーカスはボタンに戻ります。メニューの項目にはアクセスキーがあります。Options のメニューでは、Edit・Share・Copy・Settings は最初の文字を使うので、Share と Settings はどちらも S になり、S を押すと二つの間を動きます。日本語の「ダウンロード」は D で、「ダウンロード(D)」と表示します。View のメニューでは Starred が R、Share が H なので、どのキーも一つの項目だけのものです。

## 画面の型

`lamina/widgets/content_slot.hpp`・`content_host.hpp`・`scrim.hpp`・`pattern_strings.hpp`：画面のパターンが共通に使うもの。`ContentSlot`（配置の節点・`WidgetSet`・`FocusManager`。`DialogContent` は今はこの型です）は、プログラムがシートやドロワーに入れる好きな中身です。`ContentHost` はそれをある幅で並べ、表示する範囲より高いときは `ScrollArea` の中でスクロールさせます。ポインターの出来事は中身自身の座標に直して渡し、キーと文字は中身にフォーカスがある間に渡します。フォーカスのある部品を見える所に保ち、読み上げ用の節点も集めます。`chain_focus(parts, direction, wrap)` は、いくつかの `FocusManager` を順にたどってフォーカスを動かします。モーダルな入れ物では端で先頭へ戻ります。`paint_scrim` は scrim の役割の色を 32 % にした幕を描きます（`tokens/scrim.hpp`）。`PatternStrings` は、これらの部品の Compose の文字列を英語（US と UK）と日本語で持ちます（`english_us_patterns()`・`english_gb_patterns()`・`japanese_patterns()`）。

## 窓の大きさの区分

`lamina/layout/window_size_class.hpp`：window-core の区切りによるウィンドウの大きさの区分です（幅：compact、600 dp から medium、840 から expanded、1200 から large、1600 から extra-large。高さ：compact、480 から medium、900 から expanded）。`compute_window_size_class(幅, 高さ)` が区分を返し、`sdl3::Frame::size_class` が毎フレームのウィンドウの区分を持ちます。

## ナビゲーションスイートの足場

`lamina/widgets/navigation_suite.hpp`：`NavigationSuiteScaffold` は、一つの行き先の並びをナビゲーションバーか `SideNavigation`（アイコンだけのレール・幅のあるレール・ドロワー）で見せ、残りを中身に渡します。`update(info)` は `widening_navigation_suite` でウィンドウの大きさの区分から種類を選びます。`navigation_suite_type(info)` は Compose の選び方です。バーと横の並びはばねで入れ替わります。レールとドロワーのあいだは、重ねて薄れさせずに横の並びが形を変えます。どれかで選んだ行き先はすべてで選ばれます。`set_labels`（または `NavigationSuiteScaffoldOptions::labels`）はバーの名前を出し入れし、横の並びは種類にかかわらずアイコンだけにたたみます。`snap_to_targets()` は種類の変更と名前を動かさずに行き着かせます（最初の一コマのためです）。`side()` で `SideNavigation` を取れます。

  - 互換性のない変更：足場から `rail()` と `drawer()` を無くしました（前は `NavigationRail` と `DrawerSheet` を持ち、重ねて切り替えていました）。代わりに `side()` を使います。`wide_navigation_rail_collapsed` は、どの場合も 80 dp のアイコンだけのレールになりました。アイコンの下に名前を置く 96 dp のレールは、行が高くなってアイコンが動くので、足場からは外しました（`NavigationRail` そのものにはあります）。`NavigationSuiteTokens::drawer_width` は `SideNavigationTokens` に移しました。足場のドロワーはスクロールしなくなり、行の終わりに文字のバッジも出しません。バッジはどの形でもアイコンの上に出します。

足場が出す横の並びは `SideNavigation` で、[ナビゲーション](components/navigation.ja.md#横のナビゲーション)に書いています。

## ペインの足場

`lamina/widgets/pane_navigator.hpp`・`pane_scaffold.hpp`：Compose のペインの足場の指示・値・案内役（戻り方つきの `ThreePaneScaffoldNavigator`）と、`ThreePaneScaffold` です。`ThreePaneScaffold` は、リストと詳細（`list_detail_pane_navigator`・`list_detail_pane_order`）か補助のペイン（`supporting_pane_navigator`・`supporting_pane_order`）の足場を、ウィンドウの大きさの区分に応じて 1・2・3 の区画に並べ、ペインをばねで出し入れし、Escape で戻ります。`expansion_handle` を付けると、止まる位置つきのペインの広さを変えるドラッグハンドルを持ちます（ドラッグかはじく操作、Enter で次の止まる位置、← と → で少しずつ）。各ペインは `ContentSlot` で、隠れたペインは入力もフォーカスも読み上げの節も持ちません。

## 分割ペイン

`lamina/widgets/split_pane.hpp`：`SplitPane` は、二つの配置の節の間で場所を分けます。横に並べるか縦に積み、入れ子にでき、最小の大きさを持ちます。仕切りは、ポインターが乗るとドラッグハンドルを見せて大きさを変えるカーソルを求め（`cursor()`）、ドラッグに付いて動き、フォーカスがあると矢印のキー・Page Up と Page Down・Home・End・Enter（最初のペインを畳んで戻す）を受けます。

## 密度

`lamina/tokens/density.hpp`：密度の段階（`tokens::Density`。0・-1・-2 と続き、一段 4 dp で、部品ごとの最小で止まる）です。ボタンとアイコンボタン（極小から中まで）・テキスト欄・リストの項目とリストビュー・チップとチップの組・タブの行・区分ボタン・上部アプリバー・ナビゲーションバー・レール・ドロワーのシート・メニューは、設定に `density` を取り、`density()` と `set_density` を持ちます。`WidgetSet::set_density` は、密度を持つすべての部品（`Densifiable`）に設定します。詰めた部品は、48 dp の範囲ではなく自分の範囲だけでクリックを受けます。密度 0 の見た目はこれまでと同じです。高さ・最小・出どころは [docs/design-tokens.ja.md](design-tokens.ja.md#密度) にあります。

## 出典

- Material 3 の 窓の大きさの区分：<https://m3.material.io/foundations/layout/applying-layout/window-size-classes>
- Material 3 の リストと詳細のレイアウト：<https://m3.material.io/foundations/layout/canonical-layouts/list-detail>
- Material 3 の ナビゲーションレール：<https://m3.material.io/components/navigation-rail/overview>
- 値ごとのソースのファイルとコミット：[デザイントークン](design-tokens.ja.md)
