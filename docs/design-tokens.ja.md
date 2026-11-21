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

`1b31f4ddbd0d` の時点では、太字にした三つの字間が二つの出典で違っていました。C の値はどれも、W の値を小数第 1 位で丸めたもの（ちょうど半分のときは偶数側へ：−0.25 → −0.2、0.15 → 0.2、0.25 → 0.2）だったので、W を丸める前の値とみなし、lamina-ui は W に従いました。2026-11-21 に androidx の `6068a9492ba8` で読み直すと、C も −0.25・0.15・0.25 になっていて、二つの出典は一致しています。

**強調の段階。** `6068a9492ba8` の C の `tokens/TypeScaleTokens.kt` には、15 の役割すべてに `*EmphasizedSize`・`*EmphasizedLineHeight`・`*EmphasizedTracking`・`*EmphasizedWeight` があり、`TypographyTokens.kt` がそれを `displayLargeEmphasized` から `labelSmallEmphasized` に読み込みます。大きさ・行の高さ・字間は標準の役割と同じで、太さは Display・Headline・Title L・Body が 500（Medium）、Title M・Title S・Label が 700（Bold）です。これらの値はまだ「生成したトークンが出たら差し替える」という注記の下にあるので、暫定です。lamina-ui は `TypeRole` の 15 の `*_emphasized` として持っています。

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

### 役割

`color::Role` には 53 個の役割があります。background・on-background・surface・surface-dim・surface-bright・surface-container の lowest / low /（無印）/ high / highest・on-surface・surface-variant・on-surface-variant・inverse-surface・inverse-on-surface・outline・outline-variant・shadow・scrim・surface-tint、primary・on-primary・primary-container・on-primary-container・inverse-primary、secondary・tertiary・error にも同じ 4 つずつ（inverse-primary に当たるものはありません）、primary・secondary・tertiary の -fixed・-fixed-dim・on-*-fixed・on-*-fixed-variant、そして 2025 年の役割の primary-dim・secondary-dim・tertiary-dim・error-dim です。

2021 年の仕様には dim の役割がありません（U の `ColorSpec2021.primaryDim()` は null を返します）。`spec_2021()` は、dim の役割に元の役割の色を入れます（primary-dim は primary、ほかも同じ）。U の `ColorSpec2026` も dim の役割を元の役割に戻しています。

### 2025 年の仕様

`lamina/color/spec_2025.cpp` は、U の `java/dynamiccolor/ColorSpec2025.java` と、それが使う `DynamicScheme` の規則を、スマートフォン向け（U の `DynamicScheme.Platform.PHONE`）の部分だけ移したものです。腕時計向け・2026 年の仕様・系列の代表色・Android だけの役割（`controlActivated`・`textPrimaryInverse` など）は移していません。HCT・HCT の逆算・コントラストの関数は U の C++ 版を使います。次の二つは U の古い C++ ではなく Java から移しました。色相の範囲ごとに色相を回す計算（`DynamicScheme.getRotatedHue`・`getPiecewiseValue`）と、黄色の色相（105° 以上 125° 未満）での明度 99 の色を明度 98 と 100 の平均にする処理（`TonalPalette.getHct`）です。

**配色の種類。** 2025 年の規則があるのは Neutral・TonalSpot・Vibrant・Expressive です。Monochrome・Fidelity・Content・Rainbow・FruitSalad では、U は 2021 年の仕様とその色の系列に戻る（`DynamicScheme.maybeFallbackSpecVersion`）ので、`spec_2025()` もそれらでは `spec_2021()` の色を返します。dim の役割は、その場合に Java が出す値にしています。2025 年の dim の規則を、2021 年の系列の上で 2021 年の明度の計算にかけたものです。

**色の系列**（スマートフォン向け。「回転」は元の色相に、色相の範囲ごとに決まった角度を足したもので、角度の表は `spec_2025.cpp` にあります）：

| 種類 | primary | secondary | tertiary | neutral | neutral-variant | error |
|---|---|---|---|---|---|---|
| Neutral | 元の色、8（青、つまり 250° 以上 270° 未満なら 12） | 元の色、4（青なら 6） | 回転、20 | 元の色、1.4 | 元の色、1.4 × 2.2 | 50 |
| TonalSpot | 元の色、32（ダークは 26） | 元の色、16 | 回転、28 | 元の色、5 | 元の色、5 × 1.7 | 60 |
| Vibrant | 元の色、74 | 回転、56 | 回転、56 | 回転、28 | 回転、28 × 1.29 | 80 |
| Expressive | 元の色、48（ダークは 36） | 回転、24（ダークは 16） | 回転、48 | 回転、18（ダークは 14、neutral の色相が黄色なら 6） | neutral と同じ色相、× 2.3（黄色なら × 1.6） | 64 |

error の色相は、元の色相の範囲によって 12°・22°・32° のどれかになります。2021 年はいつも 25°・彩度 84 でした。

**面**（neutral の系列）：

| 役割 | ライト（その他 / Vibrant / neutral の色相が黄色） | ダーク |
|---|---|---|
| surface | 98 / 97 / 99 | 4 |
| surface-dim | 87 / 85 / 90 | 4 |
| surface-bright | 98 / 97 / 99 | 18 |
| surface-container-lowest | 100 | 0 |
| surface-container-low | 96 / 95 / 98 | 6 |
| surface-container | 94 / 92 / 96 | 9 |
| surface-container-high | 92 / 90 / 94 | 12 |
| surface-container-highest | 90 / 88 / 92 | 15 |
| inverse-surface | 4 | 98 |

background は surface、surface-variant は surface-container-highest、surface-tint は primary と同じです。surface-container の各段・surface-dim（ライトだけ）・surface-bright（ダークだけ）は neutral の彩度に倍率を掛けます。TonalSpot なら low / 無印 / high / highest で 1.25 / 1.4 / 1.5 / 1.7 です。

**面の上の文字と線。** on-surface・on-surface-variant・outline・outline-variant は neutral の系列（neutral-variant ではなくなりました）の彩度に倍率（TonalSpot 1.7、Neutral 2.2、Expressive 1.6）を掛けて使い、明度は、ライトでは surface-dim、ダークでは surface-bright とのコントラストで決めます。それぞれ 9（ダークは 11）・4.5（ダークは 6）・3・1.5 です。Vibrant の on-surface は、neutral の彩度の 1.1 倍で彩度が最も高くなる明度から始めます。

**アクセントの明度。** アクセントの明度の多くは決まった数ではなくなりました。「彩度最大の明度」は、明度 100 から 1 ずつ下げながら、それまでに見つけた最大の彩度が系列の彩度に届かない間は探し続け、彩度が最も高かった明度を取ります。「彩度最小側の明度」は明度 0 から上げていきます。どちらも、そのあと範囲に収めます。TonalSpot では次のとおりです。

| 役割 | ライト | ダーク |
|---|---|---|
| primary | 彩度最大の明度 | 80 |
| primary-container | 彩度最大の明度、90 以下 | 彩度最小側の明度、35 から 93 |
| secondary | 彩度最大の明度 | 80 |
| secondary-container | 90 | 25 |
| tertiary | 彩度最大の明度 | 彩度最大の明度、98 以下 |
| tertiary-container | 彩度最大の明度 | 彩度最大の明度、93 以下 |
| error | 彩度最大の明度 | 彩度最小側の明度、98 以下 |
| error-container | 彩度最大の明度、90 以下 | 彩度最小側の明度、30 から 93 |
| primary・secondary・tertiary の -dim | 彩度最大の明度、90 以下 | 同じ |
| error-dim | 彩度最小側の明度 | 同じ |

ほかの種類にはそれぞれの範囲があり（Expressive と Vibrant には、シアン、つまり 170° 以上 207° 未満の色相と、Expressive の黄色の primary のための別の上限もあります）、`spec_2025.cpp` の表に置いてあります。そのうえで次の規則が掛かります。

- primary・secondary・tertiary・error は、surface-dim（ライト）か surface-bright（ダーク）に対してコントラスト 4.5 が要り、自分の container から 5 以上離れます。ライトでは暗い側、ダークでは明るい側です。
- container と *-fixed は、コントラストの段階が 0 より上のときだけ、コントラスト 1.5 が要ります。
- アクセントと dim の役割、それにコントラストの段階が 0 より上のときの container と *-fixed は、明度 49 と 65 の間を避けます。57 以上なら 65 以上に上げ、それより下なら 49 以下に下げます。面と *-fixed-dim は動かしません。
- dim の役割は、元の役割より 5 以上暗く、surface-container-high に対してコントラスト 4.5 が要ります。
- *-fixed は、container がライト・コントラスト 0 のときの明度です。*-fixed-dim はそれよりちょうど 5 暗くなります。
- on-primary・on-secondary・on-tertiary・on-error・on-*-container と inverse-primary（inverse-surface に対して）はコントラスト 6、on-error-container は 4.5、inverse-on-surface は 7、on-*-fixed は 7、on-*-fixed-variant は 4.5 です。

コントラストの曲線（コントラストの段階 −1 / 0 / 0.5 / 1 での値。間は直線でつなぎます）：1.5 → 1.5 / 1.5 / 3 / 5.5、3 → 3 / 3 / 4.5 / 7、4.5 → 4.5 / 4.5 / 7 / 11、6 → 6 / 6 / 7 / 11、7 → 7 / 7 / 11 / 21、9 → 9 / 9 / 11 / 21、11 → 11 / 11 / 21 / 21。コントラストの段階が 0 より下では、コントラストの条件がある役割は、自分の明度が条件を満たしていても、いつも `foregroundTone` が返す明度を取ります。

`tools/color-dump/` で全部の役割を Java のコードと比べています。結果はその README にあります。

### C++ 版の色の計算ライブラリの状況

- U には C++ 版（`cpp/`）があり、HCT・色の系列・減色・色の採点・DynamicScheme と、配色の種類（Monochrome, Neutral, TonalSpot, Vibrant, Expressive, Fidelity, Content, Rainbow, FruitSalad）が入っています。
- **U の C++ 版は 2021 年の仕様しか実装していません。** Java 版には `ColorSpec2021`・`ColorSpec2025`・`ColorSpec2026`（`SpecVersion`）がありますが、C++ 版には仕様を選ぶ仕組みも、2025 年の仕様で増えた役割（`primary_dim` など）もありません。`cpp/` の最後の変更は 2024-10-01 で、Java 版は 2026-10 まで更新されています。
- ビルド用のファイルが無く（インクルードのパスが Google の社内向けの形）、abseil（`flat_hash_map`・`str_cat`）と gtest に依存しています。

### 決定

lamina-ui は U の C++ 版と 2021 年の仕様で始め、役割ごとの明度を決める層を `ColorSpec` の向こうで差し替えられる形にしておきました。2025 年の仕様は、いまは Java のコードから移した `spec_2025()` としてあります（上の「2025 年の仕様」）。U の C++ の HCT とコントラストの関数の上に作った lamina-ui 自身のコードなので、取り込んだ C++ は変えておらず、`spec_2021()` の結果も変わりません。2025 年の仕様は明度の規則だけでなく色の系列も変えるので、移したものは両方を含みます。

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

