# icon-subset

[English](README.md)

`lamina_add_resources`（[docs/resources.ja.md](../../docs/resources.ja.md)）の裏でビルドの段階に動く道具です。Material Symbols の可変フォントを、ターゲットが書いたアイコンだけに絞り、ターゲットのアイコンのフォントごとに一つずつ書き出します。CMake が実行するもので、手で動かすことは考えていません。

```
python -B -I tools/icon-subset/subset.py <元のフォント> <一覧> <書き出し先のフォルダー>
```

一覧は CMake がビルドのフォルダーに `lamina_resources/<ターゲット>/icons.txt` として書き、アイコンのフォントごとに一行、アイコンごとに一行あります。

```
font symbols
font filled FILL=1
icon e145
```

`font` の行ごとに `<書き出し先のフォルダー>/<id>.ttf` を書き出します。4 つの可変軸と名前の表は残し、合字の表とヒンティングは除き、`font` の行にある軸は固定します（`fontTools.varLib.instancer`）。元のフォントに無いコードポイントがあると失敗します。フォントのヘッダーの時刻は元のフォントのまま残すので、同じ入力からは同じバイトが出ます。

fontTools と、WOFF2 の元のフォントを読むための brotli が要ります。`requirements.txt` に、試した版を固定して書いています（`python -m pip install -r tools/icon-subset/requirements.txt`）。
