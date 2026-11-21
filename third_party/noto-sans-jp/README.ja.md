# Noto Sans JP

[English](README.md)

可変フォントの Noto Sans JP です。[google/fonts](https://github.com/google/fonts) の `ofl/notosansjp/` から 2026-11-15 に取ったもので（`source.txt` に各ファイルの大きさと MD5 があります）、SIL Open Font License 1.1（`OFL.txt`）に従います。`METADATA.pb` は同じディレクトリのファイルで、元の [notofonts/noto-cjk](https://github.com/notofonts/noto-cjk) のコミットが書いてあります。

| ファイル | 元の名前 | 大きさ |
|---|---|---|
| `NotoSansJP.ttf` | `NotoSansJP[wght].ttf` | 9 589 900 バイト |

フォントは変えておらず、プログラムにもそのまま埋め込みます。角かっこは CMake のパスで扱いにくいので、名前だけ短くしました。軸は `wght` 100〜900 の一つです（[docs/resources.ja.md](../../docs/resources.ja.md)）。