### フォーカスの表示

2026-11-19 に、material-web の `9d2f79851cae` と androidx の `9b9d1fdd16f4` から読みました。上の表の版より新しいものです。

- W の `tokens/_md-comp-focus-ring.scss`：幅 3 dp、外側への間隔 2 dp、内側への間隔 0、色は secondary。それに、`duration-long4` をかけた、幅 8 dp のアクティブ幅（active width）。
- W の `focus/internal/_focus-ring.scss` は、外側への間隔だけ広げた箱の外に輪を描きます。角の半径も同じ間隔だけ大きくします。
- 輪は部品の形に沿います。W の `button/internal/_shared.scss`、`iconbutton/internal/_shared.scss`、`fab/internal/_shared.scss` は入れ物の角に、W の `switch/internal/_switch.scss` はトラックの角に合わせます。W の `checkbox/internal/_checkbox.scss` と `radio/internal/_radio.scss` は 44 dp の円にします。40 dp の状態の重ねに間隔を足した大きさです。
- Compose Material3 はボタンの周りに輪を描きません。フォーカスは状態の重ねを `FocusStateLayerOpacity` の 0.1（C の `tokens/StateTokens.kt`）にして示します。C の `tokens/CheckboxTokens.kt` と `tokens/SwitchTokens.kt` は `FocusIndicatorColor` を secondary としています。

lamina-ui は、輪（`lamina/tokens/focus.hpp`）とフォーカスの状態の重ねの両方を描きます。輪は速い効果のばねで現れ、消えます。material-web の 8 dp に広がる動きは作っていません。

### 項目の並ぶ部品のキー

2026-11-20 に、material-web の `9d2f79851cae` と、上に挙げた Compose のファイルから読みました。

- タブ：W の `tabs/internal/tabs.ts` は、列を Tab の一つの止まり位置にします（`tabIndex` を動かす方式）。ArrowLeft と ArrowRight で前後のタブへ動き、端では反対の端へ戻り、Home と End で両端へ動きます。`autoActivate` のときは、動いた先のタブを選びます。lamina-ui のタブの列は `autoActivate` と同じ動きです。W の `tabs/internal/tab.ts` は、フォーカスの輪をタブ全体の内側（`inward`）に描きます。内側への間隔は 0 です（W の `tokens/_md-comp-focus-ring.scss`）。Compose はタブを一つずつ `selectable` な止まり位置にし（C の `Tab.kt`）、矢印のキーはありません。
- ナビゲーションバーとレール：W の `labs/navigationbar/internal/navigation-bar.ts`（`role="tablist"`）は、ArrowLeft と ArrowRight でフォーカスを動かし（端では戻ります）、Home と End で両端へ動かしますが、選びはしません。Enter か Space でフォーカスのある項目を選びます。lamina-ui はこれに従い、タブと同じくバーやレールを一つの止まり位置にし、縦のレールでは上下の矢印を使い、輪は項目の印の周りに描きます。Compose は項目を一つずつ `selectable` な止まり位置にします（C の `NavigationItem.kt`）。material-web のバーが一つの止まり位置かどうかは、そのファイル（`navigationtab`）を読んでいないので分かりません。
- メニュー：W の `menu/internal/menu.ts` は `defaultFocus` の既定を `FIRST_ITEM` にし（W の `menu/internal/controllers/shared.ts`）、W の `list/internal/list-controller.ts` は ArrowUp と ArrowDown で動き（既定では端で戻ります）、Home と End で両端へ動きます。W の `menu/internal/controllers/menuItemController.ts` は Enter・Space・Escape でメニューを閉じ、フォーカスはメニューを開く前の要素に戻ります。フォーカスがメニューの外へ出ると（Tab）、フォーカスを戻さずに閉じます。lamina-ui は Tab でも閉じ、Escape と同じく開いた部品にフォーカスを戻します。W の `menu/internal/menuitem/menu-item.ts` は項目の輪を内側（`inward`）に描きます。文字を打って項目へ飛ぶ機能（W の `typeaheadController.ts`）は作っていません。
- スナックバー：操作にフォーカスがある間に時間を止めるのは lamina-ui 独自の決めごとで、WCAG 2.1 の達成基準 2.2.1（タイミング調整可能）によります。Compose の `SnackbarHost` から読んだものではありません。
- スナックバー：操作のフォーカスの枠は secondary ではなく、操作の文字と同じ inverse-primary で描きます。inverse-surface の面の上では secondary の差が小さく、見えにくいためです。これは lamina-ui 独自の決めごとです。どのボタンでも `ButtonColors::focus_ring` で枠の色の役割を指定できます。
- タブを受け取る入力欄は、WinForms と WPF の `AcceptsTab` のテキストボックスと同じく Ctrl+Tab で外へ出られるので、キーボードの罠になりません（WCAG 2.1 の達成基準 2.1.2）。

## 無効の状態

2026-11-21 に androidx の `6068a9492ba8`（この節の C はこのコミット）で読みました。各部品の Compose の既定の無効の色で、トークンのファイルと違うこともあります。

| 部品 | 入れ物 | 中身 | ほか |
|---|---|---|---|
| filled・elevated のボタン | on-surface 10 % | on-surface-variant 38 % | 高さ Level 0 |
| tonal のボタン | on-surface 12 % | on-surface 38 % | 高さ 0（`FilledTonalButtonTokens` はまだ `v0_103`） |
| outlined のボタン | なし | on-surface-variant 38 % | 枠線は outline-variant の 10 %（`outlinedButtonBorder(enabled)` が `DisabledContainerOpacity` を使います） |
| text のボタン | なし | on-surface-variant 38 % | |
| filled・tonal のアイコンボタン | on-surface 10 % | on-surface 38 % | 切り替え式も、オンかオフかによらず同じ色 |
| outlined のアイコンボタン | なし | on-surface 38 % | 枠線はオフの間 outline-variant の 38 %、オンの間はなし（`outlinedIconButtonVibrantBorder`） |
| standard のアイコンボタン | なし | on-surface 38 % | |
| スイッチ | 溝：オンは on-surface 12 %、オフは surface-container-highest 12 % | つまみ：オンは surface、オフは on-surface 38 %。アイコン：on-surface 38 %・surface-container-highest 38 % | オフの間の枠線は on-surface 12 %。どの色も surface の上に重ねます（`compositeOver(surface)`） |
| チェックボックス | 箱：オンと途中は on-surface 38 % | チェックの印：surface | オフの間の枠線は on-surface 38 % |
| ラジオボタン | | on-surface 38 % | |
| スライダー | | つまみは on-surface 38 % を surface の上に重ねた色 | 進んだ側の溝と終わりの点は on-surface 38 %、残りの溝は on-surface 12 %。目盛りは有効のときと同じく入れ替わります（進んだ側は on-surface 12 %、残りの側は 38 %） |
| 枠線つきの入力欄 | なし | 文字・ラベル・案内の文字は on-surface 38 % | 枠線は on-surface 12 %、1 dp |
| ナビゲーションバーとレールの項目 | | アイコンとラベル：選ばれていないときの色に `DisabledAlpha`（0.38） | |
| メニューの項目・まとまりに分けたメニューの項目 | まとまりの入れ物の色（選んだ色にしない） | 文字とアイコンは on-surface 38 % | |

- Compose は `enabled` が変わると色を一度に切り替えます。lamina-ui は既定の効果のばねで有効と無効の色の間を移します。
- Compose はボタンの高さを無効の高さへ一度に変えます（`Button.kt` の「無効になるときは移り変わりなし」）。lamina-ui は高さのばねで動かし、移り変わりを途切れさせません。
- `Tab.kt` は `enabled` を `selectable` に渡すだけで無効の色を持たず、タブのトークンにもありません。そのため Compose の無効なタブは有効なタブと同じに見えます。lamina-ui は、ナビゲーションの項目と同じく、無効なタブのアイコンとラベルを `DisabledAlpha`（0.38）で描きます。これは lamina-ui が決めたことです。
- `FloatingActionButton` には `enabled` がなく、FAB のトークンにも無効の色がないので、lamina-ui の FAB にも無効の状態はありません。

## 部品

### ボタン

C の `tokens/Button{XSmall,Small,Medium,Large,XLarge}Tokens.kt`（高さ・四角いときと押したときの角・アイコンの大きさと間隔・前後の余白・枠線の太さ）と `tokens/{Filled,Tonal,Outlined,Elevated,Text}ButtonTokens.kt`（状態ごとの色と高さ）を `lamina/tokens/button.hpp` に写しました。トークンのファイルではなく C の `Button.kt` に従ったところ：

- text のボタンのラベルは `primary` です。トークンのファイルは `on-surface-variant` ですが、`Button.kt` が「トークンが直ったら置き換える」として `Primary` で上書きしています。
- ラベルの文字の段階は、`ButtonDefaults.textStyleFor` と同じく高さで決めます：medium 未満は Label L、large 未満は Title M、extra-large 未満は Headline S、それ以上は Headline L。
- extra-small のボタンの前後の余白は 12 dp で、`ButtonDefaults.ExtraSmallContentPadding`（「ButtonXSmallTokens.kt の値が直ったら置き換える」）から取りました。トークンのファイルは 16 です。
- 押したときの形は、`Button.kt` が「この部品が弾まないように」使っている標準の効果のばねで動かします。最小の幅は 58 dp（`ButtonDefaults.MinWidth`）です。

### 枠線つきの入力欄

C の `tokens/OutlinedTextFieldTokens.kt`（高さ 56 dp・角は extra-small・枠線 1 dp で焦点時は 2 dp・入力とラベルは Body L・状態ごとの色）と C の `internal/TextFieldImpl.kt`（余白 16 dp・浮いたラベルは Body S）を `lamina/tokens/text_field.hpp` に写しました。選択範囲の色は `primary` の不透明度 0.4 で、C の `MaterialTheme.kt` の `TextSelectionBackgroundOpacity` から取りました。薄い案内の文字（プレースホルダー）は、2026-11-21 に `6068a9492ba8` の C の `internal/TextFieldImpl.kt` と `TextFieldDefaults.kt` から読みました。Body L で、色は `InputPlaceholderColor`（on-surface-variant）、無効のときは on-surface の 38 % です。不透明度は、フォーカスがある間は 1、フォーカスがなく空でラベルがあるときは 0（ラベルがなければ 1）、文字があるときは 0 です。「フォーカスなしで空」から「フォーカスあり」へと、「フォーカスなしで文字あり」から「フォーカスなしで空」へは遅い効果のばね、それ以外は速い効果のばねで移ります（`placeholderOpacity`）。描くのは文字（IME の変換中の文字を含む）が空の間だけなので、最初の一文字ですぐに消えます。取り消しの履歴は 100 まで残し（`undo_depth`）、1 秒より長く手を止めると打った文字のまとまりを区切ります（`undo_pause_seconds`）。Material 3 にはこれらのトークンがないので、lamina-ui が決めた値です。

