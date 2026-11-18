# material-color-utilities（C++）

[English](README.md)

[material-foundation/material-color-utilities](https://github.com/material-foundation/material-color-utilities) の `cpp/` を、コミット `f242efc74afc20b15753562d4afc6b6e32eb202b` の時点で取り込んだものです。ライセンスは Apache License 2.0（`LICENSE`）です。2021 年の色の仕様を実装しています。

lamina-ui の 2025 年の色の仕様（`src/lamina/color/spec_2025.cpp`）は、同じコミットの Java 版の `ColorSpec2025` を、このライブラリの HCT とコントラストの関数の上に lamina-ui のコードとして移したものです。そのためにここで変えたものはありません。元の色の色相と彩度を確かめる計算には、小数 9 桁に丸めた値を持つ `kDefaultViewingConditions` ではなく `DefaultWithBackgroundLstar(50)` を使います。Java は既定の観察条件を丸めずに計算するためです（[docs/design-tokens.ja.md](../../docs/design-tokens.ja.md) の「色」）。

元のリポジトリにはビルド用のファイルがありません。`CMakeLists.txt` で、vcpkg の abseil を使って静的ライブラリ `mcu::mcu` としてビルドし、元のテストも GoogleTest で `mcu_tests` としてビルドします。`shim/` は、Google の社内向けのテストのヘッダー（`testing/base/public/gunit.h`・`gmock.h`）を GoogleTest につなぎます。

## 元からの変更

MSVC でコンパイルを通すための変更だけです。動作は変わらず、元のテスト 1045 件がすべて通ります。

| ファイル | 変更 |
|---|---|
| `cpp/cam/hct_solver.cc`・`cpp/utils/utils.cc` | `(Vec3){...}` → `Vec3{...}`（複合リテラルは GCC と Clang の拡張） |
| `cpp/cam/viewing_conditions.h` | `(ViewingConditions){...}` → `ViewingConditions{...}`（同上） |
| `cpp/score/score.h` | `0xff4285f4` → `static_cast<int>(0xff4285f4)`（既定のメンバー初期化子での縮小変換） |
| `cpp/temperature/temperature_cache.h` | `#include <optional>` を追加（間接的にしか読み込まれていなかった） |
