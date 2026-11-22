# 日付と時刻

[English](dates-and-times.md) · [README](../../README.ja.md#資料)

## 暦

`lamina/calendar/`：ピッカーが使う日付の計算で、部品のコードは含みません。`date.hpp` は先発グレゴリオ暦（`Date`・`YearMonth`・うるう年・月の日数・どの年でも 1970-01-01 からの日の番号・曜日・日と月の足し算・週の始まりに合わせた月の並び）です。`format.hpp` は CLDR の書式（`MMMM`・`EEEE`・`d`・`y`・`h`・`H`・`mm`・`a`・引用符で囲んだ文字）で日付と時刻を書き、`MM/dd/yyyy` のような入力の書式で 8 桁の数字を読みます。`locale.hpp` の `CalendarLocale` には、週の始まりの曜日、月と曜日の名前、Compose が使う書式（`yMMMM`・`yMMMd`・`yMMMMEEEEd` を CLDR が展開したもの）、入力の書式、12 時間制か 24 時間制か、ピッカーが出すすべての文字（`PickerStrings`。Compose の文字列の資源から取りました）が入っています。`english_us()`・`english_gb()`（月曜始まり・日が月より先・24 時間制）・`japanese()` を用意しました。プログラムが自分で作ることもできます。

## 日付の選択

`lamina/widgets/date_picker.hpp`：Compose の `DatePicker`。幅 360 dp です。上に見出し（Label L の題、Headline L の選んだ日付、カレンダーと文字入力を切り替えるボタン）と区切り線があり、その下に月と年のボタンと前後の月のボタンの行、曜日の頭文字（短い名前で、ロケールの週の始まりから）、48 dp のます目に 40 dp の日が 6 週並びます。選んだ日は primary の丸で、標準の効果のばねで現れます。今日には 1 dp の primary の輪を付けます。`SelectableDates` が許さない日と、年の範囲（既定は 1900〜2100）の外の日は 38 % で、選べません。矢印のボタンで、月が標準の効果のばねで横へ滑ります。月と年のボタンはカレンダーの上に年の一覧を開きます。72 × 36 dp の年を 3 列、16 dp 空けて並べ、表示中の年が 2 行目に来るようにスクロールした状態で、下へ広がりながら標準の効果のばねで 60 % から現れ、閉じるときは速いばねで消えながら畳まれます。年を選んでも月はそのままです。文字入力では、ロケールの書式（MM/DD/YYYY や YYYY/MM/DD）を枠にした入力欄が数字だけを受け取り、区切りの文字はその部分が埋まった時点で出ます。8 桁そろうと Compose と同じく調べ、ありえない日付・範囲外の年・許されない日付には、それぞれ Compose の文を入力欄の下に赤で出します。モードを切り替えると、新しい中身が滑り込み、古い中身が出ていきながら薄れ（標準の空間のばね）、高さも追いつきます。切り替えて 300 ms 後に入力欄にフォーカスが移ります。`add_focus_stops(focus)` は、モードのボタン・月と年のボタン・矢印・日のます目・年の一覧・入力欄を、出ているときだけ止まる位置として `FocusManager` に足します。日のます目は Tab の止まり位置一つで、フォーカスのある日を持ちます。矢印で 1 日か 1 週、`Page Up` と `Page Down` で 1 か月（Shift と一緒なら 1 年）、`Home` と `End` で週の初めと終わりへ動き、`Space` か `Enter` で選びます。Compose はこれらのキーを既定のフォーカス探しに任せているので、WAI-ARIA の日付の表に従いました。別の月へ入るとその月へ滑ります。年の一覧は矢印・`Home`・`End` と `Space` か `Enter` を受けます。日は読み上げではボタンで、Compose の説明と同じく日付をすべて書いた名前を持ち、今日には "Today" が前に付きます。年のボタンは "Navigate to year 2026" です。`show_header` を切るとカレンダーだけになります。

## 期間の選択

`lamina/widgets/date_range_picker.hpp`：Compose の `DateRangePicker`。見出しに Title L で「開始日 - 終了日」、その下に曜日の行と、縦にスクロールする月の一覧があり、月ごとに月の名前（Title S）と 6 週が並びます。描くのは見えている月だけです。最初のクリックで開始日、その日以降の次のクリックで終了日が決まり、開始日より前か、期間がそろった後のクリックでやり直します。間の日は secondary-container の帯でつなぎ、帯は二つの丸の中心で始まって終わります。文字入力では二つの欄が横に並び、終了日が開始日より前（または開始日が終了日より後）だと "Invalid date range input" を出します。モードは速い効果のばねで入れ替わります。ます目は同じキーを受け、フォーカスのある日が見えるようにスクロールします。

## 欄の下に開く日付の選択

`lamina/widgets/docked_date_picker.hpp`：末尾にカレンダーのボタンが付いた日付の入力欄で、ボタンを押すと欄の 4 dp 下にカレンダー（見出しのない `DatePicker`）を開きます。面は surface-container-high、角は 16 dp、影は 6 dp です。日を選ぶと欄に入って閉じ、欄に日付を打つとカレンダーがその日へ移ります。`Esc`・外を押す・ボタンで閉じ、欄で `Alt+↓` を押すと、選んだ日にフォーカスを置いて開きます。Compose には欄の下に開く日付の選択がないので、面の色・角・影・幅は material-web の docked の日付の選択のトークンから取り、隙間と動きは lamina-ui が決めました。ピッカーはアイコンボタンを見せる（`mode_toggle()`・`previous_button()`・`next_button()`・`open_button()`・`PickerDialog::leading()`）ので、Compose の `IconButtonWithTooltip` のように、プログラムが名前のツールチップを付けられます。components の例はそうしています。

## 選択のダイアログ

`lamina/widgets/picker_dialog.hpp`：ピッカーをダイアログにする部品で、Compose の `DatePickerDialog` と `TimePickerDialog` にあたります。`DialogPane`（`PaneOf<W>` が部品をつなぎます）でどのピッカーも持ち、その下に Cancel と OK の文字ボタンを置きます。日付の配置では、ピッカーを角 28 dp の面いっぱいに置き、ボタンは末尾から 6 dp・下から 8 dp です。時刻の配置では 24 dp 空け、ピッカーの上に Label M の題、ボタンの行の先頭にアイコンボタン（モードの切り替え）を置きます。ダイアログと同じく幕とともに 0.8 から大きくなって開きます。Tab はピッカーとボタンを巡り、`Esc` か幕を押すと閉じ、`handle` は `DialogResult` を返します。

## 時刻の選択

`lamina/widgets/time_picker.hpp`：Compose の `TimePicker` と `TimeInput`。文字盤のモードでは、時と分を 96 × 80 dp の箱（Display L。選ばれた側は primary-container）にコロンを挟んで出し、12 時間制では AM/PM の切り替え（4 dp 離した二つのボタン。丸い形で、選ばれるか押されると速い空間のばねで角 12 dp になり、選ばれた側は primary-container で文字が太字）を置きます。その下が 256 dp の文字盤で、数字は半径 101 dp の円に（24 時間制の時は 12〜23 を半径 69 dp の内側の円にも）並び、48 dp の primary のつまみの下では数字が on-primary になり、2 dp の線と 8 dp の中心があります。`TimePickerLayout` で、表示を文字盤の上（縦）か横（横。切り替えは箱の下）に置きます。押すと、つまみが標準の空間のばねでいちばん近い時か 5 分へ動き、時を選んだときは、つまみが止まってから 100 ms 後に分へ移ります。ドラッグではつまみが指に付いて動き、いちばん近い時か分になり、離したときに分へ移ります。24 時間制では、中心からの距離（74 dp）で内側か外側かを決めます。数字の組は標準の効果のばねで入れ替わります。入力のモードは 96 × 72 dp の二つの欄（Display M）で、2 桁ずつ受け取り、埋まっていれば打った数字で置き換え、時を 2 桁（12 時間制なら 2〜9 の 1 桁）打つと分へ進み、下に "Hour" と "Minute"、範囲外なら Compose の文を赤で出します。Tab は、時と分の箱（ラジオボタン）、AM/PM の切り替え（止まり位置一つで、矢印で選び、`Space` か `Enter` で切り替え）、文字盤の順に巡ります。文字盤では Compose と同じく矢印で数字の間をフォーカスが動き、`Space` か `Enter` で選びます。入力のモードでは二つの欄と切り替えを巡ります。文字盤の数字は読み上げではボタンで、名前は Compose と同じです（"3 o'clock"・"15 hours"・"20 minutes"）。`is_24_hour` の既定はロケールの時計です。Compose の時刻の選択には無効の状態がありません。`set_enabled(false)` は、ほかの部品と同じく 38 % に薄めて操作を止めます。

## 出典

- Material 3 の 日付の選択：<https://m3.material.io/components/date-pickers/overview>
- Material 3 の 時刻の選択：<https://m3.material.io/components/time-pickers/overview>
- 値ごとのソースのファイルとコミット：[デザイントークン](../design-tokens.ja.md)
