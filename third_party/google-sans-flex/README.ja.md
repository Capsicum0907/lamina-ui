# Google Sans Flex

[English](README.md)

可変フォントの Google Sans Flex です。[google/fonts](https://github.com/google/fonts) のコミット `06bfe0d62a98375cb09c942fa18e68bd32cff057` の `ofl/googlesansflex/` から取り、SIL Open Font License 1.1（`OFL.txt`）に従います。`TRADEMARKS.md`・`METADATA.pb`・`upstream_info.md` は同じディレクトリのファイルです。lamina-ui は Google と関係がなく、Google の推奨も受けていません。ここで「Google Sans Flex」と書くのは、`TRADEMARKS.md` が認めるとおり、元のフォントを指すためです。

| ファイル | 元の名前 | 大きさ |
|---|---|---|
| `GoogleSansFlex.ttf` | `GoogleSansFlex[ROND,opsz,slnt,wdth,wght].ttf` | 3 910 784 バイト |

フォントはバイト単位で変えておらず、プログラムにもそのまま埋め込みます（切り出しも変換もしません）。角かっことコンマは CMake のパスで扱いにくいので、名前だけ短くしました。軸は `opsz` 1〜144・`wdth` 25〜151・`wght` 1〜1000・`ROND` 0〜100・`slnt` -10〜0 です（[docs/resources.ja.md](../../docs/resources.ja.md)）。