### ボタンのアイコン

C の `Button.kt`（`ButtonDefaults.iconSizeFor`・`iconSpacingFor`・`contentPaddingFor`）：アイコンは extra-small から extra-large まで 20 / 20 / 24 / 32 / 40 dp で、アイコンがあっても前後の余白は変わりません。extra-small のアイコンとラベルの間は 4 dp です（`ExtraSmallIconSpacing`、「ButtonXSmallTokens.kt が直ったらその値を使う」）。トークンのファイルは 8 dp です。

### アイコンボタン

C の `tokens/{XSmall,Small,Medium,Large,XLarge}IconButtonTokens.kt`（高さ・アイコンの大きさ・narrow / default / wide の左右の余白・四角いとき・押したとき・オンのときの角・枠線の太さ）と、`tokens/{Filled,FilledTonal,Outlined}IconButtonTokens.kt`・`tokens/IconButtonTokens.kt`（`StandardIconButtonTokens`、色）を `lamina/tokens/icon_button.hpp` に写しました。C の `IconButton.kt` と `IconButtonDefaults.kt` から：

- uniform の幅は、アイコンに default の余白を左右に足したもので（`*ContainerSize(IconButtonWidthOption.Uniform)`）、どの大きさでも正方形になります。large だけは `DefaultLeadingSpace` の代わりに `UniformLeadingSpace` を持ちます。
- 形は、押している間は押したときの形、オンならオンの形、それ以外は通常の形で、標準の効果のばねで動かします（「弾まないように」）。
- Compose の outlined と standard は既定で `LocalContentColor` を使います。lamina-ui には文脈の色が無いので、トークンの色（Compose の `*VibrantColors`）を使います。
- Compose の outlined の枠線は、大きさによらず `SmallIconButtonTokens.OutlinedOutlineWidth`（1 dp）です。lamina-ui はボタンと同じく大きさごとの `OutlinedOutlineWidth`（large は 2 dp、extra-large は 3 dp）を使います。
- Compose は切り替えのアイコンボタンの色を一度に変えますが、lamina-ui は標準の効果のばねで移します。

### FAB

C の `tokens/Fab{Baseline,Medium,Large}Tokens.kt`（大きさ・角・アイコンの大きさ）と `tokens/Fab{Primary,Secondary}ContainerTokens.kt`（色、高さ 3・乗せたとき 4・押したとき 3）を `lamina/tokens/fab.hpp` に写しました。C の `FloatingActionButton.kt` から：

- medium の FAB の角は large-increased（20 dp、`ShapeDefaults.LargeIncreased`）です。トークンは TODO つきでコメントになっています。
- large の FAB のアイコンは 36 dp です（`LargeIconSize`、「FabLargeTokens.IconSize は誤り」）。トークンのファイルは 32 dp です。
- FAB は押しても形が変わりません。

### スイッチ

C の `tokens/SwitchTokens.kt`（つまみの通り道 52 × 32 dp と 2 dp の枠線、つまみ 16 / 24 / 28 dp、アイコン 16 dp、色）を `lamina/tokens/switch.hpp` に写しました。C の `Switch.kt` から：

- つまみの位置と大きさは速い空間のばね（`MotionSchemeKeyTokens.FastSpatial`）で動きます。つまみの中心は 16 dp から 36 dp まで（通り道の幅から高さを引いた分）動きます。
- つまみはオンのときかアイコンを載せたときに 24 dp、押している間は 28 dp です。
- 色はオンかオフかだけで決まります。トークンのファイルにある、乗せたときと押したときのつまみの色（primary-container・on-surface-variant）は使われていません。オンの通り道に枠線は無く（`Color.Transparent`）、オフの枠線は `UnselectedFocusTrackOutlineColor`（outline）です。
- Compose は色を一度に変えますが、lamina-ui はつまみと同じく連続して変わるよう、標準の効果のばねで移します。
- つまみのドラッグは作っていません。押すと切り替わります。

### チェックボックス

C の `tokens/CheckboxTokens.kt`（色・2 dp の枠線・40 dp の状態の重ね）と C の `Checkbox.kt` を `lamina/tokens/checkbox.hpp` に写しました。`Checkbox.kt` は、`ComposeMaterial3Flags.isCheckboxStylingFixEnabled` が false（既定）の間は今までの形で描くので、トークンのファイルではなくそちらに従います：

- 箱は 20 dp で、周りに 2 dp の余白があります（`CheckboxSize`・`CheckboxDefaultPadding`）。18 dp の `ContainerSize` ではありません。角は 2 dp、線は 2 dp で、線の端は四角です。
- チェックの印は箱の (0.2, 0.5) → (0.4, 0.7) → (0.8, 0.3) を通ります。中間の状態では、これらの点が y = 0.5 へ（角の点は x = 0.5 へも）寄って横線になります。
- 描く割合と横線への寄り方は、標準の空間のばねで動きます（印の全長を超えないよう切り詰めます）。オフにすると、どちらも 100 ms 後に 0 へ飛びます（`SnapAnimationDelay`）。
- 箱と印の色は、現れるときは標準の効果のばね、消えるときは速い効果のばねで移ります。

### ラジオボタン

C の `tokens/RadioButtonTokens.kt`（アイコン 20 dp・状態の重ね 40 dp・色）と C の `RadioButton.kt`（余白 2 dp・点 12 dp・線 2 dp）を `lamina/tokens/radio_button.hpp` に写しました。点の半径は速い空間のばね、色は標準の効果のばねで動きます。点は、`RadioButton.kt` と同じく半径から線の太さの半分を引いて描きます。

### 選ぶ部品の状態の重ね

スイッチ・チェックボックス・ラジオボタンは、状態の重ねとして 40 dp の円を部品の外まで描きます。Compose はこの波紋に `LocalContentColor` を使います。lamina-ui は、面の上の文脈の色にあたる on-surface を使います。押せる範囲は、小さいアイコンボタンも含めて少なくとも 48 dp です（`minimumInteractiveComponentSize`）。

### スライダー

C の `tokens/SliderTokens.kt`（通り道 16 dp・つまみ 4 × 44 dp・押したときのつまみ 2 dp・つまみの隙間 6 dp・終わりの印 4 dp・色）と C の `Slider.kt` を `lamina/tokens/slider.hpp` に写しました。`Slider.kt` から：

- 通り道は、つまみの幅の半分の所から、反対の端よりつまみの幅の半分だけ手前まで伸びます。つまみの両側の隙間は 4 dp のつまみの箱の半分に 6 dp を足した幅なので、つまみが細くなっても変わりません。つまみの側の角は 2 dp（`TrackInsideCornerSize`）、外側の角は通り道の高さの半分です。
- ポインターはこの範囲で値を比例して決め、そのあと値をいちばん近い段に寄せます。段があるときは、最初と最後の段を除いて、つまみと目盛りを通り道の角の半径だけ内側に描きます。`SliderImpl` の置き方と同じです。
- 目盛りの色は `SliderDefaults.colors` で入れ違いになっています。動いた側の目盛りは secondary-container、残りの側の目盛りは primary です。終わりの印と重なる最後の目盛りと、つまみの隙間に入る目盛りは描きません。終わりの印は動いた側の通り道の色です。
- 押している間とドラッグの間、つまみは幅が半分になります（`ThumbContent`）。Compose と同じく、lamina-ui も幅を一度に変えます。Compose と同じく、キーボードのフォーカスがあるときも細くなります（Compose はフォーカス・押下・ドラッグのどれがあっても `thumbSize` を半分にします）。
- キーは C `Slider.kt` の `slideOnKeyEvents` に従います。1 段、段が無ければ範囲の 1 %（`actualSteps` が 100）動き、`Page Up` と `Page Down` で `(actualSteps / 10).coerceIn(1, 10)` 段動き、`Home` と `End` で両端へ移ります（`slider_tokens.keyboard_*`）。横向きの Compose のスライダーは左右の矢印しか受けませんが、lamina-ui は上下の矢印も受けます。material-web のスライダーが標準の `<input type=range>` を通して受けるのと同じです（W `slider/internal/slider.ts`。`9d2f79851cae` で読みました。独自のキーの処理はありません）。material-web はフォーカスの輪をつまみの周りに描き（つまみの要素の中の `md-focus-ring`）、lamina-ui も同じです。

### タブ

C の `tokens/PrimaryNavigationTabTokens.kt` と `tokens/SecondaryNavigationTabTokens.kt`（48 dp の列・3 dp の印・Title S のラベル・色）、C の `tokens/DividerTokens.kt`、C の `Tab.kt` と `TabRow.kt` を `lamina/tokens/tabs.hpp` に写しました。トークンのファイルではなく .kt に従った所：

- ラベルの上にアイコンがあるタブは 72 dp の高さです（`LargeTabHeight`）。`IconAndLabelTextContainerHeight` は 64 です。ラベルのベースラインは印の 14 dp 上で、アイコンの下端はそのベースラインの 20 dp 上です。前に置くアイコンとラベルの間は 8 dp です（`LeadingIconTab`）。
- primary の印はタブの中身（左右 16 dp の余白を除いたラベル）と同じ幅で、少なくとも 24 dp です。タブの中央に置き、角は 3 dp です。secondary の印はタブの幅いっぱいで、高さは 3 dp です。`SecondaryIndicator` が高さを primary のトークンから取るためです。
- 下の区切り線は `HorizontalDivider`（1 dp の outline-variant）で、secondary のトークンにある surface-variant ではありません。
- 印の位置と幅は標準の空間のばねで動き、最初に置くときは動かしません。ラベルとアイコンの色は、現れるときは標準の効果のばね、消えるときは速い効果のばねで移ります（`TabTransition`）。状態の重ねは、選んだときの文字の色でタブ全体に掛かります。
- Compose の `Tab` は、色を渡さなければ選んでいないときの色を選んだときと同じにします。lamina-ui はトークンのファイルにある選んでいないときの色（on-surface-variant）を使います。

### ナビゲーションバー

C の `tokens/NavigationBarTokens.kt`（64 dp・surface-container・色・Label M）、`tokens/NavigationBarVerticalItemTokens.kt`（印 56 × 32 dp・アイコン 24 dp・項目の余白 6 dp）と C の `ShortNavigationBar.kt` と `NavigationItem.kt` を `lamina/tokens/navigation_bar.hpp` に写しました。lamina-ui にあるのは、項目を同じ幅に並べてアイコンをラベルの上に置く、短いナビゲーションバーです。

