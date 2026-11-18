# lamina-ui

[English](README.md)

Material 3 Expressive を手本にした、SDL3 の上で動く C++ のデスクトップ向け UI ライブラリです。

lamina-ui は Google とは関係がなく、Google の承認も受けていません。Material Design は Google のデザインの体系で、lamina-ui はその公開されている指針とトークンの値に従っています。

## 現状

作り始めたところです。今あるのは、設計の値の層・2D の描画・SDL3 の窓です。

- `lamina/tokens/motion.hpp`：standard と expressive の二つの動きの体系の、ばね・緩急・時間のトークン。
- `lamina/motion/spring.hpp`：減衰するばね。解を式で直接求めます。`advance` は値を目標に向けて好きな時間だけ進めます。時間をどうコマに分けても結果は変わりません。`is_settled` は動きが止まったかを判定します。

- `lamina/tokens/shape.hpp`・`typography.hpp`・`elevation.hpp`：角の丸めの段階、文字の段階（強調の太さを含む）、高さの段階、状態の重ねの不透明度。
- `lamina/color/scheme.hpp`：1 色から作る配色。`spec_2021().build(options)` は、元の色・ライトかダークか・配色の種類（TonalSpot・Expressive・Vibrant など）・コントラストの段階から、49 個の色の役割（primary・surface-container・outline など）を返します。計算は Google の material-color-utilities（`third_party/`）が行い、2021 年の色の仕様に従います。

- `lamina/render/canvas.hpp`：曲線・角丸の四角・影・座標変換の積み重ねを持つ 2D の描画面。描くのは ThorVG です（`lamina::thorvg`）。ThorVG を選んだ理由は [docs/renderer-comparison.ja.md](docs/renderer-comparison.ja.md) にあります。
- `lamina/platform/window.hpp`：描画面を映す SDL3 の窓。ポインターの位置を dp に直し、何も動いていない間はイベントを待って止まります（`lamina::sdl3`）。
- `lamina/motion/animated.hpp`・`lamina/color/mix.hpp`：ばねで動く値と、色の混ぜ合わせ。

色の仕様は `ColorSpec` という口の向こうに置いています。HCT と色の系列は 2021 年と 2025 年の仕様で同じで、違うのは役割ごとに明度を選ぶ規則だけです。そのため 2025 年の仕様は、配色を使う側のコードを変えずに、別の `ColorSpec` として足せます。

トークンの値と出典は [docs/design-tokens.ja.md](docs/design-tokens.ja.md) にあります。

## ばねの模型

ばねは質量 1、硬さ k、減衰比 ζ とします。目標からのずれ y は y'' + 2ζ√k y' + k y = 0 に従います。`advance` は場合ごとの厳密な解を使います：ζ < 1（振動して行き過ぎる）、ζ = 1（行き過ぎない中でいちばん速い）、ζ > 1（遅く、行き過ぎない）。expressive の空間のばね（位置・大きさ・形）は ζ < 1 で行き過ぎ、効果のばね（色・透明度）は ζ = 1 です。

## ビルド

Visual Studio 2022、vcpkg（`VCPKG_ROOT` を設定）、CMake 3.25 以上が要ります。

```
cmake --preset default
cmake --build --preset release
ctest --preset release
```

`build/Release/playground.exe` を開くと、カードが 6 枚並んだ窓が出ます。カードをクリックすると、浮き上がり・角の丸みの増加・色の変化が、それぞれ expressive の体系のばねで動きます。

## 出典

トークンの値は、Google が Apache-2.0 で公開しているソースから取っています：androidx（Jetpack Compose の Material 3 と graphics-shapes）、material-components-android、material-web、material-color-utilities。ファイルとコミットは [docs/design-tokens.ja.md](docs/design-tokens.ja.md) にあります。
