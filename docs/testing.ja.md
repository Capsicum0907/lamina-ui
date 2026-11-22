# テスト

[English](testing.md) · [README](../README.ja.md#資料)

## 単体テスト

テストは `tests/` にあり、分野ごとに doctest のファイルが一つあります（`tabs_test.cpp`・`side_navigation_test.cpp` など）。ビルドすると `build/Release/lamina_tests.exe` になります。`ctest --preset release` はこれと、リソースのビルドの誤りを確かめる三つのテスト（[リソース](resources.ja.md#テスト)）と、同梱の material-color-utilities の GoogleTest のテスト `mcu_tests` と、一覧の配置を確かめる `gallery_layout_412`・`gallery_layout_800`・`gallery_layout_1440`（[下](#配置の確認)）を走らせます。`lamina_tests.exe -tc="*scroll*"` で、名前の合うテストだけを走らせます。

`tests/support.hpp` には共通の道具があります：`fonts()`・`icon_font()`・`scheme()`、`settle(widget)`（1/60 秒を 240 コマ進めます）、`click(widget, x, y)`、`count_color`。部品は窓なしで確かめます。部品を作って幅と出来事を与え、時間を進めて四角形を読むか、`thorvg::Canvas` に描いて画素を読みます。

テスト用のフォントとそのライセンスは [README の出典](../README.ja.md#出典)にあります。

## 窓なしの記録

一覧は `examples/components/gallery.hpp` にあり、小さな受け口（ホスト）の向こうにあるので、`build/Release/components_capture.exe` は窓を開かずにこれを動かせます。台本どおりのポインター・キー・文字・ホイールの出来事を毎秒 60 コマの決まった間隔で与えて ThorVG で描き、操作の相手は読み上げ用の名前で探し、ポインターと最後に押したキーを自分で描き、画素を生の BGRA で書き出します（`size <scale>`、一枚なら `still <scene> <light|dark> <scale> <file>`（各ページの場面 `chips`・`chips_focus`・`chips_hover`・`chips_disabled`・`lists`・`lists_focus`・`lists_disabled`・`bars`・`bars_scrolled` があります）、動画なら ffmpeg に渡すため標準出力へ `video <scene> <light|dark> <scale>`（第 2 段階の部品には `filter_chips`・`input_chip`・`card`・`list_keys`・`badge`・`app_bar`、第 3 段階には `segments`・`range_slider`・`date_picker`・`date_range`・`time_picker` があります。第 3 段階の静止画は `inputs`・`inputs_focus`・`inputs_disabled`・`dates`・`dates_years`・`dates_input`・`dates_dialog`・`dates_range_dialog`・`dates_docked`・`dates_japanese`・`dates_focus`・`dates_disabled`・`times`・`times_24h`・`times_dialog`・`times_input_dialog`・`times_focus`・`times_disabled`、アイコンだけの区画とピッカーのアイコンボタンの名前のツールチップは `inputs_tooltip`・`dates_tooltip`・`times_tooltip` です。第 4 段階の静止画は `sheets`・`sheets_modal`・`sheets_expanded`・`sheets_side_modal`・`sheets_side_detached`・`sheets_side_standard`・`sheets_focus`・`sheets_tooltip`・`sheets_disabled`・`drawers`・`drawers_modal`・`drawers_dismissible`・`drawers_permanent`・`drawers_rail`・`drawers_hiding`・`drawers_focus`・`drawers_disabled`・`search`・`search_full`・`search_docked`・`search_focus`・`search_tooltip`・`search_disabled`、動画は `bottom_sheet`・`side_sheet`・`drawer`・`search` です。動画のドラッグとはじく動きには時刻が付いているので、離したときに速さが出ます。第 5 段階の静止画は `groups`・`groups_pressed`・`groups_overflow`・`groups_split`・`groups_focus`・`groups_tooltip`・`groups_collapsed`・`groups_disabled`・`progress`・`progress_running`・`progress_handoff`・`toolbars`・`toolbars_scrolled`・`toolbars_menu`・`toolbars_focus`・`toolbars_tooltip`・`toolbars_disabled`、動画は `button_groups`・`split_button`・`fab_menu`・`loading`・`wavy_progress`・`toolbars`、第 6 段階の静止画は `adaptive`・`adaptive_compact`・`adaptive_medium`・`adaptive_expanded`・`adaptive_large`・`adaptive_detail`・`adaptive_supporting`・`adaptive_handle`・`adaptive_focus`・`adaptive_disabled`・`desktop`・`desktop_focus`・`desktop_editor`・`desktop_editor_scrolled`・`desktop_divider`・`desktop_editing`・`desktop_disabled`、動画は `adaptive_sweep`・`adaptive_panes`・`pane_divider`・`numeric_scrub`・`timeline_scroll`・`density` です。記録の道具は、一覧が求める大きさを変えるカーソルと文字のカーソルも描きます。第 7 段階の静止画は `files`・`files_file_menu`・`files_edit_menu`・`files_context_edge`・`files_sorted`・`files_resized`・`files_scrolled`・`files_dense`・`files_disabled`、動画は `files_menubar`・`files_context`・`files_table`・`files_scroll` で、右クリック、Ctrl と Shift を押したクリック、Alt+F や Ctrl+S などのキーの組み合わせを与えます）、`tree` は場面のあとの読み上げ用の木を表示します）。画面がなくても動く開発用の道具で、CI はビルドし、配置の確認だけを走らせます。

`--window <幅>x<高さ>`（dp）で、どのモードでも窓の大きさを決めます。既定は 1440 × 860 dp です。ページの場面は、まずホイールでページのタブを見える位置まで送ります。それ以外の操作の相手が見えないところにあると、空いた場所をクリックせずに `… is out of view` と出して場面を止めます。静止画の `scrolled`（Basics のページをホイールで下へ送る）と `focus_reveal`（Tab で Name の欄へ移り、欄が見える位置までスクロールする）で、ページのスクロールを確かめられます。場面 `gallery_pages`（七つのページを順にクリック）・`gallery_pages_keys`（ページのタブを矢印のキーでたどる）・`gallery_resize`（窓の大きさの区分の境目のすぐ下と 360 dp まで窓を狭め、また広げる。窓の外は灰色で塗る）は、一覧そのものを記録するためのものです。`times_resize` は Times のページを窓の幅から 360 dp まで狭め、また広げます。静止画の `times_side_scrolled`（横向きの時刻ピッカーの上で Shift を押してホイールを回す）と `times_side_focus`（Tab でその文字盤へ移る）で、横のスクロールを確かめられます。

例：電話の幅での Basics のページの一コマと、ffmpeg に渡す動画です。

```
components_capture --window 412x860 still default light 1 basics.bgra
components_capture video gallery_resize dark 1 | ffmpeg -f rawvideo -pix_fmt bgra -s 1440x860 -r 60 -i - out.mp4
```

`components_capture size <scale>` は、同じオプションでの一コマの画素の大きさを書き出します。

### 配置の確認

`components_capture [--window <幅>x<高さ>] check` は、十五のページを順に開いて動きが落ち着くのを待ち、窓の右端をはみ出す部品、どちらかの向きに押しつぶされて大きさがなくなった部品、別の塊の部品と重なる部品をすべて書き出します。塊とは、ページが置く区画・見出し・項目のことです（`Gallery::layout_blocks`）。枠の中の部品どうしのように、一つの塊の中の部品は比べません。問題が一つでもあれば 1 で終わります。窓に収まりようのない部品は `CaptureStyle::known_overflows` に載せられます。載せた部品は、中身の左端から始まっている間は、分かっているはみ出しとして書き出すだけで数えません。ページから消えたときは確認が失敗します。今この一覧は空です。412 dp では、セグメントボタンの行と分割ボタンが使える幅を分け合い、横向きの時刻ピッカーはカードの中で横にスクロールします。ページのあとで、`CaptureStyle::redraw_target`（Basics の Send）を、押す・離す・外へ出るの三つが一つのフレームに届くタッチでタップします。ギャラリーが動いている間フレームを進め、止まったフレームと、入力のないもう一つのフレームを比べます。差が `CaptureStyle::redraw_tolerance` の段階より大きければ問題として書き出します。フレームの繰り返しは、止まったフレームを次のイベントまで出したままにするからです（[アプリの組み立て](app-structure.ja.md)）。また、見本が暗い配色かどうかを、起動したときに一度と、配色を切り替えたときにホストへ伝えることも確かめます（`GalleryHost::set_dark_theme`。窓はそれをシステムのバーに渡します。[Android](android.ja.md#システムのバー)）。CTest は、これを 412・800・1440 dp で走らせます。
