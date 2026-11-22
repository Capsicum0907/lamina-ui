# 知らせ

[English](feedback.md) · [README](../../README.ja.md#資料)

## スナックバー

`lamina/widgets/snackbar.hpp`：スナックバー。窓の下に短い知らせを出し、テキストボタンの操作を一つ付けられます（`SnackbarOptions::action_label`）。`set_area(rect)` で置く場所を渡すと、下から 12 dp 上の中央に、場所の幅いっぱい（最大 600 dp）で置きます。高さは Body M の 1 行で 48 dp（2 行で 68 dp）、面は `inverse_surface`、角は 4 dp、影は Level 3 です。知らせの文字は `inverse_on_surface`、操作は `inverse_primary` です。`show(message)` は Compose の `SnackbarHost` と同じく、0.8 倍から速い空間のばねで大きくしながら速い効果のばねで現し、時間を数え直します。`SnackbarOptions::duration_seconds`（既定は Compose の短い時間の 4 秒）がたつと自分で消えますが、ポインターが乗っている間は時間が進みません。残りの時間は動きではなく起きる時刻（`next_wake_seconds()`）として知らせるので、待つ間も窓は止まっていられます。`handle(PointerEvent)` は `SnackbarResponse` を返し、操作が押されたら `action`（スナックバーは閉じます）、ポインターが上にある間は `consumed` が立ちます。そのため `OverlayStack` のモードレスな重なりとして使えます。操作のボタンはキーボードのフォーカスを受けます。スナックバーは `Focusable` で `FocusGated`（表示中で操作があるとき `can_focus()` が true）なので、プログラムは `FocusManager` に加えます。フォーカスのある操作で `Space` か `Enter` を押すと、`handle(KeyEvent)` はクリックと同じ `SnackbarResponse` を返して閉じます。操作にフォーカスがある間は、ポインターが乗っている間と同じく時間が進まないので、キーボードで操作している最中に消えることはありません（WCAG 2.2.1「タイミング調整可能」）。閉じるとフォーカスは外れます。次にどこへ移すかはプログラムが決めます。

## 円の進み具合の表示

`lamina/widgets/progress_indicator.hpp`：Expressive の円形の進み具合の表示（`CircularProgressIndicator`）。表示（primary）は上から時計回りに伸び、下地（secondary container）が残りを占め、両端で 4 dp 離れます。進み具合が 0 のとき、下地は閉じた円になります。進み具合が 10 % から 95 % の間では、Compose の `CircularWavyProgressIndicator` と同じく、表示が 1 秒に 1 波長進む波になります。ただし波は円周に沿った長さの正弦で、振幅と波長を dp で持つので、大きな輪でもトークンのとおりの穏やかな波のままです。Compose の星形への変形では、大きな輪ほど波が深くなります（`docs/design-tokens.ja.md`）。振幅は遅い効果のばねで大きくなったり平らになったりします。`set_wavy(false)` で波を平らにでき、波が平らでほかに動くものがなければ、動きを止めます。`set_progress` は値にすぐ合わせるので、フレームごとに変わる進み具合に使います。`ease_progress` は、新しい値へ標準の空間のばねで移り、最後はちょうどその値になります。`set_colors` は別の役割の色へ入れ替えます。大きさ（既定は 48 dp）は `set_size` に従い、太さ・すき間・振幅・波長は、トークンの値を既定にした設定です。タイマーの見本は、これで輪を描きます。

## 読み込み中の表示

`LoadingIndicator`（`lamina/widgets/loading_indicator.hpp`）は、進みが決まっていないあいだ、48 dp の入れ物の中で七つの Material の形を順に変えていきます。新しい変形は 650 ms ごとに始まります。進みが決まっているときは、0 から 1 に進むにつれて円から SoftBurst に変わります（`LoadingIndicatorOptions::progress`・`set_progress`）。`contained` で primary-container の上に置き、`size` で全体の大きさを変えます。値は[読み込み中の表示](../design-tokens.ja.md#読み込み中の表示)にあります。

## 波のある進み具合の表示

`LinearWavyProgressIndicator` と `CircularWavyProgressIndicator`（`lamina/widgets/wavy_progress.hpp`）は、Expressive の波打つ表示を描きます。既定の大きさは横長が 240 × 10 dp、円が 48 dp で、どちらも線の太さ 4 dp、下の線までの隙間 4 dp です。`WavyProgressOptions::progress` を渡さないと進みの決まらない表示になり、`set_progress` で、飛ばずに決まった値の表示へ移ります。0 と 1 の近くでは波が平らになります（`wavy_indicator_amplitude`）。値は[波のある進み具合の表示](../design-tokens.ja.md#波のある進み具合の表示)にあります。

## ツールチップ

`lamina/widgets/tooltip.hpp`：簡素なツールチップと詳しいツールチップ。好きな部品（`anchor_to(widget)`。その部品の位置と、`Focusable` ならフォーカスを追います）か四角形（`set_anchor`）に結びつけます。簡素なものは inverse-surface の上に inverse-on-surface の Body S で、角は 4 dp、余白は 8 × 4 dp、大きさは 40 × 24 dp 以上、幅は 200 dp までです。詳しいもの（`TooltipKind::rich`）は、見出し（Title S、なくてもよい）・本文（Body M）・テキストボタンの操作（`action_label`、なくてもよい）を、surface-container の上に角 12 dp・高さ Level 2 の影で並べ、幅は 320 dp までです。置き場所は結びつけた部品の 4 dp 上の中央で、上に入らないときは下にし、窓の中に収めます（`set_window`）。`TooltipPosition` で Compose の `TooltipAnchorPosition` と同じく下・左・右にも置け、`TooltipOptions::caret` は部品の真ん中を指す 16 × 8 dp の吹き出しの先を足します。速い空間のばねで 0.8 倍から大きくなり、速い効果のばねで現れます。マウスが部品に乗るとすぐ出て（Compose と同じです。`hover_delay_seconds` で待ちを入れられます）、離れると消えます。キーボードのフォーカスが部品に来ると 1.5 秒出ます。指かペンで部品を 0.5 秒押し続けると、離すまで出ます。Escape・ほかの場所を押すこと・フォーカスが外れることで消え、`persistent` にするとそのどれかまで残ります。待ち時間は `next_wake_seconds()` が返します。`handle` は `TooltipResponse` を返し、操作が押されると `action` が立ちます。ツールチップは `FocusGated` なので、操作を Tab の順に入れられます。`name_tooltip(widget, fonts)` は部品の読み上げ用の名前を出す簡素なツールチップを作ります。部品の一覧の例では、アイコンボタンにこれを付けています。ツールチップはほかのすべての後に描きます。

## バッジ

`lamina/widgets/badge.hpp`：小さいバッジ（6 dp の点）と大きいバッジ（高さ 16 dp、幅 16 dp 以上、両端の内側に 4 dp、error の上に on-error の Label S）。Compose の `Badge` と `BadgeTokens` から取りました。`Badge(BadgeSpec{text, accessible_description}, fonts)` は、文字が空なら点になります。`badge_rect(anchor, size, has_content, bounds)` は `BadgedBox` と同じ場所に置きます。点は印の終わりの辺から 6 dp 内側で、上の辺にそろえます。大きいバッジは終わりの辺の 12 dp 手前から始まり、上の辺より 2 dp 上に出て、終わりの側へ伸びます。`bounds` を渡すとその中に収めます。ナビゲーションの項目では、`badgeBounds()` の定規がバッジを項目の中に収めるのと同じです。`badge_count(n, max = 999)` は、上限を超えると `"999+"` を返します。Compose の見本と同じ書き方で、Compose 自身は文字を切りません。`NavigationItem::badge` は、ナビゲーションバーとレールの項目の 24 dp のアイコンにバッジを付け、項目の中に収めます。`set_item_badge(index, spec)` で変えられます。`IconButton::set_badge` はアイコンボタンのアイコンに付けます。大きさは Compose と同じく文字に合わせてすぐ変わります。バッジの説明（なければ文字）は、項目やボタンの読み上げ用の `description` になります。Compose はこれをプログラムに任せていて（`NavigationBarItem` はアイコンの意味づけを消します）、lamina-ui が足したものです。

## 出典

- Material 3 の スナックバー：<https://m3.material.io/components/snackbar/overview>
- Material 3 の 進み具合の表示：<https://m3.material.io/components/progress-indicators/overview>
- Material 3 の 読み込み中の表示：<https://m3.material.io/components/loading-indicator/overview>
- Material 3 の ツールチップ：<https://m3.material.io/components/tooltips/overview>
- Material 3 の バッジ：<https://m3.material.io/components/badges/overview>
- 値ごとのソースのファイルとコミット：[デザイントークン](../design-tokens.ja.md)
