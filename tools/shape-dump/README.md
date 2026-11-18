# shape-dump

[日本語](README.ja.md)

Checks `lamina/shapes` against Google's `graphics-shapes` library running on the JVM.

- `shape-dump` (built with the library) prints the cubics of the 35 Material shapes, of morphs between each adjacent pair at progress 0, 0.3 and 1, and of three factory shapes.
- `reference/Ref.java` prints the same with `androidx.graphics:graphics-shapes-desktop:1.1.0`. It rebuilds the 35 shapes from the parameters in `MaterialShapes.kt`, because that file belongs to Compose, not to `graphics-shapes`.
- `compare.py` compares the two outputs.

```
javac -cp "lib/*" -d out tools/shape-dump/reference/Ref.java
java -cp "out;lib/*" Ref > ref.txt
build/Release/shape-dump.exe > cpp.txt
python tools/shape-dump/compare.py ref.txt cpp.txt
```

`lib/` holds `graphics-shapes-desktop-1.1.0.jar`, `kotlin-stdlib-2.0.21.jar`, `collection-jvm-1.5.0.jar` and `annotation-jvm-1.9.1.jar` from Maven (Google Maven and Maven Central).

Result on 2026-11-19: all 140 entries have the same number of cubics and the same values to the 9 printed digits. Version 1.1.0 is not the commit the port was made from, but the outputs agree.
