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

`lamina/motion/easing.hpp`・`lamina/motion/tween.hpp`：3 次ベジェの緩急（`ease(curve, fraction)`。Compose の `CubicBezierEasing`）と、決まった時間で曲線に沿って目標へ動く値 `Tweened`。Compose がばねでなくトゥイーンで動かす、チップとカードの高さ（影）と、2 段のアプリバーの題名の消え方に使います。`tokens::CubicBezier` は `motion::CubicBezier` です。

`lamina/motion/spring.hpp`：減衰するばね。解を式で直接求めます。`advance` は値を目標に向けて好きな時間だけ進めます。時間をどうコマに分けても結果は変わりません。`is_settled` は動きが止まったかを判定します。

`lamina/motion/animated.hpp`・`lamina/motion/animated_radii.hpp`・`lamina/motion/lerp.hpp`・`lamina/color/mix.hpp`：ばねで動く値と、角ごとにばねで動く四つの角の半径（`AnimatedRadii`。ばねが行き過ぎても 0 を下回りません。グループ分けしたメニューのグループと項目は、これで角の形を変えます）と、2つの数の間の線形補間と、色の混ぜ合わせと重ね合わせ。`color::source_over` はある色を別の色の上に重ね、`color::state_layer(container, content, opacity)` は、入れ物の色に状態の重ねを重ねた色を返します。ボタン・アイコンボタン・FAB は状態の重ねが入れ物と同じ形なので、入れ物をこの色で塗ります。状態の重ねが別の形のとき（スイッチの円、メニューの項目、レールの印）は、すでに描いたものの上に `with_alpha(content, opacity)` で塗り、キャンバスが同じ方法で重ねます。`color::faded(scheme, opacity)` は、すべての役割の色の不透明度に `opacity` を掛けます。ダイアログ・メニュー・グループ分けしたメニューは、現れるときと消えるときにこの薄めた配色で描き、ダイアログは中のラベルとボタンにもそれを渡します。アイコンボタンは、状態の重ねの不透明度に中身の色の不透明度を掛けます。そのため、押されているか焦点のあるアイコンボタンを薄めた配色で描くと、状態の重ねも元の濃さのままにならず、一緒に薄くなります。

## ばねの模型

ばねは質量 1、硬さ k、減衰比 ζ とします。目標からのずれ y は y'' + 2ζ√k y' + k y = 0 に従います。`advance` は場合ごとの厳密な解を使います：ζ < 1（振動して行き過ぎる）、ζ = 1（行き過ぎない中でいちばん速い）、ζ > 1（遅く、行き過ぎない）。expressive の空間のばね（位置・大きさ・形）は ζ < 1 で行き過ぎ、効果のばね（色・透明度）は ζ = 1 です。

## スタイル

`lamina/style/style.hpp`：`lamina::Style` は、部品が読む設計の値を全部、実行時の値として持ちます。色の仕様（`Style::color`、`const color::ColorSpec*`）と、部品ごとのトークン（`button`・`icon_button`・`navigation_bar`・`dialog`・`scrim`・`menu`・`list`・`wavy_progress` など）、それに全部の部品が共有する土台のトークンを持ちます。土台は `shape`（角の丸みの段階）・`type`（文字の段階）・`motion`（動きの体系）・`elevation`（高さの段階・影の層・高さの変わり方）・`state_layer`（状態の重ねの不透明度）・`interaction`（触れる範囲の最小の大きさと、使えないときの不透明度）・`density`（密度ごとの長さ）・`gesture`（指の遊び・速さの追跡・はじいたときの減速・指でのスクロール）・`zoom` です。メンバーは、部品のトークンの構造体そのもの（`tokens::ScrimTokens scrim`）か、表がいくつもある部品では、表をまとめて、これまで自由関数だった引き方を持つ組です（`tokens::ButtonSet` の `size(ButtonSize)`・`style(ButtonStyle)`・`toggle_style(ButtonStyle)`、`tokens::ShapeTokens` の `radius(CornerSize, width, height)`・`radii(Corners, width, height)`、`tokens::TypeTokens` の `style(TypeRole)`・`text(TypeRole)`）。`Style::minimum_target(density)` は密度ごとの触れる範囲です。

`m3_expressive_style()` は Material 3 Expressive のスタイルです。既定の値で作った `Style` も同じものです。メンバーの既定の値は、どれも `tokens/` の対応する `constexpr` の表なので、数値はトークンのヘッダーに 1 回だけ書かれ、表が既定の値の出どころのままです。`tokens/` の自由関数（`tokens::button_size`・`tokens::corner_radius`・`tokens::type_style` など）は、今もその既定の値を返します。使っているスタイルには従いません。

### 部品がトークンを受け取る道

`current_style()` は、呼んだスレッドで使っているスタイルです。ほかのスタイルを入れていなければ `m3_expressive_style()` です。部品はどれもトークンをここから読みます（`current_style().button.size(options_.size)`、`current_style().scrim.opacity`）。`tokens::` の表を直に読む部品は、下に挙げるショートカットの書き方のほかにはありません。どのスタイルを使っているかで分かれる部品もありません。

