# ナビゲーション

[English](navigation.md) · [README](../../README.ja.md#資料)

## タブ

`lamina/widgets/tabs.hpp`：primary と secondary のタブの列（`TabRow`）。タブは幅を等しく分けます。ラベルの上（このとき列は 72 dp の高さ）か前（`TabIconPosition::leading`）にアイコンを置けます。印は標準の空間のばねで選んだタブへ滑ります。primary のタブではラベルと同じ幅（少なくとも 24 dp）、secondary のタブではタブの幅いっぱいです。ラベルの色は、Compose と同じく、現れるときは標準の効果のばね、消えるときは速い効果のばねで移ります。タブの列は Tab の順の中で一つの止まり位置です（`Focusable`）。フォーカスがある間は、左右の矢印で前後のタブを選び（端では反対の端へ戻ります）、`Home` と `End` で最初と最後のタブを選びます。`handle(KeyEvent)` は、クリックと同じく選択が変わったときに true を返します。選ばれたタブはフォーカスの状態の重ねを出し、material-web のタブと同じく、フォーカスの輪をタブの縁の内側に描きます（項目の並ぶ部品が共通に使うキーの決まりは `widgets/item_focus.hpp` にあります）。

### 横に送れるタブ

`TabRowOptions::scrollable` を有効にすると、タブは幅を分け合わなくなります。各タブの幅は中身に左右 16 dp ずつ足したもので、少なくとも 90 dp です。列の始まりと終わりには `edge_padding`（既定は 52 dp。Compose の `ScrollableTabRowEdgeStartPadding`）をあけます。タブが入りきるときは始まりに寄せ、引き伸ばしません。入りきらないときは列が横に送られます。

- クリック・キー・`select()` でタブを選ぶと、そのタブが列の中央に来るように、両端で止めつつ、標準の空間のばねで送ります（Compose も同じばねです）。印も同時に自分のばねで動くので、どちらも飛びません。
- 列の上でホイールを回すと、1 ノッチで 64 dp（`scroll_area_tokens.wheel_step`）送ります。縦のホイールも横に送ります。送ったとき `handle(WheelEvent)` は true を返します。
- 列の外に出たタブはクリックできず、読み上げの範囲は見えている部分だけです（隠れているときは空です）。
- `scroll_extent()`・`max_scroll()`・`scroll_offset()` で、中身の幅・送れる範囲・今の送り量を得られます。幅が変わっても、送り量は飛ばずに範囲の中に収まります。

指やマウスのドラッグで送ることは、まだできません。部品の一覧は、十五のページを選ぶのに横に送れる secondary のタブを使い、最初のラベルが見出しとそろうように `edge_padding` を決めています。

## ナビゲーションバー

`lamina/widgets/navigation_bar.hpp`：Expressive の短いナビゲーションバー。高さ 64 dp で、項目は同じ幅に並びます。選んだ項目の印（56 × 32 dp）は、標準の空間のばねで中央から広がりながら現れます。マウスを乗せたときと押したときは、印の形に状態の重ねが出ます。`NavigationItem::selected_style`（`lamina/widgets/navigation_item.hpp`。レールと共通です）で、選んだときのアイコンに別の Material Symbols の軸の値（たとえば塗り 1）を与えられます。アイコンは色と一緒にその値へ移ります。`set_labels(false)`（または `NavigationBarOptions::labels`）で名前を隠せます。標準の空間のばねでバーは高さ 48 dp に縮み、印は上下の中央に来ます。名前は動きの前半で消えます。

## ナビゲーションレール

`lamina/widgets/navigation_rail.hpp`：Expressive の幅の広いナビゲーションレール（`WideNavigationRail`）。畳んだときは幅 96 dp で、アイコンの下に名前があります。`set_expanded(true)` で幅を 220 dp 以上（名前が長ければ 360 dp まで）に広げ、名前をアイコンの横に移し、印は両方を囲む高さ 56 dp に育ちます。幅・項目の高さと間隔・項目の中の配置は標準の空間のばねで動くので、少し行き過ぎます。半分まで進んだところで名前は Label M から Label L に替わり、いったん消えてから現れ直します。選んだ項目の印は始めの側の端から広がります。`NavigationRailOptions::header` には上端に置く配置の部品を渡せます。たとえばレールを開け閉めするメニューのボタンです。バーとレールは、タブと同じくそれぞれ Tab の順の中で一つの止まり位置です（`Focusable`）。ただし矢印で動くのは選択ではなくフォーカスで、material-web のナビゲーションバーと同じです。バーでは左右の矢印、レールでは上下の矢印で動き（端では反対の端へ戻ります）、`Home` と `End` で最初と最後へ動きます。行き先を選ぶとページが替わるので、選ぶのはその後の `Space` か `Enter` です（選択が変わったとき `handle(KeyEvent)` は true を返します）。フォーカスを受けたときは選ばれている項目から始めます（`focused_item()`）。フォーカスのある項目は、印の形にフォーカスの状態の重ねを出し、印の周りにフォーカスの輪を描きます。`set_labels(false)`（または `NavigationRailOptions::labels`）で、畳んだレールをアイコンだけにできます。標準の空間のばねで幅 80 dp（56 × 32 dp の印の左右に 12 dp）に狭まり、項目は高さ 56 dp になり、名前は動きの前半で消えます。広げたレールを畳むのと名前を隠すのを同時に行っても、上の位置の名前は現れ直しません。`snap_to_targets()` は幅・名前・項目のばねをその場で終わらせます。

## 横のナビゲーション

`lamina/widgets/side_navigation.hpp`：`SideNavigation` は、縦に並ぶ一つの行き先の並びです。同じ項目から、アイコンだけのレール（80 dp）、幅のあるレール（220〜360 dp。いちばん長い名前に合わせた幅）、標準のドロワー（360 dp）のどれにもなります。動かす値は二つで、広がり（`set_expanded`）とドロワーへの進み（`set_drawer`）です。どちらも減衰の臨界のばねで動きます。どの形でも、動いている途中のどのコマでも、アイコンは同じ x と y にあります。行はドロワーのもの（上から 12 dp、高さはドロワーの項目と同じ）で、アイコンは始まりの側から 12 + 16 dp のところです。選んだ項目の印は始まりの側の端を 12 dp に保ったまま、アイコンを囲む 56 × 32 dp の形から、アイコンと名前を囲む形（幅のあるレール）か行の全体（ドロワー）へ形を変えます。入れ物の幅もそれについていきます。名前は、たたむときは前半で消え、広げるときは後半で現れます。幅のあるレールとドロワーのあいだでは消えません。`icon_rect`・`label_rect`・`shape_rect`・`indicator_rect`・`item_rect`・`expansion()`・`drawer_amount()`・`label_opacity()` で形を読めます。

## ナビゲーションドロワー

`lamina/widgets/navigation_drawer.hpp`：Compose のナビゲーションドロワー。`DrawerSheet` は `DrawerEntry` を並べます。並べるのは項目・Title S の見出し・区切り線で、最初の項目の上に 12 dp を空けます。項目は付けてもよいアイコン・ラベル・未読の数のような文字のバッジを持ち、左右を 12 dp 空けた高さ 56 dp の両端の丸い形に置きます。選んだ項目は secondary-container です。幅は 360 dp（窓が狭ければそれより狭く）で、並べたものが窓より高いときはスクロールします。Tab の止まり位置は一つで、↑ と ↓ で有効な項目の間をフォーカスが動き、Space か Enter で選びます。`ModalNavigationDrawer` は、開いた割合に合わせて不透明度が変わる幕の上にシートを滑らせます。`open` は標準の空間のばね、`close` は速い効果のばねで動きます。範囲のどこでもドラッグでき（閉じた中身の上でも。`DrawerResponse::took_over` は、プログラムに自分の押下を取り消すよう伝えます）、離すと Compose の 256 ms のトゥイーンで落ち着きます。Escape と幕のクリックで閉じ、ばねが行き過ぎると入れ物が伸びます。`DismissibleNavigationDrawer` は、シートの幅だけ中身を横へ押しやる配置の部品です。`permanent_drawer(sheet, content)` はモーダルでないシートを中身の横に置きます。シートは "Navigation menu" という名前の `navigation` を返し、子は `navigation_item`・`heading`・`separator` です。

## モーダルな幅のあるレール

`lamina/widgets/modal_navigation_rail.hpp`：Expressive の `ModalWideNavigationRail`。配置の中では畳んだレールの 96 dp を取ったまま、中身の上に幕を敷いて `NavigationRail` を広げます。面は surface-container で角は 16 dp です。Escape と幕のクリックで畳みます。`hide_on_collapse` にすると畳んだ形はありません。広げたレールが窓の外から滑り込み、ドラッグでしまえます。このために `NavigationRail::paint_items` は入れ物なしでレールを描きます。

## トップアプリバー

`lamina/widgets/top_app_bar.hpp`・`top_app_bar_state.hpp`：Compose の `AppBar.kt` のトップアプリバー（`TopAppBarKind`、`tokens/top_app_bar.hpp`）。小さいものと中央そろえのもの（64 dp、Title L）、中くらいのもの（112 dp、Headline S）、大きいもの（152 dp、Headline M）と、Expressive の中くらいの伸縮するもの（112 dp、副題つきで 136 dp、Headline M）と大きい伸縮するもの（120 dp、副題つきで 152 dp、Display S）があります。2 段のバーは 64 dp まで縮みます。戻るアイコンと操作はアイコンボタン（on-surface と on-surface-variant。`IconButtonOptions::content`）です。題名は左から 16 dp、戻るアイコンがあれば 56 dp から始まり、中央そろえの題名はアイコンにぶつかるときだけ横へずれます。1 段のバーと伸縮するバーは副題を持てます（1 段では Label M、伸縮するバーの大きい題名の下では Label L か Title M）。`TopAppBarState` は Compose の `heightOffset`・`heightOffsetLimit`・`contentOffset`・`collapsedFraction`・`overlappedFraction` を持ち、広げたバーの下から中身が始まるスクロールする範囲（`ListView::top_inset`）の位置に、三つのスクロールの仕方を当てはめます。`pinned` はバーをそのままにし、`enter_always` は中身を先へ送るとバーを隠して少しでも戻すと出し、`exit_until_collapsed` はまずバーを縮め、中身が先頭に戻ってから広げます。`TopAppBar::follow(scroll_area)` で毎フレーム動かします。小さいバーは、下に中身が来ると標準の効果のばねでスクロール時の入れ物の色（surface-container）へ移ります。2 段のバーは縮んだ割合（`FastOutLinearInEasing`）で色を混ぜ、下の段を縮め、大きい題名を直線的に消しながら小さい題名を Compose の `TopTitleAlphaEasing` に沿って出します。バーが 1 % から 100 % の間で縮んだままスクロールが止まると、バーは標準の効果のばねで近いほうの端まで動き、中身もついて動きます。バーは読み上げ用の `toolbar` で、題名（2 段のバーでは半分より多く見えている段のもの）とアイコンボタンを含みます。

## 浮かぶツールバー

`FloatingToolbar`（`lamina/widgets/floating_toolbar.hpp`）は、アイコンの操作（`ToolbarAction`）を並べた 64 dp の丸い帯です。横か縦（`ToolbarOrientation`）、標準か鮮やかな色（`ToolbarColorStyle`）を選べ、前と後ろの中身と FAB（`ToolbarFab`）を足せます。`set_expanded(false)` で、帯は速い空間のばねで FAB の方へたたまれ、FAB は 56 dp から 80 dp に大きくなります。FAB が無いときは、前と後ろの中身が縮んで消えます。終わりの方へ 40 dp 送るとたたみ、40 dp 戻すと開きます。`handle` は、`clicked`（`ToolbarSlot` と番号）と `fab_clicked` を持つ `FloatingToolbarResponse` を返します。値は[浮かぶツールバー](../design-tokens.ja.md#浮かぶツールバー)にあります。

## 柔軟な下のアプリバー

`FlexibleBottomAppBar`（`lamina/widgets/flexible_bottom_app_bar.hpp`）は、surface-container の上に操作を並べた 64 dp のバーです。並べ方は、あいだを等しく空けるか、32 dp ずつ詰めるかです（`BottomBarArrangement`）。`exit_always` のときは、中身を下へ送るにつれて下の縁から外へ滑り出し、送るのが止まると、速い空間のばねで近い方の端に落ち着きます。値は[柔軟な下のアプリバー](../design-tokens.ja.md#柔軟な下のアプリバー)にあります。

## 出典

- Material 3 の タブ：<https://m3.material.io/components/tabs/overview>
- Material 3 の ナビゲーションバー：<https://m3.material.io/components/navigation-bar/overview>
- Material 3 の ナビゲーションレール：<https://m3.material.io/components/navigation-rail/overview>
- Material 3 の ナビゲーションドロワー：<https://m3.material.io/components/navigation-drawer/overview>
- Material 3 の アプリバー：<https://m3.material.io/components/app-bars/overview>
- Material 3 の ツールバー：<https://m3.material.io/components/toolbars/overview>
- Compose Material 3 の `TabRow.kt`（横に送れるタブの値）：<https://github.com/androidx/androidx/blob/androidx-main/compose/material3/material3/src/commonMain/kotlin/androidx/compose/material3/TabRow.kt>
- 値ごとのソースのファイルとコミット：[デザイントークン](../design-tokens.ja.md)
