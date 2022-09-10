# Open SESAME

## 概要
CANDY HOUSE  社から発売されているスマートロック SESAME Mini に対し、M5Atomデバイスが指定 Wi-Fi に入った時に自動でロックを解除するためのプログラムです。

## 準備
src フォルダ中の Setting.template.h をコピーして Setting.h ファイルを作成します。環境に合わせて、Setting.h の内容を修正します。

## 使い方

### 自動制御
* 電源に接続されると、指定の Wi-Fi を探す。見つからない場合は継続的に探す。Wi-Fiが切断した場合も再度探す。
* 接続までに10分以上かかった場合、かつ鍵の状態が「施錠だったら、」外出して帰宅したとみなし、カウントダウン(5秒)開始。
* カウントダウン終了後にWi-Fi経由でCANDY HOUSE社のWeb API経由で「開錠」を行う。

* Wi-Fi 接続試行中は黄色の点滅
* 鍵の状態取得中は黄色の点灯
* 鍵が開いているときは緑の点滅(3秒ごと)
* 鍵が閉じているときは赤の点滅(3秒ごと)
* カウントダウン中は緑の点滅(毎秒・明るめ)
* 開錠処理中は緑の点灯

### 手動操作
* ボタンを短く(3秒未満)押すと開錠操作（緑の点灯）
* ボタンを長く(3秒以上)押すと施錠操作（赤の点灯）