- 印は項目の上端から 6 dp 下にあり、ラベルは印の 4 dp 下にあります（`TopIconIndicatorToLabelPadding`）。
- 印の幅は 56 dp に進み具合を掛けたもので、進み具合は標準の空間のばねで動きます。そのため印は中央から広がり、少し行き過ぎます。不透明度も同じ進み具合で、1 を超えないよう切り詰めます。
- 状態の重ねは 56 × 32 dp の印の形の全体で、ポインターが項目のどこにあっても出ます。色は on-surface（バーの文字の色）です。
- 入れ物に高さはありません。`ShortNavigationBar` は `ContainerElevation`（Level2）を使いません。
- Compose はアイコンとラベルの色を一度に変えますが、lamina-ui は標準の効果のばねで移します。

### ナビゲーションレール

C の `tokens/NavigationRailCollapsedTokens.kt`（96 dp・上の 44 dp・項目の間 4 dp・surface）、C の `tokens/NavigationRailExpandedTokens.kt`（220 から 360 dp）、C の `tokens/NavigationRailBaselineItemTokens.kt`（項目 64 dp・アイコン 24 dp・見出しの部品の下 40 dp）、C の `tokens/NavigationRailVerticalItemTokens.kt`（印 56 × 32 dp・名前まで 4 dp・Label M）、C の `tokens/NavigationRailHorizontalItemTokens.kt`（高さ 56 dp の印・前の余白 16 dp・名前まで 8 dp・Label L）、C の `tokens/NavigationRailColorTokens.kt`、C の `WideNavigationRail.kt` と C の `NavigationItem.kt`（`AnimatedNavigationItem`・`AnimatedMeasurePolicy`）を `lamina/tokens/navigation_rail.hpp` に写しました。lamina-ui にあるのは、上から並べる既定の配置の、手前に重ならない `WideNavigationRail` です。

- 畳んだ状態と広げた状態は、同じばねの組の二つの行き先です。どれも標準の空間のばねで動きます：レールの最小の幅（96 から 220 dp）、全範囲の幅（96 から 360 dp）、項目の間（4 から 0 dp）、項目の最小の高さ（64 から 48 dp。触れる部品の最小の大きさ）、各項目のアイコンの位置の進み具合。幅は `WideNavigationRailLayout` の測り方に従います。広げたレールは、動いている最小の幅になります。いちばん広い項目か見出しの部品が 220 dp より広ければ、その幅（動いている全範囲の幅まで）になります。
- 項目はレールの始めの側に置き、印の前に 20 dp（`WNRItemHorizontalPadding`。.kt の中の定数）、アイコンが上にある間は印の後ろにも 20 dp を空けます（アイコンが前に移ると 0 dp。この余白は Compose と同じく一度に切り替わります）。最小の高さより小さい中身は上下の中央に置くので、畳んだときの印は項目の上端から 6 dp 下になります。
- 印のアイコンの周りの余白は、進み具合につれて 16 × 4 dp から 16 × 16 dp に変わり、大きさはアイコン（24 × 24 dp）から「アイコン・8 dp・名前」に変わります。上にあるときは 56 × 32 dp、前にあるときは高さ 56 dp で、角はいつも丸め切ります。項目を選ぶと、印の幅は標準の空間のばねで 0 から始めの側の端から広がります（`placeRelativeWithLayer(startPadding, …)`）。ナビゲーションバーのように中央からではありません。不透明度も同じ進み具合です。
- 名前は、進み具合が 0.5 を過ぎたところで Label M から Label L に、上の位置から前の位置に替わります。不透明度は 4 (p − 0.5)² です。Compose は項目が最初に現れたときにアイコンが上かどうかを覚えるので、広げた状態で始まったレールは畳んでも Label L のままです。lamina-ui はいつも進み具合だけで決めます。
- 色：印は secondary-container、選んだアイコンは on-secondary-container、選んだ名前は上にあるとき secondary・前にあるとき on-secondary-container（.kt はここで `ItemActiveIcon` を使い回し、足りないトークンについての `TODO` があります）、選んでいないものは on-surface-variant です。Compose は名前の色を標準の効果のばねで動かし、アイコンの色は一度に変えます。lamina-ui は両方をそのばねで動かします。
- 状態の重ねは、全幅の印の形の全体で、ポインターが項目のどこにあっても出ます。色は on-surface です。波紋はレールの文字の色を取るので、`NavigationRailColorTokens` の状態の重ねの値（on-secondary-container）は使われません。
- 見出しの部品は中身の領域の左上に置き、項目はその 40 dp 下から始めます（`HeaderSpaceMinimum`）。
- まだないもの：手前に重なるレール（`ModalWideNavigationRail`）、中央と下の配置、名前のない項目、バッジ、幅 80 dp の狭い `NavigationRail`。

### メニュー

C の `tokens/MenuTokens.kt`（surface-container・extra-small の角・Level2 の影）、C の `tokens/ListTokens.kt`（前のアイコン 24 dp・on-surface のラベル・on-surface-variant のアイコン）、C の `Menu.kt`・`internal/MenuPosition.kt` と `androidMain/.../AndroidMenu.android.kt` を `lamina/tokens/menu.hpp` に写しました。この節は従来のドロップダウンメニュー（`DropdownMenu`・`DropdownMenuItem`）です。Expressive のまとまりに分けたメニューは下の節にあります。

- 項目の上下に 8 dp、項目は高さ 48 dp で幅 112 から 280 dp です。左右の余白とアイコンとラベルの間は 12 dp（`DropdownMenuItemHorizontalPadding`）、文字は Label L です。
- 置き場所（`MenuAnchorPosition.Below`）：縦は、元の部品の下・上・上端を中心にした所・窓の上端か下端の順に試し、窓の上下から 48 dp 離れて収まる最初の所に置きます。横は、始めをそろえる・終わりをそろえる・窓の左端か右端の順に試し、横の余白は 0 です（`AndroidMenu` が 0 を渡します）。
- メニューは元の部品と接する点（`calculateTransformOrigin`）を中心に、0.8 倍から速い空間のばねで大きくなり、速い効果のばねで現れます。閉じるときはその逆です。
- 描画面には全体の不透明度がありません。lamina-ui は描く色をそれぞれ薄くしてメニューを消すので、重なった部分の混ざり方は消える途中でわずかに違います。

### まとまりに分けたメニュー

C の `tokens/StandardMenuTokens.kt`（surface-container-low・選んだときの tertiary-container と on-tertiary-container・on-surface の名前・on-surface-variant のアイコン）、C の `tokens/SegmentedMenuTokens.kt`（項目 44 dp・アイコン 20 dp・角の大きさ・まとまりの間 2 dp）、C の `tokens/MenuTokens.kt`（Level2 の影）、C の `tokens/DividerTokens.kt`（1 dp・outline-variant）、C の `MenuDefaults.kt` と C の `Menu.kt`（`DropdownMenuPopup`・`DropdownMenuGroup`・`SelectableDropdownMenuItem`・`CheckableDropdownMenuItem`）を `lamina/tokens/grouped_menu.hpp` の `grouped_menu_tokens` に写しました。

- まとまり：それぞれが Level2 の影を持つ一枚の面で、中身の上下に 2 dp の余白があり、次のまとまりとは 2 dp 離れます（見本どおりの `GroupSpacing`）。メニュー全体の幅は、いちばん広い項目か見出しの幅（`IntrinsicSize.Max`）なので、どのまとまりも同じ幅です。まとまり全体を囲む入れ物はありません。
- まとまりの角は位置で決まります：一つだけなら 16 dp、最初は上 16 dp・下 8 dp、間のものは 8 dp、最後は上 8 dp・下 16 dp です。マウスが一度乗ってから離れたまとまりは角が 8 dp（`inactiveShape`）になり、角は速い空間のばねで動きます。マウスが乗っているかどうかは、`DropdownMenuGroup` の既定どおりまとまりごとに持ちます。見本はすべてのまとまりで一つの操作の元を共有していますが、lamina-ui はそうしていません。
- 項目：まとまりの左右から 4 dp 引っ込み、幅 112 から 280 dp、高さ 44 dp 以上、四方に 12 dp の余白、前に 20 dp のアイコンと文字までの 8 dp、終わりの側に 20 dp のアイコンです。これは既定で切ってある `isPrecisionPointerComponentSizingEnabled` を使わないときの値です。文字は Label L です。`Menu.kt` は `SegmentedMenuTokens.ItemLabelTextFont`（Body L）ではなく `labelLarge` を指定しています（`TODO` 付き）。
- 項目の角：一つだけなら 4 dp、最初は上 12 dp・下 4 dp、間のものは 4 dp、最後は上 4 dp・下 12 dp です。選んだ項目の角は 12 dp です。角は速い空間のばねで、入れ物の色は速い効果のばねで変わります。Compose は文字とアイコンの色を一度に変えますが、lamina-ui は同じ効果のばねで動かします。
- まとまりの見出しは Label L・on-surface で、高さ 32 dp 以上、前に 12 dp・後ろに 4 dp を空けます。その下の区切り線は、左右に 12 dp・上下に 2 dp を空けます。どちらも付けなくて構いません。
- 置き場所：従来のメニューと同じ候補を試し、窓の上下から 48 dp、左右から 8 dp 離します（`MenuHorizontalMargin`。`DropdownMenuPositionProvider` の既定値です。従来のメニューは `AndroidMenu` から 0 を受け取ります）。開け閉めの動きは従来のメニューと同じです。
- 押したときにどうするかは、Compose ではアプリが決めます。lamina-ui はまとまりごとに `MenuSelection` で選べます。`none` は項目を知らせてメニューを閉じ、`single` は一つを選び、`multiple` は切り替えます（`CheckableDropdownMenuItem` と同じ）。後の二つではメニューは開いたままです。
- まだないもの：補足の文字、鮮やかな色（`VibrantMenuTokens.kt`）、入れ子のメニュー、前のアイコンがない項目に選んだときだけ出るアイコン（Compose は横に広げて出します）。`selected_leading_icon` は、すでにある前のアイコンを差し替えるだけです。

### ダイアログ

C の `tokens/DialogTokens.kt`（surface-container-high・extra-large の角・Headline S・Body M・色）、C の `tokens/ScrimTokens.kt`（窓を覆う幕。色は scrim、不透明度 0.32）と C の `AlertDialog.kt`（余白 24 dp・見出しの下 16 dp・本文の下 24 dp・幅 280 から 560 dp・ボタンの間 8 dp）を `lamina/tokens/dialog.hpp` に写しました。