使うスタイルを引数でなくスレッドごとの設定にしたのは、部品が描くときだけでなく、どの段階でもトークンを読むからです。ボタンは、作るときに高さ・ラベルの文字・ばねを、測るときに最小の幅を、押されたときに触れる範囲を、描くときに色と枠線を読みます。配色は `paint(canvas, scheme)` にしか届かないので、配色と並べてスタイルを渡しても、ほかの段階は定数の表のままになります。部品を作る関数のどれにもスタイルを足すと、全部の部品の引数が変わります。スレッドごとの設定なら、公開している関数の引数は一つも変わりません。配色は描く道に残しました。配色はアプリが元の色・ライトかダークか・コントラストから作るもので、描くたびに変わることもあるからです（ダイアログとメニューは、出入りの間、薄めた配色で中身を描きます）。

スタイルが持つのを、作った配色でなく色の仕様にしたのも同じ理由です。仕様はアプリの `SchemeOptions` から色の役割を作る規則で、スタイルに属します。元の色などの選び方と薄め方はアプリに属します。スタイルに従うアプリは `style.color->build(options)` で配色を作ります。Material 3 Expressive のスタイルの仕様は `spec_2025()` です。見本は今も自分で仕様を選んでいます。

### スタイルの渡し方

- `StyleScope scope(style)` は、そのスコープの終わりまでスタイルを入れ、終わると前のスタイルに戻します。入れ子にできます。
- `set_current_style(style)` は、そのスレッドのその後ずっとのスタイルを入れます。スタイルが一つのアプリ向けです。
- `sdl3::Window::set_style(style)` は窓に自分のスタイルを持たせます。`Window::style()` はそれを返し、設定していなければ使っているスタイルを返します。`sdl3::run_app` は、毎回の更新と描画の間、窓のスタイルを入れます。自分でフレームの繰り返しを書くアプリは、更新と描画を `StyleScope` で囲みます。

スタイルは参照で持つので、入れている間は呼んだ側が生かしておきます。スタイルは、ほかの構造体と同じように写して変えます。

```cpp
lamina::Style quiet = lamina::m3_expressive_style();
quiet.scrim.opacity = 0.5;
quiet.button.styles[static_cast<std::size_t>(lamina::tokens::ButtonStyle::outlined)].outline = lamina::color::Role::outline;
window.set_style(quiet);
```

部品は値を使うときに読みます。描くときに使う値（色・枠線・幕・状態の重ね・大きさから出す角の丸み）は、後から入れたスタイルにすぐ従います。作るときに取る値（高さ・並べたラベル・ばね）は、作ったときのスタイルのものです。なので、スタイルを切り替えた後、大きさや動きを変えたい部品は作り直します。

`tokens/` には、ほかの表から作る表があります。密度ごとの長さは部品の高さから始まり、多くの部品のばねは動きの体系から来ます。`Style` は作った後の値をそのまま持つので、高さや動きの体系を変えるスタイルは、そこから作るメンバーも変えます。

スタイルに入れていないもの：窓の描画の設定（`tokens::raster`）、フレームの統計の表示（`tokens::frame_stats`）、アイコンのフォントの軸（`tokens::symbol_axes`）、動画のトークン、ショートカットの書き方（`tokens::default_shortcut_format()`、OS で決まります）。描画面と文字の並べ方はスタイルを読みません。

### 次のリリースで互換性が変わるところ

- `lamina::Style` は `namespace lamina` の新しい名前です。`using namespace lamina;` を書いて、名前空間の範囲で自分の `Style` を宣言しているファイルでは、名前があいまいになります。どちらかの名前を変えるか、修飾してください。レイアウトの見本の `Style` は `LayoutStyle` にしました。
- 既定の値をトークンから取るオプションの構造体（`ButtonGroupOptions::expanded_ratio`・`ChipGroupOptions::spacing`・`PagerOptions::tokens`・`PullToRefreshOptions::tokens` など）は、その構造体を作ったときに使っているスタイルから既定の値を取ります。`badge_count` の `max` のような引数の既定の値は、呼んだときに使っているスタイルから取ります。`PagerOptions::tokens` と `PullToRefreshOptions::tokens` はそのスタイルの中を指します。
- `tokens::` の表を直に読むコードは、ほかのスタイルを使っているときも Material 3 Expressive の値のままです。スタイルに従うには `current_style()` を読んでください。
- `paint(render::Canvas&, const color::ColorScheme&)`・`Paintable`・部品を作る関数はどれも変わりません。

## 出典

- Material 3 の 色：<https://m3.material.io/styles/color/overview>
- Material 3 の 形：<https://m3.material.io/styles/shape/overview>
- Material 3 の 動き：<https://m3.material.io/styles/motion/overview>
- Material 3 の 高さ：<https://m3.material.io/styles/elevation/overview>
- material-color-utilities：<https://github.com/material-foundation/material-color-utilities>
- androidx の graphics-shapes：<https://github.com/androidx/androidx/tree/androidx-main/graphics/graphics-shapes>
- 値ごとのソースのファイルとコミット：[デザイントークン](design-tokens.ja.md)
