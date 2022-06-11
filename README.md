# XIAO WAND

## Overview
<img src="https://raw.githubusercontent.com/osafune/xiaowand/master/img/xiaowand_top.jpg">

XIAO WANDはSeeedstudioの小型MCUモジュール "XIAO" シリーズ専用のI/O・電源ベースボードです。  
W20mm×L115mm×H22mmのコンパクトサイズに、高出力・高効率の電源モジュール"[ENEBATT](https://osafune.github.io/enebatt_jp.html)"を搭載しており、単三形ニッケル水素電池1本でGroveセンサーモジュールやLEDテープの制御が可能です。 
ボードの電源制御およびボタンイベント取得にはArduino IDE用のライブラリ(inoファイル)が提供され、XIAO BLEと組み合わせれば簡単にBluetooth制御の電飾ユニットが製作できます。

- ボード搭載ペリフェラル
	* 電源モジュール×1 (Ni-MH AA,3.3V/500mA)
	* タクタイルスイッチ×1 (電源ボタン・ユーザーボタン兼用)
	* Groveコネクタ×2 (LED/UART用×1, I2C/GPIO/AIN用×1)
	* 圧電ブザー×1
	* microSDカードスロット×1

- 対応ボード
	* [Seeeduino XIAO](https://wiki.seeedstudio.com/Seeeduino-XIAO/)
	* [XIAO RP2040](https://wiki.seeedstudio.com/XIAO-RP2040/)
	* [XIAO BLE(Sense)](https://wiki.seeedstudio.com/XIAO_BLE/)

- 外形サイズ
	* 幅20mm×長さ115mm×厚み22mm（電池含む）  
	固定用ネジ穴 φ3.1×2

## Board Layout

## Documents
- [XAIO WAND プログラミングガイド](xiaowand_programming_guide_jp.md)

## Resources
- [電源コントロールライブラリ(Arduino IDE用)](src/xiaowand_power_lib.ino)
- 回路図(Rev.B)
- [ENEBATT](https://osafune.github.io/enebatt_jp.html)

## Contant Us
- [GitHub - Shun OSAFUNE](https://github.com/osafune)
- [Twitter - @s_osafune](https://twitter.com/s_osafune)