- ダイアログに影はありません。`AlertDialog` は影を付けず色の高さも 0 なので、`ContainerElevation`（Level3）は使われていません。
- 付けてもよいアイコン（`DialogTokens.IconSize` 24 dp・`IconColor` は secondary）は見出しの上の中央に置き、下に 16 dp を空けます（`IconPadding`）。このとき見出しも中央にそろえます（`AlertDialogContent`）。
- ボタンは `AlertDialogFlowRow` に並べたテキストボタンです。並べる向きを逆にした折り返しの行で、ボタンの間は 8 dp です。そのため一行なら閉じるボタンが確定のボタンの前に来て終わりの側にそろい、収まらなければ確定のボタンが一行目に残り、閉じるボタンが次の行に移ります。どちらも終わりの側にそろえます。Compose の行の間は、8 dp から「触れる範囲 48 dp がボタンの高さ 40 dp を超える 8 dp」を引いた値、つまり 48 dp の枠どうしの間で 0 dp です。lamina-ui はボタンを高さ 40 dp で並べるので、その 8 dp を戻し、Compose と同じく行と行は 8 dp 離れます。最後の行の下にある 4 dp の触れる範囲は足していないので、最後のボタンは下端から 24 dp です（Compose では 28 dp）。
- 見出しが複数行に折り返すとき、lamina-ui は最も長い行の幅の塊として中央に置きます。Compose の折り返す文字は与えられた最大の幅を取るはずなので、アイコンがあっても各行は始めの側から並ぶと思われます（未確認）。一行の見出しは同じです。
- Compose は現れるときと消えるときの動きを OS の窓に任せています。lamina-ui は独自に、面を 0.8 倍から標準の空間のばねで大きくし、面と幕を標準の効果のばねで現します。
- 中身（`6068a9492ba8` で読みました）：`AlertDialogContent` は `text` の部品を `weight(1f, fill = false)` の `Box` に入れ、下に 24 dp を空けます。そのため好きな中身を置け、ダイアログが高すぎるときは縮みます。Compose はこれをスクロールさせず、呼ぶ側が `verticalScroll` を足します。lamina-ui は本文と `DialogOptions::content` をこの枠に入れ、自分で `ScrollArea` の中でスクロールさせます。高さの上限（範囲から上下に `window_margin` の 48 dp を引いた高さ）と、本文と中身の間の 16 dp（`supporting_text_content_space`）は lamina-ui が決めた値です。Compose は高さを OS のダイアログの窓に任せ、本文の枠も一つだけです。M3 の案内がスクロールする中身の端に描く区切り線は描きません。Compose も描きません。

### スナックバー

C の `tokens/SnackbarTokens.kt`（面は inverse-surface、角は extra-small、Level 3、知らせは inverse-on-surface の Body M、操作は inverse-primary の Label L、高さは 1 行で 48 dp・2 行で 68 dp）、C の `Snackbar.kt`（幅は最大 600 dp、周りに 12 dp、文字の前に 16 dp、操作の後に 8 dp、文字と操作の間に 8 dp、文字の上下に 14 dp）と C の `SnackbarHost.kt`（短い時間 4 秒・長い時間 10 秒。0.8 倍から速い空間のばねで大きくし、速い効果のばねで現す）を読み、`lamina/tokens/snackbar.hpp` に入れました。

- 高さは知らせの行数に Body M の行の高さ（20 dp）を掛けて決めます。そのため、フォント自身の上下の高さにかかわらず、1 行で 48 dp、2 行で 68 dp です。
- Compose は、操作のあるスナックバーの時間を既定で無期限にします。lamina-ui は指定がなければ短い時間を使い、ポインターが乗っている間は時間を止めます。Compose はこれをしません。

### ツールチップ

2026-11-21 に `6068a9492ba8` の C の `Tooltip.kt`・`internal/BasicTooltip.kt`・`tokens/PlainTooltipTokens.kt`・`tokens/RichTooltipTokens.kt`（`VERSION: v0_210`）を読み、`lamina/tokens/tooltip.hpp` に入れました。

- 簡素なもの：入れ物は inverse-surface、文字は inverse-on-surface の Body S、角は extra-small（4 dp）、余白は 8 × 4 dp（`PlainTooltipContentPadding`）、大きさは 40 × 24 dp 以上（`TooltipMinWidth`・`TooltipMinHeight`）、幅は 200 dp まで（`plainTooltipMaxWidth`）、影なし。
- 詳しいもの：入れ物は surface-container、角は medium（12 dp）、影は Level 2（`ContainerElevation`）、幅は 320 dp まで、左右の余白は 16 dp。見出し（Title S、on-surface-variant）は最初の行の基線を上から 28 dp に置きます（`HeightToSubheadFirstLine`）。本文（Body M、on-surface-variant）は、見出しか操作があるときは最初の行の基線を見出しの下 24 dp に置き（`HeightFromSubheadToTextFirstLine`）、下に 16 dp を空けます。どちらもないときは上下 4 dp です。操作（Label L、primary）は高さ 36 dp 以上で、下に 8 dp を空けます（`ActionLabelMinHeight`・`ActionLabelBottomPadding`）。lamina-ui は操作を小さいテキストボタンで描きます。
- 置き場所（`TooltipPositionProviderImpl`）：部品から 4 dp（`SpacingBetweenTooltipAndAnchor`）。上に置くときは横は中央にそろえ、上にはみ出すなら下に置き、窓の中に収めます。下・左・右も同じ考え方です。吹き出しの先は、形を渡したときだけ描きます（既定ではなし）。`caretSize` は 16 × 8 dp で、部品の真ん中を指します（`caretX`）。Compose は吹き出しの先をツールチップの外に描き、間は同じ 4 dp のままなので、先が部品に 4 dp 食い込みます。lamina-ui は間に先の高さを足し、先と部品の間を 4 dp に保ちます。
- 動き：速い空間のばねで 0.8 倍から大きくなり、速い効果のばねで不透明度が変わります（`MotionSchemeKeyTokens.FastSpatial`・`FastEffects`）。
- 出し方（`BasicTooltip.kt`）：マウスが部品に入るとすぐ（待ちなしで）出て、出ると消えます（`persistent` でなければ）。指かペンで長押しすると、離すまで出ます。キーボードのフォーカスが部品に来ると出て、外れると消え、Escape でも消えます。フォーカスか長押しで出したもので `persistent` でないものは、1500 ms で自分で消えます（`TooltipDuration`）。ポインターで出したものは、ポインターが乗っている間は残ります。
- lamina-ui が決めたこと：ポインターが乗ってから出るまでの待ちはトークン `hover_show_delay_seconds` で、Compose と同じ 0 にしてあり、`TooltipOptions::hover_delay_seconds` でツールチップごとに変えられます。長押しの時間 0.5 秒（`long_press_seconds`）は lamina-ui が決めた値です。Compose は OS の `ViewConfiguration` を使います。Compose は共有の `MutatorMutex` で一度に一つしか出しませんが、lamina-ui はこれを強制しません。部品とツールチップの外を押すと、出ているツールチップは消えます。高さは、スナックバーと同じく、行数に文字の段階の行の高さを掛けて数えます。詳しいツールチップの操作は、スナックバーの操作と同じく `FocusGated` でキーボードのフォーカスを受けます。Compose のように Tab でポップアップの中へ強制的にフォーカスを移すことはしません。

### 円形の進み具合の表示

C `tokens/CircularProgressIndicatorTokens.kt`（40 dp、波のある形は 48 dp、表示と下地の太さは 4 dp、間は 4 dp、波の振幅は 1.6 dp、波長は 15 dp）、C `tokens/ProgressIndicatorTokens.kt`（表示は primary、下地は secondary-container）、C `WavyProgressIndicator.kt` と `internal/CircularWavyProgressModifiers.kt` を読み、`lamina/tokens/progress_indicator.hpp` に入れました。

- 表示は上から始まり、時計回りに伸びます。下地は表示の終わりから上まで続き、両端で表示との間に、すき間と両側の丸い端の分をあけます。進み具合が 0 のときはすき間が閉じ、下地は一周の円になります。`CircularProgressDrawingCache.updateDrawPaths` と同じです。
- `WavyProgressIndicatorDefaults.indicatorAmplitude` は、進み具合が 10 % から 95 % の間で波を最大にし、その外では波をなくします。波は 1 秒に 1 波長進みます（`waveSpeed = wavelength`）。
- Compose は、円を内側の半径 0.75 の星形へ変形させ、正規化して大きさに合わせることで波を描きます。そのため波の深さは直径の決まった割合になり、直径とともに大きくなります（lamina-ui では Compose の形を描いて比べてはいません）。lamina-ui は代わりに、円周に沿った長さの正弦で波を描き、振幅と波長を dp で持ちます。どの大きさでも同じに見え、波長が円周を割り切らなくてもかまいません（波があるときは両端が出会わないので、大きさを変えても波は跳びません）。
- Compose は振幅を 500 ms の補間（増えるときは standard、減るときは emphasized-accelerate）で変えます。lamina-ui は遅い効果のばねで動かし、波を止めた部品（`set_wavy(false)`）は波を平らにするので、一時停止したタイマーは動きを止めます。
- Compose の線は端が丸く、長さ 0 の区間は何も描きません。lamina-ui は太さより短い弧を長さに合わせて細くするので、伸び始める表示や縮み切る下地が、点ごと急に現れたり消えたりしません。
- `sample_length`（1 dp）は lamina-ui 独自の値です。弧は輪郭として塗る（`render::stroke_path`）ので、弧に沿って 1 dp ごとに点を取ります。

### スクロールする範囲

Material 3 にはスクロールバーの定めがなく、Compose Material 3 にもありません。`lamina/tokens/scroll_area.hpp` の値はすべて lamina-ui 独自のもので、出典から読んだものはありません。

- 送る量は標準の効果のばねで動かします。臨界減衰なので行き過ぎません。Expressive の空間のばねは行き過ぎるので、中身の端を越えて送ってしまいます。
- ホイール 1 ノッチで 64 dp、Page Up と Page Down で画面に残す 48 dp、幅 4 dp のつまみ（ポインターを乗せると 8 dp）、端から 2 dp、長さは 16 dp 以上、`on_surface` の 38 %（乗せると 50 %）、バーの上とみなす終わりの側の帯 12 dp、送り終えてから消えるまで 1 秒です。

### 高さのトゥイーン

2026-11-21 に `6068a9492ba8` の C の `internal/Elevation.kt` を読み、`lamina/tokens/elevation.hpp`（`elevation_tweens`）と `widgets/interaction_elevation.hpp` に入れました。

- Compose はチップとカードの高さ（影）を、ばねでなくトゥイーンで動かします。マウスが乗る・フォーカス・押す・引きずるに向かうときは 120 ms の `FastOutSlowInEasing`（0.4, 0, 0.2, 1）、元に戻るときは 150 ms の（0.4, 0, 0.6, 1）、マウスが離れたあとは 120 ms です。目標は最後に始まった操作で決まり（`interactions.lastOrNull()`）、無効になるとすぐに無効の高さへ切り替わります。lamina-ui は `motion::Tweened` で同じことをします。第 1 段階のボタンは、今もばねで高さを動かします。

### 区切り線

2026-11-21 に `6068a9492ba8` の C の `Divider.kt` と `tokens/DividerTokens.kt`（`VERSION: v0_117`）を読み、`lamina/tokens/divider.hpp` に入れました。outline-variant の 1 dp で、横の線は場所の幅いっぱい、縦の線は高さいっぱいです。

