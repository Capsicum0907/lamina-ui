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
| A | androidx/androidx の `compose/material3/adaptive/`（adaptive・adaptive-layout・adaptive-navigation）、`compose/material3/material3-adaptive-navigation-suite/`、`window/window-core/` | `6068a9492ba8` | Apache-2.0 |
| D | [material-components/material-components-web](https://github.com/material-components/material-components-web) の `packages/mdc-density/` と部品ごとの `$density-config` | `6ceadb81df50` | MIT |
| S | Android Studio が入れた Android SDK：SDK Platform `android-36.1`（リビジョン 1）の `platforms/android-36.1/data/res/values/config.xml` と、Sources for Android `android-36.1`（リビジョン 1）の `sources/android-36.1/android/view/ViewConfiguration.java`。拡大の表示のために、同じフォルダーの `ScaleGestureDetector.java` と `GestureDetector.java` も、ページ送りのために `sources/android-36.1/com/android/internal/widget/ViewPager.java` も読みました（どれも 2026-11-23） | 2026-11-22 に読みました | Apache-2.0 |
| J | [JetBrains/compose-multiplatform-core](https://github.com/JetBrains/compose-multiplatform-core) の `compose/foundation/foundation/src/desktopMain/` と `compose/ui/ui/src/desktopMain/`（Compose Multiplatform Desktop） | 2026-11-21 に読んだ `jb-main` ブランチ（コミットは記録していません） | Apache-2.0 |

値を使うときは Apache-2.0 の表示と出典を残します（D は MIT の表示）。

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

ThorVG の描画面は、影の形を一度だけぼかしてキャッシュから描き、いちばん近い整数の画素の位置に置きます（[描画面](app-structure.ja.md#描画面)）。`lamina/tokens/raster.hpp`：

| トークン | 値 | 使い道 |
|---|---|---|
| `shadow_quality` | 100 | ThorVG のガウスぼかしの質（0–100） |
| `shadow_margin_sigmas` | 3 | キャッシュの画像で形の周りに取る余白と、影が描画面の外かを決めるときに影が届く範囲。標準偏差の何倍か |
| `shadow_shape_steps` | 4 | キャッシュのキーでは形を 1/4 px に丸める |
| `shadow_cache_bytes` | 16 MiB | 取っておく影の画像 |
| `cull_glyph_outset_em` | 0.5 | 文字の行やアイコンが箱の外へはみ出しうる幅。大きさの何倍か |
| `worker_threads_max` | 3 | ThorVG のワーカーのスレッドの上限 |
| `reserved_cores` | 1 | ThorVG のワーカーに回さないコアの数 |
| `layers_max` | 3 | 1 フレームの描画面の層の上限。それより多く要る身元のある画像は ThorVG で描く（[GPU で描く画像](app-structure.ja.md#gpu-で描く画像)） |
| `retained_min_scale` | 0.5 | どちらかの向きにこれより小さく描く身元のある画像は ThorVG で描く |
| `image_texture_bytes` | 256 MiB | 身元のある画像のテクスチャの既定の上限（`WindowOptions::image_texture_bytes`） |
| `image_texture_smooth` | true | そのテクスチャをバイリニアで補間する（false は最近傍） |

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

### タッチの範囲

2026-11-23 に androidx の `6068a9492ba8` から読みました。material3 の `InteractiveComponentSize.kt`（`Modifier.minimumInteractiveComponentSize`、`LocalMinimumInteractiveComponentSize` = 48 dp）と `Surface.kt`、ui の `platform/ViewConfiguration.kt`（`minimumTouchTargetSize` = 48 × 48 dp。`androidMain` の `AndroidViewConfiguration.android.kt` では変えていません）、`node/NodeCoordinator.kt`（`hitTest`、`distanceInMinimumTouchTarget`）、`node/HitTestResult.kt`（`isHitInMinimumTouchTargetBetter`、`DistanceAndFlags.compareTo`）です。値と仕組みは `lamina/tokens/interaction.hpp`（`minimum_interactive_size`）と `lamina/widgets/interaction.hpp`（`touch_target`・`hit_area`・`target_hit`・`TargetArbiter`）にあります。

- 48 dp より小さい部品は、見た目の範囲を中心にした 48 dp の範囲でポインターを受けます。対象は、すべての大きさのボタン（ボタンの組・分割ボタン・切り替えボタンの中のものも）、アイコンボタン、チップ、チェックボックス、ラジオボタン、スイッチ、セグメントボタン（隣の区画と重ならないよう、高さだけ）、スライダー、範囲スライダー、カードです。描く大きさと並べる大きさは変わりません。
- いくつかの部品の範囲が同じ点を含むときは、見た目の範囲にその点を含む部品が勝ちます。どれも含まないときは、点にいちばん近い部品が勝ちます。近さは Compose と同じく、部品の縁からの距離の 2 乗で比べます。同じ近さなら、後から足した部品（上に描かれる部品）が勝ちます。Compose は上から順に調べて最初に当たったものを残すので、こちらも上の部品が勝ちます。勝つ部品は押したときに決め、離すまで変えません。指が隣の部品のほうへずれても、押した部品は替わりません。
- 負けた部品にもイベントは届きます。そのときは `PointerEvent::margin_claimed` が立っていて、部品は自分の見た目の範囲だけで判定します（`hit_area`）。なので、見た目の範囲の中の点には今までどおり反応し、ポップアップは外を押されると今までどおり閉じます。
- 勝ち負けを決める場所：`WidgetSet` の中の部品どうし（入れ子の `WidgetSet` は、中でいちばんよい部品の範囲で競います）と、`ButtonGroup`・`ChipGroup`・`FloatingToolbar`・`TopAppBar`・`FlexibleBottomAppBar`・`SplitButton` の中です。部品は `target_hit`（`Targeted` の型の条件）で競争に加わります。これを持たない部品（リストの行・入力欄・メニューなど）は競わないので、隣の部品の範囲がその上にかかることがあります。
- 違い：Compose の `minimumInteractiveComponentSize` は、並べる大きさにも 48 dp を取ります（「Reserves at least 48.dp in size」）。押せる `Surface(onClick)` は修飾の最初にこれを置くので、Compose の 40 dp のボタンは 48 dp の場所を取ります。lamina-ui は、ダイアログのボタン（[ダイアログ](#ダイアログ)）と同じく、並べる大きさを見た目の大きさのままにしています。Compose が範囲を広げるのはタッチだけで（`PointerType.Touch`。マウスとスタイラスは見た目の範囲）、lamina-ui はマウスとペンでも広げます。Compose には密度がなく、`LocalMinimumInteractiveComponentSize` を 0 dp にしてもタッチの範囲は広げたままです。lamina-ui は MDC と同じく、負の密度では範囲を外します（[密度](#密度)）。無効の部品も競争に加わるので、無効の部品が勝った余白を押しても何も起きません。

### 指のあとのマウスのこだま

lamina-ui が自分で決めた決まりです。Windows で SDL 3.4.10 が送るものを見て決めました（[テスト](testing.ja.md#windows-で指の入力を送る)）。仕組みは `lamina/input/touch_echo.hpp` の `input::TouchEchoFilter` で、許す幅は `lamina/tokens/interaction.hpp` の `touch_echo_tolerance_pixels`（1 px）です。SDL の窓は、SDL から来るポインターの動く・押す・離すをすべてこれに通してから部品に渡します。

- 指が触れているあいだは、マウスの動きを部品に渡しません。
- 指が離れたあとは、画面の上で最後に指があった点から、縦横どちらも 1 画素未満のところへのマウスの動きを部品に渡しません。それ以外のマウスの動きはすぐに渡します。なので、マウスが本当に動けば、ホバーはすぐ戻ります。
- 点は画面の座標で覚えます（窓の位置とイベントの位置の和）。窓が動いたあとでも、画面の同じ場所への動きは渡しません。
- マウスの押す・離す・外へ出る、ホイール、指とペンのイベントは、いつも渡します。マウスを押すと、離したことが届かなかった指も終わったものとします。

理由。Windows では、指で触るとシステムのカーソルもその点へ動きます。そのあと Windows は、カーソルの下の窓へマウスの動きを自分で送ります。これには指の印がありません。SDL は、Windows が指から作るマウスのメッセージをこの印で見分けて捨てています（`GetMouseMessageSource`）。なので、この動きは本物のマウス（`SDL_GLOBAL_MOUSE_ID`）として届き、最後に触った部品がホバーの見た目になります。この PC で記録を取ると、こうした動きは次のときに来ました。指を置いてから離すまでのあいだか離した直後、窓の大きさを変えたあと、最小化から戻したあと、カーソルの下で窓を動かしたあと、そして、時刻が重なるだけですが、ほかのプロセスが始まったり終わったりしたあとです。ドラッグの途中では、指がもう通り過ぎた点への動きも来ました。どれもカーソルの位置、つまり最後に指があった点でした。SDL は位置の変わらない動きを捨てますが、これらは通ります。SDL が指から作る位置には画素の端数があり、カーソルにはないからです。また SDL は、マウスが窓に入り直すと覚えていた位置を忘れます。どこから来たかでは本物の動きと見分けられません。どこへ来たかでは見分けられます。本物のマウスが動けば指の点を離れますが、こだまは離れません。

- 許す幅が 1 画素なのは、こだまが、端数のある指の点をカーソルの整数の画素に直した位置だからです。二つは縦横どちらも 1 画素未満しか違いません（記録では 296.002 と 296）。
- 時間では決めません。指のあとの一定の時間だけ止めるやり方では、指のすぐあとに動かした本物のマウスを止めてしまい、戻したあとや大きさを変えたあとのように遅れて来るこだまは通してしまいます。
- 引き換え：指の点から 1 画素未満だけ動いた本物のマウスや、ちょうどその点に戻った本物のマウスは、その一回の動きが届きません。次の動きは届きます。指が触れているあいだに動かした本物のマウスは、指が離れるまで無視します。
- 扱えないもの：タイトルバーのシステムのボタン（最大化）へのタップは、指ではなく、クライアント領域の外のマウスのメッセージとして届きます。なので、その点は覚えません。

ほかとの比べ（一般に知られていることで、今回は読んでいません）：

- ブラウザーはタップのあと、指の点で互換のマウスのイベントを出します。`:hover` は、ほかのところに触るまでタップした要素に残ります。メディアクエリの `(hover: none)` があるのは、これも理由の一つです。lamina-ui はここをわざと変えています。指が離れるとホバーを終え（`leaves_after_up`）、そのまま終えておきます。
- Android の Compose は、ホバーをマウスかスタイラスからしか受けません（ホバーの `MotionEvent`）。なので、指のあとにホバーは残りません。この決まりで、Windows でも同じ結果になります。Windows のデスクトップ版の Compose は見ていません。
- Windows の Chromium は、指のあとしばらくマウスのメッセージを捨てる、と記憶しています。今回は確かめていません。

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
- 横に送れるタブの列（C の `TabRow.kt`。androidx-main を 2026-11-22 に読みました。`PrimaryScrollableTabRow`・`SecondaryScrollableTabRow`。`TabRowOptions::scrollable`）：各タブの幅は少なくとも 90 dp（`TabRowDefaults.ScrollableTabRowMinTabWidth`）で、最初のタブの前と最後のタブの後に 52 dp をあけます（`ScrollableTabRowEdgeStartPadding`。`ScrollableTabRow` の配置が両端にあけます）。入りきるタブは引き伸ばしません。タブを選ぶと、そのタブが列の中央に来るように、両端で止めつつ送ります（`ScrollableTabData.calculateTabOffset`）。送る動きは、Compose の `scrollAnimationSpec` と同じく標準の空間のばねです。ホイールの 1 ノッチの量（64 dp、`scroll_area_tokens.wheel_step`）は lamina-ui の値です。Compose の列は `horizontalScroll` で送ります。指でなぞると、スクロールする範囲と同じく列が動き、はじくと滑ります。遊びを越えてなぞると、なぞり始めたタブの押下は取り消されます（[指でのスクロール](#指でのスクロール)）。

### ナビゲーションバー

C の `tokens/NavigationBarTokens.kt`（64 dp・surface-container・色・Label M）、`tokens/NavigationBarVerticalItemTokens.kt`（印 56 × 32 dp・アイコン 24 dp・項目の余白 6 dp）と C の `ShortNavigationBar.kt` と `NavigationItem.kt` を `lamina/tokens/navigation_bar.hpp` に写しました。lamina-ui にあるのは、項目を同じ幅に並べてアイコンをラベルの上に置く、短いナビゲーションバーです。

- 印は項目の上端から 6 dp 下にあり、ラベルは印の 4 dp 下にあります（`TopIconIndicatorToLabelPadding`）。
- 印の幅は 56 dp に進み具合を掛けたもので、進み具合は標準の空間のばねで動きます。そのため印は中央から広がり、少し行き過ぎます。不透明度も同じ進み具合で、1 を超えないよう切り詰めます。
- 状態の重ねは 56 × 32 dp の印の形の全体で、ポインターが項目のどこにあっても出ます。色は on-surface（バーの文字の色）です。
- 入れ物に高さはありません。`ShortNavigationBar` は `ContainerElevation`（Level2）を使いません。
- Compose はアイコンとラベルの色を一度に変えますが、lamina-ui は標準の効果のばねで移します。
- 名前を隠すのは lamina-ui が足したものです。バーは標準の空間のばねで、印とその余白の高さ（少なくとも 48 dp。触れる部品の最小の大きさ）まで縮みます。名前の不透明度は、名前の進み具合 L に対して (2 (L − 0.5))² です。

### ナビゲーションレール

C の `tokens/NavigationRailCollapsedTokens.kt`（96 dp・上の 44 dp・項目の間 4 dp・surface）、C の `tokens/NavigationRailExpandedTokens.kt`（220 から 360 dp）、C の `tokens/NavigationRailBaselineItemTokens.kt`（項目 64 dp・アイコン 24 dp・見出しの部品の下 40 dp）、C の `tokens/NavigationRailVerticalItemTokens.kt`（印 56 × 32 dp・名前まで 4 dp・Label M）、C の `tokens/NavigationRailHorizontalItemTokens.kt`（高さ 56 dp の印・前の余白 16 dp・名前まで 8 dp・Label L）、C の `tokens/NavigationRailColorTokens.kt`、C の `WideNavigationRail.kt` と C の `NavigationItem.kt`（`AnimatedNavigationItem`・`AnimatedMeasurePolicy`）を `lamina/tokens/navigation_rail.hpp` に写しました。lamina-ui にあるのは、上から並べる既定の配置の、手前に重ならない `WideNavigationRail` です。

- 畳んだ状態と広げた状態は、同じばねの組の二つの行き先です。どれも標準の空間のばねで動きます：レールの最小の幅（96 から 220 dp）、全範囲の幅（96 から 360 dp）、項目の間（4 から 0 dp）、項目の最小の高さ（64 から 48 dp。触れる部品の最小の大きさ）、各項目のアイコンの位置の進み具合。幅は `WideNavigationRailLayout` の測り方に従います。広げたレールは、動いている最小の幅になります。いちばん広い項目か見出しの部品が 220 dp より広ければ、その幅（動いている全範囲の幅まで）になります。
- 項目はレールの始めの側に置き、印の前に 20 dp（`WNRItemHorizontalPadding`。.kt の中の定数）、アイコンが上にある間は印の後ろにも 20 dp を空けます（アイコンが前に移ると 0 dp。この余白は Compose と同じく一度に切り替わります）。最小の高さより小さい中身は上下の中央に置くので、畳んだときの印は項目の上端から 6 dp 下になります。
- 印のアイコンの周りの余白は、進み具合につれて 16 × 4 dp から 16 × 16 dp に変わり、大きさはアイコン（24 × 24 dp）から「アイコン・8 dp・名前」に変わります。上にあるときは 56 × 32 dp、前にあるときは高さ 56 dp で、角はいつも丸め切ります。項目を選ぶと、印の幅は標準の空間のばねで 0 から始めの側の端から広がります（`placeRelativeWithLayer(startPadding, …)`）。ナビゲーションバーのように中央からではありません。不透明度も同じ進み具合です。
- 名前は、進み具合が 0.5 を過ぎたところで Label M から Label L に、上の位置から前の位置に替わります。不透明度は 4 (p − 0.5)² です。Compose は項目が最初に現れたときにアイコンが上かどうかを覚えるので、広げた状態で始まったレールは畳んでも Label L のままです。lamina-ui はいつも進み具合だけで決めます。
- 色：印は secondary-container、選んだアイコンは on-secondary-container、選んだ名前は上にあるとき secondary・前にあるとき on-secondary-container（.kt はここで `ItemActiveIcon` を使い回し、足りないトークンについての `TODO` があります）、選んでいないものは on-surface-variant です。Compose は名前の色を標準の効果のばねで動かし、アイコンの色は一度に変えます。lamina-ui は両方をそのばねで動かします。
- 状態の重ねは、全幅の印の形の全体で、ポインターが項目のどこにあっても出ます。色は on-surface です。波紋はレールの文字の色を取るので、`NavigationRailColorTokens` の状態の重ねの値（on-secondary-container）は使われません。
- 見出しの部品は中身の領域の左上に置き、項目はその 40 dp 下から始めます（`HeaderSpaceMinimum`）。
- 名前を隠すのは lamina-ui が足したもので、Compose にはありません。畳んだレールは幅 80 dp（狭い `NavigationRail` の幅で、印は中央）に、項目は高さ 56 dp に狭まります。どちらも 64 dp の項目と同じく密度で下がります。名前の不透明度は、名前の進み具合 L に対して (2 (L − 0.5))² で、半分で消えます。
- まだないもの：中央と下の配置、最初から名前のない項目、幅 80 dp の狭い `NavigationRail` そのもの。手前に重なるレール（`ModalWideNavigationRail`）は下のモーダルな幅の広いナビゲーションレールの節にあります。

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
- 止まり方：Compose は `onPostFling`（はじいたあとか、バー自身を引きずったあと）で止め方を決めます。1 % 未満か 100 % ならそのまま、それ以外は近いほうの端へ `snapAnimationSpec`（標準の効果のばね）で動きます。lamina-ui は、スクロールする範囲が止まったとき（目標が残っておらず、つまみも指も引きずっておらず、はじいた動きも終わったとき。`ScrollArea::scroll_in_progress`）に止め方を決め、中身もバーと一緒に動かします。
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
- 幅：どちらの行も `Row(modifier.defaultMinSize(minHeight = ContainerHeight).width(IntrinsicSize.Min), horizontalArrangement = Arrangement.spacedBy(-space))` で、区画の `Surface` は `.weight(1f)` のあとに `.defaultMinSize(minWidth = ButtonDefaults.MinWidth, minHeight = ButtonDefaults.MinHeight)` が付きます。行が本来の幅より狭いとき（たとえば狭い親の中で `fillMaxWidth()` を付けたとき）、重みのある区画はちょうどの取り分の幅で測られ、`defaultMinSize` は最小が 0 のときにしか効かないので、Compose の区画には縮む下限がありません。`SegmentedButtonContentMeasurePolicy` はアイコンと文字を同じ制約で測り、文字の前に必ず max(18 dp, アイコン) + 8 dp を数えます。文字は呼び出し側の部品で、このファイルは `maxLines` も `overflow` も付けないので、既定のままの `Text`（折り返しあり・行数の上限なし）は狭い区画で複数行に折り返します。lamina-ui は文字を 1 行のまま「…」（`tokens::ellipsis`）で切り、同じ幅の区画を `min_width()` で止めます。いちばん幅の要る区画について 2 × 12 dp + 18 + 8 dp + 文字と「…」の狭いほう（アイコンだけの区画は 18 dp）で、58 dp 以上です。`fill_width` がなければ行は本来の幅より広くならず、あれば `fillMaxWidth()` と同じく幅いっぱいに広がります。
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
- 幅：`TimePickerDefaults.layoutType()` は `defaultTimePickerLayoutType` を返し、その KDoc は「画面の大きさを使う」と書いています。`TimePickerLayoutType.Horizontal` は「横長の向きで使う」ものです。`ClockFaceSizeModifier` は文字盤の大きさを渡された高さから決め（`TimePickerMaxHeight` の 384 dp 以上なら 256 dp、`TimePickerMidHeight` の 330 dp 以上なら `ClockDialMidContainerSize` の 238 dp、それより低ければ `ClockDialMinContainerSize` の 200 dp）、幅からは決めないので、横の配置は狭くなりません。lamina-ui の横の時刻の選択はどの幅でも 508 dp で、文字盤はどの高さでも 256 dp のままです。部品の一覧は、窓のほうが狭いとき、カードの中で横にスクロールさせます（[見本](examples.ja.md#components部品の一覧)）。
- 動き：つまみは近い向きを回って `DefaultSpatial` で動きます。押したときは分を 5 分に、時をいちばん近い時に合わせ、時を押したときは動きが終わってから 100 ms 待って分を出します。ドラッグではつまみが指に付き、離したときに切り替えます。数字の組は `DefaultEffects` で入れ替わります。
- 入力：欄は 96 × 72 dp で Display M、フォーカスがあると primary-container の地に 2 dp の枠線（outline）。下 7 dp に Body S で 2 行の補助の文字："Hour" と "Minute"、範囲外なら "Hour must be 1–12"・"Hour must be 0–23"・"Minute must be 0–59" をエラーの色で出します。埋まった欄に打つと新しい数字だけが残り、時を 2 桁（12 時間制なら 2〜9 の 1 桁）打つと分へ進みます。
- ダイアログ：内側に 24 dp、中身の 20 dp 上に Label M の題（"Select Time" / "Enter Time"）、ボタンの行は先頭にモードの切り替え（`Keyboard` / `Schedule`）、末尾に Cancel と OK を 8 dp 空けて置きます。
- 読み上げ：時と分の箱は "Select hour" と "Select minutes" のラジオボタン、文字盤の数字は "%1$d o'clock"・"%1$d hours"（24 時間制）・"%1$d minutes"、午前・午後の切り替えは "Select AM or PM"、入力欄は "for hour" と "for minutes" です。AM と PM は文字列の資源で、日本語でも "AM" と "PM" です。
- 違い：24 時間制の Compose のドラッグは、動くたびにつまみを時の位置に戻すので、つまみが 1 時間ずつ跳びます。lamina-ui はそこでも指に付いて動かします。Compose は 12 時間制で 0 と 00 を時として受けますが、文は 1〜12 と言っています。lamina-ui はこれを無効にしました。キーでは、時と分の箱、午前・午後の切り替え（止まり位置一つ）、文字盤（フォーカスのある数字を持つ止まり位置一つ）を Tab で巡ります。Compose の文字盤の数字は、箱を押したあとでないとフォーカスを受けません。入力欄は文字入力の部品ではなく、lamina-ui の 2 桁の欄です。ダイアログはモードの間を速い効果のばねで入れ替えます。Compose の時刻の選択には無効の状態がなく、lamina-ui は 38 % に薄めます。Compose がすぐに切り替える色は、ばねで移しています。

### 入力欄：補助の文字とエラー

C の `OutlinedTextField` の既定：補助の文字は Body S で、上に 4 dp（`SupportingTopPadding`）、始まりから 16 dp。色は on-surface-variant、`isError` ならエラーの色です。`isError` では枠線・ラベル・カーソルもエラーの色、ポインターを乗せた枠線は on-error-container。lamina-ui は標準の効果のばねで移します。

### ドラッグの操作

2026-11-21 に `6068a9492ba8` の androidx の foundation の `gestures/AnchoredDraggable.kt` と ui の `input/pointer/util/VelocityTracker.kt`・`PlatformVelocityTracker.kt` を読み、`lamina/motion/velocity_tracker.hpp`・`lamina/motion/anchored_draggable.hpp`・`lamina/tokens/drag.hpp` に入れました。

- 速さ：デスクトップの `PlatformVelocityTracker` が使う、`VelocityTracker1D` の `Lsq2` の方式です。使うのは直近の 20 個のサンプル（`HistorySize`）で、いちばん新しいものの 100 ms 前（`HorizonMilliseconds`）までさかのぼり、40 ms を超える間（`AssumePointerMoveStoppedMilliseconds`）があればそこで止めます。サンプルは 2 個以上要ります。時刻に対する位置を 2 次の最小二乗で当てはめ、いちばん新しいサンプルでの傾きを速さにします。最後の動きから 40 ms より後に離したときは、動きを忘れます（b/238654963 の修正）。時刻は `PointerEvent::time_seconds` が運び、SDL の窓は出来事の時刻からこれを埋めます。
- 行き先：`computeTarget` そのままです。速さがなければいちばん近いアンカーです。速さのしきい値以上なら、はじいた向きの次のアンカーです。それ未満なら、後ろのアンカーから位置のしきい値だけ離れたときは次のアンカー、そうでなければ後ろのアンカーです。ドラッグの間、位置は外側のアンカーの間に収めます。
- lamina-ui が決めたこと：ドラッグは軸に沿って 8 dp を超えて動いてから始まります（Android のタッチの遊び。S の `config_viewConfigurationTouchSlop`。[指でのスクロール](#指でのスクロール)を見てください。Compose のデスクトップの値は読んでいません）。遊びを越えた分は、Compose の `overSlop` と同じくすぐに位置を動かします。離したときの速さは、Android のはじく速さの上限の 8000 dp/s までに抑えます（S の `config_viewMaxFlingVelocity`）。ここでの落ち着き方には Compose の減速の段階（`exponentialDecay`）を使わず、いつもばね一つかトゥイーン一つです。[指でのスクロール](#指でのスクロール)の減速の曲線を使うのは、スクロールする範囲と横に送れるタブの列だけです。
- 子が先：シートやドロワーの中の部品が先に押下を受けます。入れ物が操作を引き取るのは、遊びを越えて子がその動きを使ったと知らせなかったときだけで、そのあと子の押下を取り消します。Compose はポインターの出来事の消費で同じ順を決めます。

### 指でのスクロール

2026-11-22 に読みました。`6068a9492ba8` の androidx（この節の C はこのコミットで、`material3` の外のファイルです）の animation の `SplineBasedDecay.kt`（`AndroidFlingSpline`）・`FlingCalculator.kt`・`SplineBasedFloatDecayAnimationSpec.kt` と `androidMain` の `SplineBasedFloatDecayAnimationSpec.android.kt`、foundation の `gestures/Draggable.kt`・`gestures/DragGestureDetector.kt`・`gestures/AbstractScrollableNode.kt`・`gestures/Scrollable.kt` と `androidMain` の `gestures/Scrollable.android.kt`、ui の `input/pointer/util/VelocityTracker.kt` と `androidMain` の `platform/AndroidViewConfiguration.android.kt` です。Android 自身の値は S から読みました。入れた先は `lamina/tokens/drag.hpp`（`touch_scroll`・`fling_decay`）、`lamina/motion/spline_decay.hpp`（`SplineDecay`・`Fling`）、`lamina/widgets/touch_scroll.hpp`（`TouchScroll`）と `ScrollArea::route` です。

| 値 | lamina-ui | 出どころ |
|---|---|---|
| タッチの遊び | 8 dp（`touch_slop`） | S の `config.xml` の `config_viewConfigurationTouchSlop` = 8dp。`ViewConfiguration` がこれを読んで `getScaledTouchSlop()` で返します。`ViewConfiguration.java` の `TOUCH_SLOP = 8`。C の `AndroidViewConfiguration.touchSlop` は `scaledTouchSlop` を返します。 |
| はじく速さの上限 | 8000 dp/s（`maximum_fling_velocity`） | S の `config_viewMaxFlingVelocity` = 8000dp と `MAXIMUM_FLING_VELOCITY = 8000`。C の `AndroidViewConfiguration.maximumFlingVelocity` |
| スクロールの摩擦 | 0.015（`fling_decay.friction`） | S の `config_scrollFriction` = 0.015 と `SCROLL_FRICTION = 0.015f`。`getScrollFriction()` が返します。C の `platformFlingScrollFriction = ViewConfiguration.getScrollFriction()` |
| 曲線 | 曲がり目 0.35、始めの張り 0.5、終わりの張り 1.0、標本 100 個、二分法で 1e-5 まで | C の `SplineBasedDecay.kt`：`Inflection`・`StartTension`・`EndTension`・`P1 = StartTension * Inflection`・`P2 = 1 - EndTension * (1 - Inflection)`・`NbSamples = 100`・`computeSplineInfo`（`abs(tx - alpha) < 1E-5`）・`flingPosition`・`deceleration = ln(Inflection * abs(velocity) / friction)` |
| 減速 | 9.80665 m/s²、1 m あたり 39.37 インチ、1 インチあたり 160 dp、0.84。率は ln 0.78 / ln 0.9 | C の `FlingCalculator.kt`：`GravityEarth`・`InchesPerMeter`・`computeDeceleration(0.84f, density)` = `GravityEarth * InchesPerMeter * density * 160f * friction`・`DecelerationRate`・`flingDuration`・`flingDistance`・`FlingInfo.position` と `velocity` |
| はじき始め | 1 dp/s より速いとき（`touch_scroll.minimum_fling_velocity`） | C の `AbstractScrollableNode.kt` の `DefaultFlingBehavior.performFling`：`if (abs(initialVelocity) > 1f)`。単位は px/s で、lamina-ui は dp/s として読みます（密度 1 なら同じです） |
| 端で止まる | 端を 0.5 dp より越えるとき（`touch_scroll.edge_tolerance`） | C の同じ所：`if (abs(delta - consumed) > 0.5f) this.cancelAnimation()`。単位は px で、lamina-ui は dp として読みます |
| 向きの決まり方 | 30° 以下は横、30° を超えて 90° までは縦（`horizontal_angle_limit_degrees`・`vertical_angle_limit_degrees`） | C の `Draggable.kt`：`HorizontalAngleUpperBounds = 30`・`VerticalAngleUpperBounds = 90`・`isDragAngleAlignedWithOrientation`。角度は `atan2(abs(y), abs(x))` を度にしたもの |
| 速さの測り方 | [ドラッグの操作](#ドラッグの操作)と同じ | C の `VelocityTracker.kt`：`HistorySize = 20`・`HorizonMilliseconds = 100`・`AssumePointerMoveStoppedMilliseconds = 40`・`MinSampleSize = 2` |

- ポインターの種類：中身を引きずって動かすのは `PointerKind::touch` だけです。SDL の窓は、指から SDL が作るマウスの出来事（`SDL_TOUCH_MOUSEID`）を指として渡すので、指の出来事を別に読むことはしません。マウスはこれまでどおり、ホイール・バー・部品自身のドラッグ（文字の選択・スライダー・つまみ）に使えます。指はバーに乗らず、バーをつかみません。指を離すと、窓はどの OS でも、離した出来事のあとに外へ出た出来事を足します（`input::leaves_after_up`）。なので、タップした部品にホバーの色は残りません。マウスとペンはホバーを保ちます。タイトルバーから来る出来事は種類を持たず、マウスとして扱われます。
- 遊び：片方の向きにだけスクロールするものは、その軸に沿った動きだけを比べます（向きがあるときの `TouchSlopDetector.mainAxis()`）。両方の向きにスクロールするものは距離で比べます（向きが `null`）。遊びを越えた分はすぐに中身を動かします（`overSlop`）。lamina-ui は、自分の `AnchoredDraggable` と同じく 8 dp を超えたら始めます。Compose の `hasCrossedSlop` は「以上」です。
- 子が先：スクロールするものは、自分の遊びを調べる前に、動きをまず中身に渡します。Compose で Main の段階が先に子へ届くのと同じ順です。押下を持っている部品があれば、その部品が操作を続けます（`holds_pointer()`。`WidgetSet` が中の部品から集めます）。スライダーと範囲スライダー、分割の境目とペインの境目のつまみ、時刻の文字盤、値をなぞって変えている数値欄、ドラッグを引き取った内側のスクロールやタブの列、引きずられているシート・ドロワー・レール、開いているメニュー・ダイアログ・モーダルなシートがそうです。そうでなければ、遊びを越えたところでスクロールするものが操作を引き取り、中身には離れる出来事が届きます。これで中身の押下とポインターを乗せた状態が消えるので、指を離してもクリックになりません。
- 入れ子：遊びを待っているスクロールするものは、中身に渡す出来事の `PointerEvent::waiting_scrollers` に自分の軸を記します。内側のスクロールするものは、遊びを越えても、なぞった角度が自分の軸に合わず、待っている外側の軸に合うときは操作を手放します。Compose が `traverseParentDraggableGestures` で調べるのと同じです。縦のページの中の横のスクロールの上で 40° になぞるとページが動き、20° なら内側が動きます。
- なぞっている間、中身は指と同じだけ動きます。指を離すと、軸ごとの速さを速さの測り方から取って 8000 dp/s までに抑え、減速の曲線ではじきます。lamina-ui はこれを密度 1 の dp で計算します。密度は `computeDeceleration` と px の速さの両方に入って打ち消し合うので、px での距離を密度で割ると dp での距離になり、時間は同じです。
- はじいている間に指を置くと、動きが止まり、遊びを待たずにすぐ引きずれます。その押下は中身に届かないので、タップにはなりません。Compose では `shouldScrollImmediately()` が `isScrollInProgress` を返し、そのとき `startDragImmediately` が true になり、`DragGestureNode.processInitialDownState` が押下を消費して（`firstDown.consume()`）ドラッグを始めます。
- 端：中身は端で止まります。Compose の端で伸びる動き（`OverscrollEffect`）は作っていません。
- アプリバー：`ScrollArea::scroll_in_progress()` は指でのドラッグとはじいた動きの間も true で、トップアプリバーとフレキシブルなボトムアプリバーは、それが終わってから止め方を決めます（Compose は `onPostFling` で決めます）。
- シートの中身：ボトムシートの中身を指でなぞると、なぞり始めにその向きへ中身がスクロールできるときは中身が先に動きます。下向きは中身が一番上にないとき、上向きはシートが Expanded の位置にあって中身が一番下にないときです。そうでなければシートが動きます。Compose は操作の途中でも、入れ子のスクロールで残りの量を中身とシートの間で受け渡しますが、lamina-ui は一回の操作ごとに一度だけ決めます。
- 入力欄（lamina-ui が決めたこと）：指では、押したときではなく、欄の中で指を離したとき（タップ）に入力欄へ焦点が移ります。そのため欄の上から始めたスクロールで焦点が移ることはなく、ほかの場所のタップは指を離したときに焦点を外します。マウスはこれまでどおり、押したときに焦点を移して選択を始めます。
- 作っていないもの：端で伸びる動き。入れ子のスクロール（ページの中のリストが端に着いたとき、なぞりやはじきの残りをページに渡すこと。拡大の表示は、ついていけないなぞりを遊びのところで丸ごとスクロールやページ送りへ渡します。[拡大の表示](#拡大の表示)）。Android の仕組みの速さの測り方（Compose は `AndroidComposeUiFlags.isFrameworkVelocityTrackerEnabled` のときに使います。その既定値は読んでいません。lamina-ui は Lsq2 のままです）。スクロールの領域での二本目からの指（SDL がマウスに写すのは最初の指だけです。拡大の表示のために窓はすべての指を `Frame::fingers` で知らせるようになりましたが、スクロールの領域はそれを読みません）。Android のはじく速さの下限（S の `config_viewMinFlingVelocity` = 50dp）。Compose のスクロールもこれは使いません。データ表は押したときに行を選ぶので、行の上から始めた指のなぞりはその行を選びます。

### 拡大の表示

2026-11-23 に S の `ScaleGestureDetector.java`（`onTouchEvent`・コンストラクター）、`GestureDetector.java`（`onTouchEvent`・`isConsideredDoubleTap`・`cancelTaps`）、`ViewConfiguration.java`、`config.xml` を読みました。androidx（Compose の `transformable`・`detectTransformGestures`）と material-components-android のソースはこの機械に無かったので、読んでいません。二つ目の表の値は lamina-ui の値です。入れた先は `lamina/tokens/zoom.hpp`（`zoom`）、`lamina/motion/zoom.hpp`（計算）、`lamina/widgets/zoom_view.hpp`（`ZoomView`）です。

| 値 | lamina-ui | 出典 |
|---|---|---|
| 指の間隔の遊び | 16 dp（`span_slop`） | S の `ScaleGestureDetector(Context, ...)` が `spanSlop` に `ViewConfiguration.getScaledTouchSlop() * 2` を渡します。タッチの遊びは 8 dp です（[指でのスクロール](#指でのスクロール)） |
| 最小の間隔 | 27 mm、170.08 dp（`min_span`） | S の `config.xml` の `config_minScalingSpan` = 27mm を `getScaledMinimumScalingSpan()` が読みます。Android は画面の本当の密度でミリメートルを画素に直します。lamina-ui は 1 インチを 160 dp として直すので、1 dp がちょうど 1/160 インチでない画面では少しずれます |
| 間隔 | 指の重心からの距離の平均を軸ごとに 2 倍し、`hypot` で合わせたもの | S の `onTouchEvent`：`devSumX / div`、`spanX = devX * 2`、`span = Math.hypot(spanX, spanY)` |
| 拡大の始まりと終わり | 間隔が最小以上で、指の本数が変わってから遊びより大きく変わったら始まり、最小を下回ったら終わります。拡大の途中で指が増えたり減ったりしたときは、間隔が最小以上ならすぐにまた始まります | S の `onTouchEvent`：`span >= minSpan && (wasInProgress || Math.abs(span - mInitialSpan) > mSpanSlop)`、`span < mMinSpan`、`configChanged` |
| 一回の倍率 | 動くたびに、今の間隔を前の間隔で割った分を掛けます | S の `getScaleFactor()` = `mCurrSpan / mPrevSpan` |
| 二度たたき | 二回目の押し下げが一回目を離してから 40〜300 ms、一回目の押し下げから 100 dp 未満、一回目の指の動きが 8 dp 以内。二本目の指が来たら取り消します。拡大は二回目の押し下げで、一回目の位置を中心に始まります | S の `config_doubleTapTimeoutMillis` = 300、`config_doubleTapMinTimeMillis` = 40、`ViewConfiguration.DOUBLE_TAP_SLOP = 100`、`DOUBLE_TAP_TOUCH_SLOP = TOUCH_SLOP`。`GestureDetector.isConsideredDoubleTap`、`mAlwaysInBiggerTapRegion`、`ACTION_POINTER_DOWN` での `cancelTaps`、二回目の `ACTION_DOWN` での `onDoubleTap(mCurrentDownEvent)` |
| 動かすときの遊び・はじく・端 | 8 dp。Lsq2 の速さを 8000 dp/s で抑え、スプラインで減速し、1 dp/s を超えたらはじき、端を 0.5 dp より越えたら止めます | [指でのスクロール](#指でのスクロール)と同じです |
| 一度たたき | 二度たたきの一回目になりうるたたきは、二度たたきの待ち時間 300 ms のうちに二回目の押し下げが来なかったときにだけ確かめます。lamina-ui はこの時間を、二度たたきと同じく指を離したときから数えます。Android は一回目の押し下げから数え、そのとき指がまだ触れていれば離したときに確かめます | S の `GestureDetector.onTouchEvent`：`ACTION_DOWN` で一回目のたたきは `TAP` を `mDoubleTapTimeout` だけ遅らせて送り（`:666`）、二回目の押し下げはそれを取り除きます（`:651-652`）。`TAP` は `onSingleTapConfirmed` を呼び、指が触れている間は離すときまで延ばします（`:333-341`・`:796-797`）。2026-11-24 に読みました |

出典の無い lamina-ui の値：

| 値 | lamina-ui |
|---|---|
| 倍率の範囲 | 1（収まる大きさ）〜5（`min_scale`・`max_scale`） |
| 二度たたいたときの倍率 | 2.5（`double_tap_scale`）。拡大しているときに二度たたくと収まる大きさに戻ります |
| 拡大している | 最小の倍率より 0.001 を超えて大きいとき（`zoomed_epsilon`） |
| Ctrl とホイール | 一段でカーソルを中心に 1.25 倍（`wheel_zoom_factor`） |
| ダブルクリックの待ち時間 | 500 ms（`double_click_timeout_seconds`）。マウスの一回のクリックを確かめるまで待つ時間です。Windows のダブルクリックの既定の時間で、記憶によるもので、読んではいません。SDL はシステムの時間でダブルクリックを数えるので、これより早く確かめると、そのクリックがあとでダブルクリックになることがあります |
| 動かすときのゴムの手応え | `extent × (1 − 1 / (overshoot × 0.55 / extent + 1))`。`extent` は表示の半分（`pan_band_coefficient`・`pan_band_extent`）。UIKit がこの形を使うとよく説明されていますが、記憶によるもので、読んではいません |
| 倍率のゴムの手応え | 同じ曲線を倍率の対数にかけたもの。限りを越えるのは最大で 1.5 倍まで（`scale_band_coefficient`・`scale_band_ratio`） |
| 戻る動き | 既定の効果のばね（減衰 1.0）。道のりの 0.0001 以内・毎秒 0.001 以内で止まったとみなします（`settle_spring`・`settle_threshold`）。倍率と位置は同じ割合で進みます。中心を保つ位置は倍率に対して一次なので、これで中心がずれません |

- 収まる大きさとは、表示の中に収まるように縮めて中央にそろえた大きさです。拡大した中身が表示より小さい軸では中央にそろったままで、大きい軸では中身の端が表示の内側に入りません。
- 倍率 1 のときの一本の指は動かさず、押さえもしません。そのため周りのスクロールやページ送りがなぞりを受け取れます。二本の指は、倍率 1 でもいつも押さえます。
- なぞりを渡す：拡大した中身の上の一本の指も、8 dp の遊びを越えるまでは押さえません。遊びを越えたところで、なぞりが水平から 30° 以内（`handoff_angle_degrees`。スクロールが Compose の `Draggable` から取った角度。[指でのスクロール](#指でのスクロール)）か垂直から 30° 以内で、その軸でスクロールが待っていて（`waiting_scrollers`）、中身がその向きに動けない（`can_pan_horizontally`・`can_pan_vertically`。端の許し 0.5 dp 以内）ときは手放します。その間の斜めのなぞりはいつも動かします。スクロールやページ送りがなぞりを取るまでは、二本目の指で操作が表示に戻るので、二本の指はいつも表示のものです。lamina-ui の決まりです。Android の `ViewPager` は自分の遊びのところで、指の下のどれかのビューがその向きにスクロールできるかを尋ねて決めます（`canScroll`・`canScrollHorizontally`。S の `ViewPager.java`）。ページ送りの中の拡大の表示では結果は同じです。lamina-ui がページ送りではなく表示に尋ねるのは、ページ送りがページの上の表示を知らないからです。
- この判断はなぞり一回につき一度です。端に着いてもなぞり続けると、ゴムの手応えの中へ動き続け、次のなぞりがページ送りへ渡ります。Compose は入れ子のスクロールの間で、なぞりの途中に残りの量を渡します（記憶によるもので、ここでは読んでいません）。
- 指が触れている間の離脱は操作を手放します。ただし、タッチのポインターを離したあとに続く離脱は除き、残りの指は続きます。外側のスクロールの領域やページ送りが、自分のはじきや落ち着く動きを止めたためにすぐ取ったタッチも、これに当たります。指の出来事はポインターの出来事より先に届くので、表示はその指をもう受け入れています。以前は、表示がそのタッチ自身のポインターの押し下げを見たあとの離脱でしか手放さず、そういう中身は外側のスクロールと一緒に動いていました。
- まだ押さえている指がもう一度下りたときは、先に古いほうを捨てます。Windows では次のタッチに同じ指の番号が戻ってくるので、離した出来事を受け取れなかった表示（ページが渡さなかった場合）は、そうしないと次のタッチを無視します。
- 二度たたきは、Android の `GestureDetector` と同じく二回目の押し下げで起こります。Android のクイックスケール（二度たたいてなぞる、`ANCHORED_SCALE_MODE_DOUBLE_TAP`）は作っていません。マウスのダブルクリックは、これらの値ではなくシステムのクリックの回数を使います。
- 一度たたきは、待ち時間が過ぎたときに `step` で確かめます。数えるのは、指を離した出来事を運んだフレームより後のフレームだけです（そのフレームの時間はたたく前に始まっているからです）。次の押し下げの時刻から待ち時間より後だったと分かれば、そこでも確かめます。確かめたたたきは二度たたきの一回目を消すので、二つが両方起こることはありません。
- 作っていないもの：回転、クイックスケール、キーでの拡大、マウスでのドラッグのあとのはじき、端を越えたときの伸び。

### ページ送り

2026-11-23 に S の `com/android/internal/widget/ViewPager.java`（サポートライブラリーの `ViewPager` をプラットフォームが内部に写したもの。`onInterceptTouchEvent`・`onTouchEvent`・`determineTargetPage`・`smoothScrollTo`・`canScroll`）と `ViewConfiguration.java` を読みました。androidx の基盤のソース（`HorizontalPager`・`PagerState`・`PagerDefaults`・スナップのはじきの振る舞い）はこの機械になく（Gradle のメタデータだけで jar はありません）、Compose のページ送りの値は一つも読んでいません。値は `lamina/tokens/pager.hpp`（`pager`）、`lamina/motion/paging.hpp`（`snap_page`・`pages_in_view`・`pages_around`）、`lamina/widgets/pager.hpp`（`Pager`）に入れました。

| 値 | lamina-ui | 出典 |
|---|---|---|
| ページ送りの遊び | 16 dp（`touch_slop`） | S の `ViewConfiguration.java` の `PAGING_TOUCH_SLOP = TOUCH_SLOP * 2`。`ViewPager` は `getScaledPagingTouchSlop()` で読みます |
| 先に縦 | 横より先に上か下へ遊びを越えたなぞりは、その操作の終わりまでページに任せます | `onInterceptTouchEvent` の `else if (yDiff > mTouchSlop)` が `mIsUnableToDrag` を立てます |
| はじき | 押した所から 25 dp を超え、400 dp/s より速いとき。指が左へ動いていれば最初に見えているページの次へ、右へ動いていれば最初に見えているページへ | `MIN_DISTANCE_FOR_FLING = 25`、`MIN_FLING_VELOCITY = 400`、`determineTargetPage` の `targetPage = currentPage - (velocity < 0 ? mLeftIncr : 0)`。`mLeftIncr` は左から右の並びで −1 |
| はじかないとき | どちらの向きでもページの 60 %。止まったページ以上にいるときは `floor(position + 0.4)`、手前にいるときは `floor(position + 0.6)`（`snap.positional_threshold` 0.6） | `determineTargetPage` の `truncator = currentPage >= mCurItem ? 0.4f : 0.6f`、`targetPage = (int) (currentPage - mLeftIncr * (pageOffset + truncator))` |
| 落ち着く動きをつかむ | 向かうページから 2 dp より離れているときに触れると、止めてすぐなぞりに移ります。近いときは落ち着く動きを終わらせ、タッチは中身へ渡します（`catch_distance`） | `CLOSE_ENOUGH = 2`、`onInterceptTouchEvent` の ACTION_DOWN |
| 表示の周りのページ | 両側に一ページずつ（`beyond_viewport_pages`） | `DEFAULT_OFFSCREEN_PAGES = 1` |
| 入れ子の中身 | なぞりについていける中身がそれを持ちます | `canScroll`。lamina-ui では中身に先に尋ね、中身が押下を持つか、拡大の表示がなぞりを手放します（[拡大の表示](#拡大の表示)） |
| 速さ | Lsq2、8000 dp/s で頭打ち。離した点は標本に入れません | [指でのスクロール](#指でのスクロール)と同じ。ViewPager はプラットフォームの速さの計測と `getScaledMaximumFlingVelocity()` を使います |

ViewPager との違い：

- 向きの決め方：ViewPager は `xDiff * 0.5f > yDiff`（約 26.6° 未満）でなぞりを取ります。lamina-ui は、スクロールが Compose の `Draggable` から取った 30°（`horizontal_angle_limit_degrees`）を使います。こうすると、ページ送りを待っている縦のスクロールとページ送りが、どのなぞりを横とみなすかで食い違いません。
- 落ち着く動き：ViewPager は五次の補間（`sInterpolator`）の `Scroller` で最大 600 ms かけてスクロールします（`MAX_SETTLE_DURATION`・`smoothScrollTo`・`distanceInfluenceForSnapDuration`）。lamina-ui はほかのなぞりと同じくばねで落ち着きます。
- ViewPager の端の効果、縁からのなぞり（`DEFAULT_GUTTER_SIZE`）、ページの間の描画、ページの幅の割合は作っていません。

lamina-ui の値（出典なし）：

| 値 | lamina-ui |
|---|---|
| 落ち着くばね | 減衰 1.0、硬さ 400。離したときの速さ（ページ毎秒）を引き継ぎ、ページに着いたところで止まります（行き過ぎません）。そうでなければ 0.001 ページ・毎秒 0.01 ページ以内で止まったとみなします（`settle_spring`・`settle_threshold`） |
| `animate_to_page` | 一ページより離れたページへは、まず隣まで飛びます（`animate_jump_pages` 1）。見えるのは二ページまでです。遠いページについての Compose の決まりは読んでいません |
| ページの間 | 0 dp（`page_spacing`） |
| フォーカスの輪 | 表示の内側に、角を丸めずに描きます（`focus_ring_corner`） |
| マウス | 既定では指と同じくページを送ります（`PagerOptions::mouse_drag`）。見本では切ってあります |
| キー | キーボードのフォーカスがある間、左右の矢印で一ページ、`Home` と `End` で端へ。端で反対側へは戻りません |

- タブ：`TabRow::set_indicator_position` は印を小数のタブの番号に置きます。二つのタブの中心と幅の間を取り、ばねなしで置くので、指に遅れません。なぞっている間、見本はページ送りの `current_page()`（いちばん近いページ。半分のところで変わります）もタブの列で選び、その色はクリックのときと同じく移ります。タブをタップしたときは、タブ自身のばねはそのままで、ページ送りは `animate_to_page` で滑るので、選択が間のタブを通りません。この組み方は lamina-ui のものです。Compose のタブとページ送りの見本は読んでいません。
- ページを必要になってから作る：`pages_to_build()` は見えているページとその両隣です。Viewer のページはそこで写真ごとに `ZoomView` を作り、ほかを捨てます。ページ送りが止まったとき、止まったページでない写真は収まる大きさに戻します。
- 作っていないもの：なぞりの途中で渡すこと（なぞりの途中で端に着いた拡大の表示はそのなぞりを持ち続けます）、最初と最後のページで端を越えたときの伸び、ホイールでのページ送り、右から左の並び、縦のページ送り、表示より狭いページ。

### ボトムシート

`6068a9492ba8` の C の `SheetDefaults.kt`・`BottomSheet.kt`・`ModalBottomSheet.kt`・`BottomSheetScaffold.kt`・`Scrim.kt`・`ComposeMaterial3Flags.kt`・`tokens/SheetBottomTokens.kt`・`tokens/ScrimTokens.kt` と、`androidMain/res/values{,-ja,-en-rGB}/strings.xml` と ui の `close_sheet` の文字列を読み、`lamina/tokens/bottom_sheet.hpp`・`lamina/tokens/scrim.hpp`・`lamina/widgets/pattern_strings.cpp` に入れました。

- 入れ物は surface-container-low で、上の角は extra-large（28 dp。`ExpandedShape`）です。幅は 640 dp まで（`SheetMaxWidth`）で、中央に置きます。モーダルなシートに影はありません（`BottomSheet` の `shadowElevation` は 0、`ModalBottomSheet` の `tonalElevation` は 0）。`BottomSheetScaffold` のモーダルでないシートは段階 1 の影を持ちます（`sheetShadowElevation = Elevation`）。幕：scrim の役割の色の 32 %。
- ドラッグハンドル：32 × 4 dp・on-surface-variant・`shapes.extraLarge`（この大きさでは両端の丸い形）で、上下に 22 dp。Compose はこれを上に出る簡素なツールチップ（`DragHandleWithTooltip`）で包み、文字は "Drag handle" です。ハンドルをクリックしたとき、モーダルなシートは広げた状態なら隠して閉じます。半分開いた状態なら広げ、それ以外なら表示します。モーダルでないシートは広げた状態のとき、隠せるなら隠し、そうでなければ半分開いた状態にします。
- アンカー（モーダル）：隠れた状態は全体の高さです。半分開いた状態は飛ばさない限りあり、全体の高さから min(全体の高さの半分, シートの半分) を引いた所です。これは `isBottomSheetPartiallyExpandedDeterministicEnabled` の決まりで、このコミットでは既定で true です（非推奨の `rememberModalBottomSheetState` はこれを切り、半分より高いシートにだけ全体の高さの半分を使います）。広げた状態は max(0, 全体の高さ − シートの高さ) です。シートの高さはハンドルと中身を合わせた高さで、全体の高さまでです。それを超える分は中身がスクロールします。モーダルでないもの（`BottomSheetScaffold`）：半分開いた状態は全体の高さからのぞかせる高さ（既定は 56 dp）を引いた所で、二つが等しいときはありません。隠れた状態は隠すのを許すとき（`skip_hidden` が false）だけあり、広げた状態は全体の高さからシートの高さを引いた所です。アンカーが変わったときは、Compose の `draggableAnchors` のラムダと同じように行き先を動かします。
- 動き：表示と広げるのは標準の空間のばね（`showMotionSpec`）、隠すのと半分開いた状態にするのは速い効果のばね（`hideMotionSpec`）です。離したドラッグは、離したときの速さを持って標準の空間のばねで落ち着きます（`AnchoredDraggableDefaults.flingBehavior`）。しきい値：位置は 56 dp（`PositionalThreshold`。割合ではなく長さ）、速さは 125 dp/s です。モーダルなシートは、隠れた状態から 125 dp 以内で下へはじく速さを弱めます（`BoundaryDampeningZone`）。速さに「残りの距離 ÷ 125 dp」の 2 乗を掛け、しきい値以上ではじいたときはしきい値を下回らないようにします。幕の不透明度は、行き先が隠れた状態でない間は標準の効果のばねで 1 へ動きます。
- 行き過ぎ：Expressive の標準の空間のばねは、広げた状態のアンカーを行き過ぎます。Compose は入れ物を上から伸ばし（`verticalScaleUp`）、中身を縮め戻す（`verticalScaleDown`）ので、下にすき間ができません。lamina-ui は入れ物をシートの上端から広げたシートの下端まで描き、中身は拡大縮小しません。見た目は同じです。
- 閉じ方：戻る操作（`settleToDismiss`）は半分開いた状態を持つ広げたシートをまず半分開いた状態にし、そうでなければ隠します。幕のクリックはすぐに隠します（`animateToDismiss`）。隠れた状態に落ち着くはじきやドラッグでも閉じます。lamina-ui にはキーを離した出来事がないので、戻る操作は Escape を押したときに当てます。`BottomSheetResponse::dismissed` は閉じ始めたときに立ちます。Compose は隠す動きが終わってから `onDismissRequest` を呼びます。
- 読み上げ：モーダルなシートは "Bottom Sheet" という名前のモーダルな `dialog`（`BottomSheetPaneTitle`。en-GB では "Bottom sheet"、日本語ではボトムシート）で、モーダルでないシートは `group` です。ドラッグハンドルは "Drag handle" のボタンで、説明はクリックでする操作（"Expand bottom sheet"・"Collapse bottom sheet"・"Dismiss bottom sheet"）です。これは Compose の独自の読み上げ用の操作の代わりで、lamina-ui の木にはそれを置く場所がありません。幕は "Close sheet" のボタンです。
- フォーカス：モーダルなシートは、Compose のダイアログの窓と同じく Tab を自分の中にとどめます（ハンドル・中身の止まり位置の順で、端では先頭へ戻ります）。キーボードで開くとハンドルにフォーカスを置きます。モーダルでないシートのハンドルと中身は、プログラムの Tab の順に加わります。
- 違い：ハンドルのマウスを乗せたとき・フォーカス・押下の重ねは 32 × 48 dp の当たりを囲む両端の丸い形で、フォーカスの輪もその周りに描きます。Compose の波紋は四角で、輪はありません。マウスのホイールは中身をスクロールし、シートは動かしません（Compose はホイールの量を入れ子のスクロールに送るので、シートが動くことがあります）。中身を指でなぞると、なぞり始めにその向きへ中身がスクロールできるときだけ中身が動き、そうでなければシートが動きます（[指でのスクロール](#指でのスクロール)）。予測型の戻る操作は作っていません。

### サイドシート

`6068a9492ba8` の Compose にはサイドシートがなく（`SideSheet*.kt` も `SheetSideTokens.kt` もありません）、`9d2f79851cae` の material-web にもサイドシートの部品はなくトークンだけです。値は W の `tokens/versions/latest/sass/_md-comp-sheet-side.scss`（Material 3 の 34.0.21 版から作られたもの）と `6061a69915d0` の M の `lib/java/com/google/android/material/sidesheet/`（`SideSheetBehavior.java`・`RightSheetDelegate.java`・`res/values/{dimens,styles,tokens,strings}.xml`・`res/anim/m3_side_sheet_{enter_from_right,exit_to_right}.xml`・`res/values-ja/strings.xml`）から取り、`lamina/tokens/side_sheet.hpp` に入れました。M3 のデザインのサイトにあるサイドシートの仕様は読めませんでした（ページはスクリプトで描かれ、文字が返ってきませんでした）。

- 幅は 256 dp（`docked-container-width`）。モーダルでない入れ物は surface で段階 0、モーダルな入れ物は surface-container-low で段階 1 です。窓の端に付けたモーダルな形は始めの側が corner-large（中身に向いた角が 16 dp）、端に付けたモーダルでない形は角なし、離した形は四隅が corner-large で四方に 16 dp の余白を取ります（M の `m3_side_sheet_margin_detached`）。見出しは on-surface-variant の Title L、操作の上の区切り線は outline です。
- 動き：MDC のモーダルなサイドシートは emphasized の緩急で 275 ms かけて滑り込み、滑り出ます。lamina-ui は開くとき・閉じるとき・ドラッグのあと落ち着くときにこのトゥイーンを使い、幕を滑りに合わせて現し消しします（MDC は代わりにダイアログの窓を暗くします）。
- 離したとき（`calculateTargetStateOnViewReleased`）：外向きにはじくと開いたままです。そうでなければ、500（MDC の `SIGNIFICANT_VEL_THRESHOLD`。単位は毎秒のピクセルです。lamina-ui は毎秒の dp にしています）より速くスワイプしたときか動ける幅の半分を越えて離したときに隠れ、それ以外は開いたままです。
- lamina-ui が決めた配置（どの出典にも定めがないため）：見出しの行は高さ 72 dp です。付けてもよい戻るアイコンボタンは始めの側から 12 dp、見出しは始めの側から 24 dp（または戻るボタンの 4 dp 後）、閉じるアイコンボタンは終わりの側から 12 dp です。中身は左右から 24 dp です。操作は 1 dp の区切り線のあとの行で、内側に 16 dp・間に 8 dp を空けて、プログラムが渡したボタンを並べます。MDC のカタログは 16 dp の余白と、基線をそろえた閉じるボタンを使っています。
- 振る舞い：モーダルなシートは、Escape・幕のクリック・閉じるボタン・ドラッグで閉じます。Tab を中にとどめ（見出しの行のボタン・中身・操作の順で、端では先頭へ戻ります）、見出しを名前にしたモーダルな `dialog`（見出しがなければ MDC の `side_sheet_accessibility_pane_title` の "Side Sheet"。日本語ではサイドシート）と "Close sheet" の幕のボタンを返します。モーダルでないシートは配置の一部です。MDC の同じ平面に並ぶシートと同じく開くにつれて幅が 0 から広がるので、横の中身は狭まります。そのボタンはプログラムの Tab の順に加わります。どちらの端にも置けます（`SideSheetEdge`）。

### ナビゲーションドロワー

`6068a9492ba8` の C の `NavigationDrawer.kt` と `tokens/NavigationDrawerTokens.kt`・見本の `DrawerSamples.kt` を読み、見出しと区切り線の引っ込みは `6061a69915d0` の M の `lib/java/com/google/android/material/navigation/res/values/dimens.xml` から取って`lamina/tokens/navigation_drawer.hpp` に入れました。

- シート：幅は 240 dp 以上 360 dp 以下です。項目が幅いっぱいに広がるので、窓が狭くなければ 360 dp になります。モーダル：surface-container-low で、終わりの側が corner-large（`ContainerShape`）です。影も色の高さもありません（トークンのファイルには段階 1 の `ModalContainerElevation` がありますが、`ModalDrawerElevation` は段階 0 です）。閉じられるものと常に出ているもの：surface で角なし。
- 項目（`NavigationDrawerItem`）：高さ 56 dp 以上で、左右を 12 dp 引っ込めた角を丸め切った形です（`ItemPadding`。見本の置き方どおり）。中身の前に 16 dp・後ろに 24 dp を空け、アイコンは 24 dp でラベルまで 12 dp、ラベルは Label L です。バッジはラベルの 12 dp 後に Label L で置きます。選んだ項目は secondary-container で、アイコン・ラベル・バッジは on-secondary-container です。選んでいない項目は透明で on-surface-variant です。状態の重ねは中身の色（on-surface、選んでいる間は on-secondary-container）です。見本は最初の項目の上に 12 dp を空けます。
- 見出しと区切り線：Compose にはドロワーの見出しも区切り線もありません。見出しは Title S と on-surface-variant のトークン（`HeadlineFont`・`HeadlineColor`）を使い、始めの側の引っ込みは MDC の 28 dp です。高さ 56 dp は lamina-ui が決めました。区切り線は outline-variant の 1 dp で 28 dp 引っ込み、上の項目の 8 dp 下・下の項目の 3 dp 上に置きます（MDC の `m3_navigation_menu_divider_horizontal_padding`・`m3_navigation_divider_top_margin`・`m3_navigation_divider_bottom_margin`）。
- 動き：`open()` は標準の空間のばね、`close()` は速い効果のばねです。離したドラッグは `FastOutSlowInEasing` の 256 ms のトゥイーンで落ち着きます。`DrawerState` は `anchoredDraggableMotionSpec` を設定する `SideEffect` が動く前に `AnchoredDraggableDefaultAnimationSpec` で `AnchoredDraggableState` を作るので、ばねは届きません。しきい値：位置は幅の半分、速さは 400 dp/s です。モーダルな幕の不透明度は、位置から求めた開いた割合です（幕だけの動きはありません）。
- 行き過ぎ：開くばねは行き過ぎます。Compose は入れ物を終わりの側から伸ばし（`horizontalScaleUp`）、中身を縮め戻します（`horizontalScaleDown`）。lamina-ui は入れ物を窓の端からシートの終わりの側まで描き、中身は拡大縮小しません。
- モーダル：範囲全体をドラッグできるので（Compose は中身を持つ箱に `anchoredDraggable` を付けます）、閉じた中身の上のドラッグで開きます。`DrawerResponse::took_over` は、プログラムが自分の部品に渡した押下を取り消すよう伝えます。幕のクリックと Escape（キーを押したとき）で閉じます。閉じられるもの：中身はシートの幅だけ押しやられ、自分の幅を保ちます。シートにフォーカスがある間は Escape で閉じます。常に出ているもの：`permanent_drawer(sheet, content)` はシートと中身を並べた行です。
- 読み上げ：シートは "Navigation menu"（ナビゲーションメニュー）という名前の `navigation` です。項目は `navigation_item`（Compose の `Role.Tab`）で、選択とバッジを説明に入れます。見出しは `heading`、区切り線は `separator` です。モーダルなドロワーの幕は "Close navigation menu" のボタンです。
- 違い：Compose の項目には無効の状態がありません。lamina-ui の項目は 38 % に薄め、飛ばします。Compose は色を一度に切り替えますが、lamina-ui は標準の効果のばねで移します。キー：シートは Tab の止まり位置一つです。↑ と ↓ で有効な項目の間をフォーカスが動き（見出しと区切り線は飛ばします）、`Home` と `End` も受け、Space か Enter で選びます。ナビゲーションレールと同じです。Compose はどの項目も Tab の止まり位置です。モーダルなドロワーは Tab を中にとどめ、キーボードで開いたときはフォーカスを取ります。Compose のモーダルなドロワーはフォーカスを求めますが、閉じ込めはしません。シートは並べたものが窓より高いときにスクロールします。Compose はそれを呼ぶ側に任せます。フォーカスの輪は両端の丸い形の周りに描きます。予測型の戻る操作は作っていません。

### モーダルな幅の広いナビゲーションレール

`6068a9492ba8` の C の `WideNavigationRail.kt` と `tokens/NavigationRailExpandedTokens.kt` を読み、`lamina/tokens/navigation_rail.hpp`（`modal_navigation_rail_tokens`）と `lamina/widgets/modal_navigation_rail.hpp` に入れました。

- `ModalWideNavigationRail` はこのコミットにあります。畳んだときは配置の中の 96 dp のレールです。広げると中身の上に幕を敷き、surface-container で四隅が corner-large の 220〜360 dp のレールとして開きます（`ModalContainerShape`。`ModalContainerElevation` は段階 2 ですが、影は付けていません）。幕は広がりに合わせた標準の効果の動きに従います。lamina-ui は、行き先が広げた状態の間は標準の効果のばねで幕を現します。
- `hideOnCollapse`：畳んだレールはありません。広げたレールは窓の外から標準の空間のばねで滑り込み、ドラッグでしまえます（位置のしきい値は幅の半分・速さは 125 dp/s）。
- Escape（キーを押したとき）と幕のクリックで畳みます。components の例では、隠れる形で項目を選ぶと畳みます。読み上げ：レールはモーダルな `navigation` で、"Close rail" の幕のボタンを持ちます。プログラムが名前を付けなかったときは、区画の題名 "Navigation rail"（ナビゲーション レール）が名前になります。
- 違い：Compose は効果の動きが 0 になるとすぐ、幅の動きが終わる前にモーダルなレールを外します。lamina-ui はレールが畳んだ幅に戻るまで残します。見出しの部品（たとえばメニューのボタン）はプログラムの部品で、components の例はそれを幕の上に描きます。

### 検索バー

`6068a9492ba8` の C の `SearchBar.kt`・`tokens/SearchBarTokens.kt`・`tokens/SearchViewTokens.kt`・`internal/TextFieldImpl.kt` と見本の `SearchBarSamples.kt` を読み、600 dp の境目は `material3-window-size-class/.../WindowSizeClass.kt` から取って`lamina/tokens/search.hpp` に入れました。

- どの API か：結果に `ExpandedFullScreenSearchBar` か `ExpandedDockedSearchBar` を使う `SearchBar(state, inputField)` です。`SearchBar(inputField, expanded, onExpandedChange)` と `DockedSearchBar` の版は非推奨です。このコミットにある Expressive の版の `AppBarWithSearch`・`ExpandedFullScreenContainedSearchBar`・`ExpandedDockedSearchBarWithGap` は作っていません。
- バー：高さ 56 dp・幅 360 dp 以上 720 dp 以下・角は丸め切り・surface-container-high で、高さはありません（色の高さも影も段階 0）。入力は入れ物も下線もない入力欄です。文字は Body L の on-surface、薄い案内の文字は on-surface-variant、カーソルは primary です。先頭と末尾のアイコンは内側へ 4 dp ずらした 48 dp の枠に入ります。そのためアイコンは両端から 16 dp の所にあり、文字は 52 dp の所から始まります（`textFieldHorizontalIconPadding` と `SearchBarIconOffsetX`）。見本と同じく先頭のアイコンは畳んでいる間は検索のアイコン、広げている間は戻るアイコンボタンです。
- 広げる：遅い空間のばねで広げ、標準の空間のばねで畳みます（`rememberSearchBarState`）。進み具合が 0.02 を超えている間は表示を出します。バーのクリック・フォーカスがある間の ↓・検索語を長くする入力で広がります。中では ↓ で結果へ入り、Escape で畳み、欄で Enter を押すと検索語を知らせます（`onSearch`）。
- 全画面の表示：面はバーから範囲全体へ広がり（位置と大きさは進み具合で線形補間します）、角は 28 dp から 0 になります。欄の幅と中心は、切り詰めない進み具合でバーから範囲へ動きます。欄の上下に 8 dp の余白が現れ、その下に outline の区切り線と結果が進み具合に合わせて現れます。
- バーの下に開く表示：バーの位置と幅のままです。高さは進み具合につれて窓の高さの 2/3 まで育ち、240 dp を下回りません（上限のほうが小さいときは上限まで）。結果の量にも合わせます。角は extra-large（28 dp）で、区切り線と結果は欄の下です。幕はありません。
- 狭い窓と広い窓：Compose は二つの表示を切り替えません。見本はスマートフォンで全画面の表示を、大きな画面でバーの下に開く表示を使います。lamina-ui の `SearchViewMode::automatic` は窓の幅の区分が中くらいになる 600 dp より狭いときは全画面の表示を、そこからはバーの下に開く表示を使います。
- 読み上げ：欄の名前は "Search"（検索）で、広げている間は "Suggestions below"（検索候補は次のとおりです）と伝えます。広げた表示は "Search" という名前の `dialog` で、全画面のときはモーダルです。
- 違い：最初の結果で ↑ を押すと欄に戻ります。Tab は広げた表示の中（戻るボタン・欄・末尾のボタン・結果）にとどまります。Compose のダイアログとフォーカスを受けるポップアップと同じです。バーの下に開く表示の外を押すと畳みます。予測型の戻る操作と、`AppBarWithSearch` のスクロールの振る舞いは作っていません。

### 切り替えボタンと角ごとの形

`6068a9492ba8` の C の `ToggleButton.kt`・`internal/AnimatedShape.kt`・`HorizontalCenterOptically.kt`・`tokens/{Filled,Tonal,Outlined,Elevated}ButtonTokens.kt` を読み、`lamina/tokens/button.hpp`（`toggle_button_styles`・`toggle_button_motion`）と `lamina/widgets/button.hpp` の `ButtonOptions::toggle`・`ButtonOptions::shapes` に入れました。

- 色（`ToggleButtonDefaults.colors()` と tonal・outlined・elevated の既定）：filled は surface-container と on-surface-variant で、オンのときは primary と on-primary です。tonal は secondary-container と on-secondary-container で、オンのときは secondary と on-secondary です。outlined は透明で outline-variant の枠があり、オンのときは枠のない inverse-surface と inverse-on-surface です。elevated は surface-container-low と primary で、オンのときは primary と on-primary です。無効のときの入れ物は on-surface の 10 %（outlined は outline-variant の 10 %）、中身は on-surface-variant の 38 % です。text のボタンは切り替えボタンにできません。
- 形（`ToggleButtonDefaults.shapesFor`）：ふだんは丸め切り、押している間は大きさごとの押したときの角、オンのときは大きさごとの四角い形の角です。押した形がオンの形より優先されます。形の移り変わりは `AnimatedShapeState` と同じで、すべての角が一つの進み具合で元の形から目標の形へ速い空間のばねで移ります。元の形へ戻るときは、同じ進み具合を速度の向きを逆にして逆向きに進めます。lamina-ui は角ごとに dp と短い辺に対する割合で持つので、移る途中でも丸め切りの角は大きさについていきます。
- outlined の枠の太さは、オンになると速い空間のばねで 0 になります（`animateBorderStrokeAsState`）。
- `center_optically` は中身を 0.11 ×（始め側の角の平均 − 終わり側の角の平均）だけずらします。それぞれの角は高さの半分までに、ずらす量は左右の余白までに抑えます（`ShapeWithHorizontalCenterOptically`）。分割ボタンの後ろ半分が使います。
- 読み上げ：Compose は切り替えボタンを `Role.Checkbox` にします。lamina-ui はオンかオフを持つ `toggle_button` を返します。`radio` の切り替えボタンは `selected` を持つ `radio_button` を返し、クリックではオフになりません。一つだけ選ぶ見本（`Role.RadioButton`）と同じです。
- 違い：オンの色は Compose では一度に変わりますが、lamina-ui は標準の効果のばねで移します。オンのアイコンは FILL 1 で描きます。見本が線のアイコンを塗ったアイコンに替えるのの代わりです。

### ボタンの組

`6068a9492ba8` の C の `ButtonGroup.kt`・`tokens/ButtonGroupSmallTokens.kt`（`VERSION: v0_11_0`）・`tokens/ConnectedButtonGroupSmallTokens.kt`（`VERSION: 14_1_0`）と見本の `ButtonGroupSamples.kt` を読み、`lamina/tokens/button_group.hpp` と `lamina/widgets/button_group.hpp` に入れました。

- 並べ方：ボタンの間は 12 dp、つながった組は 2 dp です。ボタンはそれぞれ中身に合った幅を取ります。重みを渡したボタンは、残りの幅を重みで分けます。
- 押したときの幅（`ButtonGroupMeasurePolicy`・`EnlargeOnPressNode`）：押したボタンは左右の隣へ、押し具合 × min(0.15 × 自分の幅 / 2、隣の縮められる量) ずつ広がります。最初と最後のボタンは、ただ一つの隣へ押し具合 × min(0.15 × 幅、縮められる量) だけ広がります。隣は同じだけ縮むので、組の幅は変わりません。縮められる量は隣の終わり側の余白（small の大きさで 16 dp）です。押し具合は速い空間のばねで進み、離したあとも 0.75 を超えるまで進んでから戻ります。そのため短いクリックでも隣は最後まで縮みます。Compose は幅をピクセルに丸めますが、lamina-ui は端数のある dp のまま持ちます。
- はみ出し：ボタンが収まらないときは、はみ出しのボタンの前に収まる分だけを残し、残りをメニューに移します。はみ出しのボタンは `more_vert` の filled のアイコンボタンで、名前は "More options"（その他のオプション、`m3c_button_group_more_options`）です。残したボタンの終わりに置きます。メニューの項目はそのボタンをクリックするか切り替えて、メニューを閉じます。
- つながった形（`ButtonGroupDefaults.connected*ButtonShapes`）：先頭のボタンは外側が丸め切りで内側が small（8 dp）、中ほどは四隅が small、最後は先頭の左右逆です。押している間の内側の角は extra-small（4 dp）、オンのボタンは両端の丸い形です。Compose のつながった見本は幅の動きのない `FlowRow` に切り替えボタンを並べ、好きな項目を入れる見本は `animateWidth` を付けたつながった形を `ButtonGroup` に入れています。lamina-ui のつながった組はその両方をします。
- 選び方：なし（`clickableItem`、ふつうのボタン）、いくつでも（`toggleableItem`、切り替えボタン）、一つだけ（見本と同じくラジオの役割を持つ切り替えボタン）。
- 違い：キーで押してもボタンは広がりません（Compose はキーで押しても広がります）。ボタンとはみ出しのボタンはどれも Tab で止まり、Compose と同じく矢印キーでは動きません。組は `group` の項目を返します。項目ごとの縦のそろえ方と縦のボタンの組は作っていません。つながった組では、ふつうのボタンにもつながった形を使います。

### 分割ボタン

`6068a9492ba8` の C の `SplitButton.kt`・`tokens/SplitButton{XSmall,Small,Medium,Large,XLarge}Tokens.kt`（`VERSION: 18_0_18`）と見本の `SplitButtonSamples.kt` を読み、`lamina/tokens/split_button.hpp` と `lamina/widgets/split_button.hpp` に入れました。

- 大きさは 32・40・56・96・136 dp で、二つの半分の間は 2 dp です。内側の角は extra-small・extra-small・extra-small・small・medium で、押している間は small・medium・medium・large-increased・large-increased に広がります（ホバーのトークンは `SplitButton.kt` では使われていません）。前半分の余白は 12+10・16+12・24+24・48+48・64+64 dp、後ろ半分の余白は左右とも 13・13・15・29・43 dp、後ろ半分のアイコンは 22・22・26・38・50 dp で、どちらの半分も幅 48 dp 以上です。文字と前半分のアイコンは見本と同じくボタンの大きさに合わせます（`ButtonDefaults.textStyleFor`・`iconSizeFor`・`iconSpacingFor`）。
- 形は標準の効果のばねで移ります。オンの後ろ半分は円になり、中身の色を 10 % 重ね（`TrailingButtonStateLayerAlpha`）、中身を見た目の中心に置きます。
- 下向きの山形（`keyboard_arrow_down`）は、オンになると半回転します。ばねは見本の `animateFloatAsState` の既定（減衰 1、硬さ 1500）です。
- メニュー：`SplitButtonWithDropdownMenuSample` と同じく、後ろ半分をオンにすると分割ボタン全体の下にメニューが開き、項目を選ぶかメニューを閉じるとオフに戻ります。後ろ半分は見本の `stateDescription` と同じく、値で "Expanded" か "Collapsed" を伝えます。
- 幅：`SplitButtonLayout` は後ろの半分の本来の幅を見積もり、それと間の幅を最大の幅から引いたものを前の半分に回します。後ろの半分を先に測り（「幅を優先するため」）、残りで前の半分を測ります。二つの半分の行には `defaultMinSize(minWidth = LeadingButtonMinWidth / TrailingButtonMinWidth)` があり、`widthIn` はなく、文字に `maxLines` も `overflow` も付けません。lamina-ui も後ろの半分と 2 dp の間に先に幅を渡し、前の半分を左右の余白・アイコン・「…」の幅（48 dp 以上）まで縮めます。Compose の `Text` なら折り返すところを、1 行のまま切ります。
- 違い：filled・tonal・outlined・elevated の色はボタンの色（`ButtonDefaults.*Colors`）です。lamina-ui は色を標準の効果のばねで移します。

### 拡張 FAB

`6068a9492ba8` の C の `FloatingActionButton.kt` と `tokens/ExtendedFab{Small,Medium,Large}Tokens.kt` を読み、`lamina/tokens/fab.hpp`（`extended_fab_sizes`）と `lamina/widgets/extended_fab.hpp` に入れました。

- small は 56 dp（角 large・アイコン 24 dp・余白 16 dp・間 8 dp・Title M）、medium は 80 dp（large-increased・28 dp・26 dp・12 dp・Title L）、large は 96 dp（extra-large・32 dp・28 dp・16 dp・Headline S）です。medium と large の間はコードの 12 dp と 16 dp です。トークンのファイルは 16 dp と 20 dp としています（"IconLabelSpace is incorrect"）。
- 畳む：幅は広げたときの幅から高さまで速い空間のばねで縮み、文字は速い効果のばねで消えます。アイコンは始め側の余白の所から動きません。
- 高さと色は FAB と同じです。Compose の FAB に無効の状態はなく、lamina-ui にもありません。

### FAB メニュー

`6068a9492ba8` の C の `FloatingActionButtonMenu.kt`・`tokens/FabMenuBaselineTokens.kt` と見本の `FloatingActionButtonMenuSamples.kt` を読み、`lamina/tokens/fab_menu.hpp` と `lamina/widgets/fab_menu.hpp` に入れました。

- 切り替えの FAB：進み具合は速い空間のばねで進みます。大きさは 56・80・96 dp から 56 dp へ、上の終わり側を基準に縮みます。角は 16・20・28 dp から 28 dp へ、入れ物は primary-container から primary へ、アイコンは on-primary-container から on-primary へ、24・28・36 dp から 20 dp へ移ります。見本が追加のアイコンを閉じるアイコンに替えるのと同じく、アイコンは 0.5 で替わります。影は段階 3 です。
- 項目：primary-container の 56 dp の両端の丸い形で、余白 24 dp・アイコン 24 dp・Title M の文字の前に 8 dp です。項目の間は 4 dp、ボタンの上は 8 dp、左右とボタンの下は 16 dp あけ、終わり側にそろえます。
- 順に出す動き：出す項目の数は遅い効果のばね（しきい値 1）に従う整数で、下から数えます。そのためボタンに近い項目から現れ、一番上の項目から消えます。項目ごとに、幅は終わり側から速い空間のばねで広がり、不透明度は速い効果のばねで変わります。項目に影はありません（トークンは段階 3 ですが、`Surface` は影を付けません）。
- キー：開いているボタンで Tab か ↓ を押すと項目に入り、Enter で選び、Escape で閉じてフォーカスをボタンに戻します（見本の `BackHandler`）。見本と同じく、項目を選ぶとメニューを閉じます。
- 違い：メニューが範囲より高くてもスクロールしません。Compose と同じく、メニューと切り替えの FAB に無効の状態はないので、一覧の Enabled のスイッチを切ってもそのままです。

### 読み込み中の表示

`6068a9492ba8` の C の `LoadingIndicator.kt`・`MaterialShapes.kt`・`tokens/LoadingIndicatorTokens.kt` を読み、lamina-ui に移した graphics-shapes とともに `lamina/tokens/loading_indicator.hpp` と `lamina/widgets/loading_indicator.hpp` に入れました。

- 入れ物は 48 dp（丸め切り）、表示は 38 dp です。形は、大きさ × 多角形の外枠と最大の外枠の比の一番小さいもの × 38 / 48 に縮め、毎回制御点の外枠の中心を真ん中に合わせます。表示は primary、入れ物のある形は primary-container の上の on-primary-container です。形は入れ物の中に切り抜きます。
- 終わりの見えない形：SoftBurst・Cookie9Sided・Pentagon・Pill・Sunny・Cookie4Sided・Oval と回って最初に戻ります。多角形はどれも正規化します。形の移り変わりは減衰 0.6・硬さ 200 のばねで進み、しきい値 0.1 に対する Compose の長さの見積もり（約 0.30 秒）が過ぎると終わります。650 ms ごとに次の移り変わりが始まります。回転は 進み具合 × 90° に、終わった移り変わりごとに 90° と、4666 ms で一回りする等速の回転を足したものです。
- 進み具合のある形：進み具合が 0 から 1 へ進むにつれて、18° 回した円が SoftBurst に変わり、形は 進み具合 × 180° だけ逆に回ります。
- `LoadingIndicatorOptions::size` は表示全体を大きくします。Compose で大きさの修飾子を大きくするのと同じです。
- Compose の読み込み中の表示に無効の状態はなく、lamina-ui の読み込み中の表示と波のある進み具合の表示にもありません。

### 波のある進み具合の表示

`6068a9492ba8` の C の `WavyProgressIndicator.kt`・`internal/LinearWavyProgressModifiers.kt`・`internal/CircularWavyProgressModifiers.kt`・`ProgressIndicator.kt`（キーフレーム）・`tokens/{Linear,Circular}ProgressIndicatorTokens.kt` を読み、`lamina/tokens/wavy_progress.hpp` と `lamina/widgets/wavy_progress.hpp` に入れました。

- 直線：既定は 240 × 10 dp で、表示と下地は 4 dp、間は 4 dp です。進み具合があるときは下地の終わりに 4 dp の点を置きます。波は Compose と同じ二次曲線の線で、制御点を中心線から `高さ − 太さ`（6 dp）離すので、山の高さは 3 dp です。進み具合は x に合う長さ（線の長さ ÷ 幅）だけ線に沿って取り、振幅は線を中心線を軸に縮めます。波長は進み具合があるとき 40 dp、ないとき 20 dp で、波は 1 秒に 1 波長進みます。
- 円：48 dp で、太さは 4 dp、間は 4 dp です。下地は円の多角形、表示はそこから星形（内側の半径 0.75、角の丸め 0.35 となめらかさ 0.4、内側の丸め 0.5）へ移る形で、頂点の数は max(5, round(2πr / 15 dp)) です。12 時の所から時計回りに始まるように回し、大きさから太さを引いた大きさに合わせます。動く波は、二周分の線の上で始まりをずらしてから戻す向きに回すので、両端は動きません。
- 振幅：`wavy_indicator_amplitude`（10 % まで、と 95 % からはなし）を 500 ms のトゥイーンで移します。上げるときは standard、下げるときは emphasized-accelerate のイージングです。`ease_progress` は進み具合を Compose の 500 ms の等速の `ProgressAnimationSpec` で移します。
- 終わりの見えない直線：1750 ms のキーフレームに沿う 2 本の線です（先と後ろの遅れは 0・250・650・900 ms、長さは 1000・1000・850・850 ms、イージングは emphasized-accelerate）。終わりの見えない円：6000 ms の伸び縮みと回転のキーフレームに、1080° の等速の回転と 90° の基準を足します。Compose のキーフレームのイージングはそのキーフレームの後ろの区間にかかるので、伸びは 0.87 まで等速で伸びて standard で縮み、四分の一の回転は等速です（emphasized-decelerate は止まっている区間にかかります）。lamina-ui も同じように計算します。
- 受け渡し：直線の表示は、進み具合を持つと線と点を 500 ms で伸ばします。円の表示は、終わりの見えない伸びと回転から、進み具合と 360° へ 300〜600 ms（emphasized decelerate）で移ります。回す量が多いほど長くかかります。
- 違い：直線の表示は進み具合を持つとき、先に線を速めて走り切らせることはしません。線は曲線ごとに 12 の折れ線にして、丸い端で描きます。前からある `CircularProgressIndicator` は、使っているプログラムのためにそのまま残します。

### 浮かぶツールバー

`6068a9492ba8` の C の `FloatingToolbar.kt` と `tokens/FloatingToolbarTokens.kt` を読み、`lamina/tokens/floating_toolbar.hpp` と `lamina/widgets/floating_toolbar.hpp` に入れました。

- 高さ（縦のときは幅）64 dp・角は丸め切り・余白 8 dp で、操作は 48 dp の枠に入ります（最小の触れる大きさを持つ `IconButton`）。standard の色は surface-container と on-surface で FAB は primary-container、vibrant の色は primary-container と on-primary-container で FAB は tertiary-container です。FAB の角は 16 dp のまま、影は段階 2（ホバーで段階 3）です。
- FAB なし：先頭の中身は始め側から広がって終わり側へ縮み、末尾の中身はその逆で、どちらも速い空間のばねです（`expandHorizontally`・`shrinkHorizontally`）。影はありません。
- FAB あり：バーの幅（縦のときは高さ）は速い空間のばねで全体から 0 まで FAB の方へ縮み、FAB は 56 dp から 80 dp に育ちます。影は段階 1 から 0 になります。FAB は横のときは終わり側、縦のときは下です。
- スクロールで畳む（`floatingToolbarVerticalNestedScroll`）：終わりの方へ 40 dp スクロールすると畳み、40 dp 戻すと広げます。`ToolbarScrollExpansion` がスクロールの位置を追います。
- 読み上げ：`toolbar` の項目で、説明に自前の操作の名前（"Collapse toolbar" か "Expand toolbar"、ツールバーを閉じます・ツールバーを開きます）を入れます。畳んだ操作には Tab で止まりません。
- 違い：ツールバーを無効にすると、FAB も反応しなくなり、アイコンが 38 % に薄くなります。これは lamina-ui が足したものです（Compose の FAB に無効の状態はありません）。スクロールで常に退く振る舞いと `minimumInteractiveBalancedPadding` は作っていません。ツールバーの操作は切り替えないので、vibrant の切り替えボタンの色は使っていません。

### 柔軟な下のアプリバー

`6068a9492ba8` の C の `AppBar.kt`（`FlexibleBottomAppBar`・`BottomAppBarDefaults`・`settleAppBarBottom`）と `tokens/DockedToolbarTokens.kt` を読み、`lamina/tokens/floating_toolbar.hpp`（`flexible_bottom_app_bar_tokens`）と `lamina/widgets/flexible_bottom_app_bar.hpp` に入れました。

- 64 dp・surface-container・影なし・余白 16 dp で、操作は両端に合わせて均等にあけます（または 32 dp ずつあけて中央に寄せます、`FlexibleFixedHorizontalArrangement`）。
- 常に退く：高さのずれはスクロールに従って −64 dp まで動くので、バーは下の端から外へ滑り出ます。スクロールが止まると、近い方の端へ速い空間のばねで落ち着きます（畳んだ割合 0.5 で分け、0.01 未満なら何もしません）。隠れている間、操作には Tab で止まりません。
- 違い：落ち着く前のはじいた勢いの減速は作っていません。

### 密度

2026-11-21 に読みました。密度の参照先として挙げられた material-web のコミット（`9d2f79851cae`）には密度の段階がなく、密度にかかわる設定は `touch-target` の属性（`wrapper` か `none`）だけです。Compose にもなく、あるのは `minimumInteractiveComponentSize`（48 dp、`LocalMinimumInteractiveComponentSize`）だけです。段階は MDC Web（material-components-web、MIT ライセンス）の `6ceadb81df50` の `packages/mdc-density/_density.scss` から取りました。一段ごとに部品の既定の高さへ `段階 × 4 px` を足し、部品ごとに最小と最大（既定と同じ）を決め、段階が 0 でなければ 48 px のタッチの範囲を外します。値は `lamina/tokens/density.hpp`（`density_tokens`）にあり、部品ごとの `density` の設定と `set_density` が読みます。`WidgetSet::set_density` は、密度を持つ（`Densifiable`）すべての部品へ渡します。入れ子の部品の集まりにも届きます。

| 部品 | 既定 | 最小 | 段階 | 最小の出どころ |
|---|---|---|---|---|
| ボタン（極小・小・中） | 32・40・56 | 24・28・44 | −2・−3・−3 | 小は MDC のボタン（36 → 24）、ほかは lamina-ui |
| アイコンボタン（極小・小・中。幅も縮み、アイコンより狭くはならない） | 32・40・56 | 24・28・44 | −2・−3・−3 | MDC のアイコンボタン（48 → 28）、lamina-ui |
| 枠線のテキスト欄（複数行の上下の余白は一段ごとに半段ずつ縮む） | 56 | 36 | −5 | MDC のテキスト欄 |
| リストの項目（1 行・2 行・3 行。上下 10 dp の余白も縮む） | 56・72・88 | 40・56・56 | −4・−4・−8 | MDC のリスト（`tall` の設定） |
| チップ | 32 | 24 | −2 | MDC のチップ |
| タブ（文字か横のアイコン・上のアイコン） | 48・72 | 30・56 | −4・−4 | MDC のタブバー |
| 区分ボタン | 40 | 28 | −3 | lamina-ui |
| 小さい・中央ぞろえの上部アプリバー、ほかのバーの畳んだ行（広げた高さも同じだけ縮む） | 64 | 48 | −4 | MDC の詰めた上部アプリバー（48 px） |
| ナビゲーションバー（項目の上下 6 dp の余白が縮む） | 64 | 52 | −3 | lamina-ui |
| 幅のあるナビゲーションレールの項目（畳んだとき・広げたとき） | 64・56 | 52・40 | −3・−4 | lamina-ui |
| ナビゲーションドロワーの項目 | 56 | 40 | −4 | lamina-ui |
| メニューの項目 | 48 | 32 | −4 | lamina-ui |
| メニューバー | 40 | 28 | −3 | lamina-ui |
| 表の行・見出し | 52 / 56 | 36 / 40 | −4 / −4 | MDC のデータテーブル |

- 大・特大のボタンとアイコンボタンは密度を持ちません。96 dp や 136 dp から 4 dp ずつ削っても、載る量は変わらないためです。
- 違い：MDC は最小を下回る段階をコンパイルの誤りにしますが、lamina-ui は最小で止めます。一つの段階を多くの部品にまとめて設定するためです。正の段階は既定で止めます。区分ボタン・ナビゲーションの項目・メニューは MDC に密度がないので、最小は lamina-ui が決めました。メニューバーも MDC にはありません。表は MDC のデータテーブルの行（52 → 36）に合わせ、見出しは MDC と同じく 4 dp 高くしました。
- タッチの範囲：密度 0 では、部品は 48 dp の当たり判定を保ちます（`tokens::minimum_target`）。負の密度では、タッチやペンでも当たり判定は部品そのものの範囲です。MDC がタッチの範囲を外すのと同じです。48 dp より詰めて並べた部品で 48 dp の範囲を取ると、隣の範囲と重なるためです。密度 0 の部品どうしの範囲が重なるところでは、いちばん近い部品がポインターを受けます（[タッチの範囲](#タッチの範囲)）。
- チェックボックス・ラジオボタン・スイッチは、まだ密度を持ちません。

### ウィンドウの大きさの区分

2026-11-21 に androidx の `6068a9492ba8` の `window/window-core/src/commonMain/kotlin/androidx/window/core/layout/WindowSizeClass.kt` と `WindowSizeClassSelectors.kt`（`BREAKPOINTS_V2`・`computeWindowSizeClass`）を読み、`lamina/layout/window_size_class.hpp` に移しました。

- 幅：600 dp 未満が compact、600 から medium、840 から expanded、1200 から large、1600 から extra-large です。高さ：480 dp 未満が compact、480 から medium、900 から expanded です。
- 区分は window-core と同じく下限の組（`min_width_dp`・`min_height_dp`）です。ウィンドウの大きさは比べる前に dp の整数へ切り捨てるので、599.9 dp は compact です。
- `compute_window_size_class(幅, 高さ)` は大きさだけで決まる関数です。`sdl3::Frame::size_class` は、ウィンドウの区分を毎フレーム持ちます（大きさを変えている途中も）。`WindowAdaptiveInfo`（`lamina/widgets/pane_navigator.hpp`）は区分と卓上の姿勢の印を組にします。Compose の `WindowAdaptiveInfo` が `Posture` と組にするのに当たります。デスクトップのウィンドウには姿勢がないので、プログラムが設定しない限り印は false です。

### ナビゲーションスイートの足場

`6068a9492ba8` の C の `compose/material3/material3-adaptive-navigation-suite/src/commonMain/kotlin/androidx/compose/material3/adaptive/navigationsuite/NavigationSuiteScaffold.kt` を読み、`lamina/widgets/navigation_suite.hpp` に移しました。

- `navigation_suite_type(info)` は Compose の `NavigationSuiteScaffoldDefaults.navigationSuiteType` です。幅が compact なら短いナビゲーションバー、卓上の姿勢か高さが compact なら中くらいの短いバー、それ以外は畳んだ幅のあるレールを選びます。広げたレールやドロワーは選びません。
- バーと横の部品は、Compose の `AnimationSpec`（ばね、減衰 0.9、剛性 700）で出入りします。中身は、その進み具合だけバーの高さか横の部品の幅を譲ります。
- 違い：`navigation_suite_type(info, choice)` と `widening_navigation_suite`（compact はバー、medium は畳んだレール、expanded は広げたレール、large と extra-large はドロワー、高さが compact なら中くらいのバー）は lamina-ui の選び方です。広いウィンドウほどナビゲーションを多く見せるという M3 の方針に沿いました。Compose は種類が変わると部品を切り替えるだけで移り変わりがありませんが、lamina-ui は同じばねでバーを下へ出してから横の部品を入れます。畳んだレール・広げたレール・ドロワーは一つの `SideNavigation`（「横のナビゲーション」を見てください）で見せるので、そのあいだの切り替えは一つの並びの形が変わるだけで、アイコンは動きません。畳んだレールは 80 dp のアイコンだけのレールです。アイコンの下に名前を置く 96 dp のレールは行が高く、アイコンが動いてしまうためです。名前を消すと、横の種類はどれもアイコンだけのレールにたたみます。中くらいの短いバー（`ShortNavigationBarMedium`、アイコンの横にラベル）は compact と同じ形で描きます。lamina-ui のナビゲーションバーには縦に積む並びしかないためです。主な操作の枠（`primaryActionContent`）は作っていません。

### 横のナビゲーション

lamina-ui 独自の部品で、上の値から組み立てました。`lamina/tokens/side_navigation.hpp`（`side_navigation_tokens`）と `lamina/widgets/side_navigation.hpp` にあります。Compose には、レールとドロワーを兼ねる一つの並びはありません。

- M3 から取った値：畳んだ幅 80 dp（狭い `NavigationRail`。上のアイコンだけのレールと同じ）、幅のあるレールの 220〜360 dp、ドロワーの 360 dp、上の 12 dp・内側への 12 dp・アイコンの前の 16 dp・名前までの 12 dp・Label L（どれも `NavigationDrawerTokens`）、56 × 32 dp の形（`NavigationRailVerticalItemTokens`）、行の高さ（ドロワーの項目の 56 dp。密度で下がります）、レールの色です。
- 一つの形ですべての姿をまかなえるように値がそろっています。80 = 12 + 56 + 12、56 = 16 + 24 + 16 なので、アイコンは畳んだ形の中でもドロワーの行でも x = 28 dp にあり、どの姿でも y = 12 dp + n × 行 + (行 − 24) / 2 にあります。
- 幅のあるレールとの違い：項目を始まりの側から 20 + 16 dp ではなく 12 + 16 dp に置き、名前までを 8 dp ではなく 12 dp にし、行の上の 44 dp と行のあいだの 4 dp を無くしました。そうしないと、たたむときやドロワーに変わるときにアイコンが動きます。印はこれまでどおりアイコンと名前を囲みます。
- 動き（lamina-ui 独自の値）：広がり E とドロワーへの進み D は、減衰 1.0・標準の空間の剛性（380）のばねで動きます。幅と印は行き過ぎず、一方向にだけ変わります。幅 = lerp(80, lerp(レール, ドロワー, D), E)、印の幅は lerp(56, lerp(名前まで囲む幅, 行, D), E)、高さは lerp(32, 行, E) で、行の上下の中央に置きます。名前の不透明度は (2 (E − 0.5))²（0〜1 に収める）で、たたむときは前半で消え、広げるときは後半で現れます。D には左右されません。選んだときは、レールと同じく標準の空間のばねで印の幅が始まりの側の端から伸びます。

### ペインの足場

androidx の `6068a9492ba8` の `compose/material3/adaptive/adaptive-layout/.../layout/`（`PaneScaffoldDirective.kt`・`ThreePaneScaffoldValue.kt`・`ThreePaneScaffold.kt`・`PaneMotion.kt`・`PaneExpansionState.kt`・`PaneExpansionDraggableModifier.kt`・`ListDetailPaneScaffold.kt`・`SupportingPaneScaffold.kt`）と `adaptive-navigation/.../navigation/`（`ThreePaneScaffoldNavigator.kt`・`BackNavigationBehavior.kt`）を読み、`lamina/widgets/pane_navigator.hpp` と `pane_scaffold.hpp` に移しました。

- 指示（`calculate_pane_scaffold_directive`）：横の区画は、幅が compact と medium なら 1 つ、expanded から 2 つ（間は 24 dp）、large から 3 つで、ペインの望む幅は 412 dp（それより狭いと 360 dp）です。横の区画が 1 つで高さが expanded のとき、または卓上の姿勢のとき、縦の区画は 2 つ（間は 24 dp）です。`..._with_two_panes_on_medium_width` は medium の幅でも区画を 2 つにします。
- 値（`calculate_pane_scaffold_value`）：行き先を新しいほうから、続いて primary・secondary・tertiary の順に、区画が残っている間だけ区画を取ります。戦略が reflow のペインは、横の区画が 1 つで縦の区画が 2 つ以上のとき、基準のペインの下へ回ります。リストと詳細：list は secondary、detail は primary、並びは list・detail・extra、戦略はどれも hide です。補助：main は primary、supporting は secondary（main の下へ reflow）、並びは main・supporting・extra です。
- 案内役：行き先（ペインと中身の鍵）の履歴を持ち、`navigate_to`、`can_navigate_back`、`navigate_back` を四つの `BackNavigationBehavior`（直前を外す、値が変わるまで（既定）、行き先が変わるまで、中身が変わるまで）で行います。値は履歴を踏まえて決めます。足場は、ペインの部品が使わなかった Escape で `navigate_back` を呼びます。
- 配置：広げたペインは望む幅を取り、余りは優先度の一番高いペイン（primary 10、secondary 5、tertiary 1）が取ります。足りなければ全体を同じ割合で縮めます。下へ回ったペインは基準の区画の下の部分を取り、高さは望む高さ 420 dp と区画の半分の小さいほうまでです。
- 動き：Compose の `calculatePaneMotion` に従います（見えたままのペインは範囲を動かし、入るペインは何もふさいでいない側から入るか中央から広がり、出るペインも同じように出るか縮む）。ばねは減衰 0.8、剛性 380 で、遅れて入る動きはばねが落ち着くまでの見積もりの 0.1 だけ待ちます。
- フォーカス：隠れたペインはポインター・キー・文字を受けず、アクセシビリティの木からも外れ、フォーカスは外れます。ペインがキーボードのフォーカスを持つ間に行き先が変わって値が変わると、フォーカスは行き先のペインへ移ります（Compose の `shouldAutoFocusCurrentDestination`）。プログラムは各ペインの `FocusManager` を `FocusManager::add_group` で自分のものに入れ、`pane_shown` で見えるときだけにします。
- 違い：Compose は指示が変わると（大きさを変えると）値を一気に切り替え、動きを付けるのは移動のときだけです。lamina-ui も既定では同じで、`animate_directive_changes` で指示の変化にも動きを付けられます。浮かせたペイン、ヒンジと除外の範囲、予測型の戻る操作、状態の保存は作っていません。下へ回ったペインの高さは、Compose の区画の測り方を lamina-ui が簡単にしたものです。

### ペインの広さを変えるドラッグハンドル

`6068a9492ba8` の C の `DragHandle.kt` と `tokens/DragHandleTokens.kt`（`VERSION: 7_0_1`）を読み、`lamina/tokens/drag_handle.hpp` に移しました。広げ方は `PaneExpansionState.kt` に沿って `PaneExpansionState` に移しました。

- ハンドル：4 × 48 dp、丸め切り、outline の色です。押している間とドラッグの間は 12 × 52 dp、角は medium（12 dp）、on-surface です。タッチの範囲は 48 dp で、広げた二つのペインの間の真ん中に置き、足場の端から間の半分より内側に留めます。
- 止まる位置：割合、始まりからの距離、終わりからの距離のどれかです（`PaneExpansionAnchor`）。ドラッグでは 0 から足場の幅まで自由に動き、端から間の半分以内に寄せると、もう一方のペインだけになります。離すと一番近い止まる位置へ、毎秒 200 dp 以上で離したときはその向きで一番近い位置へ（後ろの位置は足場の幅だけ遠いとみなす）、離したときの速さから減衰 0.8、剛性 380 のばねで落ち着きます。`set_first_pane_width` と `set_first_pane_proportion` は止まる位置なしで分け目を置きます。
- 意味づけ：「Pane expansion drag handle」という名前の区切りで、値は「Current pane split, %s」、説明は「Change pane split to %s」です。止まる位置の説明（「%d percent」「%d DPs from start」「%d DPs from end」）を入れます。訳があり（`PatternStrings`。日本語は androidx の `values-ja`）、Enter か Space で次の止まる位置へ移ります。Compose のクリックの操作に当たります。
- 違い：Compose の 200 は毎秒のピクセルですが、lamina-ui は毎秒の dp として読みます。拡大率 1 なら同じです。Compose は止まる位置を選ぶ前にフリングの減速を走らせますが、lamina-ui は離したときの速さからすぐに選びます。← と → で分け目を 16 dp 動かすのは lamina-ui の追加です。ハンドルの大きさと色は、Compose がすぐ切り替えるところを fast spatial と fast effects のばねで動かします。

### 分割ペイン

`lamina/widgets/split_pane.hpp` と `lamina/tokens/split_pane.hpp` です。Material 3 にはデスクトップのウィンドウ向けの大きさを変えられる分割がないので、値は lamina-ui が決めました。1 dp の outline-variant の仕切り（仕切りのトークン）、大きさを変えるカーソルを出す 8 dp の当たり判定、ポインターが乗ったとき・ドラッグの間・キーボードのフォーカスがあるときに出る M3 のドラッグハンドル、WAI-ARIA のウィンドウ分割の型に沿ったキーです。矢印のキーで仕切りを 16 dp、Page Up と Page Down で 96 dp 動かし、Home と End で最小と最大へ移し、Enter で最初のペインを最小へ畳んで元に戻します。アクセシビリティ：最初のペインの大きさを値と範囲に持つ区切りです。

### スクロールの範囲：横

上の縦のスクロールの範囲に並べた lamina-ui の追加です。`set_content_width` で中身を横にもスクロールさせます。ホイールの横の軸（タッチパッドや傾けるホイール）で横へ動き、Shift を押しながら縦のホイールを回すと、中身が表示より広ければ横へ、そうでなければ縦へ動きます（`WheelEvent::shift`。`SDL_GetModState` から取る）。横のバーは下の辺に沿い、大きさは縦のバーと同じで、それぞれのバーの溝は角をもう一方に譲ります。中身が縦にも横にも表示より大きいときは、指でなぞると両方の向きに動きます（[指でのスクロール](#指でのスクロール)）。

### 数値欄

`lamina/widgets/numeric_field.hpp` と `lamina/tokens/numeric_field.hpp` です。インスペクターやプロパティの欄に向けたもので、Material 3 には詰めた数値欄がないので、値は lamina-ui が決めました。密度 0 で高さ 40 dp、最小 28 dp（一段 4 dp）、既定の幅は 96 dp、角は極小です。ラベルは始まり（欄の外、枠線の欄）か中（surface-container-highest の上、値の前）に置き、単位は終わりに置きます。欄か始まりのラベルの上を 3 dp より大きくドラッグすると、4 dp ごとに一段ずつ値が変わります。クリックすると文字を編集でき、編集は枠のない `TextField` が受け持ちます（選択・IME・クリップボード・元に戻す）。↑ と ↓ で一段ずつ（Shift で大きな段）、Page Up と Page Down で大きな段だけ変え、Enter で確定し、Escape で値に戻し、フォーカスが外れると確定します。値は範囲に収め、`decimals` の桁に丸めます。数でない文字のあいだ、欄は誤りの色のままで、無効の印が付きます。アクセシビリティ：値と単位、範囲を持つスピンボタン（`AccessibleRole::spin_button`）です。

### メニュー：ショートカットの表示・区切り線・サブメニュー

`6068a9492ba8` の C `Menu.kt`・`MenuDefaults.kt`・`tokens/ListTokens.kt` と、`9d2f79851cae` の W `menu/internal/submenu/sub-menu.ts` から、`menu_tokens` と `lamina/widgets/menu.hpp` に入れました。

- 終わりの枠：Compose の `DropdownMenuItem` に `trailingIcon` を渡すと、文字は行の残りを取って終わりに 12 dp の余白を持ち（`DropdownMenuItemHorizontalPadding`）、枠は幅が 24 dp 以上で（`ListTokens.ItemTrailingIconSize`）、on-surface-variant（`ItemTrailingIconColor`）、無効のときは on-surface の 38 % で描きます。行の文字は Label L です。lamina-ui はこの枠に、ショートカットの表示（`MenuItem::shortcut` を `format_chord` で書いたもの）かサブメニューの矢印（`keyboard_arrow_right`、24 dp）を項目の終わりに置きます。Compose にはショートカットの表示がなく、終わりの枠にキーの組み合わせを出すのは lamina-ui の決めたことです。
- 区切り線：`divider_before` は項目の前に outline-variant の 1 dp の線を、左右 12 dp、上下 2 dp をあけて引きます（`MenuDefaults.HorizontalDividerPadding`）。
- サブメニューは material-web の `md-sub-menu` に合わせました。サブメニューのある項目にポインターを乗せると 400 ms 後に開き（`hoverOpenDelay`）、ほかの項目に乗せると 400 ms 後に閉じます（`hoverCloseDelay`）。→・Enter・Space で最初の項目にフォーカスを置いて開き、←・Escape で閉じて元の項目に戻ります。サブメニューの上の始まりの角を、項目の上の終わりの角に合わせます（`anchorCorner` は START_END、`menuCorner` は START_START）。右に入らないときは項目の左に開き、ウィンドウの内側 4 dp に収まるよう上へずらします。サブメニューを開いている項目は、ポインターを乗せたときの状態の層を保ちます。サブメニューには矢印のため `MenuOptions::icon_font` が要ります。違い：項目を押すとすぐにサブメニューを開き、Tab ではメニュー全体を閉じます。Compose にはサブメニューがありません。
- 選ぶと `MenuResponse::path`（いちばん上のメニューからの番号の列）と項目の `command` を返します。`chosen` はこれまでどおり、そのメニュー自身の項目を選んだときだけ入ります。
- `open_at(point)` の位置は、Compose Desktop の `PopupPositionProviderAtPosition`（J `DesktopPopup.desktop.kt`、そのコンテキストメニューが使うもの）と同じです。メニューの上の始まりの角を点に置き、ウィンドウの右端から 4 dp 手前を越えるなら幅だけ左へ、下端から 4 dp 手前を越えるなら高さだけ上へずらし、最後に左と上から 4 dp 以上離します（`windowMargin`、`context_window_margin`）。拡大はその点を中心にします。

### コンテキストメニューの範囲

J `ContextMenuProvider.desktop.kt` と `BasicContextMenuRepresentation.desktop.kt`：Compose Desktop の `ContextMenuArea` は、二つ目のボタンを押したときにポインターの位置で開きます。`ContextMenuArea`（`lamina/widgets/context_menu.hpp`）は範囲と `Menu` を受け取り、範囲の中の `input::ContextClick` でメニューを `open_at` で開きます。SDL3 の窓は右ボタンを押したことを `PointerEvent` ではなく `Frame::context` の `ContextClick` として知らせるので、ほかの部品がクリックと取り違えることはありません。範囲の `focused` が真のあいだは、Shift+F10 かメニューキー（`requests_context_menu`）で、`keyboard_point`（既定は範囲の上の始まりの角）に最初の項目へフォーカスを置いて開きます。開く前に `before_open` がその点を受け取ります。Files の見本ではこれでポインターの下の行を選びます（`Table::handle(ContextClick)` は、その行がまだ選ばれていなければその行だけを選びます。ファイルの管理のアプリと同じです）。違い：キーボードで開くのは Windows にならった lamina-ui の追加で、Compose のこのファイルにはキーの扱いがありません。

### キーボードショートカット

J `KeyShortcut.desktop.kt`：`KeyShortcut` はキーと ctrl・meta・alt・shift で、`Ctrl+`・`Meta+`・`Alt+`・`Shift+` のあとにキーを書きます。lamina-ui の `KeyChord`（`lamina/widgets/shortcuts.hpp`）はキーと Ctrl・Shift・Alt で、`format_chord` が `lamina/tokens/shortcut.hpp` の `ShortcutFormat` で書きます。プラットフォームごとの書き方を持つのはこの一か所だけです。`pc_shortcut_format` は Windows と Linux 向けで、Ctrl・Alt・Shift の順に `+` でつなぎ、名前のあるキーは Del・Ins・PgUp・PgDn・Esc・Enter・Space・Backspace・Left・Up・F1〜F12・Plus・Minus と書きます。`mac_shortcut_format` は ⌃ ⌥ ⇧ を区切りなしで並べ、⌫ ⌦ ↩ ⎋ ⇥ ← → ↑ ↓ ⇞ ⇟ ↖ ↘ を使います。`default_shortcut_format()` がプラットフォームで選びます。lamina-ui の Ctrl はどのプラットフォームでも Control キーなので、mac の書き方では ⌃ と描きます。窓はまだ Command キーを対応させていません。

- `ShortcutRegistry` は、キーの組み合わせ（なくてもよい）・動作・有効かどうかを持つコマンドを ID で保ちます。`handle(KeyEvent)` は組み合わせがちょうど一致する有効なコマンドを走らせ、キーを使ったと返します。無効なコマンドは走らず、キーも使いません。同じ ID か同じ組み合わせのコマンドを二つ目に足すと例外を投げます。
- メニューも同じ定義で動きます。`apply_shortcuts` は項目のコマンドの組み合わせを項目に写し、`sync_commands` は有効かどうかを写します。`MenuBar` はその両方をして、選ばれた項目のコマンドを走らせます。
- フォーカス：`handle(event, focus)` は `FocusManager::claims_key` が偽のときだけコマンドを走らせるので、フォーカスのある部品は自分のものにしたキーを保ちます。フォーカスのある `TextField` と `NumericField` は Ctrl+A・Ctrl+C・Ctrl+X・Ctrl+V と、元に戻すか やり直すものがあるあいだの Ctrl+Z・Ctrl+Y（これまでどおり `handles_history_key`）を自分のものにします。フォーカスのある `Table` は Ctrl+A を自分のものにします。この振り分けは lamina-ui の決めたことで、Compose Desktop はメニューのショートカットを Swing に任せています。

### メニューバー

Compose Desktop の `MenuBar` は Swing の本物のメニューバーです（J `Menu.desktop.kt` は `JMenu` を組み、ショートカットを Swing のアクセラレーターとして渡します）。Material 3 にもメニューバーはないので、見た目は M3 の色の役割から lamina-ui が決め、`lamina/tokens/menu_bar.hpp` に置きました。surface-container-low のバーで高さ 40 dp（最小 28 dp、一段 4 dp）。項目は Label L の文字の左右に 12 dp の余白を持ち、バーの上下と始まりから 4 dp 内側にあり、角は小（8 dp）です。開いている項目は secondary-container の上に on-secondary-container の文字で、既定の effects のばねで色が移ります。ポインターを乗せたときと押したときの on-surface の状態の層と、内側のフォーカスの輪を描きます。それぞれのメニューはふつうの `Menu` で、項目の下に開きます。ウィンドウとの間は Compose の 48 dp ではなく 4 dp にしました。48 dp ではメニューバーのメニューがバーから離れてしまうためです。

- キー（Windows にならいました）：Alt だけを押して離す（窓はこれを `Key::alt` として知らせます）か F10 でバーにフォーカスが移り、アクセスキーの下線が出ます。← と → でバーの上を動き（端で折り返し）、Home と End で両端へ、↓・↑・Enter・Space でメニューを開き、文字でその文字のメニューを開きます。Alt と文字なら、どこからでもメニューを開けます。開いたメニューの中では、サブメニューがキーを使わない限り、← と → で前後のメニューを開きます。Escape でメニューを閉じてバーのフォーカスを残し、もう一度 Escape か Alt か F10 でバーを離れます。メニューが開いているあいだ、バーはすべてのキーを受け取ります。
- ポインター：項目を押すとメニューが開きます。メニューが開いているあいだにほかの項目へ動くと、そのメニューに替わります。開いている項目を押すと閉じます。項目を押したままメニューの項目までドラッグして離すと、その項目を選びます。
- アクセシビリティ：`menu_bar` の子が `menu_item` で、値は Expanded か Collapsed、開いているメニューはその項目の中に入ります。

### 表

Material 3 にはデータテーブルがありません。下の値は、MDC Web のデータテーブル（D、`6ceadb81df50` の `packages/mdc-data-table/_data-table-theme.scss` と見出しのセルの様式）にあるものはそれを M3 の役割に置き換え、ないものは lamina-ui が決めました。`lamina/tokens/table.hpp` と `lamina/widgets/table.hpp` です。

| 値 | lamina-ui | 出典 |
|---|---|---|
| 行の高さ | 52 dp、最小 36 dp（密度 −4） | D `$row-height`・`$minimum-row-height` |
| 見出しの高さ | 行の高さ + 4：56 dp、最小 40 dp | D `get-header-row-height` |
| セルの余白 | 始まりと終わりに 16 dp | D `$cell-leading-padding`・`$cell-trailing-padding` |
| 見出しの文字 | Title S、on-surface | D `subtitle2` を M3 の Title S に |
| セルの文字 | Body M、on-surface | D `body2` を M3 の Body M に |
| 行の区切り線 | outline-variant の 1 dp、最後の行の下にはなし | D の on-surface 12 % の 1 px を M3 の区切り線のトークンに |
| 選んだ行 | secondary-container、文字は on-secondary-container | M3 のリストの選んだ入れ物（D は primary の 4 %） |
| ポインターを乗せた行 | on-surface の状態の層 8 % | M3 の状態の層（D は 4 %） |
| 並べ替えの矢印 | `arrow_upward`・`arrow_downward`、18 dp、ラベルの 4 dp 後ろ（終わりにそろえる列ではラベルの前）。並べ替えた列は on-surface、ほかはポインターを乗せると on-surface-variant で現れる | D の 4 px の間とポインターで現れる動き。大きさは lamina-ui |
| 見出しの入れ物 | surface-container | lamina-ui |
| 見出しの列の区切り | outline-variant の 1 dp、上下から 16 dp | lamina-ui |
| 幅を変える範囲 | 列の終わりの端を中心に 8 dp、大きさを変えるカーソル | lamina-ui（D には列の幅の変更がない） |
| 列の幅 | 既定 160 dp、最小 56 dp | lamina-ui |

- 並べ替え：並べ替えられる見出しをクリックすると昇順、もう一度で降順になります。ほかの列をクリックするとその列の昇順です。並べ替えは安定で、列の `less`（元の行の番号で比べる）か、なければセルの文字で比べます。表は見た目の行から元の行への並びを持つので、選択とフォーカスは同じ行についていきます。
- 幅の変更：見出しの区切りをドラッグすると列の幅が変わり、`min_width` より狭くはなりません。列の合計が表より広いと、横にスクロールします。
- 選択（`TableSelection::multiple`、Windows のリストにならいました）：クリックで一行を選んで起点にし、Ctrl+クリックで一行を切り替え、Shift+クリックで起点からの範囲を選び、Ctrl+Shift+クリックで範囲を足します。行のない所をクリックすると選択を外します。ダブルクリックか Enter で開く操作になります。矢印・Home・End・Page Up・Page Down でフォーカスを動かしてその行を選び、Shift では起点から広げ、Ctrl ではフォーカスだけを動かします。Space でフォーカスの行を選び、Ctrl+Space で切り替え、Ctrl+A ですべて選びます。表は Tab の止まる所が一つで、キーを使ったあとはフォーカスの行に内側の輪を描きます。
- スクロール：見出しの下の `ScrollArea` が縦横にスクロールします（ホイール、Shift とホイール、バー）。見出しは上に残って行と一緒に横へ動き、見出しの上のホイールは行をスクロールします。
- 仮想化：見えている行と前後二行ずつだけを並べて描きます。文字は見えているあいだに一度だけ並べ、見えなくなると捨てます。読み上げ用の木には、列の見出し（並べ替えた列は Ascending か Descending を持つ）を持つ見出しの行と、見えている行とそのセルが入ります。選んだ色とポインターの層はばねで動きます。
- 作っていないもの：省略記号（セルは文字を切り取ります）、列の並べ替え、チェックボックスの列とページ分け（D にはどちらもあります）、列を中身に合わせる操作、セルの編集。

## 未決の件

- 上に挙げた部品のほかは、まだトークンを調べていません。
- 第 2 段階の部品で作っていないもの：チップの `shapes:` の版（`ChipShapes`）、2 dp の間と位置ごとの角を持つ `SegmentedListItem`、チップ・カード・リストの項目を引きずる操作、トップアプリバー自身を引きずる操作。
- 第 3 段階の部品で作っていないもの：月の間のスワイプ、Compose の vibrant な時刻の選択（`TimeScroll` と `shapes:` の版）、低い高さでの時刻の選択の文字盤の 238 dp と 200 dp の大きさ、Compose にないスライダーの形（トラックの中のアイコン、XS〜XL の大きさ）。
- 第 4 段階の画面のパターンで作っていないもの：予測型の戻る操作（シート・ドロワー・レール・検索）、アンカーへはじく動きの減速の段階、シートの中身とシートの間の入れ子のスクロール、`AppBarWithSearch`・`ExpandedFullScreenContainedSearchBar`・`ExpandedDockedSearchBarWithGap`、M3 のデザインのサイトにあるサイドシートの寸法（スクリプトなしでは読めません）。
- 第 5 段階の Expressive の部品で作っていないもの：縦のボタンの組と項目ごとのそろえ方、キーで押したときのボタンの広がり、スクロールで常に退く浮かぶツールバーと `minimumInteractiveBalancedPadding`、受け渡しの前に直線の波の表示の線を速めて走り切らせる動き、スクロールする FAB メニュー、`Modifier.animateFloatingActionButton`、`AppBarWithSearch` などの Expressive の検索バー、はみ出しを持つ `AppBarRow` と `AppBarColumn`、`CheckableDropdownMenuItem` と `SelectableDropdownMenuItem`、`ScrollField` と `Modifier.nonInteractiveScrollbar`。
- 第 6 段階の画面の広さとデスクトップ向けの作業で作っていないもの：チェックボックス・ラジオボタン・スイッチの密度、中くらいの短いナビゲーションバーの並び、ナビゲーションスイートの主な操作、浮かせたペイン、ヒンジ、予測型の戻る操作とペインの状態の保存、ペインの止まる位置を選ぶ前の減速。
- ページ送りで作っていないもの：なぞりの途中で渡すこと、端を越えたときの伸び、ホイールでのページ送り、右から左の並び、縦のページ送り。Compose のページ送りの値は、ソースがこの機械になかったので読んでいません（[ページ送り](#ページ送り)）。
- 第 7 段階のデスクトップ向けの作業で作っていないもの：メニューの先頭文字での移動、macOS の Command キー、チェックやラジオのメニュー項目、表のセルの省略記号、表の列の並べ替えと中身に合わせる操作、チェックボックスの列、ページ分け、セルの編集。
