# Material 3 との違い

[English](differences-from-m3.md) · [README](../README.ja.md#資料)

lamina-ui が Material 3 とその Compose の実装に従っていないところをまとめます。自分で決めた値、Material 3 に無い部品、ふるまいの違いです。どの項目にも、値の全部と理由を書いた[デザイントークン](design-tokens.ja.md)の節へのリンクを付けています。

## Material 3 に無い部品

- スクロールの領域：Material 3 はスクロールバーを決めていないので、`lamina/tokens/scroll_area.hpp` の値は横のスクロールも含めてすべて lamina-ui のものです。指でのスクロールは Compose と Android に合わせましたが、端で伸びる動きと入れ子のスクロールはなく、入力欄は押したときではなく指を離したときに焦点を受けます。[スクロールする範囲](design-tokens.ja.md#スクロールする範囲)・[スクロールの範囲：横](design-tokens.ja.md#スクロールの範囲横)・[指でのスクロール](design-tokens.ja.md#指でのスクロール)
- 横のナビゲーション：アイコンだけのレール・幅のあるレール・ドロワーを一つの並びで表します。ばね（減衰 1.0、硬さ 380）と名前の消え方は lamina-ui の値です。[横のナビゲーション](design-tokens.ja.md#横のナビゲーション)
- 分割ペイン・数値欄・メニューバー：M3 の色の役割から組み立て、大きさは lamina-ui が決めました。[分割ペイン](design-tokens.ja.md#分割ペイン)・[数値欄](design-tokens.ja.md#数値欄)・[メニューバー](design-tokens.ja.md#メニューバー)
- データの表：MDC Web のデータの表にある値はそこから取り、無いものは lamina-ui が決めました。[表](design-tokens.ja.md#表)
- 欄の下に開く日付の選択：Compose にはありません。浮かぶ入れ物の値は material-web から取り、4 dp の隙間と 0.8 からの拡大は lamina-ui の値です。[日付の選択](design-tokens.ja.md#日付の選択)
- サイドシートの見出しと操作の行：決めているソースが無いので、72 dp の見出しと操作の行は lamina-ui の形です。[サイドシート](design-tokens.ja.md#サイドシート)
- ページ送り：Material 3 はタブを Compose の基盤の `HorizontalPager` と組ませますが、そのソースはこの機械にありませんでした。しきい値は Android 自身の `ViewPager`（プラットフォームの内部の写し）から取りました。落ち着く動きは ViewPager の 600 ms の `Scroller` ではなくばねで、向きの決め方は lamina-ui のスクロールと同じ 30° です。[ページ送り](design-tokens.ja.md#ページ送り)
- 拡大の表示：Material 3 には拡大できる画像の部品がありません。指の操作のしきい値は Android の `ScaleGestureDetector` と `GestureDetector` から取りました。倍率の範囲（1〜5）、二度たたいたときの倍率（2.5）、ホイールの一段、ゴムのような手応え、戻る動きは lamina-ui の値です。[拡大の表示](design-tokens.ja.md#拡大の表示)

## lamina-ui が決めた値

- ナビゲーションスイート：`widening_navigation_suite`（`compact` はバー、`medium` は畳んだレール、`expanded` は広げたレール、`large` より広いとドロワー）は lamina-ui の選び方です。バーと横の並びは、入れ替えるのではなくばねで移り変わります。[ナビゲーションスイートの足場](design-tokens.ja.md#ナビゲーションスイートの足場)
- ナビゲーションバーとレールの名前を隠せるのは lamina-ui が足したものです。[ナビゲーションバー](design-tokens.ja.md#ナビゲーションバー)・[ナビゲーションレール](design-tokens.ja.md#ナビゲーションレール)
- ダイアログ：Compose は開くときと閉じるときの動きを OS に任せます。lamina-ui は、標準の空間のばねで 0.8 から大きくし、標準の効果のばねで本体と幕を薄れさせます。[ダイアログ](design-tokens.ja.md#ダイアログ)
- スナックバー：操作のあるスナックバーも、指定が無ければ短い表示時間です。ポインターが上にあるあいだと、操作にフォーカスがあるあいだは時間を止めます（WCAG 2.1 の 2.2.1）。操作のフォーカスの輪は `inverse-primary` です。[スナックバー](design-tokens.ja.md#スナックバー)・[項目の並ぶ部品のキー](design-tokens.ja.md#項目の並ぶ部品のキー)
- ツールチップ：長押しの時間は 0.5 秒で、ポインターを乗せてから出るまでの時間はトークンで変えられます。[ツールチップ](design-tokens.ja.md#ツールチップ)
- ドラッグ：8 dp 動いてから始まり、離したときの速さは 8000 dp/s までに抑え、減速の段階はありません。[ドラッグの操作](design-tokens.ja.md#ドラッグの操作)
- 密度：部品の最小より下の値は、誤りにせず最小に収めます。セグメントボタン・ナビゲーションの項目・メニューの最小は lamina-ui の値です。[密度](design-tokens.ja.md#密度)
- 円形の進み具合の表示：弧を 1 dp ごとに区切って描きます（`sample_length`）。[円形の進み具合の表示](design-tokens.ja.md#円形の進み具合の表示)
- 横に送れるタブ：最小の幅 90 dp、端の余白 52 dp、標準の空間のばねは Compose と同じです。ホイールの 1 ノッチの量（64 dp、スクロールの領域と同じ）は lamina-ui の値です。[タブ](design-tokens.ja.md#タブ)

## ふるまいの違い

- Compose がすぐに切り替える色を、標準の効果のばねで移します。セグメントボタン・切り替えボタン・分割ボタン・日付の選択で選んだ日が当たります。[セグメントボタン](design-tokens.ja.md#セグメントボタン)・[切り替えボタンと角ごとの形](design-tokens.ja.md#切り替えボタンと角ごとの形)
- 無効のタブを 38 % で描きます。Compose では有効と同じに見えます。[無効の状態](design-tokens.ja.md#無効の状態)
- 枠線のアイコンボタンは、大きさごとの枠の太さ（大 2 dp、特大 3 dp）を使います。Compose はいつも 1 dp です。[アイコンボタン](design-tokens.ja.md#アイコンボタン)
- スイッチ・チェックボックス・ラジオボタンの状態の重ねは、周りの中身の色ではなく `on-surface` です。[選ぶ部品の状態の重ね](design-tokens.ja.md#選ぶ部品の状態の重ね)
- 外せる入力チップには、外すための押す場所があり、Delete か Backspace でも外せます。[チップ](design-tokens.ja.md#チップ)
- セグメントボタンの列は Tab の止まり位置が一つで、矢印のキーで動きます。日付の選択は WAI-ARIA の日付の表のキーに従います。[セグメントボタン](design-tokens.ja.md#セグメントボタン)・[日付の選択](design-tokens.ja.md#日付の選択)
- セグメントボタンの文字や分割ボタンの前の半分の文字が収まらないときは、1 行のまま「…」で切ります。Compose の既定のままの `Text` は折り返します。セグメントボタンの行は、どの区画にも左右の余白・アイコンの場所・「…」が残る幅で縮むのを止めます。Compose の重みのある区画には下限がありません。[セグメントボタン](design-tokens.ja.md#セグメントボタン)・[分割ボタン](design-tokens.ja.md#分割ボタン)
- 24 時間の表示では、時刻の選択のつまみがポインターに付いて動きます。12 時間の表示では、0 と 00 を正しくない時として扱います。[時刻の選択](design-tokens.ja.md#時刻の選択)
- 無効にした浮かぶツールバーは、FAB も薄くして反応しなくします。Compose の FAB には無効の状態がありません。[浮かぶツールバー](design-tokens.ja.md#浮かぶツールバー)
- ぼかした影は、形を一度ぼかした画像から、いちばん近い整数の画素の位置に描きます。面の端数の位置から最大で半画素ずれます。[高さと状態の重ね](design-tokens.ja.md#高さと状態の重ね)
- 48 dp のタッチの範囲は、当たり判定を広げるだけで、並べる場所は取りません。Compose の `minimumInteractiveComponentSize` は 48 dp の場所を取ります。また、タッチだけでなくマウスとペンでも広げます。Compose が広げるのはタッチだけです。負の密度では範囲を外します。[タッチの範囲](design-tokens.ja.md#タッチの範囲)

デザイントークンのページには、部品ごとの細かな違いも「違い」としてまとめています。