- Compose には空ける指定がなく、呼ぶ側が余白を付けます。C の `tokens/ListTokens.kt` には 16 dp の `DividerLeadingSpace` と `DividerTrailingSpace` がありますが、Compose のどこからも使われていません。material-web の区切り線（W の `9d2f79851cae`、`divider/internal/_divider.scss`）は、`inset`・`inset-start`・`inset-end` で 16 px を空けます。lamina-ui の `DividerInset` はこの 16 dp を使います。
- 読み上げ用の役割 `separator` は lamina-ui が決めたものです。Compose の区切り線には意味づけがありません。

### バッジ

2026-11-21 に `6068a9492ba8` の C の `Badge.kt` と `tokens/BadgeTokens.kt` を読み、`lamina/tokens/badge.hpp` に入れました。

- 小さいバッジは 6 dp（`Size`）の丸。大きいバッジは 16 dp 以上（`LargeSize`）で角は丸く、両端の内側に 4 dp（`Badge.kt`）、文字は Label S です。どちらも error の上に on-error です。
- `BadgedBox`：幅が 6 dp より広いバッジは中身を持つとみなし、印の終わりの辺の 12 dp 手前から始め（`BadgeWithContentHorizontalOffset`）、下の辺を印の上の辺から 14 dp 下に置きます（`BadgeWithContentVerticalOffset`）。16 dp のバッジは印より 2 dp 上に出ます。点はどちらも 6 dp（`BadgeOffset`）で、上の終わりの角の内側に収まります。ナビゲーションの項目では `Modifier.badgeBounds()` が `BadgeEndRuler` と `BadgeTopRuler` を決め、バッジを項目の中に収めます。この定規がなければ何も収めません。lamina-ui は、ナビゲーションバーとレールでは項目の範囲に収め、アイコンボタンでは収めません。
- Compose ではバッジはアイコンに付けます（`IconButton { BadgedBox { Icon } }`、ナビゲーションの項目のアイコンの枠）。lamina-ui はアイコンの 24 dp の四角に付け、レールがアイコンを動かす間もついていきます。
- Compose は数を短くしません。`"999+"` は見本（`samples/BadgeSamples.kt`）に出てくるだけで、上限 999 の `badge_count` は、その書き方のために lamina-ui が足したものです。
- Compose はバッジに意味づけを付けず、`NavigationBarItem` はラベルを出している間アイコンの意味づけを消します。lamina-ui は、バッジの説明を項目やボタンの読み上げ用の説明に入れます。

### チップ

2026-11-21 に `6068a9492ba8` の C の `Chip.kt` と `tokens/{Assist,Filter,Input,Suggestion}ChipTokens.kt` を読み、`lamina/tokens/chip.hpp` に入れました。

- 高さは 32 dp 以上、角は small（8 dp）、文字は Label L、アイコンは 18 dp、アバターは 24 dp（丸く切り抜き、無効の間は 38 %）。両端の余白は 8 dp で、`ChipArrangement(8 dp)` が先頭の枠・文字・末尾の枠の間に、枠が空でも 8 dp を空けます。そのため文字だけなら両側が 16 dp になります。入力チップは、アバターがあるか先頭のアイコンがないときは始まりの側を 4 dp、アイコンがあるときは 8 dp にし、末尾のアイコンがあれば終わりの側を 8 dp、なければ 4 dp にします（`InputChipDefaults.contentPadding`）。
- 色（`*ChipDefaults`）：アシストの平らなものは入れ物なし、文字は on-surface、アイコンは primary、枠線は outline-variant の 1 dp。浮いたものは surface-container-low で枠線なし。提案はアシストと同じで、文字が on-surface-variant。フィルターは、文字が on-surface-variant、先頭のアイコンが primary、末尾のアイコンが on-surface-variant で、選ぶと secondary-container の上に on-secondary-container になり、枠線は消えます（0 dp）。入力はフィルターと同じで、先頭のアイコンが on-surface-variant、選ぶと primary です。無効の色は、中身が on-surface の 38 %、枠線が on-surface の 12 %、浮いた入れ物と選ばれた入れ物が on-surface の 12 % で、選ばれているかどうかに関わらず無効の色を使います。
- 高さ（影）：平らなものは 0（平らなフィルターチップはマウスが乗ると 1 dp）。浮いたものは 1 dp、乗ると 3 dp、無効で 0。引きずる間の 8 dp は、lamina-ui ではまだ使いません。
- フィルターと入力のチップは選択つきの `Role.Checkbox`、アシストと提案は `Role.Button` です。当たりは 48 dp です。
- `AnimatingChipContent`：先頭の枠は、速い空間のばねの `expandHorizontally` と遅い効果のばねの浮かび上がりで現れ、標準の効果のばねの `shrinkHorizontally` と速い効果のばねの消え方で消えます。アイコンを別のアイコンに替えるときはすぐに替わります。Compose は色と枠線をすぐに切り替えますが、lamina-ui は標準の効果のばねで移します。
- 取り入れていないもの：`shapes:` の版（`ChipShapes`。12 dp・丸・8 dp の角の間を速い空間のばねで形を変える）と、それと組の色。引きずる操作。
- lamina-ui が決めたこと：Compose の末尾のアイコンはチップの一部で、操作を持ちません。lamina-ui は、material-web の入力チップに外すボタンがあるのにならい、`removable` の入力チップの末尾のアイコンに外すための当たり（チップの中の「Remove …」という名前の読み上げ用のボタン。状態の重ねも自分で持ちます）を付けました。Delete と Backspace でも外せますが、これは material-web にもありません。`ChipGroup` は material-web のチップの組（W の `chips/internal/chip-set.ts`・`_chip-set.scss`）に従います：8 px の間、折り返し、Tab の止まり位置一つ、無効なチップを飛ばして端で戻る ArrowLeft と ArrowRight、Home と End。Compose にはチップの組がなく（見本ではチップをそれぞれ 4 dp の余白つきで `FlowRow` に並べます）、チップがそれぞれ Tab の止まり位置です。外したチップが縮んで消えることと、動くチップが滑ることは lamina-ui が決めました。

### カード

2026-11-21 に `6068a9492ba8` の C の `Card.kt`・`Surface.kt`・`tokens/{Elevated,Filled,Outlined}CardTokens.kt` を読み、`lamina/tokens/card.hpp` に入れました。

- 角はどれも medium（12 dp）。入れ物は、浮いたカードが surface-container-low、塗ったカードが surface-container-highest、枠線のカードが surface で outline-variant の 1 dp の枠線、中身は on-surface です。
- 無効（`CardDefaults.*cardColors`）：塗ったカードは surface-container-highest の上に surface-variant の 38 %、浮いたカードは surface の上に surface の 38 %（つまり surface）、枠線のカードは surface、中身は on-surface の 38 % です。枠線のカードの線は、surface ではなく浮いたカードの入れ物 surface-container-low の上に outline の 12 % を重ねた色で（`outlinedCardBorder`）、lamina-ui もそれに従います。
- 高さ（影）：塗ったカードは 0、乗ると 1 dp。浮いたカードは 1 dp、乗ると 3 dp、無効で 1 dp。枠線のカードはどの状態でも 0 です（`Card.kt` が押下・フォーカス・乗ったときの高さを既定の値にするので、乗ったときのトークンは使われません）。引きずる間の 6 dp と 8 dp は lamina-ui では使いません。`onClick` のないカードの高さは動かず、押せるカードは高さのトゥイーンで動きます。
- `Surface` は枠線を中身の上に描き（`Modifier.border`）、中身を形で切り抜きます。lamina-ui も両方そうします。
- 押せるカードは役割なしの `clickable` な `Surface` で、48 dp 以上です。押せないカードは入れ物（`isContainer`）です。lamina-ui は `button` と `group` を返します。Compose は無効なカードの中身を `LocalContentColor` に任せますが、lamina-ui のラベルはそれぞれ色の役割を持つので、中身全体を 38 % で描きます。

### リスト

2026-11-21 に `6068a9492ba8` の C の `ListItem.kt`・`ListItemDefaults.kt`・`tokens/ListTokens.kt` を読み、`lamina/tokens/list.hpp` に入れました。lamina-ui は操作できる版の `ListItem` に従います（上下の余白が 8 dp と 12 dp の従来の版は非推奨です）。

- 余白は始まりと終わりに 16 dp、上下に 10 dp（`ListItemDefaults.ContentPadding`）、枠と文字の間は 12 dp（`ItemBetweenSpace`）です。最小の高さは 1 行・2 行・3 行で 56・72・88 dp で、小見出しと補足の文が両方あるか、補足の文が 2 行以上になると 3 行です。内側の高さが 60 dp（(88 + 72) / 2 − 10 − 10）未満なら枠と文字を縦の中央に、それ以上なら上にそろえます。
- 見出しは on-surface の Body L、小見出しは Label S、補足の文は Body M、枠は on-surface-variant、末尾の文字は Label S です。選ばれた項目や入っている項目は secondary-container の上に on-secondary-container です。無効なら、選ばれているかどうかに関わらず surface の上に on-surface の 38 % です。
- 形：止まっているときは extra-small（4 dp）、マウスが乗ると medium（12 dp）、フォーカス・押下・選択で large（16 dp）（`shapeForInteraction`）。速い空間のばねで動きます。色は標準の効果のばね（`updateTransition`）、状態の重ねは中身の色です。
- 選べる項目は `selected` つきの `Role.RadioButton`、切り替えられる項目は状態つきの `Role.Checkbox` で、入っていると選ばれた色にもなります。押せるだけの項目は役割なしです（lamina-ui は `list_item`）。
- Compose は枠の大きさのトークンを持ちますが、強制しません。lamina-ui はトークンどおりの大きさにします：アイコン 24 dp、アバター 40 dp（primary-container の上に on-primary-container の Title M の頭文字）、画像 56 dp で角は small（`ItemLeadingImageExpressiveShape`）。チェックボックスとラジオボタンは 24 dp、スイッチは 52 × 32 dp で、Compose の部品が項目の中で取る大きさです。枠の部品は項目に従います。Compose の見本でも `onCheckedChange` を渡していません。
- キー：Compose は項目ごとにフォーカスの止まり位置を作ります。`ListView` は material-web のリスト（W の `list/internal/list-controller.ts`）に従い、止まり位置一つで、無効な行を飛ばして端で戻る ArrowUp と ArrowDown、Home と End を受けます。Page Up と Page Down、行の内側に描くフォーカスの輪は lamina-ui が決めました。Compose は Space か Enter を押している間、押した形を見せますが、lamina-ui はキーを離したことを受け取れないので、押した形を見せずにすぐ動きます。
- `ListView`、行の高さの見積もり、`overscan_rows`（2）は lamina-ui が決めたものです。Compose の遅延リストは別のライブラリです。

### トップアプリバー

2026-11-21 に `6068a9492ba8` の C の `AppBar.kt` と `tokens/AppBar{,Small,Medium,Large,MediumFlexible,LargeFlexible}Tokens.kt` を読み、`lamina/tokens/top_app_bar.hpp` に入れました。

