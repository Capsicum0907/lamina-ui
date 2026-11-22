# 文字とフォント

[English](text-and-fonts.md) · [README](../README.ja.md#資料)

フォント、字形の並べ方と行の折り返し、可変フォントの値を決めた書体、フォントから描くアイコン、日本語の入力について書きます。

## 文字

`lamina/text/font.hpp`・`layout.hpp`：フォントと、1 行の文字・折り返す段落。文字の並べ方と字形の輪郭はどちらも HarfBuzz から取り、輪郭はほかの図形と同じく描画面で塗るので、FreeType は使いません。可変フォントは文字の段階の太さに合わせて太さを変え、`opsz` 軸があればその大きさを光学サイズにし（`text::Instance`）、フォントに無い文字は `FontStack` の次のフォントで補います。`FontStack` はフォントを `shared_ptr` で持つので、写しを作っても軽く済みます。部品はこの写しを持つので、部品を作ったときに渡した `FontStack` が部品より先に消えても構いません。フォントを持つ部品は代入もできます。段落は、欧文では空白で、日本語では文字の間で折り返し、。や 」などの約物は行頭に来ないようにします（`text/line_break.hpp`）。タブ文字は何も描かず、次のタブ位置まで進みます。タブ位置の間隔は空白 `TextStyle::tab_width` 個分（その書式での空白の幅で、字間も含みます）で、行の頭から数えます。`tokens::text_style` はこれを `tokens::tab_width_spaces`（4）にします。キャレット・クリック位置の判定・折り返しは、タブをその置かれた位置での幅で測り、単語ごとの移動ではタブを空白として扱います。単語ごとの移動や選択で使う単語は、同じ種類の文字（英字と数字・漢字・ひらがな・カタカナ・約物）の連なりです（`text/word_boundary.hpp`）。折り返しと単語の種類は、Unicode の範囲を同じ一つの表から取ります（`text/script.hpp`）。入力欄の編集の中身（文字列・選択・IME の変換中の文字・文字や単語や行ごとの移動）は `text::EditBuffer`（`text/edit_buffer.hpp`。改行とタブを残すかどうかは `EditBufferOptions` で決めます）で、フォントも描画面も使いません。行は文字の位置の範囲を並べた `text::LineMap`（`text/line_map.hpp`）として受け取り、横の位置は `text::CaretGeometry` という口を通して尋ねます。この口は、入力欄が並べ終えた行を使って実装しています。取り消しの履歴もこのバッファーが持ちます（`text::EditHistory`、`text/edit_history.hpp`）。履歴のひとまとまりごとに、編集の前と後の文字・選択の起点・カーソルを残し、取り消しとやり直しではそのどちらかに戻します。`type` は打った文字で、直前のまとまりに続けてまとめることがあります。`insert`・`erase_selection`・IME の確定は、それぞれ一つのまとまりになります。`advance(dt)` で履歴に時間を渡し、`close_step()` で打っている途中のまとまりを区切ります。履歴の上限は `EditBufferOptions::history` で渡し、渡さなければ履歴は残しません。`text::underline(fonts, style)` は、その書式で下線を引く位置を返します。`offset` はベースラインから線の上端までの下向きの距離、`thickness` は線の太さで、どちらも最初のフォントが持つ下線の値（`Font::underline_em()`）から取ります。フォントにその値が無いときは、HarfBuzz がフォントの高さの値から作ります。`text::ellipsize_line(fonts, utf8, style, max_width, ellipsis)` は、幅が `max_width` を超えない 1 行を並べます。収まるときは文字全体を、収まらないときは字形のまとまりの切れ目で終わるいちばん長い頭の部分を、末尾の空白を除いて並べ、そのあとに `ellipsis`（`tokens::ellipsis`、U+2026 の「…」）を付けます。「…」さえ収まらないときは「…」だけにします。

## 書体と埋め込む文字のフォント

`text::Typeface` は、フォントの代わりの順番と、文字の役割ごとの可変フォントの値をまとめた書体の名前です（一覧は `text::typefaces`：Google Sans Flex と Noto Sans JP、Roboto Flex と Noto Sans JP、OS のフォント）。`sdl3::typeface_stack(typeface, embedded_fonts)` は、`lamina_add_resources(... TEXT_FONTS ...)` で埋め込んだフォントから、その書体の `text::FontStack` を作ります。役割ごとに固定する軸などの詳しいことは[リソースの「文字のフォントと書体」](resources.ja.md#文字のフォントと書体)にあります。

## OS のフォント

`lamina_sdl3/system_fonts.hpp`：OS のフォントから、日本語の画面の文字に使うフォントを探します。フォントのフォルダーは環境変数（`WINDIR`・`SystemRoot`・`LOCALAPPDATA`）から決め、候補を Noto Sans JP・Yu Gothic UI・Meiryo UI・BIZ UDPゴシック・MS UI Gothic の順に試します。Android ではフォルダーは `ANDROID_ROOT` の下の `fonts`（`/system/fonts`）で、候補は Noto Sans CJK の日本語の書体（`NotoSansCJK-Regular.ttc` の 0 番目。システムの `fonts.xml` が `ja` に挙げているもの）です。一覧はどれもヘッダーに置いたデータで、OS ごとに一組ずつあります。一つも見つからなければ、試したパスを全部並べたエラーを投げます。`font_stack_from_arguments` は、コマンドラインにフォントのファイルが渡されていれば、そちらを使います。`system_fallback_stack` は例外を投げずに空のスタックを返し、`typeface_stack(typeface, embedded)` は、埋め込んだフォントと、最後の代わりとしての OS のフォントから、書体のスタックを作ります（[docs/resources.ja.md](resources.ja.md#文字のフォントと書体)）。

## アイコン

`lamina/icons/`：フォントとしての Material Symbols。`Symbol` はコードポイントで、`IconFont` は HarfBuzz から取った輪郭で記号を好きな大きさと色で描きます。フォントの可変軸（塗り・太さ・グレード・光学サイズ）も指定でき、光学サイズは指定しなければ描く大きさに合わせます。`text::Font` には `glyph_for` と、軸の値を受け取る `glyph_outline_em` を足しました。

## 日本語の入力

入力欄は、確定した文字と IME の変換中の文字を `input::TextEvent`（`TextEventKind::commit` と `TextEventKind::compose`）として `handle(TextEvent)` で受け取ります。プログラムは `sdl3::Window::set_text_input(caret_area)` でキャレットの位置を窓に伝え、IME の候補の窓はその横に開きます。フォーカスのある欄の位置は `FocusManager::caret_area()` で得られます。文字のキー（`+`・`*` など）は文字入力を始めずにキーのイベントとして届くので、かな入力の IME に取られません。行の折り返しは日本語の規則を含む UAX #14 に従います（[文字](#文字)に書いています）。

## 出典

- Google Fonts（Google Sans Flex・Roboto Flex・Noto Sans JP）：<https://github.com/google/fonts>
- Material Symbols：<https://github.com/google/material-design-icons>
- HarfBuzz：<https://github.com/harfbuzz/harfbuzz>
- Unicode の行分割のアルゴリズム（UAX #14）：<https://www.unicode.org/reports/tr14/>
- Material 3 の 文字の体系：<https://m3.material.io/styles/typography/overview>
- 値ごとのソースのファイルとコミット：[デザイントークン](design-tokens.ja.md)
