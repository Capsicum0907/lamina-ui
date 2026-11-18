# icon-subset

[English](README.md)

Material Symbols の可変フォントを lamina-ui が名前を付けた記号だけに絞り、それに合う C++ の表を書き出します。名前の一覧は `subset.py` の `NAMES` で、変えるのはそこだけです。

[google/material-design-icons](https://github.com/google/material-design-icons) の `variablefont/` から `MaterialSymbolsRounded[FILL,GRAD,opsz,wght].ttf` とその `.codepoints` を取ってきて、次のように実行します（fontTools が要ります）：

```
python -B -I tools/icon-subset/subset.py <font.ttf> <font.codepoints> <リポジトリの根>
```

`assets/icons/MaterialSymbolsRounded-subset.ttf`（4 つの可変軸と名前の表は残し、合字とヒンティングは除きます）と `src/lamina/icons/symbols.hpp` を書き出します。C++ の予約語と同じ名前には後ろに `_` を付けます（`delete_`）。取ってきたリポジトリの `LICENSE` を `assets/icons/LICENSE` に写し、README の出典のコミットを書き換えてください。
