# XIAO WAND

## Overview
<img src="https://raw.githubusercontent.com/osafune/xiaowand/master/img/xiaowand_top.jpg">

XIAO WANDはSeeedstudioの小型MCUモジュール "XIAO" シリーズ専用のI/O・電源ベースボードです。  
W20mm×L115mm×H22mmのコンパクトサイズに、高出力・高効率の電源モジュール"[ENEBATT](https://osafune.github.io/enebatt_jp.html)"を搭載しており、単三形ニッケル水素電池1本でGroveセンサーモジュールやLEDテープの制御が可能です。 
専用のArduino IDE用ライブラリ「xiaowand_lib」では、ボードの電源制御およびボタンイベント取得、LEDパターン点滅、MMLバックグランド再生の機能が提供され、XIAO BLEと組み合わせれば簡単にBluetooth制御の電飾ユニットが製作できます。

- ボード搭載ペリフェラル
	* 電源モジュール×1 (Ni-MH AA,3.3V/500mA)
	* タクタイルスイッチ×1 (電源ボタン・ユーザーボタン兼用)
	* Groveコネクタ×2 (LED/UART用×1, I2C/GPIO/AIN用×1)
	* 圧電ブザー×1
	* microSDカードスロット×1

- 対応ボード
	* [Seeeduino XIAO](https://wiki.seeedstudio.com/Seeeduino-XIAO/) ボードマネージャー 1.8.3以降
	* [XIAO RP2040](https://wiki.seeedstudio.com/XIAO-RP2040/) ボードマネージャー 2.7.2以降
	* [XIAO BLE(Sense)](https://wiki.seeedstudio.com/XIAO_BLE/) ボードマネージャー 1.0.0 または 2.6.1以降

- 外形サイズ
	* 幅20mm×長さ115mm×厚み22mm（電池含む）  
	固定用ネジ穴 φ3.1×2

## Board Layout

## Documents
- [XAIO WAND プログラミングガイド](xiaowand_programming_guide_jp.md)

## Resources
- [XIAO WAND ライブラリ(Arduino IDE用)](src/xiaowand_lib.ino)
- 回路図(Rev.B)
- [ENEBATT](https://osafune.github.io/enebatt_jp.html)

## Contant Us
- [GitHub - Shun OSAFUNE](https://github.com/osafune)
- [Twitter - @s_osafune](https://twitter.com/s_osafune)
