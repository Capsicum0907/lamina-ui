# lamina-ui

[English](README.md)

Material 3 Expressive を手本にした、SDL3 の上で動く C++ のデスクトップ向け UI ライブラリです。

lamina-ui は Google とは関係がなく、Google の承認も受けていません。Material Design は Google のデザインの体系で、lamina-ui はその公開されている指針とトークンの値に従っています。

## 現状

作り始めたところです。今あるのは、設計の値の層・2D の描画・文字・形・SDL3 の窓と、部品が 2 つ（ボタンと入力欄）です。配置の仕組みは入ったばかりで、古い見本はまだ部品の位置を手で決めています。

- `lamina/tokens/motion.hpp`：standard と expressive の二つの動きの体系の、ばね・緩急・時間のトークン。
- `lamina/motion/spring.hpp`：減衰するばね。解を式で直接求めます。`advance` は値を目標に向けて好きな時間だけ進めます。時間をどうコマに分けても結果は変わりません。`is_settled` は動きが止まったかを判定します。

- `lamina/tokens/shape.hpp`・`typography.hpp`・`elevation.hpp`：角の丸めの段階、文字の段階（強調の太さを含む）、高さの段階、状態の重ねの不透明度。
- `lamina/color/scheme.hpp`：1 色から作る配色。`spec_2021().build(options)` は、元の色・ライトかダークか・配色の種類（TonalSpot・Expressive・Vibrant など）・コントラストの段階から、49 個の色の役割（primary・surface-container・outline など）を返します。計算は Google の material-color-utilities（`third_party/`）が行い、2021 年の色の仕様に従います。

- `lamina/render/canvas.hpp`：曲線・角丸の四角・影・座標変換の積み重ねを持つ 2D の描画面。描くのは ThorVG です（`lamina::thorvg`）。ThorVG を選んだ理由は [docs/renderer-comparison.ja.md](docs/renderer-comparison.ja.md) にあります。
- `lamina/platform/window.hpp`：描画面を映す SDL3 の窓。ポインターの位置を dp に直し、何も動いていない間はイベントを待って止まります（`lamina::sdl3`）。
- `lamina/text/font.hpp`・`layout.hpp`：フォントと、1 行の文字・折り返す段落。文字の並べ方と字形の輪郭はどちらも HarfBuzz から取り、輪郭はほかの図形と同じく描画面で塗るので、FreeType は使いません。可変フォントは文字の段階の太さに合わせて太さを変え、フォントに無い文字は `FontStack` の次のフォントで補います。段落は、欧文では空白で、日本語では文字の間で折り返し、。や 」などの約物は行頭に来ないようにします（`text/line_break.hpp`）。
- `lamina/widgets/button.hpp`：Material 3 Expressive のボタン。5 種類（filled・tonal・outlined・elevated・text）・5 段階の大きさ・2 種類の形。押すと、Compose と同じく効果のばねで角が押したときの形に変わり、マウスを乗せたときと押したときには状態の重ねと高さが変わります。`build/Release/buttons.exe` で全部を並べて見られます。
- `lamina/widgets/text_field.hpp`：枠線つきの入力欄。焦点が当たるか文字が入ると、ラベルがばねで枠線の上に移ります。カーソルの点滅は、点滅の間に描き直さずに済むよう時間で起こします（`Window::wake_after`）。日本語は OS の IME で入力し、変換中の文字はその場に下線つきで描き、候補ウィンドウはカーソルの位置に出します（`Window::set_text_input`）。`build/Release/text_field.exe` は小さな入力の画面です。
- `lamina/shapes/`：androidx の `graphics-shapes` を C++ に移したもの（角ごとに丸めと滑らかさを持つ多角形と、二つの輪郭の角を対応させて 3 次曲線を補間する `Morph`）と、Material 3 Expressive の 35 個の形（`material_shape`）。出典は [third_party/NOTICE-androidx-graphics-shapes.md](third_party/NOTICE-androidx-graphics-shapes.md)。`build/Release/shapes_gallery.exe` は全部の形を並べ、どれかを押すと大きな形が expressive の空間のばねでその形に変わります。
- `lamina/layout/layout.hpp`：Compose と同じ考え方の行と列。部品は、まず制約の中で大きさを測り、次に置かれます。行と列には、間隔・余白・並ぶ向きの寄せ方（始め・中央・終わり・均等割り）・交わる向きの寄せ方（始め・中央・終わり・引き伸ばし）と、余った幅を比で分ける重みがあります。引き伸ばしの行は、`fill_cross` で全部の高さを求めない限り、いちばん高い子の高さになります。`lamina/widgets/nodes.hpp` は、ボタン・入力欄・`Label`（`widgets/label.hpp`、折り返す文字）を配置の部品に変えます。`build/Release/layout_demo.exe` は、窓の大きさを変えるとその場で組み直します。
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

`build/Release/playground.exe [フォントのファイル...]` を開くと、カードが 6 枚並んだ窓が出ます。フォントを指定しないときは `C:/Windows/Fonts/NotoSansJP-VF.ttf` を使います。カードをクリックすると、浮き上がり・角の丸みの増加・色の変化が、それぞれ expressive の体系のばねで動きます。

## 出典

`tests/fixtures/fonts/` のテスト用フォントは、Noto Sans JP と Oswald の一部を取り出して名前を変えたもので、SIL Open Font License 1.1 に従います（ライセンスの文書を同じ場所に置いています）。


トークンの値は、Google が Apache-2.0 で公開しているソースから取っています：androidx（Jetpack Compose の Material 3 と graphics-shapes）、material-components-android、material-web、material-color-utilities。ファイルとコミットは [docs/design-tokens.ja.md](docs/design-tokens.ja.md) にあります。
