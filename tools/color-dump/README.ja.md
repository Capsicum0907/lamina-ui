# color-dump

[English](README.md)

`lamina/color`（`spec_2021()` と `spec_2025()`）を、material-color-utilities の Java 版と照らし合わせます。

- `color-dump`（ライブラリと一緒にビルドされます）は、12 個の元の色 × 9 種類の配色 × ライトとダーク × コントラスト −1・0・0.5・1 について、二つの仕様それぞれで 53 個の役割の ARGB を書き出します。引数に 16 進で元の色（`ff6750a4 ff009688 ...`）を渡すと、組み込みの色の代わりにそれを使います。
- `reference/Dump.java` は同じものを、[material-foundation/material-color-utilities](https://github.com/material-foundation/material-color-utilities) の `java/`（コミット `f242efc74afc20b15753562d4afc6b6e32eb202b`。`third_party/material-color-utilities` の取り込み元と同じコミット）で書き出します。役割は `ColorSpecs.get(SPEC_2021)` と `ColorSpecs.get(SPEC_2025)` から取り、配色はスマートフォン向けの設定で作ります。その仕様に無い役割（2021 年の dim の役割）は `null` と書きます。
- `reference/stubs/` には、Java のソースが読み込む `androidx.annotation` と `com.google.errorprone.annotations` の注釈を中身の無い形で置いてあります。これで、そのライブラリが無くてもコンパイルできます。
- `compare.py` は二つの出力を比べます。照合元で `null` の項目は比べなかった数として数えます。

```
javac -d out @sources.txt
java -cp out Dump > java.txt
build/Release/color-dump.exe > cpp.txt
python tools/color-dump/compare.py java.txt cpp.txt
```

`sources.txt` には、取り込み元の `java/` と `tools/color-dump/reference/` の下にある `.java` をすべて並べます（例：`find <取り込み元>/java tools/color-dump/reference -name '*.java' > sources.txt`）。JDK 21 を使いました。

2026-11-19 の結果：

- 組み込みの元の色：2021 年は 42 336 件を比べてすべて一致しました（照合元に無い dim の 3 456 件は除く）。2025 年は 45 792 件を比べてすべて一致しました。
- 乱数で選んだ 100 個の元の色（`python -c "import random; r = random.Random(907); print(' '.join('ff%06x' % r.randrange(1 << 24) for _ in range(100)))"` で作ったもの）：2021 年は 352 800 件のうち 5 件、2025 年は 381 600 件のうち 4 件が違いました。どれも一つのチャンネルが 1 段ずれただけで、原因は `spec_2021()` と 2025 年の代替の種類が使う、取り込んだ C++ の HCT の計算にあります。その既定の観察条件（`kDefaultViewingConditions`）は小数 9 桁に丸めてあり、Java は丸めずに計算するので、変換した色相が最大で 10⁻⁵ 程度ずれます。`ff3489fb` の Vibrant・ライト・コントラスト 1 の primary-fixed-dim では、丸めずに計算した観察条件の色相を primary の系列に与えると、取り込んだコードでも Java と同じ値になりました。2025 年の 4 件は、2025 年が 2021 年のものをそのまま使う Fidelity・Content・Rainbow で出ています。Neutral・TonalSpot・Vibrant・Expressive の 2025 年の規則には違いがありませんでした。
