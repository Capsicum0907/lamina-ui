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

`lamina/tokens/motion.hpp`：standard と expressive の二つの動きの体系の、ばね・緩急・時間のトークン。どちらの体系も、Compose の `MotionScheme` と同じく6つのばねに役割の名前を付けています（`fast_spatial()`・`default_spatial()`・`slow_spatial()`・`fast_effects()`・`default_effects()`・`slow_effects()`）。部品はどれも `tokens::motion_scheme` からばねを取り、これは expressive の体系を指しています。

`lamina/motion/easing.hpp`・`lamina/motion/tween.hpp`：3 次ベジェの緩急（`ease(curve, fraction)`。Compose の `CubicBezierEasing`）と、決まった時間で曲線に沿って目標へ動く値 `Tweened`。Compose がばねでなくトゥイーンで動かす、チップとカードの高さ（影）と、2 段のアプリバーの題名の消え方に使います。`tokens::CubicBezier` は `motion::CubicBezier` です。

`lamina/motion/spring.hpp`：減衰するばね。解を式で直接求めます。`advance` は値を目標に向けて好きな時間だけ進めます。時間をどうコマに分けても結果は変わりません。`is_settled` は動きが止まったかを判定します。

`lamina/motion/animated.hpp`・`lamina/motion/animated_radii.hpp`・`lamina/motion/lerp.hpp`・`lamina/color/mix.hpp`：ばねで動く値と、角ごとにばねで動く四つの角の半径（`AnimatedRadii`。ばねが行き過ぎても 0 を下回りません。グループ分けしたメニューのグループと項目は、これで角の形を変えます）と、2つの数の間の線形補間と、色の混ぜ合わせと重ね合わせ。`color::source_over` はある色を別の色の上に重ね、`color::state_layer(container, content, opacity)` は、入れ物の色に状態の重ねを重ねた色を返します。ボタン・アイコンボタン・FAB は状態の重ねが入れ物と同じ形なので、入れ物をこの色で塗ります。状態の重ねが別の形のとき（スイッチの円、メニューの項目、レールの印）は、すでに描いたものの上に `with_alpha(content, opacity)` で塗り、キャンバスが同じ方法で重ねます。`color::faded(scheme, opacity)` は、すべての役割の色の不透明度に `opacity` を掛けます。ダイアログ・メニュー・グループ分けしたメニューは、現れるときと消えるときにこの薄めた配色で描き、ダイアログは中のラベルとボタンにもそれを渡します。

## ばねの模型

ばねは質量 1、硬さ k、減衰比 ζ とします。目標からのずれ y は y'' + 2ζ√k y' + k y = 0 に従います。`advance` は場合ごとの厳密な解を使います：ζ < 1（振動して行き過ぎる）、ζ = 1（行き過ぎない中でいちばん速い）、ζ > 1（遅く、行き過ぎない）。expressive の空間のばね（位置・大きさ・形）は ζ < 1 で行き過ぎ、効果のばね（色・透明度）は ζ = 1 です。

## 出典

- Material 3 の 色：<https://m3.material.io/styles/color/overview>
- Material 3 の 形：<https://m3.material.io/styles/shape/overview>
- Material 3 の 動き：<https://m3.material.io/styles/motion/overview>
- Material 3 の 高さ：<https://m3.material.io/styles/elevation/overview>
- material-color-utilities：<https://github.com/material-foundation/material-color-utilities>
- androidx の graphics-shapes：<https://github.com/androidx/androidx/tree/androidx-main/graphics/graphics-shapes>
- 値ごとのソースのファイルとコミット：[デザイントークン](design-tokens.ja.md)
