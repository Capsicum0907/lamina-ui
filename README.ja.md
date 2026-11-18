# lamina-ui

[English](README.md)

Material 3 Expressive を手本にした、SDL3 の上で動く C++ のデスクトップ向け UI ライブラリです。

lamina-ui は Google とは関係がなく、Google の承認も受けていません。Material Design は Google のデザインの体系で、lamina-ui はその公開されている指針とトークンの値に従っています。

## 現状

作り始めたところです。今あるのは、描画を持たない設計の値の層だけです。

- `lamina/tokens/motion.hpp`：standard と expressive の二つの動きの体系の、ばね・緩急・時間のトークン。
- `lamina/motion/spring.hpp`：減衰するばね。解を式で直接求めます。`advance` は値を目標に向けて好きな時間だけ進めます。時間をどうコマに分けても結果は変わりません。`is_settled` は動きが止まったかを判定します。

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

## 出典

トークンの値は、Google が Apache-2.0 で公開しているソースから取っています：androidx（Jetpack Compose の Material 3 と graphics-shapes）、material-components-android、material-web、material-color-utilities。ファイルとコミットは [docs/design-tokens.ja.md](docs/design-tokens.ja.md) にあります。
