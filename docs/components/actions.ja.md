# 操作

[English](actions.md) · [README](../../README.ja.md#資料)

ボタンと、ボタンから組み立てた部品です。

## ボタン

`lamina/widgets/button.hpp`：Material 3 Expressive のボタン。5 種類（filled・tonal・outlined・elevated・text）・5 段階の大きさ・2 種類の形。押すと、Compose と同じく効果のばねで角が押したときの形に変わり、マウスを乗せたときと押したときには状態の重ねと高さが変わります。`ButtonOptions::leading_icon` でラベルの前にアイコンを置けます。大きさと間隔は大きさのトークンに従います。ラベルが空なら、アイコンだけが中央に置かれます。ボタンの高さは大きさのトークンで決まり、幅は中身で決まります。`set_size(width, height)` を使うと、ほかの大きさにできます（電卓のキーはこれで格子の枠いっぱいになります）。そのときもラベル・アイコン・角は大きさのトークンに従い、丸い形のボタンは新しい高さでも丸いままです。`ButtonOptions::colors` は、Compose の `ButtonDefaults.buttonColors` と同じく、種類が決める入れ物と中身の色の役割を差し替えます。高さと輪郭は種類のままです。入れ物を `std::nullopt` にすると入れ物を描かないので、スナックバーの操作のように、ほかの面の上で中身の色だけを変えたテキストボタンにできます。`set_label` はラベルを変えます（大きさを与えていないボタンは幅を合わせ直します）。`set_content_scale(label, icon)` は、ラベルとアイコンをトークンより大きく、または小さく描きます。電卓は、これでキーの文字をキーの大きさに合わせて大きくします。`set_shape(shape)` は、角を丸と四角の間で速い空間のばねで変えるので、タイマーの一時停止と再開のキーのような切り替えは少し行き過ぎて戻ります。`set_icon` は、速い効果のばねで古いアイコンから新しいアイコンへ入れ替えます。`ButtonOptions::icon_style` は、アイコンを Material Symbols の別の軸の値で描きます（タイマーの再生と一時停止は塗り 1 です）。押したときの形の変化はこの形の上に重なるので、両方が同時に動けます。`cancel_interaction()` は、ポインターの出来事なしにマウスを乗せた状態と押した状態を取り消すので、押している途中のものはクリックになりません。ポインターの入力を受け取る部品はどれもこれを持ちます（スライダーではドラッグを、入力欄ではドラッグでの選択を、ダイアログではボタンへの押し込みを終わらせます）。ダイアログは閉じるときに自分のボタンにこれを使い、`WidgetSet::cancel_interaction()` はこれを持つ部品に伝えます。`build/Release/buttons.exe` で全部を並べて見られます。

## 切り替えボタン

`ButtonOptions::toggle`（`ButtonToggle`）を渡すと、ボタンは切り替えボタンになります。`checked` が状態で、読み上げには `toggle_button` とオンかオフを知らせます。ラジオの切り替えにすると `radio_button` を知らせ、クリックでオフにはなりません。止まっているときは両端の丸い形で、押している間は大きさごとの押したときの角、オンのときは大きさごとの四角い角になります。どの角も一つの進みで、速い空間のばねで形を変えます。オンの色は標準の効果のばねで移ります。`toggle_button_shapes(size)` は、大きさごとの M3 の切り替えボタンの形を返します。値は[切り替えボタンと角ごとの形](../design-tokens.ja.md#切り替えボタンと角ごとの形)にあります。

## アイコンボタン

`lamina/widgets/icon_button.hpp`：アイコンボタン。4 種類（filled・tonal・outlined・standard）・5 段階の大きさ・2 種類の形・3 種類の幅（narrow・uniform・wide）。ボタンと同じく、押すと効果のばねで角が変わります。切り替えのアイコンボタンは、オンのときにも形と色が変わります。`set_icon` は、ボタンと同じく速い効果のばねで別のアイコンへ入れ替えます。

## FAB

`lamina/widgets/fab.hpp`：フローティングアクションボタン。3 段階の大きさ（FAB・medium・large）で、地は primary-container か secondary-container です。高さは段階 3 で、マウスを乗せると段階 4 になります。

## 拡張 FAB

`ExtendedFab`（`lamina/widgets/extended_fab.hpp`）はラベルのある FAB で、大きさは三つです（`tokens::ExtendedFabSize`）：小 56 dp・中 80 dp・大 96 dp。`set_expanded(false)` で四角い FAB にたたみます。幅は速い空間のばねで高さまで縮み、ラベルは速い効果のばねで消え、アイコンは動きません。Compose と同じく、無効の状態はありません。値は[拡張 FAB](../design-tokens.ja.md#拡張-fab)にあります。

## FAB メニュー

`FabMenu`（`lamina/widgets/fab_menu.hpp`）は、切り替えの FAB（`ToggleFabOptions`）と、その上に開くラベルつきの項目（`FabMenuItem`）の列です。列は始まりか終わりにそろえます（`FabMenuAlignment`）。開くとボタンは角 28 dp の 56 dp に縮み、色は `primary-container` から `primary` に変わります。項目はボタンに近いものから順に現れます。`handle` は `toggled` と `chosen` を持つ `FabMenuResponse` を返します。開いたボタンから Tab か下の矢印で項目に入り、Enter で選び、Escape で閉じます。値は [FAB メニュー](../design-tokens.ja.md#fab-メニュー)にあります。

## セグメントボタン

`lamina/widgets/segmented_button.hpp`：Compose の `SingleChoiceSegmentedButtonRow` と `MultiChoiceSegmentedButtonRow`（`SegmentSelection`）。区画は高さ 40 dp、幅はいちばん広い区画にそろえ（最小 58 dp）、左右に 12 dp、18 dp のアイコンの場所と、Label L の文字の前に 8 dp を空けます。1 dp の枠線は隣の区画と重なり、最初と最後の区画は外側の角を丸くします。選ばれた区画は secondary-container で塗ります。アイコンのない区画もアイコンの場所を取っておき、選ばれていない間は文字を 13 dp 戻します。選ぶとチェックの印が左下から速い空間のばねで大きくなりながら文字が横へずれ、外すと印はすぐに消えて文字が戻ります（Compose の `SegmentedButtonDefaults.Icon`）。アイコンのある区画は、標準の効果のばねでアイコンとチェックの印を入れ替えます。アイコンだけの区画には `accessible_name` が要ります。行全体で Tab の止まり位置は一つです。一つだけ選ぶ行では、ラジオボタンの組と同じく `←`・`→`・`Home`・`End` で選び、いくつでも選べる行では、それらでフォーカスを動かして `Space` か `Enter` で切り替えます。読み上げでは、`group` の中のラジオボタンか切り替えボタンです。

行の幅は、区画が必要とする幅です（`natural_width()`）。それより狭い幅を `measure` と `arrange`（`widgets::node` が使います）か `set_width` で渡すと、区画は同じ幅のまま `min_width()` まで縮みます。`min_width()` は、どの区画にも左右の余白・アイコンの場所・「…」の入る幅が残り、しかも 58 dp 以上ある幅です。収まらない文字は 1 行のまま「…」で切ります（`shown_label`・`label_ellipsized`）。`SegmentedButtonRowOptions::fill_width` を付けると、行は渡された幅をすべて使い、区画はやはり同じ幅です。区画は毎コマ渡された幅に付いていくので、滑って動く親の中や、大きさを変えている窓の中でも跳びません。行に `widgets::node` を使うと、これまでは本来の幅で置いていましたが、今は測って縮めます。

## ボタンの組

`ButtonGroup`（`lamina/widgets/button_group.hpp`）は `ButtonGroupItem` を 12 dp あけて並べます。`ButtonGroupOptions::connected` にすると 2 dp あけ、つながった形にします。`selection` は `none`（ふつうのボタン）・`single`（ラジオの切り替え）・`multiple`（切り替えボタン）です。押したボタンは幅の 15 % までとなりへ広がり、となりは同じだけ縮むので、列の幅は変わりません（`animate_width`）。入りきらないボタンは「その他のオプション」のアイコンボタンの後ろのメニューに移ります。`handle` は `clicked` を持つ `ButtonGroupResponse` を返します。値は[ボタンの組](../design-tokens.ja.md#ボタンの組)にあります。

## 分割ボタン

`SplitButton`（`lamina/widgets/split_button.hpp`）は、前のボタンと後ろの山形の印を 2 dp あけて並べたものです。大きさはボタンの五つ（32・40・56・96・136 dp）、形は `filled`・`tonal`・`outlined`・`elevated` です。後ろの半分をオンにすると、山形が半回転し、ボタン全体の下に `SplitButtonOptions::menu` を開きます。項目を選ぶかメニューを閉じるとオフに戻ります。`handle` は `leading_clicked`・`trailing_toggled`・`chosen` を持つ `SplitButtonResponse` を返します。本来の幅（`natural_width()`）より狭い幅を `measure` と `arrange` か `set_width` で渡すと、後ろの半分と 2 dp の間はそのままで、前の半分が残りの幅を取り、`min_width()` まで縮みます。前の半分は左右の余白とアイコンを残し、文字を「…」で切ります。ボタンが文字を切るのは `ButtonOptions::ellipsize_label` を付けたときだけで、分割ボタンは前の半分にこれを付けます（`Button::min_width`・`label_ellipsized`・`shown_label`）。分割ボタンに `widgets::node` を使うと、セグメントボタンと同じく測って縮めます。値は[分割ボタン](../design-tokens.ja.md#分割ボタン)にあります。

## 出典

- Material 3 の ボタン：<https://m3.material.io/components/buttons/overview>
- Material 3 の アイコンボタン：<https://m3.material.io/components/icon-buttons/overview>
- Material 3 の FAB：<https://m3.material.io/components/floating-action-button/overview>
- Material 3 の 拡張 FAB：<https://m3.material.io/components/extended-fab/overview>
- Material 3 の セグメントボタン：<https://m3.material.io/components/segmented-buttons/overview>
- Material 3 の ボタングループ：<https://m3.material.io/components/button-groups/overview>
- Material 3 の 分割ボタン：<https://m3.material.io/components/split-button/overview>
- Material 3 の FAB メニュー：<https://m3.material.io/components/fab-menu/overview>
- 値ごとのソースのファイルとコミット：[デザイントークン](../design-tokens.ja.md)