- 高さ：小さいものと中央そろえは 64 dp、中くらいは 112、大きいものは 152、中くらいの伸縮するものは 112（副題つきで 136）、大きい伸縮するものは 120（副題つきで 152）。2 段のバーは 64 まで縮みます。題名：1 段では Title L、下の段では Headline S（中くらい）・Headline M（大きいもの、中くらいの伸縮するもの）・Display S（大きい伸縮するもの）。2 段のバーの上の段は Title L で、副題は Label M です。副題：1 段では Label M、下の段では Label L（中くらいの伸縮するもの）と Title M（大きい伸縮するもの）。伸縮するバーも強調でない文字の段階を使います。
- 色：surface、スクロールすると surface-container。戻るアイコンと題名は on-surface、操作と副題は on-surface-variant。影はありません。
- 両端に 4 dp（`TopAppBarHorizontalPadding`）。題名は max(12 dp, 戻るアイコンの枠) に 4 dp を足した所から始まるので、アイコンなしで 16 dp、48 dp の枠があれば 56 dp です。中央そろえの題名はバー全体の中央に置き、アイコンにかかるときだけ内側へ押します。下の段の題名は、最後の行の基線から測って下から 24 dp（中くらい）か 28 dp（大きいもの）に置きます。
- `TopAppBarState`：`heightOffsetLimit` は、1 段ではバーの高さ、2 段では下の段の高さにマイナスを付けたもの。`collapsedFraction` は `heightOffset / limit`、`overlappedFraction` は `|contentOffset| / |limit|` で、1 を超えません。`pinned` は中身の動きを数えるだけ、`enter_always` はどのスクロールもまずバーを動かし、`exit_until_collapsed` は先へ送るスクロールをまずバーが取り、中身が先頭で使い切れなかった分だけバーを戻します。
- 小さいバー：`overlappedFraction` が 0.01 を超えると、標準の効果のばねでスクロール時の色へ移ります。2 段のバー：アニメーションなしで `lerp(container, scrolled, FastOutLinearInEasing(f))`。下の題名は 1 − f、上の題名は `TopTitleAlphaEasing`（0.8, 0, 0.8, 0.15）で現れ消えします。下の段は縮み、題名を切り抜きます。
- 止まり方：Compose は `onPostFling`（はじいたあとか、バー自身を引きずったあと）で止め方を決めます。1 % 未満か 100 % ならそのまま、それ以外は近いほうの端へ `snapAnimationSpec`（標準の効果のばね）で動きます。lamina-ui にははじく動きがないので、スクロールする範囲が止まったとき（目標が残っておらず、つまみも引きずっていないとき）に止め方を決め、中身もバーと一緒に動かします。
- 取り入れていないもの：バー自身を引きずってはじく動き（lamina-ui のバーは引きずれません）。`onPreScroll` が、バーが少しでも動けば限界を超えた分も含めて差分をすべてバーに渡してしまう点。lamina-ui は残りを中身に渡します。
- 意味づけ：Compose のバーは役割なしのたどる順のまとまりで、題名に `heading` はありません。lamina-ui は `toolbar` と、題名の `label` を返します。2 段のバーは、`hideTopRowSemantics` と同じく、半分縮んだ所から上の段の題名を見せます。

### セグメントボタン

`6068a9492ba8` の C の `SegmentedButton.kt` と `tokens/OutlinedSegmentedButtonTokens.kt` を読み、`lamina/tokens/segmented_button.hpp` に入れました。

- どの部品か：`SegmentedButton` には `@Deprecated` がなく（`contentPadding` を取らない二つの古い形が、バイナリ互換のために隠されているだけです）、KDoc も置き換え先を挙げていません。つながったボタンの組（`ButtonGroupDefaults.connected*` の形を使う `ButtonGroup` と `ToggleButton`）はこのコミットにあり、KDoc に `@material3expressive` の印がありますが、実験的という注釈はなく、セグメントボタンに代わるとも書かれていません。そのため lamina-ui は `SegmentedButton` を作りました。つながったボタンの組はこの段階では作っていません。
- 大きさ：行の高さは 40 dp 以上（`ContainerHeight`）。項目はそれぞれ `weight(1f)` で `ButtonDefaults.MinWidth`（58 dp）を持ち、行が `IntrinsicSize.Min` なので、どの区画もいちばん広い区画と同じ幅になります。左右に 12 dp、上下に 8 dp（`ButtonDefaults.ContentPadding` の縦）、アイコンは 18 dp で、文字の前に 8 dp（非公開の `IconSpacing`）、文字は Label L です。
- 形：`CornerFull`。最初の項目は始まり側、最後の項目は終わり側の角だけを残し、間の項目は四角です。行は `spacedBy(-1 dp)` なので、1 dp の枠線が重なります。
- 色：選ばれた区画は secondary-container の地に on-secondary-container、選ばれていない区画は透明な地に on-surface、枠線は outline。無効のときは、中身が on-surface の 38 %、枠線が outline の 12 %（`DisabledOutlineColor` ではありません）、選ばれた地はそのままです。
- 重なり：選ばれた区画は 5 を足し、押されているときとフォーカスがあるときにそれぞれ 1 を足します。
- 中身：アイコンの場所は常に測られ（18 dp とアイコンの大きいほう）、アイコンがないと文字を −(18 + 8)/2 = −13 dp ずらし、最初の配置のあとは速い空間のばねで動かします。既定のアイコンは `Icons.Filled.Check` で、選ばれていないときのアイコンがなければ `AnimatedVisibility` で `fadeIn(DefaultEffects) + scaleIn(0、左下、FastSpatial)`、消えるときの動きはありません。あれば `Crossfade(DefaultEffects)` です。
- 読み上げ：一つだけ選ぶ行は `selectableGroup` の中の `Role.RadioButton` の `selectable`、いくつでも選べる行は役割のない `toggleable` です。
- 違い：Compose がすぐに切り替える色を、標準の効果のばねで移しています。行全体を Tab の止まり位置一つとし、矢印キーを受けます（一つだけ選ぶ行では選び、いくつでも選べる行ではフォーカスを動かします）。Compose はどの区画も Tab の止まり位置で、矢印キーはありません。いくつでも選べる区画は `toggle_button` を返します。区画の内側にフォーカスの輪を描くのは lamina-ui の足したものです。

### スライダー：縦・中央から・トラックの角・値の表示

`6068a9492ba8` の C の `Slider.kt` と `tokens/SliderTokens.kt` を読みました。

- `VerticalSlider`：つまみは `VerticalThumbSize` の 44 × 4 dp で、押す・ドラッグする・フォーカスがあるときは高さが半分になります。トラックは幅 16 dp。`topToBottom` の既定は true（最小値が上）。キーは `↑` と `↓`（`value − sign·delta` と `+`）、同じ符号の `Page Up` と `Page Down`、`Home` と `End` で、`←` と `→` は受けません。
- `SliderDefaults.CenteredTrack`：塗った部分は中央からつまみまでです。隙間は、つまみと反対側が 6 dp、つまみ側が `つまみ/2 + 6 dp`。塗った部分の角は両方とも 2 dp の内側の角で、止まりの点は両端だけ、目盛りは二つの隙間と両端では描きません。
- `Track(sliderState, trackCornerSize)` と `CenteredTrack` は角を縮めます。段がなければ、塗った部分と塗っていない部分を、角の大きさではなく幅 0 になるまで描きます。lamina-ui は各半径を部分の短い辺の半分までに抑えていて、それで角が縮みます。縦のスライダーの既定のトラックも縮みます。
- 値の表示：Compose にはトークン（`ValueIndicatorContainerColor` が inverse-surface、`ValueIndicatorLabelTextColor` が inverse-on-surface、`ValueIndicatorLabelTextFont` が Label L、`ValueIndicatorActiveBottomSpace` が 12 dp）しかなく、どこからも使われていません。形と動きは `9d2f79851cae` の material-web のスライダー（`labeled`、`slider/internal/_slider.scss`）から取りました。角が丸く高さ 28 dp 以上（`label-container-height`）、内側に 4 dp、フォーカスがあるときと押しているときに出て、下端の中央から `duration-short2`（100 ms）の `easing-emphasized` で大きくなります。lamina-ui はつまみを押している間とフォーカスがある間に出し（material-web はポインターを乗せたときも出します）、下の三角は描かず、横向きのスライダーだけに出します。
- このコミットの Compose にないので作っていないもの：トラックの中のアイコンと、XS〜XL のスライダーの大きさ（`SliderTokens.kt` だけがあり、トラックは 16 dp の一つです）。

### 範囲スライダー

`6068a9492ba8` の C の `Slider.kt`（`RangeSlider`、`RangeSliderState`）を読みました。つまみの名前は androidx の `compose/ui/ui/src/androidMain/res/values{,-ja}/strings.xml`（`range_start` が "Range start" / 範囲の先頭、`range_end` が "Range end" / 範囲の末尾）から取りました。

- トラック：始まり側の塗っていない部分は `始まり − 隙間` まで、塗った部分は `始まり + 隙間` から `終わり − 隙間` までで両端が 2 dp の角（幅が 2 dp を超えるときだけ描きます）、終わり側の塗っていない部分は `終わり + 隙間` からです。止まりの点は、その部分を描いているときだけ両端に付けます。目盛りは両端と二つの隙間では描きません。
- 押したとき：近いほうのつまみです。同じ距離なら、押した位置が始まりのつまみより前なら始まりのつまみ（`rawOffsetStart > posX`）。二つのつまみがどちらも押した位置からタッチの遊びの内にあるときは、ドラッグの向きで決めます。始まりの値は `[最小, 終わり]`、終わりの値は `[始まり, 最大]` に収めます。
- キー：つまみはそれぞれフォーカスを受けます。`←` と `→`、`Page Up` と `Page Down`、`Home`（始まりは最小値、終わりは始まりの値）、`End`（始まりは終わりの値、終わりは最大値）。lamina-ui はスライダーと同じく `↑` と `↓` も受けます。
- 違い：lamina-ui はスライダーと同じく押した時点で選んだつまみを動かします。Compose はタッチの遊びを越えるか離すまで待ちます。Compose は状態に書き戻すときに値を目盛りに合わせ、lamina-ui はすぐに合わせます。

### 日付の選択

`6068a9492ba8` の C の `DatePicker.kt`・`DateRangePicker.kt`・`DateInput.kt`・`DateRangeInput.kt`・`DatePickerDialog.kt`・`tokens/DatePickerModalTokens.kt` と、`androidMain/res/values{,-ja}/strings.xml` の文字列を読み、`lamina/tokens/date_picker.hpp` と `lamina/calendar/locale.cpp` に入れました。

