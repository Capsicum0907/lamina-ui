# デザイントークン（下調べ）

[English](design-tokens.md)

lamina-ui が Material 3 Expressive から借りる値です。Google が公開している実装のソースから読みました。デザインのサイトからは取っていません。2026-11-18 に元のファイルと照らして抜き取りで確かめています。

## 出典

| 記号 | リポジトリ | コミット（2026-11-18） | ライセンス |
|---|---|---|---|
| C | [androidx/androidx](https://github.com/androidx/androidx) の `compose/material3/material3/src/commonMain/kotlin/androidx/compose/material3/` | `1b31f4ddbd0d` | Apache-2.0 |
| G | androidx/androidx の `graphics/graphics-shapes/src/commonMain/kotlin/androidx/graphics/shapes/` | 同上 | Apache-2.0 |
| M | [material-components/material-components-android](https://github.com/material-components/material-components-android) の `lib/java/com/google/android/material/motion/res/values/tokens.xml` | `6061a69915d0` | Apache-2.0 |
| W | [material-components/material-web](https://github.com/material-components/material-web) の `tokens/versions/latest/sass/` | `c6e9a66360b1` | Apache-2.0 |
| U | [material-foundation/material-color-utilities](https://github.com/material-foundation/material-color-utilities) | `f242efc74afc` | Apache-2.0 |

値を使うときは Apache-2.0 の表示と出典を残します。

## 動き

### ばね

C の `tokens/StandardMotionTokens.kt` と `tokens/ExpressiveMotionTokens.kt`（`VERSION: v0_14_0`）から。M の `m3_sys_motion_{scheme}_spring_{speed}_{kind}_{damping|stiffness}` も同じ値です。

| 体系 | 速さ | 空間の減衰比 | 空間の硬さ | 効果の減衰比 | 効果の硬さ |
|---|---|---|---|---|---|
| standard | fast | 0.9 | 1400 | 1.0 | 3800 |
| standard | default | 0.9 | 700 | 1.0 | 1600 |
| standard | slow | 0.9 | 300 | 1.0 | 800 |
| expressive | fast | 0.6 | 800 | 1.0 | 3800 |
| expressive | default | 0.8 | 380 | 1.0 | 1600 |
| expressive | slow | 0.8 | 200 | 1.0 | 800 |

- 減衰比は ζ です（1.0 で臨界減衰、1 未満で行き過ぎて戻る）。硬さは質量 1 としたときのばね定数 k です。
- 効果のばねは二つの体系で同じで、違うのは空間のばねだけです。
- 空間のばねは、部品の形や大きさ・位置を変える動きに使います。効果のばねは、色や透明度のように形を変えない動きに使います（C の `MotionScheme.kt` の説明文から）。

### 時間と緩急

C の `tokens/MotionTokens.kt`（`VERSION: v0_103`）から。M とは照らしていません。

| トークン | 値 |
|---|---|
| Short1〜4 | 50 / 100 / 150 / 200 ms |
| Medium1〜4 | 250 / 300 / 350 / 400 ms |
| Long1〜4 | 450 / 500 / 550 / 600 ms |
| ExtraLong1〜4 | 700 / 800 / 900 / 1000 ms |

| 緩急（3次ベジェ） | 値 |
|---|---|
| Emphasized | (0.2, 0.0, 0.0, 1.0) |
| EmphasizedAccelerate | (0.3, 0.0, 0.8, 0.15) |
| EmphasizedDecelerate | (0.05, 0.7, 0.1, 1.0) |
| Standard | (0.2, 0.0, 0.0, 1.0) |
| StandardAccelerate | (0.3, 0.0, 1.0, 1.0) |
| StandardDecelerate | (0.0, 0.0, 0.0, 1.0) |
| Legacy | (0.4, 0.0, 0.2, 1.0) |
| LegacyAccelerate | (0.4, 0.0, 1.0, 1.0) |
| LegacyDecelerate | (0.0, 0.0, 0.2, 1.0) |
| Linear | (0.0, 0.0, 1.0, 1.0) |

ソースでも Emphasized と Standard は同じ値です。

## 形

### 角の丸め

C の `tokens/ShapeTokens.kt`（`VERSION: 14_1_0`）から。W の `_md-sys-shape.scss` とも一致します。

| 段階 | dp |
|---|---|
| none | 0 |
| extra-small | 4 |
| small | 8 |
| medium | 12 |
| large | 16 |
| large-increased | 20 |
| extra-large | 28 |
| extra-large-increased | 32 |
| extra-extra-large | 48 |
| full | 短い辺の半分 |

片側だけ丸めるものもあります：ExtraSmallTop（4, 4, 0, 0）・LargeTop・LargeStart・LargeEnd・ExtraLargeTop（28, 28, 0, 0）。

### 形の一覧

C の `MaterialShapes.kt` に 35 個の形があり、どれも正規化した `RoundedPolygon`（1×1 の正方形の中、中心は (0.5, 0.5)）です。

Circle, Square, Slanted, Arch, Fan, Arrow, SemiCircle, Oval, Pill, Triangle, Diamond, ClamShell, Pentagon, Gem, Sunny, VerySunny, Cookie4Sided, Cookie6Sided, Cookie7Sided, Cookie9Sided, Cookie12Sided, Ghostish, Clover4Leaf, Clover8Leaf, Burst, SoftBurst, Boom, SoftBoom, Flower, Puffy, PuffyDiamond, PixelCircle, PixelTriangle, Bun, Heart.

作り方：

- 共通の丸めは `CornerRounding(radius)` で、半径は 0.15 / 0.2 / 0.3 / 0.5 / 1.0 の5種類です。G の `CornerRounding(radius, smoothing ∈ [0, 1])` が元です。
- 例：Circle は `circle(numVertices = 10)`。Square は `rectangle(1, 1, 丸め 0.3)`。Triangle は頂点 3、丸め 0.2 を −90° 回したもの。Sunny は `star(半径ごとに 8 頂点, 内側の半径 0.8, 丸め 0.15)`。
- 多くの形は、丸めを個別に持つ種の点を中心の周りに `reps` 回並べて作ります。並べるときに鏡映することもあります。

### 形の変形

G の `Morph.kt` から。

1. 二つの輪郭を弧の長さで測る。
2. 一方の角を、もう一方のいちばん近い角に対応させる。進み具合 0 の位置が揃うよう、二つめの輪郭を切って回す。両方の輪郭が 3 次ベジェ曲線の列として一対一に組になるまで曲線を分割する。
3. 進み具合 t のとき、組になった曲線（8 個の数）を線形に補間する。t が [0, 1] を少し外れてもよいので、ばねの行き過ぎがそのまま形の行き過ぎになる。

## 文字

C の `tokens/TypeScaleTokens.kt`（`VERSION: v0_103`）と W の `_md-sys-typescale.scss` から。大きさの単位は sp です。太さは Regular が 400、Medium が 500、Bold が 700 です。

| 段階 | 大きさ / 行の高さ | 太さ | 字間（C） | 字間（W） |
|---|---|---|---|---|
| Display L | 57 / 64 | 400 | **−0.2** | −0.25 |
| Display M | 45 / 52 | 400 | 0 | 0 |
| Display S | 36 / 44 | 400 | 0 | 0 |
| Headline L | 32 / 40 | 400 | 0 | 0 |
| Headline M | 28 / 36 | 400 | 0 | 0 |
| Headline S | 24 / 32 | 400 | 0 | 0 |
| Title L | 22 / 28 | 400 | 0 | 0 |
| Title M | 16 / 24 | 500 | **0.2** | 0.15 |
| Title S | 14 / 20 | 500 | 0.1 | 0.1 |
| Body L | 16 / 24 | 400 | 0.5 | 0.5 |
| Body M | 14 / 20 | 400 | **0.2** | 0.25 |
| Body S | 12 / 16 | 400 | 0.4 | 0.4 |
| Label L | 14 / 20 | 500 | 0.1 | 0.1 |
| Label M | 12 / 16 | 500 | 0.5 | 0.5 |
| Label S | 11 / 16 | 500 | 0.5 | 0.5 |

太字にした三つの字間は、二つの出典で値が違います。C の値はどれも、W の値を小数第 1 位で丸めたもの（ちょうど半分のときは偶数側へ：−0.25 → −0.2、0.15 → 0.2、0.25 → 0.2）になっているので、W が丸める前の値とみなし、lamina-ui は W に従います。

**強調の段階。** 大きさと行の高さは同じで、太さだけが変わります。Display・Headline・Title L・Body は 500、Title M・Title S・Label は 700 です。W は字間を標準のまま使っています。C には強調用の字間が別にありますが、「生成したトークンが出たら差し替える」という注記の下に置かれた暫定の値です。

## 色

### 役割と明度（2021 年の仕様・TonalSpot）

U の `cpp/dynamiccolor/material_dynamic_colors.cc` から、コントラストの段階 0 のときの値です。実行時には、背景とのコントラストの条件を満たすよう明度がずれることがあります。

| 役割 | ライト | ダーク |
|---|---|---|
| primary / on-primary | 40 / 100 | 80 / 20 |
| primary-container / on-primary-container | 90 / 30 | 30 / 90 |
| inverse-primary | 80 | 40 |
| secondary・tertiary・error | primary と同じ | primary と同じ |
| surface / on-surface | 98 / 10 | 6 / 90 |
| surface-dim / surface-bright | 87 / 98 | 6 / 24 |
| surface-container の lowest / low /（無印）/ high / highest | 100 / 96 / 94 / 92 / 90 | 4 / 10 / 12 / 17 / 22 |
| surface-variant / on-surface-variant | 90 / 30 | 30 / 80 |
| inverse-surface / inverse-on-surface | 20 / 95 | 90 / 20 |
| outline / outline-variant | 50 / 80 | 60 / 30 |
| surface-tint | 40 | 80 |
| shadow・scrim | 0 | 0 |
| *-fixed / *-fixed-dim | 90 / 80 | 90 / 80 |
| on-*-fixed / on-*-fixed-variant | 10 / 30 | 10 / 30 |

C の `ColorLightTokens.kt`（`VERSION: v0_210`）では on-*-container が 30 ではなく 10 です。U は 2024-06-11 のコミット「Update MCU to for expressive on-colors」で 30 に変えています。

TonalSpot の色の系列（U の `cpp/scheme/scheme_tonal_spot.cc`）：

| 系列 | 色相 | 彩度 |
|---|---|---|
| primary | 元の色 | 36 |
| secondary | 元の色 | 16 |
| tertiary | 元の色 + 60° | 24 |
| neutral | 元の色 | 6 |
| neutral-variant | 元の色 | 8 |

基準の primary は #6750A4 です（C の `PaletteTokens.Primary40`）。

### C++ 版の色の計算ライブラリの状況

- U には C++ 版（`cpp/`）があり、HCT・色の系列・減色・色の採点・DynamicScheme と、配色の種類（Monochrome, Neutral, TonalSpot, Vibrant, Expressive, Fidelity, Content, Rainbow, FruitSalad）が入っています。
- **C++ 版は 2021 年の仕様しか実装していません。** Java 版には `ColorSpec2021`・`ColorSpec2025`・`ColorSpec2026`（`SpecVersion`）がありますが、C++ 版には仕様を選ぶ仕組みも、2025 年の仕様で増えた役割（`primary_dim` など）もありません。`cpp/` の最後の変更は 2024-10-01 で、Java 版は 2026-10 まで更新されています。
- ビルド用のファイルが無く（インクルードのパスが Google の社内向けの形）、abseil（`flat_hash_map`・`str_cat`）と gtest に依存しています。

### 決定

まず C++ 版で 2021 年の仕様のまま始め、あとで 2025 年の仕様を C++ に移したものに差し替えます。HCT と色の系列は二つの仕様で共通で、違うのは役割ごとの明度を決める層だけなので、その層を差し替えられる形にしておきます。

## 高さと状態の重ね

C の `tokens/ElevationTokens.kt`（`VERSION: v0_210`）：Level0〜5 は 0 / 1 / 3 / 6 / 8 / 12 dp。

影は W の `elevation/internal/_elevation.scss` に従います。影は 2 枚重ねで、ずれ・ぼかし・広がりは段階について折れ線の関数になっているので、段階の間の値も決まります。

| 段階 | 主の影（不透明度 0.3）：y / ぼかし | 周りの影（不透明度 0.15）：y / ぼかし / 広がり |
|---|---|---|
| 0 | 0 / 0 | 0 / 0 / 0 |
| 1 | 1 / 2 | 1 / 3 / 1 |
| 2 | 1 / 2 | 2 / 6 / 2 |
| 3 | 1 / 3 | 4 / 8 / 3 |
| 4 | 2 / 3 | 6 / 10 / 4 |
| 5 | 4 / 4 | 8 / 12 / 6 |

CSS のぼかしの半径は、ガウス分布の標準偏差の 2 倍です。

C の `tokens/StateTokens.kt`（`VERSION: v0_103`）の状態の重ねの不透明度：hover 0.08、focus 0.10、pressed 0.10、dragged 0.16。

## 部品

### ボタン

C の `tokens/Button{XSmall,Small,Medium,Large,XLarge}Tokens.kt`（高さ・四角いときと押したときの角・アイコンの大きさと間隔・前後の余白・枠線の太さ）と `tokens/{Filled,Tonal,Outlined,Elevated,Text}ButtonTokens.kt`（状態ごとの色と高さ）を `lamina/tokens/button.hpp` に写しました。トークンのファイルではなく C の `Button.kt` に従ったところ：

- text のボタンのラベルは `primary` です。トークンのファイルは `on-surface-variant` ですが、`Button.kt` が「トークンが直ったら置き換える」として `Primary` で上書きしています。
- ラベルの文字の段階は、`ButtonDefaults.textStyleFor` と同じく高さで決めます：medium 未満は Label L、large 未満は Title M、extra-large 未満は Headline S、それ以上は Headline L。
- 押したときの形は、`Button.kt` が「この部品が弾まないように」使っている標準の効果のばねで動かします。最小の幅は 58 dp（`ButtonDefaults.MinWidth`）です。

### 枠線つきの入力欄

C の `tokens/OutlinedTextFieldTokens.kt`（高さ 56 dp・角は extra-small・枠線 1 dp で焦点時は 2 dp・入力とラベルは Body L・状態ごとの色）と C の `internal/TextFieldImpl.kt`（余白 16 dp・浮いたラベルは Body S）を `lamina/tokens/text_field.hpp` に写しました。

## 未決の件

- ほかの部品のトークンはまだ調べていません。
