# color-dump

[日本語](README.ja.md)

Checks `lamina/color` (`spec_2021()` and `spec_2025()`) against the Java port of material-color-utilities.

- `color-dump` (built with the library) prints the ARGB of all 53 roles for 12 seed colors × 9 variants × light and dark × contrast −1, 0, 0.5 and 1, for both specs. Seed colors given as hex arguments (`ff6750a4 ff009688 ...`) replace the built-in ones.
- `reference/Dump.java` prints the same with the `java/` directory of [material-foundation/material-color-utilities](https://github.com/material-foundation/material-color-utilities) at commit `f242efc74afc20b15753562d4afc6b6e32eb202b`, the commit `third_party/material-color-utilities` comes from. It takes the roles from `ColorSpecs.get(SPEC_2021)` and `ColorSpecs.get(SPEC_2025)` and builds the schemes with the phone platform. It prints `null` for roles a spec does not have (the dim roles in 2021).
- `reference/stubs/` holds empty versions of the `androidx.annotation` and `com.google.errorprone.annotations` annotations that the Java sources import, so they compile without those libraries.
- `compare.py` compares the two outputs. Entries that are `null` in the reference are counted as not compared.

```
javac -d out @sources.txt
java -cp out Dump > java.txt
build/Release/color-dump.exe > cpp.txt
python tools/color-dump/compare.py java.txt cpp.txt
```

`sources.txt` lists every `.java` file under the upstream `java/` directory and under `tools/color-dump/reference/` (for example `find <upstream>/java tools/color-dump/reference -name '*.java' > sources.txt`). JDK 21 was used.

Results on 2026-11-19:

- Built-in seeds: 2021, 42 336 entries compared, all equal (3 456 dim entries not in the reference). 2025, 45 792 entries compared, all equal.
- 100 random seeds, from `python -c "import random; r = random.Random(907); print(' '.join('ff%06x' % r.randrange(1 << 24) for _ in range(100)))"`: 2021, 352 800 compared, all equal. 2025, 381 600 compared, all equal. Before the vendored default viewing conditions were computed in full ([third_party/material-color-utilities](../../third_party/material-color-utilities/README.md)), 5 and 4 of them differed by one step in one channel.
