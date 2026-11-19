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

## 部品

### ボタン

C の `tokens/Button{XSmall,Small,Medium,Large,XLarge}Tokens.kt`（高さ・四角いときと押したときの角・アイコンの大きさと間隔・前後の余白・枠線の太さ）と `tokens/{Filled,Tonal,Outlined,Elevated,Text}ButtonTokens.kt`（状態ごとの色と高さ）を `lamina/tokens/button.hpp` に写しました。トークンのファイルではなく C の `Button.kt` に従ったところ：

- text のボタンのラベルは `primary` です。トークンのファイルは `on-surface-variant` ですが、`Button.kt` が「トークンが直ったら置き換える」として `Primary` で上書きしています。
- ラベルの文字の段階は、`ButtonDefaults.textStyleFor` と同じく高さで決めます：medium 未満は Label L、large 未満は Title M、extra-large 未満は Headline S、それ以上は Headline L。
- extra-small のボタンの前後の余白は 12 dp で、`ButtonDefaults.ExtraSmallContentPadding`（「ButtonXSmallTokens.kt の値が直ったら置き換える」）から取りました。トークンのファイルは 16 です。
- 押したときの形は、`Button.kt` が「この部品が弾まないように」使っている標準の効果のばねで動かします。最小の幅は 58 dp（`ButtonDefaults.MinWidth`）です。

### 枠線つきの入力欄

C の `tokens/OutlinedTextFieldTokens.kt`（高さ 56 dp・角は extra-small・枠線 1 dp で焦点時は 2 dp・入力とラベルは Body L・状態ごとの色）と C の `internal/TextFieldImpl.kt`（余白 16 dp・浮いたラベルは Body S）を `lamina/tokens/text_field.hpp` に写しました。選択範囲の色は `primary` の不透明度 0.4 で、C の `MaterialTheme.kt` の `TextSelectionBackgroundOpacity` から取りました。

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
- 押している間とドラッグの間、つまみは幅が半分になります（`ThumbContent`）。Compose と同じく、lamina-ui も幅を一度に変えます。Compose では焦点が当たったときにも細くなりますが、焦点は作っていません。
- キー：左右の矢印で 1 段、段が無ければ範囲の 1 % 動き、`Home` と `End` で両端へ移ります。`Page Up` と `Page Down` は作っていません。

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

C の `tokens/StandardMenuTokens.kt`（surface-container-low・選んだときの tertiary-container と on-tertiary-container・on-surface の名前・on-surface-variant のアイコン）、C の `tokens/SegmentedMenuTokens.kt`（項目 44 dp・アイコン 20 dp・角の大きさ・まとまりの間 2 dp）、C の `tokens/MenuTokens.kt`（Level2 の影）、C の `tokens/DividerTokens.kt`（1 dp・outline-variant）、C の `MenuDefaults.kt` と C の `Menu.kt`（`DropdownMenuPopup`・`DropdownMenuGroup`・`SelectableDropdownMenuItem`・`CheckableDropdownMenuItem`）を `lamina/tokens/menu.hpp` の `grouped_menu_tokens` に写しました。

