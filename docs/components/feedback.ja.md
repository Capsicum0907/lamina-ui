# 知らせ

[English](feedback.md) · [README](../../README.ja.md#資料)

## スナックバー

`lamina/widgets/snackbar.hpp`：スナックバー。窓の下に短い知らせを出し、テキストボタンの操作を一つ付けられます（`SnackbarOptions::action_label`）。`set_area(rect)` で置く場所を渡すと、下から 12 dp 上の中央に、場所の幅いっぱい（最大 600 dp）で置きます。高さは Body M の 1 行で 48 dp（2 行で 68 dp）、面は `inverse_surface`、角は 4 dp、影は Level 3 です。知らせの文字は `inverse_on_surface`、操作は `inverse_primary` です。`show(message)` は Compose の `SnackbarHost` と同じく、0.8 倍から速い空間のばねで大きくしながら速い効果のばねで現し、時間を数え直します。`SnackbarOptions::duration_seconds`（既定は Compose の短い時間の 4 秒）がたつと自分で消えますが、ポインターが乗っている間は時間が進みません。残りの時間は動きではなく起きる時刻（`next_wake_seconds()`）として知らせるので、待つ間も窓は止まっていられます。`handle(PointerEvent)` は `SnackbarResponse` を返し、操作が押されたら `action`（スナックバーは閉じます）、ポインターが上にある間は `consumed` が立ちます。そのため `OverlayStack` のモードレスな重なりとして使えます。操作のボタンはキーボードのフォーカスを受けます。スナックバーは `Focusable` で `FocusGated`（表示中で操作があるとき `can_focus()` が true）なので、プログラムは `FocusManager` に加えます。フォーカスのある操作で `Space` か `Enter` を押すと、`handle(KeyEvent)` はクリックと同じ `SnackbarResponse` を返して閉じます。操作にフォーカスがある間は、ポインターが乗っている間と同じく時間が進まないので、キーボードで操作している最中に消えることはありません（WCAG 2.2.1「タイミング調整可能」）。閉じるとフォーカスは外れます。次にどこへ移すかはプログラムが決めます。

## 円の進み具合の表示

`lamina/widgets/progress_indicator.hpp`：Expressive の円形の進み具合の表示（`CircularProgressIndicator`）。表示（primary）は上から時計回りに伸び、下地（secondary container）が残りを占め、両端で 4 dp 離れます。進み具合が 0 のとき、下地は閉じた円になります。進み具合が 10 % から 95 % の間では、Compose の `CircularWavyProgressIndicator` と同じく、表示が 1 秒に 1 波長進む波になります。ただし波は円周に沿った長さの正弦で、振幅と波長を dp で持つので、大きな輪でもトークンのとおりの穏やかな波のままです。Compose の星形への変形では、大きな輪ほど波が深くなります（`docs/design-tokens.ja.md`）。振幅は遅い効果のばねで大きくなったり平らになったりします。`set_wavy(false)` で波を平らにでき、波が平らでほかに動くものがなければ、動きを止めます。`set_progress` は値にすぐ合わせるので、フレームごとに変わる進み具合に使います。`ease_progress` は、新しい値へ標準の空間のばねで移り、最後はちょうどその値になります。`set_colors` は別の役割の色へ入れ替えます。大きさ（既定は 48 dp）は `set_size` に従い、太さ・すき間・振幅・波長は、トークンの値を既定にした設定です。タイマーの見本は、これで輪を描きます。

## 読み込み中の表示

`LoadingIndicator`（`lamina/widgets/loading_indicator.hpp`）は、進みが決まっていないあいだ、48 dp の入れ物の中で七つの Material の形を順に変えていきます。新しい変形は 650 ms ごとに始まります。進みが決まっているときは、0 から 1 に進むにつれて円から `SoftBurst` の形に変わります（`LoadingIndicatorOptions::progress`・`set_progress`）。`contained` で `primary-container` の上に置き、`size` で全体の大きさを変えます。進み具合がいっぱいの表示を終わりの見えない形に変えると（`set_progress(std::nullopt)`）、回転はそのまま続き、大きさと中心は標準の効果のばねで終わりの見えない形のものへ移るので、形は跳びません。値は[読み込み中の表示](../design-tokens.ja.md#読み込み中の表示)にあります。

## 波のある進み具合の表示

`LinearWavyProgressIndicator` と `CircularWavyProgressIndicator`（`lamina/widgets/wavy_progress.hpp`）は、Expressive の波打つ表示を描きます。既定の大きさは横長が 240 × 10 dp、円が 48 dp で、どちらも線の太さ 4 dp、下の線までの隙間 4 dp です。`WavyProgressOptions::progress` を渡さないと進みの決まらない表示になり、`set_progress` で、飛ばずに決まった値の表示へ移ります。0 と 1 の近くでは波が平らになります（`wavy_indicator_amplitude`）。値は[波のある進み具合の表示](../design-tokens.ja.md#波のある進み具合の表示)にあります。

## 引っ張って更新

`lamina/widgets/pull_to_refresh.hpp` の `PullToRefresh` は、Material の「引っ張って更新」を Expressive の読み込み中の表示で描きます。Compose の `PullToRefreshBox` と `PullToRefreshDefaults.LoadingIndicator` に当たります。リストそのものは持ちません。プログラムが、リストの表示範囲（`set_area`）、リストより先にポインターの出来事、リストが一番上にあるかどうかを渡し、リストのあとに描きます。

- 引っ張り：リストが一番上にあるときに範囲の中で触れた指を見張ります（`handle(event, at_top)`。一番上から `top_tolerance` 以内なら一番上とみなします）。リストが一番上にあった位置から下へ `slop`（8 dp）より多く動き、真下から `max_angle_degrees`（45°）以内にあれば、引っ張りがその指を引き取ります。そのときの返り値は `claimed` で、プログラムはリストの押下とスクロールを取り消します。これでクリックも慣性のスクロールも起きません。それからは、その指の出来事にはすべて `consumed` が立ちます（`holds_pointer()` は真です）。スクロールのドラッグの途中でリストが一番上に着いたときは、その位置から数えます。一度のドラッグで一番上まで戻し、そのまま引っ張りに続けられます。上へのドラッグ、横へのドラッグ、一番上にないリスト、マウス、ペンでは何も起きません。lamina はマウスのドラッグでスクロールしないので、デスクトップでも引っ張りは指だけです。
- 手応え：指の動きの半分（`drag_multiplier`）が引っ張りの量です。しきい値（`threshold`、80 dp）までは表示がそのまま付いてきます。越えた分は、しきい値に対する越えた割合 x を `x - x² / 4` で弱めるので、表示はだんだん遅くなり、しきい値の 3 倍引っ張ったところで、しきい値の 2 倍の位置に止まります（`pull_offset`。`pull_distance` はその逆です）。`distance_fraction()` は表示の位置をしきい値で割ったものです。
- 表示：`primary-container` の 48 dp の円が、高さ 2 の影を付けて、範囲の上の端の向こうから入ってきます。上の端で切り抜きます。円の下の端は、上の端から `distance_fraction() × threshold` の位置です。中の形は読み込み中の表示の進み具合のある形（`on-primary-container`）で、割合が 0 から 1 へ進むにつれて円から SoftBurst に変わり、半回転逆に回り、`start_scale`（0.5）から元の大きさまで大きくなります。しきい値を越えると、しきい値ひとつ分の位置ごとに `overshoot_turn_degrees`（180°）だけ回り続けます。
- 離したとき：しきい値を越えていれば、返り値に `refresh` が立ち、更新中になります（`refreshing()`）。形はその場で終わりの見えない形に移ります。回転・大きさ・位置はそのまま引き継ぎます（[読み込み中の表示](#読み込み中の表示)）。表示は標準の空間のばねでしきい値の位置に落ち着きます。越えていなければ、同じばねで上に戻り、更新はしません。指を失ったとき（離さずに `leave` が来たとき）、`cancel_interaction()`、`set_enabled(false)`、二本目の指（プログラムが取り消しとして渡したとき）も、更新せずに戻ります。
- 更新中：プログラムは、作業が終わったら `set_refreshing(false)` を呼びます。表示は形を変えながら、ばねで上へ去ります。ただし、更新が始まってから少なくとも `min_refresh_seconds`（0.6 秒）のあいだは、しきい値の位置で回り続けます。1〜2 フレームで終わる更新でも、何かが起きたと分かるようにするためで、X のタイムラインと同じふるまいです。`refreshing()` はすぐに false になり、`spinning()` は表示が去り始めるまで true のままです。それまでは引っ張りを始めません。最短の時間より長い更新は、終わったらすぐに去ります。進み具合のある形に戻るのは、見えなくなってからです。`set_refreshing(true)` は、引っ張らずに表示を出します。キーやメニューから始めた更新に使います。更新中は引っ張りを始めないので、更新を二度求めることはありません。リストはふだんどおりスクロールします。
- 表示が戻る途中で始めた引っ張りは、表示をそのときの位置から引き継ぎます。
- 表示が出ているあいだ、`accessible()` は `accessible_name` を名前にした進み具合の表示のノードを返します。`animating()` は、表示が動いているか回っているあいだ真です。

値は[引っ張って更新](../design-tokens.ja.md#引っ張って更新)にあります。部品の例の Progress のページに見本があります。

## ツールチップ

`lamina/widgets/tooltip.hpp`：簡素なツールチップと詳しいツールチップ。好きな部品（`anchor_to(widget)`。その部品の位置と、`Focusable` ならフォーカスを追います）か四角形（`set_anchor`）に結びつけます。簡素なものは inverse-surface の上に inverse-on-surface の Body S で、角は 4 dp、余白は 8 × 4 dp、大きさは 40 × 24 dp 以上、幅は 200 dp までです。詳しいもの（`TooltipKind::rich`）は、見出し（Title S、なくてもよい）・本文（Body M）・テキストボタンの操作（`action_label`、なくてもよい）を、surface-container の上に角 12 dp・高さ Level 2 の影で並べ、幅は 320 dp までです。置き場所は結びつけた部品の 4 dp 上の中央で、上に入らないときは下にし、窓の中に収めます（`set_window`）。`TooltipPosition` で Compose の `TooltipAnchorPosition` と同じく下・左・右にも置け、`TooltipOptions::caret` は部品の真ん中を指す 16 × 8 dp の吹き出しの先を足します。速い空間のばねで 0.8 倍から大きくなり、速い効果のばねで現れます。マウスが部品に乗るとすぐ出て（Compose と同じです。`hover_delay_seconds` で待ちを入れられます）、離れると消えます。キーボードのフォーカスが部品に来ると 1.5 秒出ます。指かペンで部品を 0.5 秒押し続けると、離すまで出ます。Escape・ほかの場所を押すこと・フォーカスが外れることで消え、`persistent` にするとそのどれかまで残ります。待ち時間は `next_wake_seconds()` が返します。`handle` は `TooltipResponse` を返し、操作が押されると `action` が立ちます。ツールチップは `FocusGated` なので、操作を Tab の順に入れられます。`name_tooltip(widget, fonts)` は部品の読み上げ用の名前を出す簡素なツールチップを作ります。部品の一覧の例では、アイコンボタンにこれを付けています。ツールチップはほかのすべての後に描きます。

## バッジ

`lamina/widgets/badge.hpp`：小さいバッジ（6 dp の点）と大きいバッジ（高さ 16 dp、幅 16 dp 以上、両端の内側に 4 dp、error の上に on-error の Label S）。Compose の `Badge` と `BadgeTokens` から取りました。`Badge(BadgeSpec{text, accessible_description}, fonts)` は、文字が空なら点になります。`badge_rect(anchor, size, has_content, bounds)` は `BadgedBox` と同じ場所に置きます。点は印の終わりの辺から 6 dp 内側で、上の辺にそろえます。大きいバッジは終わりの辺の 12 dp 手前から始まり、上の辺より 2 dp 上に出て、終わりの側へ伸びます。`bounds` を渡すとその中に収めます。ナビゲーションの項目では、`badgeBounds()` の定規がバッジを項目の中に収めるのと同じです。`badge_count(n, max = 999)` は、上限を超えると `"999+"` を返します。Compose の見本と同じ書き方で、Compose 自身は文字を切りません。`NavigationItem::badge` は、ナビゲーションバーとレールの項目の 24 dp のアイコンにバッジを付け、項目の中に収めます。`set_item_badge(index, spec)` で変えられます。`IconButton::set_badge` はアイコンボタンのアイコンに付けます。大きさは Compose と同じく文字に合わせてすぐ変わります。バッジの説明（なければ文字）は、項目やボタンの読み上げ用の `description` になります。Compose はこれをプログラムに任せていて（`NavigationBarItem` はアイコンの意味づけを消します）、lamina-ui が足したものです。

## 出典

- Material 3 の スナックバー：<https://m3.material.io/components/snackbar/overview>
- Material 3 の 進み具合の表示：<https://m3.material.io/components/progress-indicators/overview>
- Material 3 の 読み込み中の表示：<https://m3.material.io/components/loading-indicator/overview>
- Material 3 の 引っ張って更新（進み具合の表示の中）：<https://m3.material.io/components/progress-indicators/overview>
- Material 3 の ツールチップ：<https://m3.material.io/components/tooltips/overview>
- Material 3 の バッジ：<https://m3.material.io/components/badges/overview>
- 値ごとのソースのファイルとコミット：[デザイントークン](../design-tokens.ja.md)
