# テーマとトークン

[English](theming.md) · [README](../README.ja.md#資料)

設計の値として、配色・形・文字・高さ・状態の重ね・動きについて書きます。値の全部と出典は[デザイントークン](design-tokens.ja.md)に、lamina-ui が自分で値を決めたところは [Material 3 との違い](differences-from-m3.ja.md)にあります。密度は[レイアウト](layout-and-adaptive.ja.md#密度)にあります。

## 色

`lamina/color/scheme.hpp`：1 色から作る配色。`spec_2021().build(options)` と `spec_2025().build(options)` は、元の色・ライトかダークか・配色の種類（TonalSpot・Expressive・Vibrant など）・コントラストの段階から、2021 年または 2025 年の色の仕様で 53 個の色の役割（primary・primary-dim・surface-container・outline など）を返します。2021 年の仕様は Google の material-color-utilities（`third_party/`）が計算します。2025 年の仕様（`color/spec_2025.cpp`）は、同じライブラリの Java 版の `ColorSpec2025` をスマートフォン向けの部分だけ移したもので、C++ 版の HCT とコントラストの関数の上に作っています。Java 版との照合は `tools/color-dump/` で行います。どちらの仕様も、`color/mcu_scheme.cpp` で作る、そのライブラリの配色の種類ごとの配色から始めます。

色の仕様は `ColorSpec` という口の向こうに置いているので、配色を使う側のコードは仕様によって変わりません。二つの仕様は HCT を共有し、配色の種類ごとに作る色の系列と、役割ごとに明度を選ぶ規則が違います。`examples/buttons` には二つを切り替えるスイッチがあります。

## 形・文字・高さ・状態の重ね

`lamina/tokens/shape.hpp`・`typography.hpp`・`elevation.hpp`・`interaction.hpp`：角の丸めの段階（`corner_radii(corners, width, height)` は `Corners` の組を四つの角の半径にします）、文字の段階（強調の段階を含む）、高さの段階、状態の重ねの不透明度と押せる範囲の最小の大きさ。これらの基礎のトークンはすべての部品で共有します。一つの部品のトークンは、その部品と同じ名前の `tokens/` のファイルにあります（`widgets/grouped_menu.hpp` には `tokens/grouped_menu.hpp`）。`TypeRole` には、Expressive の文字の段階にある 15 の役割と、その強調版 15 があります（`display_large_emphasized` から `label_small_emphasized` まで。`tokens::emphasized(role)` でも得られます）。値は Compose と同じで、大きさ・行の高さ・字間は元の役割と同じ、太さは 500（Title M・Title S・Label は 700）です。`LabelOptions::type` のように `TypeRole` を受け取る所なら、どこでも強調版を選べます。

## 形の集まりと変形

`lamina/shapes/`：androidx の `graphics-shapes` を C++ に移したもの（角ごとに丸めと滑らかさを持つ多角形と、二つの輪郭の角を対応させて 3 次曲線を補間する `Morph`）と、Material 3 Expressive の 35 個の形（`material_shape`）。出典は [third_party/NOTICE-androidx-graphics-shapes.md](../third_party/NOTICE-androidx-graphics-shapes.md)。`build/Release/shapes_gallery.exe` は全部の形を並べ、どれかを押すと大きな形が expressive の空間のばねでその形に変わります。

## 動き

`lamina/tokens/motion.hpp`：standard と expressive の二つの動きの体系の、ばね・緩急・時間のトークン。どちらの体系も、Compose の `MotionScheme` と同じく6つのばねに役割の名前を付けています（`fast_spatial()`・`default_spatial()`・`slow_spatial()`・`fast_effects()`・`default_effects()`・`slow_effects()`）。部品はどれも、使っているスタイルの `motion`（[スタイル](#スタイル)）からばねを取ります。Material 3 Expressive のスタイルでは、これは expressive の体系（`tokens::motion_scheme`）です。

`lamina/motion/easing.hpp`・`lamina/motion/tween.hpp`：3 次ベジェの緩急（`ease(curve, fraction)`。Compose の `CubicBezierEasing`）と、決まった時間で曲線に沿って目標へ動く値 `Tweened`。Compose がばねでなくトゥイーンで動かす、チップとカードの高さ（影）と、2 段のアプリバーの題名の消え方に使います。`ease_inverse(curve, eased)` は、曲線が緩急をつけた値に届く割合を返します。`Tweened::retime(spec)` と `retimed_elapsed(running, elapsed, next)` は、動いているトゥイーンを別の長さと曲線に移します（[スタイルが変わったとき](#スタイルが変わったとき)）。`tokens::CubicBezier` は `motion::CubicBezier` です。

`lamina/motion/spring.hpp`：減衰するばね。解を式で直接求めます。`advance` は値を目標に向けて好きな時間だけ進めます。時間をどうコマに分けても結果は変わりません。`is_settled` は動きが止まったかを判定します。

`lamina/motion/animated.hpp`・`lamina/motion/animated_radii.hpp`・`lamina/motion/lerp.hpp`・`lamina/color/mix.hpp`：ばねで動く値と、角ごとにばねで動く四つの角の半径（`AnimatedRadii`。ばねが行き過ぎても 0 を下回りません。グループ分けしたメニューのグループと項目は、これで角の形を変えます）と、2つの数の間の線形補間と、色の混ぜ合わせと重ね合わせ。`color::source_over` はある色を別の色の上に重ね、`color::state_layer(container, content, opacity)` は、入れ物の色に状態の重ねを重ねた色を返します。ボタン・アイコンボタン・FAB は状態の重ねが入れ物と同じ形なので、入れ物をこの色で塗ります。状態の重ねが別の形のとき（スイッチの円、メニューの項目、レールの印）は、すでに描いたものの上に `with_alpha(content, opacity)` で塗り、キャンバスが同じ方法で重ねます。`color::faded(scheme, opacity)` は、すべての役割の色の不透明度に `opacity` を掛けます。ダイアログ・メニュー・グループ分けしたメニューは、現れるときと消えるときにこの薄めた配色で描き、ダイアログは中のラベルとボタンにもそれを渡します。アイコンボタンは、状態の重ねの不透明度に中身の色の不透明度を掛けます。そのため、押されているか焦点のあるアイコンボタンを薄めた配色で描くと、状態の重ねも元の濃さのままにならず、一緒に薄くなります。

## ばねの模型

ばねは質量 1、硬さ k、減衰比 ζ とします。目標からのずれ y は y'' + 2ζ√k y' + k y = 0 に従います。`advance` は場合ごとの厳密な解を使います：ζ < 1（振動して行き過ぎる）、ζ = 1（行き過ぎない中でいちばん速い）、ζ > 1（遅く、行き過ぎない）。expressive の空間のばね（位置・大きさ・形）は ζ < 1 で行き過ぎ、効果のばね（色・透明度）は ζ = 1 です。

## スタイル

`lamina/style/style.hpp`：`lamina::Style` は、部品が読む設計の値を全部、実行時の値として持ちます。色の仕様（`Style::color`、`const color::ColorSpec*`）と、部品ごとのトークン（`button`・`icon_button`・`navigation_bar`・`dialog`・`scrim`・`menu`・`list`・`wavy_progress` など）、それに全部の部品が共有する土台のトークンを持ちます。土台は `shape`（角の丸みの段階）・`type`（文字の段階）・`motion`（動きの体系）・`elevation`（高さの段階・影の層・高さの変わり方）・`state_layer`（状態の重ねの不透明度）・`interaction`（触れる範囲の最小の大きさと、使えないときの不透明度）・`density`（密度ごとの長さ）・`gesture`（指の遊び・速さの追跡・はじいたときの減速・指でのスクロール）・`zoom` です。メンバーは、部品のトークンの構造体そのもの（`tokens::ScrimTokens scrim`）か、表がいくつもある部品では、表をまとめて、これまで自由関数だった引き方を持つ組です（`tokens::ButtonSet` の `size(ButtonSize)`・`style(ButtonStyle)`・`toggle_style(ButtonStyle)`、`tokens::ShapeTokens` の `radius(CornerSize, width, height)`・`radii(Corners, width, height)`、`tokens::TypeTokens` の `style(TypeRole)`・`text(TypeRole)`）。`Style::minimum_target(density)` は密度ごとの触れる範囲です。

`m3_expressive_style()` は Material 3 Expressive のスタイルです。既定の値で作った `Style` も同じものです。メンバーの既定の値は、どれも `tokens/` の対応する `constexpr` の表なので、数値はトークンのヘッダーに 1 回だけ書かれ、表が既定の値の出どころのままです。`tokens/` の自由関数（`tokens::button_size`・`tokens::corner_radius`・`tokens::type_style` など）は、今もその既定の値を返します。使っているスタイルには従いません。

### 部品がトークンを受け取る道

`current_style()` は、呼んだスレッドで使っているスタイルです。ほかのスタイルを入れていなければ `m3_expressive_style()` です。部品はどれもトークンをここから読みます（`current_style().button.size(options_.size)`、`current_style().scrim.opacity`）。どのスタイルを使っているかで分かれる部品はありません。`tokens/` から直に読む値は 3 種類だけ残っています。どれもスタイルでなく、OS かアイコンのフォントに属する値です。ショートカットの書き方（`tokens::default_shortcut_format()`、OS で決まります。メニューと `format_chord` はオプションで受け取ります）、アイコンのフォントの軸（`tokens::symbol_axes`、`icons::IconStyle` の既定の値とフォントの光学サイズの範囲）、密度の刻み（`tokens::density_interval`、密度を 1 段下げるごとに長さを縮める 4 dp。`DenseLength::at` が使います。長さそのものはスタイルにあります）です。

使うスタイルを引数でなくスレッドごとの設定にしたのは、部品が描くときだけでなく、どの段階でもトークンを読むからです。ボタンは、作るときに高さ・ラベルの文字・ばねを、測るときに最小の幅を、押されたときに触れる範囲を、描くときに色と枠線を読みます。配色は `paint(canvas, scheme)` にしか届かないので、配色と並べてスタイルを渡しても、ほかの段階は定数の表のままになります。部品を作る関数のどれにもスタイルを足すと、全部の部品の引数が変わります。スレッドごとの設定なら、公開している関数の引数は一つも変わりません。配色は描く道に残しました。配色はアプリが元の色・ライトかダークか・コントラストから作るもので、描くたびに変わることもあるからです（ダイアログとメニューは、出入りの間、薄めた配色で中身を描きます）。

スタイルが持つのを、作った配色でなく色の仕様にしたのも同じ理由です。仕様はアプリの `SchemeOptions` から色の役割を作る規則で、スタイルに属します。元の色などの選び方と薄め方はアプリに属します。スタイルに従うアプリは `style.color->build(options)` で配色を作ります。Material 3 Expressive のスタイルの仕様は `spec_2025()` です。見本は、使っているスタイルから配色を作ります。2021 年の仕様に合わせて作った見本（部品の一覧・ボタン・ナビゲーション・レイアウト・プレイグラウンド・形・テキストフィールド）は、`main` の始めに `examples::sample_style()` を既定のスタイルにします。これは Material 3 Expressive のスタイルを写して、仕様を `spec_2021()` にしたものです。電卓・タイマー・やることリストは Material 3 Expressive のスタイルを使います。ボタンの見本の、二つの仕様を切り替えるスイッチはそのままです。

### スタイルの渡し方

- `StyleScope scope(style)` は、そのスコープの終わりまでスタイルを入れ、終わると前のスタイルに戻します。入れ子にできます。
- `set_default_style(style)` は、スタイルをアプリの既定にします。ほかのスタイルを入れていないどのスレッドでも、これが使っているスタイルになります。`default_style()` はそれを返し、`reset_default_style()` で Material 3 Expressive のスタイルに戻ります。スタイルが一つのアプリは、`main` の始め、部品を作る前に 1 回呼びます。
- `set_current_style(style)` は、呼んだスレッドのその後ずっとのスタイルを、既定の上に入れます。`clear_current_style()` で外します。
- `sdl3::Window::set_style(style)` は窓に自分のスタイルを持たせます。`Window::style()` はそれを返し、設定していなければ使っているスタイルを返します。`sdl3::run_app` は、毎回の更新と描画の間、窓のスタイルを入れます。自分でフレームの繰り返しを書くアプリは、更新と描画を `StyleScope` で囲みます。窓ごとのスタイルには `run_app(Window&, update, paint)` を使います。`WindowOptions` を受け取るほうは窓を自分で作るからです。
- [いちばん小さなプログラム](../README.ja.md#いちばん小さなプログラム)のように、フレームの繰り返しの前に作る部品は、作った場所で使っているスタイルの下で作られます。スコープや `set_current_style` がなければ既定のスタイルです。`set_default_style` を使っていれば、それがもうアプリのスタイルです。どの部品も[スタイルを読み直し](#スタイルが変わったとき)、最初の 1 歩で窓のスタイルに従うので、別のスタイルの下で作っても自分で直ります。

スタイルは参照で持つので、入れている間は呼んだ側が生かしておきます。スタイルは、ほかの構造体と同じように写して変えます。

```cpp
lamina::Style quiet = lamina::m3_expressive_style();
quiet.scrim.opacity = 0.5;
quiet.button.styles[static_cast<std::size_t>(lamina::tokens::ButtonStyle::outlined)].outline = lamina::color::Role::outline;
window.set_style(quiet);
```

### スタイルが変わったとき

部品は、たいていの値を使うときに読みます。描くときに使う値（色・枠線・幕・状態の重ね・大きさから出す角の丸み・下の見せ方の切り替え）は、後から入れたスタイルにすぐ従います。部品が持っておく値（高さ・並べたラベル・ばね・形の変化の角・動きの行き先）は、作るときか測るときに取り、スタイルが変わると読み直します。

- `Style` はどれも版の番号（`Style::revision`、`StyleRevision`）を持ちます。新しく作ったスタイル・写したスタイル・代入したスタイルは、プロセスで一つの数え上げから、それぞれ新しい番号を取ります。なので、二つのスタイルが同じ番号になることはありません。一つを捨てて同じ番地に別のものを作ってもです。その場で書き換えたスタイルは、`style.changed()` で新しい番号を取るまで番号が変わりません。入れているスタイルを書き換えたら呼んでください。
- `StyleStamp` は、作ったときに使っていたスタイルの番号を覚えます。`stale()` は、使っているスタイルの番号が変わったかを返します。`refresh()` は新しい番号を取り、変わっていたかを返します。
- スタイルの値を持っておく部品は `StyleStamp` を持ち、公開の `restyle()` を持ちます。`restyle()` はその値を使っているスタイルから読み直し、番号を取り直します。部品の `step()` は、番号が古ければ `restyle()` を呼びます。なので、1 歩ずつ進めている部品は、アプリが何もしなくても次のフレームで新しいスタイルに従います。`step()` を持たない `Label` は `measure()` で同じことをします。ほかの部品を持つ部品は、自分の `restyle()` から持っている部品の `restyle()` を呼びます。`run_app` は更新の前に窓のスタイルを入れるので、これは窓のスタイルの下で起こります。進めていない部品には、プログラムが自分で `restyle()` を呼びます。
- `restyle()` は部品の状態を残します。ラベル・チェックの状態・フォーカス・`set_size` で決めた大きさ・どの動きの今の値と速さもです。ばねは数値だけが替わって今の位置から続き、行き先をスタイルから取る動きは新しい行き先へ動きます。なので、スタイルが変わると跳ばずに動いて変わります。あるスタイルの下で作って別のスタイルの下で進めた部品は、落ち着いた後、はじめから別のスタイルの下で作った部品と同じに描かれます。`tests/restyle_test.cpp` がこれを確かめます。
- オプションでアプリが渡した値は、渡したままです。オプションの構造体を作るときにスタイルから取る既定の値は、アプリが選んだ値と見分けられないので、これも残ります。`ButtonShapes::spring` と、互換性の節に挙げたオプションの既定の値です。形をスタイルから取ったトグルボタン（`ButtonOptions::shapes` を渡していないもの）は、`restyle()` で形を取り直します。

どの部品もスタイルを読み直します。`Button`・`IconButton`・`Badge`・`ButtonGroup`・`SplitButton`・`Label`・`Checkbox`・`Switch`・`RadioButton`・`Fab`・`ExtendedFab`・`FabMenu` とその `ToggleFab`・`ListView`・`ListItem`・`ScrollArea`・`Table`・`SegmentedButtonRow`・`Card`・`Chip`・`ChipGroup`・`TabRow`・`NavigationBar`・`NavigationRail`・`SideNavigation`・`DrawerSheet`・`ModalNavigationDrawer`・`DismissibleNavigationDrawer`・`ModalNavigationRail`・`NavigationSuiteScaffold`・`ThreePaneScaffold`・`SplitPane`・`TopAppBar`・`TitleBar`・`FloatingToolbar`・`FlexibleBottomAppBar`・`SearchBar`・`Snackbar`・`Dialog`・`PickerDialog`・`BottomSheet`・`SideSheet`・`Tooltip`・`Menu`・`GroupedMenu`・`MenuBar`・`Slider`・`RangeSlider`・`TextField`・`NumericField`・`DatePicker`・`DateRangePicker`・`DockedDatePicker`・`TimePicker`・`Pager`・`PullToRefresh`・`ZoomView`・`LoadingIndicator`・`CircularProgressIndicator`・`LinearWavyProgressIndicator`・`CircularWavyProgressIndicator` です。それらが共有する部品は、自分の `restyle()`（`FocusIndicator`・`Enablement`・`AnimatedCorners`・`InteractionElevation`・`PopupTransition`・`TopAppBarState`・`DragHandlePainter`・`SheetDragHandle`・`ContentHost`、それに `BottomAppBarState::restyle(limit)`）か、値を替える関数（`TouchScroll::set_tokens`・`Fling::set_decay`・`AnchoredDraggable::set_gesture`・`AnchoredDraggable::retune`・`AnimatedRadii::set_spec`）を持ちます。

ほかの部品を持つ部品は、中の部品の `restyle()` を呼び、並べ直します。ダイアログは中の並びを組み直し、モーダルのドロワーとレールは自分の場所の中でシートを測り直します。シートは中に入れた中身を、三つの区画の骨組みは区画ごとの部品の集まりを、選ぶためのダイアログは中の区画を読み直させます。`WidgetSet::restyle()` は、`restyle()` を持つ中の部品をどれも読み直させます（`widgets/protocol.hpp` の `Restylable` の約束です）。`DialogPane` は仮想の `restyle()` を持ちます。既定では何もせず、`PaneOf` は中の部品に渡します。開いているメニューは開いたときの基準の場所から、開いている下のメニューは項目の新しい四角から、もう一度置き直します。`Menu::reposition(anchor, window)` と `Menu::reposition(anchor, window, horizontal_margin, vertical_margin)` は、開いているメニューを開き直さずに別の基準の場所で置き直します。メニューバーは開いているメニューを項目の新しい四角の下に、分割ボタンは右のボタンの下に、これで置き直します。持ち主が並べる部品（セグメントボタンの列・ボタンのまとまり）は自分を測り直し、持ち主の次の `measure` と `arrange` で置かれます。分割ボタンと検索バーは、最後に並べられた場所でもう一度自分を並べます。分割ボタンでは、これで開いているメニューが、持ち主が右のボタンを置く場所のそばに置かれます。

スタイルでなく状態に属するものは、部品が持ったままにします。分割した区画の仕切りの位置、拡大の表示の倍率と位置、期間を選ぶ表が見せている月（新しい月の高さで同じ所まで送ります）、伸び縮みする下のアプリバーのたたみ具合です。選んだタブを真ん中に見せていた送れるタブの列は、もう一度真ん中に見せます。ほかの所へ送ってあれば、そのままです。

すでに動いているドロワー・レール・下のシートの滑りは、今の位置と速さから新しいスタイルのばねで続けます（`AnchoredDraggable::retune`）。下のシートは、出るところか、隠れるところか、収まるところかを覚えていて、ばねを選びます。

動いている途中の、時間で決まる動き（トゥイーン）は、今いる所から、同じ動きの新しいスタイルのトゥイーンで続けます。引きずった後のドロワーの収まり、横のシートの滑り、波形の進み具合の表示の進み・波の高さ・受け渡しの動き、チップとカードの浮き上がりの動き、引っぱって更新の矢印の濃さと回る印への移り変わり、スライダーと範囲のスライダーの値の吹き出しです。部品は、動いているトゥイーンをどのトークンから取ったかを覚えています（浮き上がりは、上がるところか、ポインターが離れて下がるところか、押す・フォーカスが終わって下がるところか。波の高さは、上がるところか下がるところか。円の受け渡しは、1 周の残りがどれだけだったか）。そして新しいスタイルの値を `Tweened::retime` か `AnchoredDraggable::retune(TweenSpec)` に渡します。トゥイーンは始まりと行き先をそのままにし、長さと曲線、それに進んだ時間だけを替えます。

- 曲線が同じなら、進んだ割合 `p`（進んだ時間を長さで割ったもの）をそのままにし、新しい長さの残り `新しい長さ × (1 − p)` をかけて動きます。
- 曲線が違うなら、古い曲線が `p` で出す緩急をつけた値 `e` を取り、新しい曲線が `e` を出す割合 `p'` を二分法で探し（`ease_inverse`）、`新しい長さ × (1 − p')` をかけて動きます。切り替えた時点で、緩急をつけた値は変わらず、なので位置も変わりません。
- 新しい長さが 0 以下なら、行き先で終わります。
- 長さも曲線も同じなら、何も変えません。なので、何も変えない読み直しでは、どのトゥイーンもそのままです。

位置はつながりますが、速さはつながりません。切り替えた後は、新しい曲線の `p'` での傾きと新しい長さで動きます。曲線が同じで長さが半分なら、2 倍の速さで動きます。二分法は、どの曲線でも 0 から 1 までの緩急をつけた値に届く割合を見つけます。動きのトークンの曲線はどれも終わりまで上がり続けるので、その割合は一つに決まります。行き過ぎる曲線だけが出す 0 より小さい値や 1 より大きい値は 0 か 1 として扱うので、そういうトゥイーンは始まりへ跳ぶか、行き先で終わります。スタイルが変わって行き先が動くとき（アンカーが動くシート、高さが変わる浮き上がり）は、今の値から、新しい長さと曲線でトゥイーンをはじめから始め直します。

スタイルが変わっても、まだ動きに付いてこないところがあります。

- 引きずりは、始めたときの速さの測り方のままです。`TouchScroll`・`ZoomView`・`PaneExpansionState` の速さの測り方は、作ったときのトークンのままです。`AnchoredDraggable` は、次に押したときから新しいものを使います。
- 部品が中の部品にオプションだけで渡す値は、作ったときのままです。ツールチップの動作のボタンの種類・大きさ・色、ツールバーと下のアプリバーのボタンの中身の色、横のシート・検索バー・日付の選択の見出しのボタンの大きさと中身の色、ドックした日付の選択の入力欄のいちばん小さな幅です。中の部品は、自分のトークンを読み直します。

スタイルは、その下で作ったものがまだ使うかもしれない間は、ほかのスタイルを入れた後も生かしておいてください。`PagerOptions::tokens`・`PullToRefreshOptions::tokens`・`PullToRefreshOptions::arrow_tokens` は既定では空（null）で、そのとき部品は使うたびに今のスタイルのトークンを読みます。プログラムが指す先を渡したときは、渡したままです。はじいたときの減速の曲線（`fling_decay()`）は、今のスタイルの `gesture.fling_decay` から計算し、同じ値で使うときのために取っておきます。なので、違う減速の値ごとに 1 回だけ計算します。`ScrollArea` と `TabRow` は `restyle()` で新しい曲線を取り、はじいて動いている途中なら今の位置と速さから新しい曲線で続けます。`ZoomView` も、はじいたときの動きのために `restyle()` で新しい曲線を取ります。`ScrollArea` の外の `TouchScroll` は、作ったときの指の操作のトークンを持ったままです。`ZoomViewOptions::tokens` は、ページ送りと同じく既定では空（null）で、そのとき拡大の表示は使うたびに今のスタイルの拡大のトークンを読みます。なので、収まりのばねと倍率の限りもスタイルに従います。動いている収まりは、今の進み具合と速さから新しいばねで続けます。読み込み中の表示の形と、円の波形の進み具合の表示の形は、今のスタイルのトークンで作り、同じ値で使うときのために取っておきます。減速の曲線と同じです。

### 見せ方の切り替え

四つの切り替えで、スタイルが入れ物・重なり・選択・一覧をどう見せるかを変えます。既定はどれも Material 3 の振る舞いです。部品は値を読んで従うだけで、どのスタイルを使っているかは尋ねません。

- `Style::containers`（`tokens::ContainerTokens`）：`fill` は `ContainerFill::filled`（M3）か `ContainerFill::outlined` です。縁取りの入れ物は `outlined_fill` で塗り、`outline` の色で `outline_width` の太さの線を引きます。`widgets::paint_container` で描く入れ物に効きます。メニュー・まとまったメニュー・ダイアログ・リッチなツールチップ・スナックバー・シート・モーダルと標準のドロワーの面・検索バー・浮かぶツールバーです。トークンにもう縁取りの種類がある部品（ボタン・カード・チップ・テキストフィールド）は、今までどおり自分のオプションで選びます。
- `Style::layering`（`tokens::LayeringTokens`）：`mode` は `Layering::lift`（M3：手前の面は明るい入れ物と影で浮かせる）か `Layering::recede` です。奥を沈めるときは、`paint_container` で描く重なった面が、自分の入れ物の色の代わりに `receded_container`（地の色）を取ります。メニュー（`Menu` と `GroupedMenu`）は後ろをスタイルの幕で暗くします（`paint_receded_backdrop`）。モーダルのナビゲーションドロワーは `content_shift()` を返します。横の本文をどれだけ動かすかで、ドロワーの開いた部分の幅に `drawer_push` を掛けた長さです。ドロワーは今までどおり幕で本文を暗くします。本文はドロワーの持ち物ではないので、本文を描くプログラムが `content_shift()` だけずらして描きます。`lift` のときはいつも 0 です。
- `Style::selection`（`tokens::SelectionTokens`）：`mark` は `SelectionMark::surface`（M3：選んだものの後ろに面か丸い印を置く）か `SelectionMark::foreground` です。`foreground` では、選んだ一覧の項目は面を持たず、`foreground` の色になります。`emphasized_label` が入っていれば、見出しの文字は強調の文字の役割になります。ナビゲーションバーは丸い印を出さず、選んだアイコンとラベルを `foreground` の色にし、`underline` が入っていればラベルに `underline_thickness` の太さの下線を引きます。
- `Style::list.row_dividers`：`ListView` が行の間に区切り線（スタイルの `divider` の色と太さ）を引くかです。M3 では引きません。`ListViewOptions::dividers` で一覧ごとに上書きできます。

切り替えが M3 の既定でないときだけ意味を持つ値（`outlined_fill`・`receded_container`・`drawer_push`・`foreground` など）は、切り替えと一緒に `tokens/presentation.hpp` にあります。`tests/presentation_test.cpp` は、テストの中だけで作ったスタイルで、切り替えごとに M3 の既定と比べて描きます。

`tokens/` には、ほかの表から作る表があります。密度ごとの長さは部品の高さから始まり、多くの部品のばねは動きの体系から来ます。`Style` は作った後の値をそのまま持つので、高さや動きの体系を変えるスタイルは、そこから作るメンバーも変えます。

スタイルに入れていないもの：窓の描画の設定（`tokens::raster`）、フレームの統計の表示（`tokens::frame_stats`）、アイコンのフォントの軸（`tokens::symbol_axes`）、密度の刻み（`tokens::density_interval`）、動画のトークン、ショートカットの書き方（`tokens::default_shortcut_format()`、OS で決まります）。描画面と文字の並べ方はスタイルを読みません。

### 次のリリースで互換性が変わるところ

- `lamina::Style` は `namespace lamina` の新しい名前です。`using namespace lamina;` を書いて、名前空間の範囲で自分の `Style` を宣言しているファイルでは、名前があいまいになります。どちらかの名前を変えるか、修飾してください。レイアウトの見本の `Style` は `LayoutStyle` にしました。
- 既定の値をトークンから取るオプションの構造体（`ButtonGroupOptions::expanded_ratio`・`ChipGroupOptions::spacing` など）は、その構造体を作ったときに使っているスタイルから既定の値を取ります。`badge_count` の `max` のような引数の既定の値は、呼んだときに使っているスタイルから取ります。
- `PagerOptions::tokens`・`PullToRefreshOptions::tokens`・`PullToRefreshOptions::arrow_tokens`・`ZoomViewOptions::tokens` は、既定のスタイルの中を指すのをやめ、既定では空（null）になりました。空なら今のスタイルのトークンを使います。既定のオプションのこのメンバーを読むコードは空のポインターを受け取るので、代わりに `Pager::tokens()` か `ZoomView::tokens()` を読んでください。
- `tokens::` の表を直に読むコードは、ほかのスタイルを使っているときも Material 3 Expressive の値のままです。スタイルに従うには `current_style()` を読んでください。
- `paint(render::Canvas&, const color::ColorScheme&)`・`Paintable`・部品を作る関数はどれも変わりません。
- `Style` に新しいメンバー `revision` が入りました。写しても写されず、写したものは新しい番号を取ります。
- [スタイルが変わったとき](#スタイルが変わったとき)に挙げた部品は、使っているスタイルが変わった後の最初の `step()`（`Label` は最初の `measure()`）で、持っておく値を読み直します。`StyleScope` の中で作った部品をその外で進めたときもです。わざと別のスタイルの下で作って進めるプログラムでは、部品が後のスタイルに従うようになります。
- `ButtonGroup` は、ボタンの今の高さでなく、新しい `Button::intrinsic_height()`（スタイルと密度から決まる高さ）でボタンの大きさを決めます。
- 上の一覧の部品に、公開の `restyle()` が増えました。`Menu` と `GroupedMenu` は開いたときの基準の場所を覚え、`Menu` には `reposition` が増えました。`TextField` はプログラムを始めたときのスタイルからトークンを読んでいましたが、ほかの部品と同じく今のスタイルから読むようになりました。
- `loading_indicator_morphs()` と `loading_indicator_scale()` は、初めて呼んだときのスタイルの形を持ったままでしたが、今のスタイルの形を返すようになりました。円の波形の進み具合の表示の形も同じです。
- `Button` に、`set_padding`・`set_min_width`・`set_icon_size`・`set_checked_overlay` が増えました。分割ボタンは、これで左右のボタンに新しいスタイルの余白・いちばん小さな幅・アイコンの大きさ・重ねを渡します。`WidgetSet` に `restyle()`、`widgets/protocol.hpp` に `Restylable` の約束、`DialogPane` に既定では何もしない仮想の `restyle()`、`AnchoredDraggable` に `retune`、`BottomAppBarState` に `restyle(limit)` が増えました。`DialogPane` を継いだクラスが、別の形の `restyle` を自分で宣言していると、元のものが隠れます。
- `Tweened` に `retime`・`origin`・`elapsed`・`spec`、`motion/tween.hpp` に `retimed_elapsed`、`motion/easing.hpp` に `ease_inverse`・`ease_solve_iterations`・`ease_solve_tolerance` が増えました。`CubicBezier` と `TweenSpec` は `==` で比べられます。`AnchoredDraggable::retune` に `TweenSpec` を取る形が増えました。動いているトゥイーンの時間を合わせ直し、動いているばねには何もしません。ばねを取る形が動いているトゥイーンに何もしないのと同じです。`WavyProgressCore` に仮想の `handoff_motion()`（受け渡しのトゥイーン）が増えました。継いだクラスが替えなければ、進みのトゥイーンです。`ValueIndicator` に `restyle()`、`RangeSlider` に `value_indicator_scale(thumb)` が増えました。
- `set_width` を呼んでいない `LinearWavyProgressIndicator` と、`set_size` を呼んでいない `CircularWavyProgressIndicator` は、`restyle()` で新しいスタイルの幅か大きさを取ります。プログラムが大きさを決めたものは、その大きさのままです。
- `ListViewOptions` の最後に新しいメンバー `dividers` が入りました。重なった面は入れ物を新しい `paint_container` で描きます。`paint_surface` は変わりません。

## 出典

- Material 3 の 色：<https://m3.material.io/styles/color/overview>
- Material 3 の 形：<https://m3.material.io/styles/shape/overview>
- Material 3 の 動き：<https://m3.material.io/styles/motion/overview>
- Material 3 の 高さ：<https://m3.material.io/styles/elevation/overview>
- material-color-utilities：<https://github.com/material-foundation/material-color-utilities>
- androidx の graphics-shapes：<https://github.com/androidx/androidx/tree/androidx-main/graphics/graphics-shapes>
- 値ごとのソースのファイルとコミット：[デザイントークン](design-tokens.ja.md)
