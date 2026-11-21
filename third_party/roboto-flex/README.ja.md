# Roboto Flex

[English](README.md)

可変フォントの Roboto Flex です。[google/fonts](https://github.com/google/fonts) のコミット `06bfe0d62a98375cb09c942fa18e68bd32cff057` の `ofl/robotoflex/` から取り、SIL Open Font License 1.1（`OFL.txt`）に従います。`METADATA.pb` と `upstream_info.md` は同じディレクトリのファイルです。

| ファイル | 元の名前 | 大きさ |
|---|---|---|
| `RobotoFlex.ttf` | `RobotoFlex[GRAD,XOPQ,XTRA,YOPQ,YTAS,YTDE,YTFI,YTLC,YTUC,opsz,slnt,wdth,wght].ttf` | 1 787 292 バイト |

フォントは変えておらず、プログラムにもそのまま埋め込みます。角かっことコンマは CMake のパスで扱いにくいので、名前だけ短くしました。lamina-ui が動かすのは `wght` と `opsz`（8〜144）で、ほかの軸はプログラムが固定しない限り既定の値のままです（[docs/resources.ja.md](../../docs/resources.ja.md)）。
