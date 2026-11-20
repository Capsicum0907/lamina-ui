# Material Symbols Rounded

[English](README.md)

Google の Material Symbols Rounded の可変フォントです。[google/material-design-icons](https://github.com/google/material-design-icons) のコミット `10ecc14790612ca8fd164e3f4ffe9c16fc50d5b7` の `variablefont/` から取り、Apache License 2.0 に従います（`LICENSE` はそのリポジトリのライセンスのファイルです）。`lamina_add_resources` は、ビルドの段階でここからプログラムごとのアイコンのフォントを切り出します（[docs/resources.ja.md](../../docs/resources.ja.md)）。

| ファイル | 元の名前 | 大きさ |
|---|---|---|
| `MaterialSymbolsRounded.woff2` | `MaterialSymbolsRounded[FILL,GRAD,opsz,wght].woff2` | 5 402 952 バイト |
| `MaterialSymbolsRounded.codepoints` | `MaterialSymbolsRounded[FILL,GRAD,opsz,wght].codepoints` | 79 421 バイト |

中身は変えていません。角かっことコンマは CMake のパスで扱いにくいので、名前だけ短くしました。WOFF2 は、同じコミットの 15 235 612 バイトの TTF と同じフォントを、約 3 分の 1 の大きさで持っています。ここから切り出したものは、TTF から切り出したものとバイト単位で同じです。
