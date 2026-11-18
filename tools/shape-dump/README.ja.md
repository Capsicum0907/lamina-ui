# shape-dump

[English](README.md)

`lamina/shapes` を、JVM 上で動かした Google の `graphics-shapes` ライブラリと照らし合わせます。

- `shape-dump`（ライブラリと一緒にビルドされます）は、35 個の Material の形、隣り合う形どうしの変形（進み具合 0・0.3・1）、3 つの基本の形について、3 次曲線の値を書き出します。
- `reference/Ref.java` は同じものを `androidx.graphics:graphics-shapes-desktop:1.1.0` で書き出します。35 個の形は `MaterialShapes.kt` の値から組み立て直しています。このファイルは Compose 側にあり、`graphics-shapes` には入っていないためです。
- `compare.py` は二つの出力を比べます。

```
javac -cp "lib/*" -d out tools/shape-dump/reference/Ref.java
java -cp "out;lib/*" Ref > ref.txt
build/Release/shape-dump.exe > cpp.txt
python tools/shape-dump/compare.py ref.txt cpp.txt
```

`lib/` には Maven（Google Maven と Maven Central）から取った `graphics-shapes-desktop-1.1.0.jar`・`kotlin-stdlib-2.0.21.jar`・`collection-jvm-1.5.0.jar`・`annotation-jvm-1.9.1.jar` を置きます。

2026-11-19 の結果：140 件すべてで、曲線の数も値も、書き出した 9 桁まで一致しました。1.1.0 は移植元のコミットとは別の版ですが、出力は一致しています。