- 入れ物は幅 360 dp（`ContainerWidth`）、ダイアログは高さ 568 dp まで（`ContainerHeight`）、surface-container-high で、ダイアログの形は extra-large（28 dp）です。ピッカーだけのときはただの四角を塗り、角を丸くするのはダイアログだけです。`TonalElevation` は level 0 なので、ダイアログに影はありません。
- 見出し：高さ 120 dp 以上（`HeaderContainerHeight`）。題は Label L で始まりから 24 dp・上から 16 dp、選んだ日付は Headline L で始まりから 24 dp・下から 12 dp、モードの切り替えは終わりと下から 12 dp。その下に outline-variant の 1 dp の区切り線。題は "Select date"、選んだ日付は `yMMMd` の書式か、なければ "Selected date"（文字入力では "Entered date"）です。
- 月の行は高さ 56 dp で左右に 12 dp。丸い形の文字ボタンに `yMMMM` の月と `ArrowDropDown`（開くと動きなしで 180° 回ります。lamina-ui は `arrow_drop_up` を描きます）、それに前後の月のアイコンボタンがあり、年の一覧を開いている間は消えます。
- 曜日：短い単独の名前を Body L（on-surface）で、48 dp のます目に出します。日：48 dp のます目に 40 dp の丸で Body L。選んだ日は primary に on-primary の文字、今日は選ばれていなければ primary の文字と 1 dp の primary の輪、無効な日は 38 %。いつも 6 週（288 dp）です。
- 年の一覧：行の間を 16 dp 空けた `GridCells.Fixed(3)` で、72 × 36 dp の項目をます目の中央に置き、形は `CornerFull`。表示中の年は primary に on-primary の文字、今年は 1 dp の輪と primary の文字、ほかは on-surface-variant です。曜日と日のます目を覆い（48 × 7 − 1 dp の下に区切り線）、`expandVertically(DefaultEffects) + fadeIn(DefaultEffects, 0.6)` で開き、`shrinkVertically(DefaultEffects) + fadeOut(FastEffects)` で閉じます。最初に見える項目は表示中の年の 3 つ前です。年を選んでも月はそのままです。
- モードの切り替え：`AnimatedContent` です。文字入力へは、新しい中身が自分の高さの分だけ下から滑り上がり（`DefaultSpatial`）現れ（`DefaultEffects`）、古い中身は 48 dp 上へ滑りながら消えます（`FastEffects`）。戻るときはその逆で、`SizeTransform(DefaultSpatial)` です。入力欄は `DurationMedium2`（300 ms）後にフォーカスを求めます。
- 文字入力：左右 24 dp・上 10 dp の枠線つき入力欄で、ラベルは "Date"、薄い案内の文字は書式を大文字にしたものです。数字だけ、8 桁まで受け、各部分が埋まった時点で区切りを出します（`DateVisualTransformation`）。書式は、ロケールの短い日付の書式を `dd`・`MM`・`yyyy` と区切りに縮めたもので、CLDR から `MM/dd/yyyy`（en-US）・`dd/MM/yyyy`（en-GB）・`yyyy/MM/dd`（ja-JP）です。8 桁打つと調べ、"Date does not match expected pattern: %1$s"・"Date out of expected year range %1$s - %2$s"・"Date not allowed: %1$s"（と日本語の文）を出します。
- 期間の選択：見出しは高さ 68 dp 以上（128 − 60）で、題と Title L の見出しは始まりから 64 dp、"Start date - End date"（ASCII のハイフンで、間は 4 dp）。縦の月の一覧で、月ごとに Title S の月の名前（始まり 24 dp・上 20 dp・下 8 dp）の下にます目を置きます。クリック：開始日がないか期間がそろっていれば新しい開始日、開始日以降なら終了日、開始日より前なら新しい開始日です。帯は secondary-container で高さ 40 dp、開始日の丸の中心から終了日の丸の中心まで。期間内の文字は on-secondary-container です。モードは `FastEffects` で入れ替わります。期間の文字入力は 8 dp 離した二つの欄で、"Invalid date range input" を出します。
- 読み上げ：日の名前は `yMMMMEEEEd` で、期間では "Start date"・"End date"・"In range" が、今日には "Today" が前に付きます。年は "Navigate to year %1$s"、見出しの説明は "Current selection: %1$s" か "Entered date: %1$s"（なければ "None"）です。
- 違い：選んだ日の丸は標準の効果のばねで現れます（Compose では実際にはすぐに切り替わります）。矢印とキーでは月を標準の効果のばねで滑らせます（Compose の `animateScrollToItem` の既定の動きは material3 のソースにありません）。月の間をスワイプでは移れません。キー：Compose は月の最初と最後の有効な日での `←` と `→` だけを扱い、ほかは既定のフォーカス探しに任せています。lamina-ui は WAI-ARIA の日付の表に従い（矢印、`Page Up` と `Page Down`、Shift で 1 年、週の `Home` と `End`、`Space` か `Enter`）、年の一覧にも矢印と `Home`・`End` を付けました。月と年のボタンと矢印のボタンも lamina-ui の Tab の止まり位置です。期間の文字入力で開始日が途中までだと、Compose は `setSelection(null, end)` を呼んで両方の日付を消しますが、lamina-ui は開始日だけを消します。
- 入力欄の下に開く形：Compose にはありません。lamina-ui のこの形は、枠線つきの日付の入力欄の 4 dp 下に、角 16 dp（corner-large）・影 6 dp（level 3）の surface-container-high の面でカレンダーを出します。値は `9d2f79851cae` の material-web の `tokens/versions/latest/sass/_md-comp-date-picker-docked.scss`（`container-color`・`container-shape`・`container-elevation`、`container-width` は 360 px）から取りました。4 dp の隙間と、メニューと同じく 0.8 から大きくなる動きは lamina-ui が決めたものです。ツールチップ：Compose はピッカーのアイコンボタンを `IconButtonWithTooltip` で包んでいます。lamina-ui のピッカーはアイコンボタンを見せる（`mode_toggle()`・`previous_button()`・`next_button()`・`open_button()`・`PickerDialog::leading()`）だけで、名前のツールチップはプログラムが付けます。components の例はそうしています。

### 時刻の選択

`6068a9492ba8` の C の `TimePicker.kt`・`TimePickerDialog.kt`・`tokens/TimePickerTokens.kt`・`tokens/TimeInputTokens.kt` を読み、`lamina/tokens/time_picker.hpp` に入れました。

- 文字盤は 256 dp（`ClockDialContainerSize`）で surface-container-highest。数字は Body L の on-surface で、文字盤の 101/256 の円（`OuterCircleToSizeRatio`）に、24 時間制の 12〜23 時は 69/256 の円（`InnerCircleToSizeRatio`）に並び、数字のます目は 48 dp です。つまみは 48 dp の primary、2 dp の線がつまみの手前の縁まで伸び、中心は 8 dp。つまみの下の数字は on-primary です。24 時間制では、中心から 74 dp（`MaxDistance`）未満なら内側の円です。
- 時と分の箱は 96 × 80 dp、角 8 dp、Display L。選ばれた側は primary-container と on-primary-container、ほかは surface-container-highest と on-surface。区切りは 24 dp で、":" は on-surface で 4 dp 上げます。
- 午前・午後の切り替え（新しい形。`isUpdatedTimepickerToggleEnabled` が true）：4 dp 離した二つの切り替えボタンで、52 × 80 dp（縦。分の 4 dp 後）か 216 × 38 dp（横。16 dp 下）。選ばれていない側は surface-container-lowest と on-surface-variant で角が丸く、選ばれた側は primary-container と on-primary-container で角 12 dp、文字は太字です。形は `FastSpatial` で変わります。
- 配置：縦は表示・36 dp・文字盤・24 dp、横は表示の列・36 dp・文字盤。Compose は画面が縦より横に長いと横を選びます。lamina-ui は `TimePickerLayout` で決めます。
- 動き：つまみは近い向きを回って `DefaultSpatial` で動きます。押したときは分を 5 分に、時をいちばん近い時に合わせ、時を押したときは動きが終わってから 100 ms 待って分を出します。ドラッグではつまみが指に付き、離したときに切り替えます。数字の組は `DefaultEffects` で入れ替わります。
- 入力：欄は 96 × 72 dp で Display M、フォーカスがあると primary-container の地に 2 dp の枠線（outline）。下 7 dp に Body S で 2 行の補助の文字："Hour" と "Minute"、範囲外なら "Hour must be 1–12"・"Hour must be 0–23"・"Minute must be 0–59" をエラーの色で出します。埋まった欄に打つと新しい数字だけが残り、時を 2 桁（12 時間制なら 2〜9 の 1 桁）打つと分へ進みます。
- ダイアログ：内側に 24 dp、中身の 20 dp 上に Label M の題（"Select Time" / "Enter Time"）、ボタンの行は先頭にモードの切り替え（`Keyboard` / `Schedule`）、末尾に Cancel と OK を 8 dp 空けて置きます。
- 読み上げ：時と分の箱は "Select hour" と "Select minutes" のラジオボタン、文字盤の数字は "%1$d o'clock"・"%1$d hours"（24 時間制）・"%1$d minutes"、午前・午後の切り替えは "Select AM or PM"、入力欄は "for hour" と "for minutes" です。AM と PM は文字列の資源で、日本語でも "AM" と "PM" です。
- 違い：24 時間制の Compose のドラッグは、動くたびにつまみを時の位置に戻すので、つまみが 1 時間ずつ跳びます。lamina-ui はそこでも指に付いて動かします。Compose は 12 時間制で 0 と 00 を時として受けますが、文は 1〜12 と言っています。lamina-ui はこれを無効にしました。キーでは、時と分の箱、午前・午後の切り替え（止まり位置一つ）、文字盤（フォーカスのある数字を持つ止まり位置一つ）を Tab で巡ります。Compose の文字盤の数字は、箱を押したあとでないとフォーカスを受けません。入力欄は文字入力の部品ではなく、lamina-ui の 2 桁の欄です。ダイアログはモードの間を速い効果のばねで入れ替えます。Compose の時刻の選択には無効の状態がなく、lamina-ui は 38 % に薄めます。Compose がすぐに切り替える色は、ばねで移しています。

### 入力欄：補助の文字とエラー

C の `OutlinedTextField` の既定：補助の文字は Body S で、上に 4 dp（`SupportingTopPadding`）、始まりから 16 dp。色は on-surface-variant、`isError` ならエラーの色です。`isError` では枠線・ラベル・カーソルもエラーの色、ポインターを乗せた枠線は on-error-container。lamina-ui は標準の効果のばねで移します。

## 未決の件

- 上に挙げた部品のほかは、まだトークンを調べていません。
- 第 2 段階の部品で作っていないもの：チップの `shapes:` の版（`ChipShapes`）、2 dp の間と位置ごとの角を持つ `SegmentedListItem`、チップ・カード・リストの項目を引きずる操作、トップアプリバー自身を引きずる操作。
- 第 3 段階の部品で作っていないもの：つながったボタンの組（つながった `ToggleButton` を並べた `ButtonGroup` と、押したときの幅の広がり）、月の間のスワイプ、Compose の vibrant な時刻の選択（`TimeScroll` と `shapes:` の版）、Compose にないスライダーの形（トラックの中のアイコン、XS〜XL の大きさ）。
