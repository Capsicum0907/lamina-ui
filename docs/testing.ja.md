# テスト

[English](testing.md) · [README](../README.ja.md#資料)

## 単体テスト

テストは `tests/` にあり、分野ごとに doctest のファイルが一つあります（`tabs_test.cpp`・`side_navigation_test.cpp` など）。ビルドすると `build/Release/lamina_tests.exe` になります。`ctest --preset release` はこれと、リソースのビルドの誤りを確かめる三つのテスト（[リソース](resources.ja.md#テスト)）と、同梱の material-color-utilities の GoogleTest のテスト `mcu_tests` と、一覧の配置を確かめる `gallery_layout_412`・`gallery_layout_800`・`gallery_layout_1440`（[下](#配置の確認)）を走らせます。`lamina_tests.exe -tc="*scroll*"` で、名前の合うテストだけを走らせます。

`tests/support.hpp` には共通の道具があります：`fonts()`・`icon_font()`・`scheme()`、`settle(widget)`（1/60 秒を 240 コマ進めます）、`click(widget, x, y)`、`count_color`。部品は窓なしで確かめます。部品を作って幅と出来事を与え、時間を進めて四角形を読むか、`thorvg::Canvas` に描いて画素を読みます。

テスト用のフォントとそのライセンスは [README の出典](../README.ja.md#出典)にあります。

## 窓なしの記録

一覧は `examples/components/gallery.hpp` にあり、小さな受け口（ホスト）の向こうにあるので、`build/Release/components_capture.exe` は窓を開かずにこれを動かせます。台本どおりのポインター・キー・文字・ホイールの出来事を毎秒 60 コマの決まった間隔で与えて ThorVG で描き、操作の相手は読み上げ用の名前で探し、ポインターと最後に押したキーを自分で描き、画素を生の BGRA で書き出します（`size <scale>`、一枚なら `still <scene> <light|dark> <scale> <file>`（各ページの場面 `chips`・`chips_focus`・`chips_hover`・`chips_disabled`・`lists`・`lists_focus`・`lists_disabled`・`bars`・`bars_scrolled` があります）、動画なら ffmpeg に渡すため標準出力へ `video <scene> <light|dark> <scale>`（第 2 段階の部品には `filter_chips`・`input_chip`・`card`・`list_keys`・`badge`・`app_bar`、第 3 段階には `segments`・`range_slider`・`date_picker`・`date_range`・`time_picker` があります。第 3 段階の静止画は `inputs`・`inputs_focus`・`inputs_disabled`・`dates`・`dates_years`・`dates_input`・`dates_dialog`・`dates_range_dialog`・`dates_docked`・`dates_japanese`・`dates_focus`・`dates_disabled`・`times`・`times_24h`・`times_dialog`・`times_input_dialog`・`times_focus`・`times_disabled`、アイコンだけの区画とピッカーのアイコンボタンの名前のツールチップは `inputs_tooltip`・`dates_tooltip`・`times_tooltip` です。第 4 段階の静止画は `sheets`・`sheets_modal`・`sheets_expanded`・`sheets_side_modal`・`sheets_side_detached`・`sheets_side_standard`・`sheets_focus`・`sheets_tooltip`・`sheets_disabled`・`drawers`・`drawers_modal`・`drawers_dismissible`・`drawers_permanent`・`drawers_rail`・`drawers_hiding`・`drawers_focus`・`drawers_disabled`・`search`・`search_full`・`search_docked`・`search_focus`・`search_tooltip`・`search_disabled`、動画は `bottom_sheet`・`side_sheet`・`drawer`・`search` です。動画のドラッグとはじく動きには時刻が付いているので、離したときに速さが出ます。第 5 段階の静止画は `groups`・`groups_pressed`・`groups_overflow`・`groups_split`・`groups_focus`・`groups_tooltip`・`groups_collapsed`・`groups_disabled`・`progress`・`progress_running`・`progress_handoff`・`toolbars`・`toolbars_scrolled`・`toolbars_menu`・`toolbars_focus`・`toolbars_tooltip`・`toolbars_disabled`、動画は `button_groups`・`split_button`・`fab_menu`・`loading`・`wavy_progress`・`pull_refresh`（Progress のページのリストで、しきい値に届かない引っ張り、しきい値を越えて更新して戻るまでの引っ張り、離す前に押し戻す引っ張りを、指で行います。そのあと Arrow indicator のスイッチを入れ、矢印の形でもう二回引っ張ります）・`toolbars`、第 6 段階の静止画は `adaptive`・`adaptive_compact`・`adaptive_medium`・`adaptive_expanded`・`adaptive_large`・`adaptive_detail`・`adaptive_supporting`・`adaptive_handle`・`adaptive_focus`・`adaptive_disabled`・`desktop`・`desktop_focus`・`desktop_editor`・`desktop_editor_scrolled`・`desktop_divider`・`desktop_editing`・`desktop_disabled`、動画は `adaptive_sweep`・`adaptive_panes`・`pane_divider`・`numeric_scrub`・`timeline_scroll`・`density` です。記録の道具は、一覧が求める大きさを変えるカーソルと文字のカーソルも描きます。第 7 段階の静止画は `files`・`files_file_menu`・`files_edit_menu`・`files_context_edge`・`files_sorted`・`files_resized`・`files_scrolled`・`files_dense`・`files_disabled`、動画は `files_menubar`・`files_context`・`files_table`・`files_scroll` で、右クリック、Ctrl と Shift を押したクリック、Alt+F や Ctrl+S などのキーの組み合わせを与えます。拡大の表示の静止画は `viewer`・`viewer_zoomed`（二本の指で広げる）・`viewer_panned`（広げたあと一本の指ではじいて端まで送る）・`viewer_double_tap`・`viewer_disabled`、動画は `viewer_pinch` です。ページ送りの静止画は `pages_swiping`（Basics から Chips への途中で指を止めたもの）・`pages_swiped`・`viewer_photos`・`viewer_photo_swiping`・`viewer_photo_swiped`、動画は `pages_swipe` と `viewer_photos` です。これらの場面は `Frame::fingers` と、SDL が最初の指から作るポインターのイベントを一緒に送ります）、`tree` は場面のあとの読み上げ用の木を表示します）。画面がなくても動く開発用の道具で、CI はビルドし、配置の確認だけを走らせます。

`--window <幅>x<高さ>`（dp）で、どのモードでも窓の大きさを決めます。既定は 1440 × 860 dp です。ページの場面は、まずホイールでページのタブを見える位置まで送ります。それ以外の操作の相手が見えないところにあると、空いた場所をクリックせずに `… is out of view` と出して場面を止めます。静止画の `scrolled`（Basics のページをホイールで下へ送る）と `focus_reveal`（Tab で Name の欄へ移り、欄が見える位置までスクロールする）で、ページのスクロールを確かめられます。場面 `gallery_pages`（七つのページを順にクリック）・`gallery_pages_keys`（ページのタブを矢印のキーでたどる）・`gallery_resize`（窓の大きさの区分の境目のすぐ下と 360 dp まで窓を狭め、また広げる。窓の外は灰色で塗る）は、一覧そのものを記録するためのものです。`times_resize` は Times のページを窓の幅から 360 dp まで狭め、また広げます。静止画の `times_side_scrolled`（横向きの時刻ピッカーの上で Shift を押してホイールを回す）と `times_side_focus`（Tab でその文字盤へ移る）で、横のスクロールを確かめられます。

例：電話の幅での Basics のページの一コマと、ffmpeg に渡す動画です。

```
components_capture --window 412x860 still default light 1 basics.bgra
components_capture video gallery_resize dark 1 | ffmpeg -f rawvideo -pix_fmt bgra -s 1440x860 -r 60 -i - out.mp4
```

`components_capture size <scale>` は、同じオプションでの一コマの画素の大きさを書き出します。

`components_capture bench <scene> <light|dark> <scale> <start dp> <distance dp> <seconds> [repeats] [out.bgra]` は、窓なしでページを描く時間を測ります。場面の後、ページを指で `<start dp>` だけ上へ送り（窓の高さの `bench_from` から `bench_to` まで、幅の `bench_x` の位置でなぞる。はじかないように毎回指を止めてから離し、落ち着くのを待つ）、それから `<seconds>` 秒で `<distance dp>` 上へなぞって指をそこに止めます。これを `repeats` 回、上と下へ交互にくり返し、なぞるたびに[フレームの計時](app-structure.ja.md#フレームの計時)の 1 行を書き出します。測るのは指が動いている間のフレームだけです。`update`・`paint`・`raster update`・`raster draw` はアプリと同じように測り、`upload` と `present` は 0 です。表示を待たないので、間隔は前のフレームの終わりから次のフレームの終わりまでです。`out.bgra` を渡すと最後のフレームを書き出します。スマホの大きさ（1080 × 2400 px、倍率 2.625）では次のとおりです。

```
components_capture --window 412x915 bench default light 2.625 0 300 1 4
components_capture --window 412x915 bench default light 2.625 1200 300 1 4
```

一つ目は Basics の上のほう（FAB が見える）、二つ目は Type と Theme の節をなぞります。描くのはデスクトップの CPU なので、時間は変更どうしを比べるためのもので、スマホでの時間ではありません。

`--full-repaint` を付けると[差分の描き直し](app-structure.ja.md#差分の描き直し)を切り、毎フレーム全体を描きます。時間を比べるときに使います。`--verify-repaint` は `bench` と `video` で使えて、フレームごとに 1 行を書き出します。中身は、フレームの番号、バッファー全体の 64 ビットのハッシュ（`render::Fingerprint`）、描き直した画素の数、前のフレームから変わった画素のうち描画面が描き直したと伝えた長方形の外にあるものの数です。長方形の外の画素は、窓が転送しない画素です。`video` ではこのとき動画を書き出しません。最後に `verify` の行を書き、長方形の外で変わった画素が一つでもあれば 1 で終わります。台本のフレームは何度流しても同じなので、同じ場面を `--full-repaint` ありとなしで流してフレームごとにハッシュを比べると、差分の描き直しが全体を描いたときと同じ画素になったかが分かります。一度の実行で同じフレームを二度描いて比べる方法は使えません。ギャラリーの `paint` がアニメーションを進めるためです。

### 配置の確認

`components_capture [--window <幅>x<高さ>] check` は、十六のページを順に開いて動きが落ち着くのを待ち、窓の右端をはみ出す部品、どちらかの向きに押しつぶされて大きさがなくなった部品、別の塊の部品と重なる部品をすべて書き出します。塊とは、ページが置く区画・見出し・項目のことです（`Gallery::layout_blocks`）。枠の中の部品どうしのように、一つの塊の中の部品は比べません。問題が一つでもあれば 1 で終わります。窓に収まりようのない部品は `CaptureStyle::known_overflows` に載せられます。載せた部品は、中身の左端から始まっている間は、分かっているはみ出しとして書き出すだけで数えません。ページから消えたときは確認が失敗します。今この一覧は空です。412 dp では、セグメントボタンの行と分割ボタンが使える幅を分け合い、横向きの時刻ピッカーはカードの中で横にスクロールします。ページのあとで、`CaptureStyle::redraw_target`（Basics の Send）を、押す・離す・外へ出るの三つが一つのフレームに届くタッチでタップします。ギャラリーが動いている間フレームを進め、止まったフレームと、入力のないもう一つのフレームを比べます。差が `CaptureStyle::redraw_tolerance` の段階より大きければ問題として書き出します。フレームの繰り返しは、止まったフレームを次のイベントまで出したままにするからです（[アプリの組み立て](app-structure.ja.md)）。続けて、タッチでページを上にはじく操作を `CaptureStyle::fling_redraw_flings` 回行います（窓の高さの `fling_redraw_from` から `fling_redraw_to` まで、`fling_redraw_seconds` 秒で）。はじくたびに止まるまで待ち、同じように止まったフレームと次のフレームを比べます。見本が進めたあとにもう一度配置するようになる前は、はじく動きの最後の一歩が、そのフレームの配置のあとでスクロール位置を動かしていました。そのため止まったフレームには、その一歩の始まりの位置のページが描かれていました（412 dp で 253 段階の差）。実機では、スクロールバーを隠すタイマーが窓を起こすまで、ページが終わりの手前に描かれたままでした。戻る操作も、新しく作った見本で三通り確かめます。何も開いていなければ、一回目の戻る操作がホストに渡ります（`GalleryHost::system_back`）。`CaptureStyle::back_menu` に Tab で移って Enter で開いたあとは、一回目でメニューが閉じ、二回目だけが渡ります。`CaptureStyle::back_field` に Tab で移ったあとは、一回目で欄のフォーカスが外れ、二回目だけが渡ります。確かめで開くどのページでも、何も開いていなければ一回目が渡ります。`CaptureStyle::back_dialog` に Tab で移って Enter で開いたあとと、`CaptureStyle::back_sheet_scene` の場面（モーダルのボトムシート）のあとは、一回目は渡らず、`CaptureStyle::back_press_limit` 回までのどこかで渡ります。`KeyEvent::back` の立っていない Escape は渡りません。また、見本が暗い配色かどうかを、起動したときに一度と、配色を切り替えたときにホストへ伝えることも確かめます（`GalleryHost::set_dark_theme`。窓はそれをシステムのバーに渡します。[Android](android.ja.md#システムのバー)）。CTest は、これを 412・800・1440 dp で走らせます。

## Windows で指の入力を送る

`tools/touch-inject/` は、Windows で本物の lamina の窓に作り物の指の入力を送ります。タッチ画面のない機械でも、SDL の指の経路・自分で描くタイトルバー・タッチキーボードを確かめられます。Python と ctypes と Pillow で書いてあり、`__pycache__` を書かないよう `python -B` で動かします。処理（窓を探す・送る前の確かめ・`InjectTouchInput`・窓の撮影）は `inject.py` にあり、`cli.py` は引数を読むだけです。

```
build/Release/components.exe --title-bar
python -B tools/touch-inject/cli.py --pid <pid> info
python -B tools/touch-inject/cli.py --pid <pid> run tools/touch-inject/scenarios/basics-send.toml --out shots --log shots/log.jsonl
python -B tools/touch-inject/cli.py --pid <pid> run --out shots --step 'action="tap", x=170, y=562' --step 'action="capture", name="switch"'
python -B tools/touch-inject/cli.py --pid <pid> capture shot.png
python -B tools/touch-inject/cli.py --pid <pid> restore
```

相手の窓は、`--pid` のプロセスの見えている最上位の窓（一つだけ）です。手順は `[[step]]` の表を並べた TOML のファイルに書きます。`--step` は表を一つずつ足します。両方あるときはファイルの手順が先です。座標はクライアント領域の左上から測り、既定は dp です（画素なら `unit = "px"`）。dp から画素へは `GetDpiForWindow` で直します。動きの道筋は、指が触れたときに画面の画素に直します。なので、タイトルバーを引きずると、指は画面の上の道筋のまま動き、窓がそれについてきます。

| 手順 | 項目 |
|---|---|
| `tap` | `x`・`y`・`hold`（既定は `timing.tap_hold_seconds`） |
| `press` | `tap` と同じ。`hold` の既定は `timing.long_press_seconds` |
| `drag` | `from`・`to`（`[x, y]`。指を何本も使うときはその並び）・`duration`・`profile`（`linear`・`ease_out`・`ease_in`）・`hold_start`・`hold_end`。`hold_end = 0` なら動いたまま指を離すので、はじく動きになります |
| `capture` | `name`・`settle`（撮る前に待つ秒数） |
| `wait` | `seconds` |
| `activate` | 窓を前面に出し、最小化から戻します。最小化されていれば `ShowWindow(SW_RESTORE)`、次に `SetForegroundWindow`、Windows が断ったら最小化して元に戻します。待つのは最大で `timing.foreground_wait_seconds` 秒で、その間に `timing.foreground_polls` 回確かめます |
| `resize` | クライアント領域の `width`・`height` |
| `restore` | `ShowWindow(SW_RESTORE)`。最小化のあとに使います |
| `watch` | `watch.keyboard_processes` のプロセスの最上位の窓を、DWM で隠されているかどうかと一緒に記録します |
| `info` | DPI の倍率とクライアント領域の四角形を記録します |

`tap`・`press`・`drag` には `gap`（動きのあとの待ち）と `snapshot` が付けられます。`snapshot` は `{at, name}` の並びで、指が触れてから `at` 秒の時点で、指を置いたまま撮ります。コマの間は `injection.frame_interval_seconds` 秒です。コマの間の待ちは、残りが `timing.spin_threshold_seconds` 秒を切るまで残りの `timing.sleep_fraction` だけ眠り、そのあとは回って待ちます。既定の値はすべて `tools/touch-inject/defaults.toml` にあります（`--settings` で別のファイルを読めます）。

指の入力を一コマ送るたびに、次を確かめます：プロセスの窓が同じ一つのままか、それが前面の窓で最小化されていないか、点がそのクライアント領域の中で画面の上にあるか、その点の `WindowFromPoint` がその窓のものか。一つでも外れたら、置いている指を取り消して止まり、`run` は 3 で終わります。撮影は `PrintWindow` に `PW_CLIENTONLY | PW_RENDERFULLCONTENT` を付けて行うので、写るのはその窓のクライアント領域だけで、ほかの窓は写りません。

タッチ画面のない Windows 11（SDL 3.4.10）で分かったこと：

- `InitializeTouchInjection` と `InjectTouchInput` は、デジタイザーがなくても（`SM_DIGITIZER` が 0）動きます。SDL は指を `WM_TOUCH` で受け、指の出来事と、そこから作るマウスの出来事（`SDL_TOUCH_MOUSEID`）を出します。Windows が指から作るマウスのメッセージは捨てます（`GetMouseMessageSource`）。タップでスイッチは一度だけ切り替わります。
- 指のあとのホバー。直しました。直す前に見たこと：タップのあと、カーソル（`GetCursorPos`）はタップした点にあり、その点の部品が次に指で触るまでマウスのホバーの見た目になりました。最小化から戻したとき、大きさを変えたとき、道具が窓を何も変えていないときです。メッセージのフック（`SDL_SetWindowsMessageHook`。残していないビルド）で記録を取ると、毎回 Windows がカーソルの位置へ、指の印のない `WM_MOUSEMOVE`（`GetMessageExtraInfo` が 0）を送り、SDL がそれをマウス（`SDL_GLOBAL_MOUSE_ID`）として渡していました。この動きが来たのは、指を置いてから離すまでのあいだか離した直後（ホバーが残るのは離したあとに来たときだけで、そのため出たり出なかったりしました）、大きさを変えたり窓を動かしたりした直後（`SetWindowPos`。動かしたあとは、クライアント領域では別の点、画面では同じ点に来ます）、最小化から戻したあと（SDL がマウスが窓に入ったとして、覚えていた位置を忘れたあと）、そして道具自身のプロセスが始まったり終わったりしたときです（時刻が重なるだけです）。ドラッグの途中では、指が 10 px 前に通り過ぎた点への動きも来ました。記録した最小化からの戻しでは、動きは Windows から来ていました。ソースでは、窓が前面になったときに SDL の `WIN_UpdateFocus` も一つ出します。SDL は位置の変わらない動きを捨てますが、これらは通ります。SDL が指から作る位置には画素の端数があり（296.002）、カーソルにはないからです（296）。今は、窓がポインターのイベントを `input::TouchEchoFilter` に通します（[指のあとのマウスのこだま](design-tokens.ja.md#指のあとのマウスのこだま)）。これで、タップのあと、ほかのプロセスが動くあいだ待ったあと、タイトルバーで最小化して戻したあと、大きさを変えたあと、カーソルの下で窓を 20 × 5 px 動かしたあとのどれでも、ホバーは出ませんでした。`SendInput` で送ったマウスの動き（8 × 4 px。指と同じ確かめをしてから送りました）で、ホバーはすぐ戻りました。
- 最小化から戻した窓が、とても小さな大きさから並べ直していました。直しました。直す前に見たこと：`--title-bar` の見本では、最小化している間のクライアント領域は 160 × 28 px です。戻すと、中身が縮んだ並びからばねで元へ戻り、表示に幅の区分がもう一度出ました。戻した直後に送ったタップは、ねらった部品に当たりませんでした（3 回とも）。システムの枠のある窓では、最小化している間のクライアント領域は 0 × 0 で、中身は動きませんでした。ソースで分かること：SDL は、最小化した窓の大きさを出来事では無視します（`WM_WINDOWPOSCHANGED`）。ですが `SDL_GetWindowSizeInPixels` は、幅が 0 より大きければ `GetClientRect` の値を返し、そうでなければ前の大きさを返します。`Window::measure` はその大きさを毎コマ読んでいました。今は、SDL が窓に `SDL_WINDOW_MINIMIZED` を立てている間、`Window::measure` は前のコマの大きさを保ちます（`layout::frame_pixel_size`。どの OS でも同じです）。戻した直後のコマから元の並びが出て、戻した直後のタップも当たります（3 回とも）。
- 二本の指は、それぞれの番号つきの SDL の指のイベントとして lamina に届きます（`Frame::fingers`）。タッチのポインターのイベントとしても届くのは最初の指だけです。`scenarios/viewer-pinch.toml` は Viewer のページを開き、240 dp 離した二本の指を 440 dp まで広げ、拡大した模様を一本の指ではじき、120 dp までつまんで縮め、二度たたいて、もう一度二度たたきます。見たこと：指が動いている間、模様は二本の指の真ん中を中心に大きくなりました。はじいた模様は右の端で止まりました。つまむと、収まる大きさより小さくなってから、収まって中央にそろう大きさへ戻りました。一回目の二度たたきで 250 % に拡大し、二回目で収まる大きさに戻りました。
- ページ送り：`scenarios/swipe-tabs.toml`・`swipe-photos.toml`・`swipe-keeps-controls.toml`。起動したばかりの 1440 × 860 dp の見本に合わせてあります。見たこと：Basics で 400 dp を 0.25 秒でなぞると Chips へ送りました。400 dp なぞって離す前に止めると戻り、1000 dp なぞると Basics へ進みました。指が触れている間、タブの印は二つのタブの間にありました。Viewer のタブをタップすると、手前のページから滑ってきました。Viewer のページでは、写真のページ送りの上から始めた縦のなぞりでページがスクロールしました。なぞると写真 2 へ送り、二度たたくと 250 % に拡大し、左へなぞると右端のゴムの手応えの中まで動いて端に落ち着きました。次に左へなぞるとページ送りへ渡り、写真 2 を拡大したまま写真 3 へ滑り、写真 3 が止まると写真 2 は収まる大きさに戻りました。二本の指を横に広げると、ページは送らずに写真 3 を拡大しました。写真 1 で右へなぞると Desktop のページへ移りました。Basics のスライダーと Times のページの横の文字盤は、なぞると動き、ページはそのままでした。分かったこと：なぞりの途中の撮影（`snapshot`）は、注入する動きを約 47 ms 止めます。これは速さの計測が動きを忘れる 40 ms より長いので、撮影を入れたはじきには速さがなく、位置で戻ります。そのため撮影は、離す前に止めるなぞりの中だけで行います。タブをタップした直後のタッチは、ページ送りがまだ滑っている間に届いてそれをつかむので、タブのタップのあとは 0.8 秒待ちます。最初の実行では、離した点をまだ速さの標本に入れていたビルドで、最初のなぞりの直後に見本がアクセス違反（0xc0000005）で止まりました。そのあとのビルドで同じ手順を五回試しても再び起きず、この機械にはシンボルもデバッガーもないので、場所は分かっていません。
- フレームの計時（[アプリの組み立て](app-structure.ja.md#フレームの計時)）：プログラムを `--frame-stats log` で起動し、標準エラーをファイルに向けます。シナリオがなぞりやはじきの後にリフレッシュ 2 回分より長く待つと、そのたびに `lamina.frames: gesture ...` の行が 1 行出ます。なので、なぞりの間に `wait` を挟んだシナリオなら操作ごとに 1 行になります。`--frame-stats on` は重ね表示を足します。重ね表示は操作のひと区切りの後に記録しないフレームで描くので、数字は変わりません。計るドラッグでは `snapshot` を使わないでください。撮影が差し込む動きを止め（上を参照）、それ自体の停滞が加わります。
- タッチキーボードが出ない。見たこと：入力欄をタップするとフォーカスは移りますが、`TextInputHost.exe` の窓は隠されたままです。ソースで分かること：SDL 3.4.10 で `ShowScreenKeyboard` があるのは GDK のビルドだけで、デスクトップの Windows にはありません。一般に知られていること（ここでは確かめていません）：UI オートメーションで見える入力欄にフォーカスがあると、Windows は自分からタッチキーボードを出すことがあります。lamina-ui はこれを出していません。また既定では、物理キーボードがつながっていないときだけ出します。この機械には物理キーボードがつながっていて、設定は変えていません。なので、どれがキーボードを止めているのかは切り分けていません。