- まとまり：それぞれが Level2 の影を持つ一枚の面で、中身の上下に 2 dp の余白があり、次のまとまりとは 2 dp 離れます（見本どおりの `GroupSpacing`）。メニュー全体の幅は、いちばん広い項目か見出しの幅（`IntrinsicSize.Max`）なので、どのまとまりも同じ幅です。まとまり全体を囲む入れ物はありません。
- まとまりの角は位置で決まります：一つだけなら 16 dp、最初は上 16 dp・下 8 dp、間のものは 8 dp、最後は上 8 dp・下 16 dp です。マウスが一度乗ってから離れたまとまりは角が 8 dp（`inactiveShape`）になり、角は速い空間のばねで動きます。マウスが乗っているかどうかは、`DropdownMenuGroup` の既定どおりまとまりごとに持ちます。見本はすべてのまとまりで一つの操作の元を共有していますが、lamina-ui はそうしていません。
- 項目：まとまりの左右から 4 dp 引っ込み、幅 112 から 280 dp、高さ 44 dp 以上、四方に 12 dp の余白、前に 20 dp のアイコンと文字までの 8 dp、終わりの側に 20 dp のアイコンです。これは既定で切ってある `isPrecisionPointerComponentSizingEnabled` を使わないときの値です。文字は Label L です。`Menu.kt` は `SegmentedMenuTokens.ItemLabelTextFont`（Body L）ではなく `labelLarge` を指定しています（`TODO` 付き）。
- 項目の角：一つだけなら 4 dp、最初は上 12 dp・下 4 dp、間のものは 4 dp、最後は上 4 dp・下 12 dp です。選んだ項目の角は 12 dp です。角は速い空間のばねで、入れ物の色は速い効果のばねで変わります。Compose は文字とアイコンの色を一度に変えますが、lamina-ui は同じ効果のばねで動かします。
- まとまりの見出しは Label L・on-surface で、高さ 32 dp 以上、前に 12 dp・後ろに 4 dp を空けます。その下の区切り線は、左右に 12 dp・上下に 2 dp を空けます。どちらも付けなくて構いません。
- 置き場所：従来のメニューと同じ候補を試し、窓の上下から 48 dp、左右から 8 dp 離します（`MenuHorizontalMargin`。`DropdownMenuPositionProvider` の既定値です。従来のメニューは `AndroidMenu` から 0 を受け取ります）。開け閉めの動きは従来のメニューと同じです。
- 押したときにどうするかは、Compose ではアプリが決めます。lamina-ui はまとまりごとに `MenuSelection` で選べます。`none` は項目を知らせてメニューを閉じ、`single` は一つを選び、`multiple` は切り替えます（`CheckableDropdownMenuItem` と同じ）。後の二つではメニューは開いたままです。
- まだないもの：補足の文字、押せない項目、鮮やかな色（`VibrantMenuTokens.kt`）、入れ子のメニュー、前のアイコンがない項目に選んだときだけ出るアイコン（Compose は横に広げて出します）。`selected_leading_icon` は、すでにある前のアイコンを差し替えるだけです。

### ダイアログ

C の `tokens/DialogTokens.kt`（surface-container-high・extra-large の角・Headline S・Body M・色）、C の `tokens/ScrimTokens.kt`（窓を覆う幕。色は scrim、不透明度 0.32）と C の `AlertDialog.kt`（余白 24 dp・見出しの下 16 dp・本文の下 24 dp・幅 280 から 560 dp・ボタンの間 8 dp）を `lamina/tokens/dialog.hpp` に写しました。

- ダイアログに影はありません。`AlertDialog` は影を付けず色の高さも 0 なので、`ContainerElevation`（Level3）は使われていません。
- 付けてもよいアイコン（`DialogTokens.IconSize` 24 dp・`IconColor` は secondary）は見出しの上の中央に置き、下に 16 dp を空けます（`IconPadding`）。このとき見出しも中央にそろえます（`AlertDialogContent`）。
- ボタンは `AlertDialogFlowRow` に並べたテキストボタンです。並べる向きを逆にした折り返しの行で、ボタンの間は 8 dp です。そのため一行なら閉じるボタンが確定のボタンの前に来て終わりの側にそろい、収まらなければ確定のボタンが一行目に残り、閉じるボタンが次の行に移ります。どちらも終わりの側にそろえます。Compose の行の間は、8 dp から「触れる範囲 48 dp がボタンの高さ 40 dp を超える 8 dp」を引いた値、つまり 48 dp の枠どうしの間で 0 dp です。lamina-ui はボタンを高さ 40 dp で並べるので、その 8 dp を戻し、Compose と同じく行と行は 8 dp 離れます。最後の行の下にある 4 dp の触れる範囲は足していないので、最後のボタンは下端から 24 dp です（Compose では 28 dp）。
- 見出しが複数行に折り返すとき、lamina-ui は最も長い行の幅の塊として中央に置きます。Compose の折り返す文字は与えられた最大の幅を取るはずなので、アイコンがあっても各行は始めの側から並ぶと思われます（未確認）。一行の見出しは同じです。
- Compose は現れるときと消えるときの動きを OS の窓に任せています。lamina-ui は独自に、面を 0.8 倍から標準の空間のばねで大きくし、面と幕を標準の効果のばねで現します。

## 未決の件

- 上に挙げた部品のほかは、まだトークンを調べていません。
