# 選択

[English](selection.md) · [README](../../README.ja.md#資料)

## スイッチ

`lamina/widgets/switch.hpp`：スイッチ。つまみは速い空間のばねで滑りながら大きさを変え（オフで 16 dp、オンかアイコンつきで 24 dp、押している間は 28 dp）、expressive では少し行き過ぎて戻ります。つまみには、オンとオフそれぞれのアイコンを載せられます。

## チェックボックス

`lamina/widgets/checkbox.hpp`：チェックボックス。オフ・オン・中間の 3 状態です。チェックの印は空間のばねで線の長さに沿って描かれ、中間の状態では横線へ寄っていきます。オフにすると、Compose と同じく箱の色が薄れる間に 100 ms 遅れて印が消えます。

## ラジオボタン

`lamina/widgets/radio_button.hpp`：ラジオボタン。点は速い空間のばねで大きくなります。

## スライダー

`lamina/widgets/slider.hpp`：Expressive のスライダー。4 × 44 dp のつまみが 16 dp の通り道を、動いた側（primary）と残りの側（secondary container）に分けます。つまみの両側には 6 dp の隙間があり、つまみの側の角は 2 dp です。終わりには 4 dp の点が付きます。スライダーのどこかを押すかドラッグすると、ポインターの位置から値が決まります。ドラッグの途中でポインターがスライダーの外に出ても追います。押している間、つまみは Compose と同じくすぐに 2 dp に細くなります。`SliderOptions::steps` を指定すると、値は等しい間隔の目盛りに寄ります。スライダーは `Focusable` で、`handle(KeyEvent)` はキーボードのフォーカスがある間だけ値を動かします。右か上の矢印と左か下の矢印で 1 段（段が無ければ 1 %）、`Page Up` と `Page Down` で段の数の 10 分の 1（1 段から 10 段まで。段が無ければ 10 %）、`Home` と `End` で両端へ動き、値が変わったとき true を返します。フォーカスがあると、押したときと同じくつまみが細くなり、つまみの周りにフォーカスの輪を描きます。押すとフォーカスは外れるので、クリックしただけではキーは届きません。`SliderOptions::orientation` で縦向きになります（Compose の `VerticalSlider`）。つまみは 44 × 4 dp、トラックは幅 16 dp で、`top_to_bottom` を false にしない限り最小値が上です。キーは `↑`・`↓`・`Page Up`・`Page Down`・`Home`・`End` だけで、描いた向きに動きます。`centered` は Compose の `CenteredTrack` で、塗った部分が中央からつまみまで伸び、つまみと反対側に 6 dp の隙間があり、両端に止まりの点が付きます。`track_corner` はトラックの外側の角（既定は太さの半分）を決め、Compose の `Track(trackCornerSize)` と同じく、段がなければ塗っていない部分を幅 0 まで残します。`value_label`（値から文字を作る関数。文字を渡すコンストラクタと一緒に使います）を渡すと、つまみを押している間とフォーカスがある間、つまみの上に値の表示が出て、その文字が読み上げ用の値になります。inverse-surface の丸い札に Label L の文字で、つまみの 12 dp 上に置き、下端から 100 ms の強調の曲線で大きくなります。値の表示は横向きのスライダーだけに出ます。Compose にはトークン（`SliderTokens.ValueIndicator*`）しかないので、大きさと動きは material-web の `labeled` のスライダーに合わせました。スライダーは `Measurable` で、横向きは渡された幅、縦向きは渡された高さを取るので、`widgets::node` でこれまでどおり置けます。`lamina/widgets/slider_track.hpp` に、スライダーと範囲スライダーが共有するトラックの描画・段への合わせ方・キーの段の幅があります。

## 範囲スライダー

`lamina/widgets/range_slider.hpp`：Compose の `RangeSlider`。一本のトラックに二つのつまみがあり、つまみの間を塗り、外側を塗らずに、両端に止まりの点を付けます。`steps` は両方を同じ目盛りに合わせます。押すと近いほうのつまみが動きます（同じ距離なら、押した位置がつまみより前なら始まりのつまみ、それ以外は終わりのつまみ）。つまみが重なっているときは、ドラッグの向きで動くほうを選びます。重なることはあっても、追い越すことはありません。つまみはそれぞれが Tab の止まり位置です。`thumb(RangeThumb::start)` と `thumb(RangeThumb::end)` は `Focusable` で `FocusGated` なので、プログラムが `FocusManager` に足します。キーはスライダーと同じで、`Home` と `End` はもう一方のつまみで止まります。読み上げでは `group` の中に、つまみごとの `slider` があり、`RangeSliderOptions::names`（既定は Compose の "Range start" と "Range end"）で名前を付け、範囲はもう一方のつまみまでです。`value_label` はスライダーと同じく、それぞれのつまみに値の表示を出します。

## チップ

`lamina/widgets/chip.hpp`・`chip_group.hpp`：アシスト・フィルター・入力・提案のチップ（`ChipKind`）。平らなものと浮いたもの（`ChipOptions::elevated`。入力チップには浮いたものがありません）があります。高さは 32 dp、角は 8 dp、文字は Label L です。Compose の `ChipArrangement` と同じく、文字の両側に 16 dp、18 dp のアイコンの前に 8 dp を空けます。入力チップは、何もない端とアバターの側で 4 dp、アイコンの側で 8 dp です。色・枠線・高さは Compose の `*ChipDefaults` のものです（`tokens/chip.hpp`）。フィルターと入力のチップは、クリック・Space・Enter で選ばれた状態が切り替わり、Compose の `Role.Checkbox` と同じく、選ばれているかどうかをチェックボックスとして伝えます。アシストと提案のチップはボタンです。フィルターチップの `selected_leading_icon`（チェックの印）は、選ばれている間だけ先頭のアイコンの代わりになります。先頭にアイコンのないチップは、Compose の `AnimatingChipContent` と同じく、速い空間のばねで場所を広げながら遅い効果のばねでアイコンを浮かび上がらせ、外すときは標準の効果のばねで縮めながら速い効果のばねで消します。先頭にアイコンのあるチップは、すぐに入れ替えます。選ばれたときの色は標準の効果のばねで移ります。Compose はすぐに切り替えます。入力チップは `avatar`（24 dp の円に切り抜いた `render::Image`。無効の間は 38 %）を持てます。`removable` と末尾のアイコンがあれば、外すための当たりを持ちます。末尾のアイコンのクリック、またはフォーカスがある間の Delete か Backspace は、選ぶ代わりに `ChipResponse::removed` を返します。高さ（影）は Compose の高さのトゥイーン（状態へ向かうときは 120 ms の `FastOutSlowInEasing`、戻るときは 150 ms、マウスが離れたときは 120 ms）で動き、無効になるとすぐに切り替わります。Compose の `animateElevation` と同じです（`widgets/interaction_elevation.hpp`。カードも使います）。`ChipGroup` はチップを並べ、material-web のチップの組と同じく、縦横 8 dp の間を空けて折り返します。`single_selection` で一つだけ選べるようになります。組は Tab の止まり位置一つで、左右の矢印で有効なチップの間をフォーカスが動き（端では反対の端へ戻ります）、Home と End で端へ行き、Space・Enter・Delete・Backspace はフォーカスのあるチップに届きます。外したチップは標準の効果のばねで縮みながら消え、後ろのチップが詰めます。別の行へ移るチップは標準の空間のばねで滑って移ります。`ChipGroupResponse` は、クリックしたか外した位置と、外したチップの文字を返します。

## 出典

- Material 3 の スイッチ：<https://m3.material.io/components/switch/overview>
- Material 3 の チェックボックス：<https://m3.material.io/components/checkbox/overview>
- Material 3 の ラジオボタン：<https://m3.material.io/components/radio-button/overview>
- Material 3 の スライダー：<https://m3.material.io/components/sliders/overview>
- Material 3 の チップ：<https://m3.material.io/components/chips/overview>
- 値ごとのソースのファイルとコミット：[デザイントークン](../design-tokens.ja.md)
