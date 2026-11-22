# 入れ物

[English](containers.md) · [README](../../README.ja.md#資料)

## ダイアログ

`lamina/widgets/dialog.hpp`：基本のダイアログ。窓全体を幕（色は scrim、不透明度 32 %）で覆い、面（幅 280 から 560 dp・角 28 dp）に見出し・本文・終わりの側にそろえたテキストボタンを `lamina::layout` で並べます。`DialogOptions::icon` を与えると見出しの上に 24 dp のアイコン（色は secondary）を置き、見出しは中央にそろえます。ボタンは終わりの側から並び、一行に収まらないときは、確定のボタンが一行目に残り、閉じるボタンが次の行に移ります。どちらも終わりの側にそろえます。0.8 倍から大きくなりながら現れます。確定か閉じるのボタン、幕を押す、`Escape` のどれかで閉じます。`open(size)` は窓全体を覆います。`open_in(area)` と `set_area(area)` は、幕と中央の面を窓の中の矩形（自前で描くタイトルバーの下など）に置きます。その矩形の外を押しても閉じません。開いているダイアログは、キーボードのフォーカスをどれかのボタンに置きます。`DialogOptions::default_action` があればそのボタンに、なければ読む順で最初のボタン（閉じるボタン。ボタンが折り返したときは確定のボタン）に置きます。`InputSource::keyboard`（`open` と `open_in` の二つ目の引数）で開くと、すぐにフォーカスの輪を見せます。ポインターで開いたとき（既定）は、フォーカスを持ったまま、Tab が押されるまで輪を見せません。Tab と Shift+Tab はボタンの間だけを巡り、Enter か Space でフォーカスのあるボタンが押され、Escape で閉じます。Compose の `AlertDialog` と同じ振る舞いです。`focused_action()` と `focus_shown()` で状態がわかります。`DialogOptions::content`（`DialogContent`：配置の節点、その部品を持つ `WidgetSet`、部品の Tab の順を持つ `FocusManager`（なくてもよい））を与えると、見出しと操作のボタンの間に好きな部品を置けます。本文があるときはその下です。Compose の `AlertDialog` の本文の枠と同じ位置です。ダイアログが、範囲から上下 48 dp ずつ引いた高さより高くなるときは、この枠が縮んで `ScrollArea` の中でスクロールします。ホイールは `handle(WheelEvent)` で届きます。ポインターの出来事は中身の座標に直して中身へ渡し、文字の出来事は中身の部品にフォーカスがあるときに渡します。`caret_area()` と `next_wake_seconds()` は中身の入力欄の分も返します。Tab は中身のフォーカスの順をたどってから操作のボタンへ進み、端では先頭へ戻り、フォーカスを得た部品が見えるようにスクロールします。`set_action_enabled` で操作のボタンを無効にでき、フォーカスはそのボタンを飛ばします。中身の中の Tab はいつもフォーカスを移すので、タブを受け付ける入力欄を中身に置いても、タブは打てません。

## ボトムシート

`lamina/widgets/bottom_sheet.hpp`：Compose の `ModalBottomSheet` と `BottomSheetScaffold` のシート（`BottomSheetKind`）。シート（面は surface-container-low・上の角は 28 dp・幅は 640 dp まで）は 32 × 4 dp のドラッグハンドルと好きな `ContentSlot` を持ち、シートが範囲より高いときは中身がスクロールします。置き場所は `set_area` で決めます。アンカーは Compose と同じです。モーダルなシートは下端で隠れ、高さの半分（範囲の半分まで）まで開き、広げると全体の高さになります。モーダルでないシートは `peek_height`（既定は 56 dp）だけのぞき、広げられ、`skip_hidden` でないときだけ隠れます。`show`・`hide`・`expand`・`partial_expand` は Compose のばねで動かします。シートはどこをドラッグしてもポインターに付いて動き、離したときの速さで落ち着きます。下端の近くで下へはじいたときは、Compose と同じく速さを弱めます。Expressive のばねが上端を行き過ぎたときは、すき間を空けずに入れ物が伸びます。モーダルなシートは範囲を幕で暗くし、幕は標準の効果のばねで現れ消えします。Escape は広げたシートをまず半分まで下げてから閉じ、幕を押すと閉じます。Tab は中（ハンドル・中身の順）にとどまり、キーボードで開いたときはハンドルにフォーカスを置きます。ハンドルをクリックするか Space か Enter を押すと、Compose と同じく広げる・畳む・閉じるのどれかをします。`BottomSheetResponse::dismissed` は閉じ始めたときに知らせます。モーダルなシートは "Bottom Sheet" という名前のモーダルな `dialog` で、説明でその操作を示す "Drag handle" のボタンと "Close sheet" の幕のボタンを持ちます。Compose はハンドルにツールチップを出すので、プログラムは `drag_handle()` でハンドルにツールチップを結びつけられます。

## サイドシート

`lamina/widgets/side_sheet.hpp`：モーダルなサイドシートとモーダルでないサイドシート（`SideSheetKind`）。窓の端に付けるか 16 dp の余白と 16 dp の角で端から離して置き、終わりの側か始めの側に出します（`SideSheetEdge`）。Compose にはサイドシートがありません。値は material-web のトークンから、振る舞いは Android のライブラリから取りました。幅は 256 dp です。モーダルなシートは surface-container-low に段階 1 の影で、始めの側の角を丸めます。モーダルでないシートは surface です。高さ 72 dp の見出しの行に、付けてもよい戻るボタン・Title L の見出し・付けてもよい閉じるボタンを置きます。中身は `ContentSlot` で、`actions` を与えると outline の区切り線の下にボタンの行を足します。モーダルなシートは幕の上に emphasized の緩急で 275 ms かけて滑り込みます。Escape・幕・閉じるボタン・半分を越えるか 500 dp/s より速いドラッグで閉じます。Tab は中にとどめます。モーダルでないシートは開くにつれて幅が広がる配置の部品なので、横の中身は狭まります。`SideSheetResponse` は `dismissed`・`back`・押された `action` を知らせます。

## カード

`lamina/widgets/card.hpp`：浮いた・塗った・枠線のカード（`CardStyle`）。角は 12 dp で、色と高さは Compose の `CardDefaults` のものです（`tokens/card.hpp`）。`CardOptions::content` は好きな配置の要素、`widgets` はその部品を持つ `WidgetSet` です。カードは中身と同じ大きさになり、中身を自分の形で切り抜いて描き、ポインターとキーの出来事を中身に渡し、`Modifier.border` と同じく枠線を中身の上に描きます。押せるカード（`clickable`。`accessible_name` が要ります）は 48 dp 以上で、状態の重ねを出し、マウスが乗ると高さのトゥイーンで浮き、フォーカスを受け、Space か Enter でクリックされます。カードの中の有効な操作部品を押しても、カードは押されません。押せないカードは読み上げ用の `group`、押せるカードは `button` です（Compose は役割を付けません）。無効にすると、入れ物は無効の色へ移り（塗ったカードは surface-container-highest の上に surface-variant の 38 %、浮いたカードは surface。枠線のカードの線は、`outlinedCardBorder` と同じく surface-container-low の上に outline の 12 %）、中身は 38 % で描かれます。

## リスト

`lamina/widgets/list_item.hpp`・`list_view.hpp`：Compose の操作できる `ListItem`（`tokens/list.hpp`）。見出し（Body L）、任意の上の小見出し（Label S）と補足の文（Body M）、先頭と末尾の枠（`ListSlot`：アイコン（24 dp）、アバター（40 dp。画像か、primary-container の上の頭文字）、画像（56 dp、角 8 dp）、チェックボックス、スイッチ、ラジオボタン、Label S の末尾の文字）を持ちます。項目は 1 行（56 dp 以上）、小見出しか補足の文があれば 2 行（72 dp）、両方あるか補足の文が折り返せば 3 行（88 dp）です。左右に 16 dp、上下に 10 dp の余白を取り、枠と文字の間を 12 dp 空け、内側が 60 dp 以上の高さになると枠と文字を上にそろえます。`ListItemAction` で、押せる・選べる（選ぶと選ばれたままのラジオボタン）・切り替えられる（反転するチェックボックス）のどれかにします。選ばれた項目や入っている項目は標準の効果のばねで secondary-container の色へ移り、角は Compose の `shapeForInteraction` と同じく、速い空間のばねで 4 dp から、マウスが乗ると 12 dp、フォーカス・押下・選択で 16 dp に変わります。枠の中のチェックボックス・スイッチ・ラジオボタンは項目の状態に従い、自分では入力を受けません。`ListView` は `ListItemSpec` の行を `ScrollArea` の中に並べ、見えている行と両側の 2 行（`overscan_rows`）だけを配置し、持ち、描きます。ほかの行は、初めて測るまで見積もった高さ（56・72・88 dp）のままなので、10,000 行でも一画面分の手間で済みます（`stats()` が、測った行・持っている行・描いた行を数えます）。選択と入切の状態は行の指定のほうに持つので、行が画面から出て戻っても失われません。リストは material-web のリストと同じく Tab の止まり位置一つです。上下の矢印で有効な行の間をフォーカスが動き（端では反対の端へ戻ります）、Home と End で端へ行き、Page Up と Page Down で表示の高さから 48 dp を引いた分だけ動き、フォーカスのある行は見える所までスクロールされ、Space か Enter でその行が動きます。`single_selection` は選べる行を一つだけ選ばれた状態にします。`top_inset` は最初の行の上を空け、トップアプリバーの下に来る中身に使います。`ListItemSpec::transparent` は入れ物を描かないので、行の下の面（シートや検索の表示）が見えます。Compose の見本が透明な `containerColor` でしているのと同じです。

## 区切り線

`lamina/widgets/divider.hpp`：横と縦の区切り線。outline-variant の 1 dp です（`tokens/divider.hpp`、Compose の `DividerTokens`）。`Divider` は `Measurable` で、横の線は渡された幅いっぱいに、縦の線は渡された高さいっぱいに伸びます。`DividerInset` は始まりの側・終わりの側・両側に 16 dp を空けます（縦の線では `start` が上です）。この値は M3 のリストのトークンにある区切り線の空きと、material-web の `inset` から取りました。Compose には空ける指定がなく、余白に任せています。読み上げ用の役割は `separator` で、名前はありません。Compose と同じく無効の状態はありません。

## スクロールの領域

`lamina/widgets/scroll_area.hpp`：`ScrollArea` は、表示する範囲より高い中身を縦に送ります（`set_viewport`・`set_content_height`）。送る量は臨界減衰のばね（標準の効果のばね）で動くので、端を行き過ぎません。ホイール 1 ノッチで 64 dp（`handle(WheelEvent)`。ホイールが範囲の上にあり、送れる中身があるとき true を返します）、Page Up と Page Down で範囲の高さから 48 dp を引いた分を送ります（`handle(KeyEvent)`）。送り先は先頭から終わりまでの間に収まり、中身が縮んだときも収め直して、なめらかに戻ります。`ensure_visible(top, bottom)` は、ある範囲が見えるように最小限だけ送ります。フォーカスを受けた行を見せるときに使います。中身は `begin_content` と `end_content` の間に中身自身の座標で描きます。この二つは範囲で切り抜き（`Canvas::push_clip`。テキストフィールドのように内側でも切り抜くと、入れ子になります）、送った量だけずらします。`to_content(event)` はポインターの出来事をその座標に直し、範囲の外やバーの上なら「離れた」出来事にします。`to_window(rect)` は四角形を窓の座標に戻すもので、IME のカーソルの位置に使います。ポインターが止まったまま下の中身が動いたときは、`rehover()` がポインターの位置での移動の出来事を一度だけ返すので、マウスを乗せた状態が追いつきます。スクロールバーは端から 2 dp の位置にある幅 4 dp のつまみ（`on_surface` の 38 %、長さは 16 dp 以上）で、送っている間に現れ、止まって 1 秒後に消えます。ポインターを乗せると幅 8 dp・50 % に変わり、つまみを引くと中身が同じ割合で動き、つまみの外を押すとその向きに 1 画面分送ります。消えるまでの待ちは `animating()` ではなく `next_wake_seconds()` で知らせるので（`Window::wake_after` に渡します）、その間も窓は止まっていられます。Material 3 にはスクロールバーのトークンがないので、これらの値は lamina-ui 独自のものです（`tokens/scroll_area.hpp`）。`set_content_width` で中身を横にもスクロールできます。ホイールの横の軸と、中身が表示より広いときの Shift を押しながらの縦のホイールで横へ動き、下の辺に横のバーが出ます（`offset_x`・`scroll_to_x`・`ensure_visible_x`・`thumb_x`）。

## 出典

- Material 3 の ダイアログ：<https://m3.material.io/components/dialogs/overview>
- Material 3 の ボトムシート：<https://m3.material.io/components/bottom-sheets/overview>
- Material 3 の サイドシート：<https://m3.material.io/components/side-sheets/overview>
- Material 3 の カード：<https://m3.material.io/components/cards/overview>
- Material 3 の リスト：<https://m3.material.io/components/lists/overview>
- Material 3 の 区切り線：<https://m3.material.io/components/divider/overview>
- 値ごとのソースのファイルとコミット：[デザイントークン](../design-tokens.ja.md)
