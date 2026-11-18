# icon-subset

[日本語](README.ja.md)

Cuts the Material Symbols variable font down to the symbols lamina-ui names, and writes the matching C++ table. The list of names is `NAMES` in `subset.py`; it is the only place to change.

Download `MaterialSymbolsRounded[FILL,GRAD,opsz,wght].ttf` and its `.codepoints` file from `variablefont/` of [google/material-design-icons](https://github.com/google/material-design-icons), then run (fontTools required):

```
python -B -I tools/icon-subset/subset.py <font.ttf> <font.codepoints> <repository root>
```

It writes `assets/icons/MaterialSymbolsRounded-subset.ttf` (the four variation axes and the name table are kept; ligatures and hinting are dropped) and `src/lamina/icons/symbols.hpp`. A name that is a C++ keyword gets a trailing underscore (`delete_`). Copy the repository's `LICENSE` to `assets/icons/LICENSE` and update the commit in the README attribution.
