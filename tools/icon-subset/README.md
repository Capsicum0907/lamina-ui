# icon-subset

[日本語](README.ja.md)

The build-time tool behind `lamina_add_resources` ([docs/resources.md](../../docs/resources.md)): it cuts a Material Symbols variable font down to the icons a target lists, once per icon font of the target. CMake runs it; it is not meant to be run by hand.

```
python -B -I tools/icon-subset/subset.py <source font> <manifest> <output directory>
```

The manifest, which CMake writes as `lamina_resources/<target>/icons.txt` in the build tree, has one line per icon font and one per icon:

```
font symbols
font filled FILL=1
icon e145
```

It writes `<output directory>/<id>.ttf` for each font line. The four variation axes and the name table are kept, the ligature table and hinting are dropped, and the axes on the font line are pinned (`fontTools.varLib.instancer`). It fails if the source lacks one of the codepoints. The timestamp in the font header is left as it is in the source, so the same inputs give the same bytes.

It needs fontTools, and brotli to read the WOFF2 source; `requirements.txt` pins the versions it was tried with (`python -m pip install -r tools/icon-subset/requirements.txt`).
