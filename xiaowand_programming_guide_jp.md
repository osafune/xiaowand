# XIAO WANDプログラミングガイド

## 1.簡単なサンプル例(XIAO BLEを使用)

### 基本的なフレームワーク

1. Arduno IDEで新しいスケッチを作成し、以下のコードを記述します。

```cpp :sample.ino
#define XIAOWAND_MODULE_XIAO_BLE

void setup() {
  xiaowand_power_begin();
  xiaowand_blink(1, 0);    // LED_BULTINをON
}

void loop() {
  xiaowand_polling();
}

void xiaowand_startup() {
  // ボタン長押しで起動した時に行う処理はここに書く
}

void xiaowand_loop() {
  // メインのloop()処理部分をここに書く
}

void xiaowand_shutdown() {
  // 電源OFFする時に必要な処理はここに書く
}
```

2. 作成したスケッチのフォルダに`xiaowand_power_lib.ino`ファイルをコピーします。  

3. コンパイルしてXIAO BLEに書き込みます。書き込みが完了後USBケーブルを抜いて電源ボタンを5秒以上長押しし、ボード上のLEDが消灯したらボタンを離します（電源OFF操作）。  
  ![img_step1](https://raw.githubusercontent.com/osafune/xiaowand/master/img/xiaowand_step1.jpg)  
  ⚠️ USBケーブルが接続されている間は電源制御にかかわらずXIAOモジュールは電源が入った状態のままになります。センサー側のGroveおよびSDカードの電源も供給され続けます。  

4. 電源ボタンを押してボード上のLEDが点灯したらボタンを離します（電源ON操作）。  

5. 再度電源ボタンを5秒以上長押ししてボード上のLEDが消灯したらボタンを離します（電源OFF操作）。

  
### ボード上の圧電ブザーを使う

XIAO WANDの圧電ブザーは`D0`ピンに接続されています。音を鳴らす場合は`tone()`で指定します。  

```cpp :beep_sample.ino
#define XIAOWAND_MODULE_XIAO_BLE
#define XIAOWAND_BUZZ_PIN D0   // PIN_D0に圧電ブザー

void setup() {
  xiaowand_power_begin();
  xiaowand_blink(1, 0);       // LED_BULTINをON
}

void loop() {
  xiaowand_polling();
}

void xiaowand_startup() {}

void xiaowand_loop() {
  if (xiaowand_check_click()) {
    tone(XIAOWAND_BUZZ_PIN, 1000, 100);	// クリックされたらbeep音
  }
}

void xiaowand_shutdown() {}
```

  
### ボード上のmicroSDカードスロットを使う

XIAO WANDのmciroSDカードスロットはSPI接続で使用することができます。カードのSSは`D2`ピンに接続されています。

```cpp :sd_sample.ino
#include <SD.h>
#define XIAOWAND_MODULE_XIAO_BLE
#define XIAOWAND_SD_SS_PIN D2  // PIN_D2にSDカードの/CS

File myfile;

void setup() {
  xiaowand_power_begin();
  xiaowand_blink(1, 0);       // LED_BULTINをON

  if (!SD.begin(XIAOWAND_SD_SS_PIN)) {
    xiaowand_blink(0x32, -1);
    xiaowand_halt();          // カードの初期化に失敗したらHALT
  }
  myfile = SD.open("testfile.bin", FILE_READ);
  if (!myfile) {
    xiaowand_blink(0x32, -1);
    xiaowand_halt();          // ファイルのオープンに失敗したらHALT
  }
}

void loop() {
  xiaowand_polling();
}

void xiaowand_startup() {}

void xiaowand_loop() {}

void xiaowand_shutdown() {
  if (myfile) myfile.close(); // シャットダウンの前にクローズする
}
```

  
### NeoPixelの制御をする

XIAO WANDのLED/UART側のGroveコネクタは3.3V/500mAの電源供給ができるため、NeoPixelモジュールを接続して簡単に電飾ユニットにすることができます。  

```cpp :neopixel_sample.ino
#include <Adafruit_NeoPixel.h>
#define XIAOWAND_MODULE_XIAO_BLE
#define XIAOWAND_NEOPIXEL_PIN D6   // LED側GroveのD1ピン(D6/TXD)
//#define XIAOWAND_NEOPIXEL_PIN D7   // GroveのD0ピンを使うものもある
#define NEOPIXEL_LED_NUMBER   60  // 制御するLEDの個数 

// NeoPixelの表示（60個をレインボー表示）
Adafruit_NeoPixel strip = Adafruit_NeoPixel(
                              NEOPIXEL_LED_NUMBER,
                              XIAOWAND_NEOPIXEL_PIN,
                              NEO_GRB + NEO_KHZ800);
void rainbow(void) {
  static int firstPixelHue = 0;

  for (int i = 0; i < strip.numPixels(); i++) {
    int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
  }
  strip.show();

  firstPixelHue += 256;
  if (firstPixelHue >= 3 * 65536) firstPixelHue = 0;
}

void setup() {
  xiaowand_power_begin();
  xiaowand_blink(1, 0);       // LED_BULTINをON

  strip.begin();  // NeoPixel開始、この時全てのLEDは0に設定される
  strip.show();   // LEDへデータ転送し初期化する
}

void loop() {
  xiaowand_polling();
}

void xiaowand_startup() {
}

void xiaowand_loop() {
  rainbow();
  delay(10);
}

void xiaowand_shutdown() {
  strip.clear();  // シャットダウンイベントで全消灯する
  strip.show();
}
```

***
<br>

## 2.ライブラリリファレンス

### イベントハンドラ

電源制御ライブラリから明示的に呼び出されるイベントハンドラ関数です。以下の３つはスケッチの中に**必ず**記述しなければなりません。  

- **xiaowand_startup()**  
電源OFFから電源ボタン長押しで起動したときに一度だけ呼び出される関数です。  
ブートモードを切り替えたい場合（例：Bluetoothのペアリングモードに入る）に使用します。  
この関数を抜けるまでは`xiaowand_loop()`や`xiaowand_shutdown()`には制御が移りません。  

- **xiaowand_loop()**  
電源状態がアクティブの時に常に呼び出される関数です。  
電源ステートに応じた処理を行うため、通常のユーザープログラムコードは `loop()`の代わりにこの関数へ記述します。  

- **xiaowand_shutdown()**  
アクティブの状態から電源ボタン長押しでシャットダウンイベントが発生したときに、電源がカットされる前に一度だけ呼び出される関数です。  
ファイルシステムなどの電源を切る前に開放が必要なリソースの処理や、電源OFFを明示的に処理する必要がある場合に使用します。電源ボタンが離されるまでは電源ONの状態になるため、例えばNeoPixelのLEDテープやI2C接続のディスプレイモジュールの消灯処理などはここで行います。  
この関数を抜けるまでは電源ONが維持されます。また、この関数に処理が移った時点で`SHUTDOWN`ステートに移行し、以後どのイベントも発生しません。


### XIAOモジュール識別マクロ

XIAO WANDに搭載されているモジュールの識別用のマクロで、スケッチの先頭にXIAOにあわせて一つだけ記述します。

```cpp
// XIAO BLEまたはXIAO BLE Senseを搭載している場合
#define XIAOWAND_MODULE_XIAO_BLE
```
```cpp
// XIAO RP2040を搭載している場合
#define XIAOWAND_MODULE_XIAO_RP2040
```
```cpp
// Seeeduino XIAOを搭載している場合（インターバルタイマーはTC3を使用）
#define XIAOWAND_MODULE_XIAO
```
```cpp
// Seeeduino XIAOを搭載していて、インターバルタイマーにTCC0使う場合
#define XIAOWAND_MODULE_XIAO_USE_TCC0
```


### 内部ステート遷移とイベント

XIAO WAND電源コントロールの内部ステート遷移は以下のようになっています。

```mermaid
flowchart TD
  s1((RESET))-->STARTUP
  STARTUP-->|ボタンリリースイベント|ACTIVE
  STARTUP-->|"xiaowand_halt()"|HALT
  STARTUP-->|"xiaowand_power_end()"|SHUTDOWN
  ACTIVE-->|シャットダウンイベント|SHUTDOWN
  ACTIVE-->|"xiaowand_power_end()"|SHUTDOWN
  ACTIVE-->|"xiaowand_halt()"|HALT
  HALT-->|電源OFF遷移|SHUTDOWN
```

- **STARTUPステート**  
ボード電源コントロールをイネーブルにしてから最初のボタンリリースイベント発生、あるいは`HALT`、`SHUTDOWN`への明示的な遷移が行われるまでの状態です。  
このステートではボタン押下イベント、ボタンリリースイベント、スタートアップイベントが発生します。  
- **ACTIVEステート**  
通常の動作をしている状態です。シャットダウンイベント発生、あるいは`HALT`、`SHUTDOWN`への明示的な遷移が行われるまでの状態です。  
このステートではボタン押下イベント、ボタンリリースイベント、シャットダウンイベント、ボタン長押しイベント、クリックイベントが発生します。
- **HALTステート**  
`xiaowand_halt()`でシステムを明示的に停止している状態です。ボタン長押しによる電源OFF遷移でのみ`SHUTDOWN`に遷移します。  
このステートではいかなるイベントも発生しません。  
- **SHUTDOWNステート**  
ボード電源コントロールをディセーブルにし、電源OFFを待機している状態です。  

またそれぞれのステートとボタン操作に応じて以下の6つのイベントが発生します。  

1. ボタン押下イベント  
内部ステートが`HALT`以外のときにボタンが押された場合に発生します。  
0. ボタンリリースイベント  
内部ステートが`HALT`以外のときにボタンが離された場合に発生します。  
`STARTUP`ステートでこのイベントが発生すると`ACTIVE`ステートに遷移します。  
0. スタートアップイベント  
`STARTUP`ステートのときに5秒以上ボタンが押されたままになっていると１回だけ発生します。  
0. シャットダウンイベント  
`ACTIVE`ステートのときに5秒以上ボタンが押されたままになっていると１回だけ発生します。その後は`SHUTDOWN`ステートに遷移します。  
0. ボタン長押しイベント  
`ACTIVE`ステートで2秒以上ボタンが押されたままになっていると発生します。操作上、シャットダウンイベントの前にも必ず発生することに注意してください。
0. ボタンクリックイベント  
`ACTIVE`ステートでボタンが押された時間が0.5秒以下のときに発生します。操作上、ボタンリリースイベントも同時に発生することに注意してください。

各イベントの発生時間は`xiaowand_power_lib.ino`の先頭部で宣言されている以下の値を書き換えて調整することができます。  

```cpp :xiaowand_power_lib.ino(抜粋)
// 定数
const int xiaowand_pswcount_max = 100;  // 長押し検出の最大時間(0.1秒単位で10秒まで)
const int xiaowand_startup_hold = 50;   // 電源ONから5秒間長押し(STARTUP)
const int xiaowand_shutdown_hold = 50;  // 5秒間長押しで電源OFF(SHUTDOWN)
const int xiaowand_longpush_hold = 20;  // 2秒で長押し検出(LONGPUSH)
const int xiaowand_click_hold = 5;      // 0.5秒以下でクリック検出(CLICK)
  :
  :
```

<br>

## 3.APIリファレンス

### xiaowand_power_begin()

電源制御処理を初期化し、XIAO WANDの電源コントロールを開始します。
全てのイベントフラグおよびコールバックは初期化され、内部ステートは`STARTUP`に設定されます。
この関数の実行までは電源ONの維持はされないため、通常は`setup()`の先頭に記述します。

- 書式  
*void* xiaowand_power_begin(*void*)  

  - 引数  
  なし

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
  xiaowand_power_begin();
	:
	:
}
```

---
### xiaowand_power_end()

XIAO WANDの電源コントロールを終了し、電源をOFFにします。実際にボードの電源がカットされるのはボタンが離されたときになります。  
ボタン監視を停止し、LEDをOFFにして内部ステートは`SHUTDOWN`に設定されます。  
この関数はシャットダウンイベント発生時に内部で自動的に呼ばれるため、通常は使用する必要はありません。ソフトウェア的に電源OFFが必要な場合に使用します。  
また、この関数ではシャットダウンイベントは発生しません。リソースの開放等の処理は予め済ませておく必要があります。

- 書式  
*void* xiaowand_power_end(*void*)  

  - 引数  
  なし

  - 返値  
  なし

- 記述例  

```cpp
void foo() {
  // リソース開放等の処理
	:
	:
  xiaowand_power_end();
}
```

---
### xiaowand_halt()

XIAO WANDのイベント呼び出しを全て停止し、ボタン長押しによる電源OFFを待ちます。  
システム中で続行不可能な状態が発生した場合に、`while(1)`ループの代わりに使用します。  
内部ステートは`HALT`に設定され、以後全てのイベントは発生しなくなります。ボタン長押しで電源OFFを行ったときにもシャットダウンイベントは発生しません。リソースの開放等の処理は予め済ませておく必要があります。

- 書式  
*void* xiaowand_halt(*void*)  

  - 引数  
  なし

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
  xiaowand_power_begin();
  xiaowand_blink(1, 0);   // LED_BULTINをON

  // SDカードの初期化に失敗したらHALT
  if (!SD.begin(XIAOWAND_SD_SS_PIN)) {
    xiaowand_blink(0x32, -1);	// LEDを点滅
    xiaowand_halt();
  }
}
```

---
### xiaowand_polling()

XAIO WANDの電源コントロールイベントやボタンの状態監視を行います。  
ステートの状態遷移やイベントの呼び出しを行うため、`loop()`の中に記述します。他ライブラリ等でポーリングを使用する場合はループ処理が最短になるよう注意してください。  
通常のユーザープログラムコードは`loop()`の代わりに`xiaowand_loop()`に記述します。

- 書式  
*void* xiaowand_polling(*void*)  

  - 引数  
  なし

  - 返値  
  なし

- 記述例  

```cpp
void loop() {
  xiaowand_polling();
}
```

---
### xiaowand_blink()

XIAOモジュールのLED点滅パターンを設定します。  
点滅処理はメインの処理とは独立して行われ、いつでも点滅パターンや回数を指定することができます。  
デフォルトでは`LED_BULTIN`で指定されるLEDを使用します。また`xiaowand_attach_blink()`でON/OFF時のコールバックを指定して点滅を別のリソースに割り当てることができます。  

- 書式1  
*void* xiaowand_blink(*uint32_t* pattern, *int* cycle)  

  - 引数  
    * pattern  
    点滅パターンを指定します。パターンは最大4フィールドのON時間・OFF時間をそれぞれ4bitで表し、以下のフォーマットで記述します。  
	  `0x<ON時間1><OFF時間1><ON時間2><OFF時間2><ON時間3><OFF時間3><ON時間4><OFF時間4>`  
	  点滅パターンは`ON時間1→OFF時間1→ON時間2→‥‥→OFF時間4→ON時間1→`とループします。各時間は0.1秒単位で、それぞれ0(0x0)～15(0xF)の指定ができます。0を指定した場合はその部分はスキップされます。例えば、1秒周期のデューティー50%の点滅を指定するとき、`0x55`、`0x5500`、`0x550000`、`0x55000000`は同じ動作になります。  
      * 例１）ON時間が長めの点滅パターン   
      0x32 : 0.3秒ON→0.2秒OFF
      * 例２）2秒周期で2回点滅するパターン  
      0x11170a00 : 0.1秒ON→0.1秒OFF→0.1秒ON→0.7秒OFF→1秒OFF  
      * 例３）2回消灯して点灯で終わるパターン
      0x01211000 : 0.1秒OFF→0.2秒ON→0.1秒OFF→0.1秒ON

    * cycle  
    点滅回数を指定します。1以上の整数、または-1を指定します。-1を指定した場合はループ動作になります。  
    0を指定した場合は書式2の即値ON/OFFになります。  

  - 返値  
  なし

- 書式2 
*void* xiaowand_blink(*uint32_t* led, 0)  

  - 引数  
	  * led
	  LEDのON/OFFを指定します。`0`を指定した場合はLED消灯、`≠0`を指定した場合はLED点灯します。 

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
  xiaowand_power_begin();
  xiaowand_blink(1, 0);   // LED_BULTINをON
}
	:
	:

void xiaowand_startup() {
  xiaowand_blink(0x11170a00, -1);	// 長押し起動で2回点滅パターンをループ
}

void xiaowand_loop() {
  if (xiaowand_check_click()) {
    xiaowand_blink(0x01211000, 1);	// クリックされたら１回点滅
  }
  if (xiaowand_check_longpush()) {
    tone(XIAOWAND_BUZZ_PIN, 1000, 100);	// 長押しされたらbeep音
  }
}
	:
	:
```

---
### xiaowand_is_active()

XIAO WANDの電源がアクティブかどうかを確認します。  
通常は`xiaowand_polling()`の中で適切に処理されるため不要ですが、スケジューラー等で別スレッドから電源状態を確認したい場合などに使用します。  

- 書式  
*bool* xiaowand_is_active(*void*)  

  - 引数  
  なし

  - 返値  
  内部ステートが`ACTIVE`の時に`true`、それ以外の場合の時には`false`を返します。

---
### xiaowand_check_press()

ボタンの押下イベントが発生したかどうかを確認します。  
この関数が呼ばれるとボタン押下のイベント発生フラグはクリアされます。

- 書式  
*bool* xiaowand_check_press(*void*)  

  - 引数  
  なし

  - 返値  
  この関数が呼ばれるまでに押下イベントがあれば`true`、なければ`false`を返します。  

---
### xiaowand_check_release()

ボタンのリリースイベントが発生したかどうかを確認します。  
この関数が呼ばれるとボタンリリースのイベント発生フラグはクリアされます。

- 書式  
*bool* xiaowand_check_release(*void*)  

  - 引数  
  なし

  - 返値  
  この関数が呼ばれるまでにリリースイベントがあれば`true`、なければ`false`を返します。  

---
### xiaowand_check_startup()

スタートアップイベント（ボタンの長押し起動）が発生したかどうかを確認します。  
通常は`xiaowand_polling()`の中で適切に処理されるため不要ですが、スケジューラー等で別スレッドから状態を確認したい場合などに使用します。  
この関数が呼ばれるとスタートアップのイベント発生フラグはクリアされます。

- 書式  
*bool* xiaowand_check_startup(*void*)  

  - 引数  
  なし

  - 返値  
  この関数が呼ばれるまでにスタートアップイベントがあれば`true`、なければ`false`を返します。  

---
### xiaowand_check_shutdown()

シャットダウンイベント（アクティブ時のボタンの長押し）が発生したかどうかを確認します。  
通常は`xiaowand_polling()`の中で適切に処理されるため不要ですが、スケジューラー等で別スレッドから状態を確認したい場合などに使用します。  
この関数が呼ばれるとシャットダウンのイベント発生フラグはクリアされます。

- 書式  
*bool* xiaowand_check_startup(*void*)  

  - 引数  
  なし

  - 返値  
  この関数が呼ばれるまでにシャットダウンイベントがあれば`true`、なければ`false`を返します。  

---
### xiaowand_check_longpush()

ボタンの長押しイベントが発生したかどうかを確認します。  
操作上、長押しイベントはシャットダウンイベントの前に必ず発生することに注意してください。  
この関数が呼ばれると長押しのイベント発生フラグはクリアされます。

- 書式  
*bool* xiaowand_check_longpush(*void*)  

  - 引数  
  なし

  - 返値  
  この関数が呼ばれるまでに長押しイベントがあれば`true`、なければ`false`を返します。  

---
### xiaowand_check_click()

ボタンのクリックイベントが発生したかどうかを確認します。  
この関数が呼ばれると長押しのイベント発生フラグはクリアされます。

- 書式  
*bool* xiaowand_check_click(*void*)  

  - 引数  
  なし

  - 返値  
  この関数が呼ばれるまでにクリックイベントがあれば`true`、なければ`false`を返します。  

---
### xiaowand_check_blink()

`xaiowand_blink()`で設定される点滅パターンが動作しているかどうかを確認します。

- 書式  
*bool* xiaowand_check_blink(*void*)  

  - 引数  
  なし

  - 返値  
  点滅パターンが動作していれば`true`、停止していれば`false`を返します。  

---
### xiaowand_is_eventcb()

現在の関数がイベントコールバックで呼び出されているかどうかを確認します。

- 書式  
*bool* xiaowand_is_eventcb(*void*)  

  - 引数  
  なし

  - 返値  
  イベントコールバックで呼び出されている時に`true`、それ以外の場合の時には`false`を返します。

---
### xiaowand_attach_press()

ボタン押下イベントで呼び出されるコールバックを登録します。  
コールバックは割り込みハンドラやタスクスケジューラ内から呼ばれるため、使えるリソースに制限があることに注意してください。

- 書式  
*void* xiaowand_attach_press(_void (*cb_func)(void)_)  

  - 引数  
    * cb_func  
    イベントで呼び出される関数を指定します。`NULL`を指定した場合はコールバックを解除します。  

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
    :
    :
  xiaowand_attach_press(count_press);
}

int count_press_value = 0;  // ボタンが押された数をカウント
void count_press(void) {
  count_press_value++;
}
```

---
### xiaowand_attach_release()

ボタンリリースイベントで呼び出されるコールバックを登録します。  
コールバックは割り込みハンドラやタスクスケジューラ内から呼ばれるため、使えるリソースに制限があることに注意してください。

- 書式  
*void* xiaowand_attach_release(_void (*cb_func)(void)_)  

  - 引数  
    * cb_func  
    イベントで呼び出される関数を指定します。`NULL`を指定した場合はコールバックを解除します。  

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
    :
    :
  xiaowand_attach_release(count_release);
}

int count_release_value = 0;  // ボタンが離された数をカウント
void count_release(void) {
  count_release_value++;
}
```

---
### xiaowand_attach_startup()

スタートアップイベントで呼び出されるコールバックを登録します。  
コールバックは割り込みハンドラやタスクスケジューラ内から呼ばれるため、使えるリソースに制限があることに注意してください。

- 書式  
*void* xiaowand_attach_startup(_void (*cb_func)(void)_)  

  - 引数  
    * cb_func  
    イベントで呼び出される関数を指定します。`NULL`を指定した場合はコールバックを解除します。  

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
    :
    :
  xiaowand_attach_startup(on_startup);
}

void on_startup(void) {
    :
    :
}
```

---
### xiaowand_attach_shutdown()

シャットダウンイベントで呼び出されるコールバックを登録します。  
コールバックは割り込みハンドラやタスクスケジューラ内から呼ばれるため、使えるリソースに制限があることに注意してください。  

- 書式  
*void* xiaowand_attach_shutdown(_void (*cb_func)(void)_)  

  - 引数  
    * cb_func  
    イベントで呼び出される関数を指定します。`NULL`を指定した場合はコールバックを解除します。  

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
    :
    :
  xiaowand_attach_shutdown(on_shutdown);
}

void on_shutdown(void) {
    :
    :
}
```

---
### xiaowand_attach_longpush()

長押しイベントで呼び出されるコールバックを登録します。  
コールバックは割り込みハンドラやタスクスケジューラ内から呼ばれるため、使えるリソースに制限があることに注意してください。  

- 書式  
*void* xiaowand_attach_longpush(_void (*cb_func)(void)_)  

  - 引数  
    * cb_func  
    イベントで呼び出される関数を指定します。`NULL`を指定した場合はコールバックを解除します。  

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
    :
    :
  xiaowand_attach_longpush(on_longpush);
}

void on_longpush(void) {
    :
    :
}
```

---
### xiaowand_attach_click()

クリックイベントで呼び出されるコールバックを登録します。  
コールバックは割り込みハンドラやタスクスケジューラ内から呼ばれるため、使えるリソースに制限があることに注意してください。  

- 書式  
*void* xiaowand_attach_click(_void (*cb_func)(void)_)  

  - 引数  
    * cb_func  
    イベントで呼び出される関数を指定します。`NULL`を指定した場合はコールバックを解除します。  

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
    :
    :
  xiaowand_attach_click(on_click);
}

void on_click(void) {
    :
    :
}
```

---
### xiaowand_attach_blink()

`xaiowand_blink()`で設定される点滅パターンでONおよびOFFの時に呼び出されるコールバックを登録します。
コールバックは割り込みハンドラやタスクスケジューラ内から呼ばれるため、使えるリソースに制限があることに注意してください。  

- 書式  
*void* xiaowand_attach_click(_void (*cb_on_func)(void)_, _void (*cb_off_func)(void)_)  

  - 引数  
    * cb_on_func  
    点滅ONで呼び出される関数を指定します。`NULL`を指定した場合はコールバックを解除します。  
    * cb_off_func  
    点滅OFFで呼び出される関数を指定します。`NULL`を指定した場合はコールバックを解除します。  

  - 返値  
  なし

- 記述例  

```cpp
void setup() {
	:
	:
  pinMode(LEDB, OUTPUT);
  pinMode(LEDR, OUTPUT);
  xiaowand_attach_blink(led_blue, led_red);
}

// LED ONで青色LED点灯 
void led_blue() {
  digitalWrite(LEDB, LOW);
  digitalWrite(LEDR, HIGH);
}

// LED OFFで赤色LED点灯 
void led_red() {
  digitalWrite(LEDB, HIGH);
  digitalWrite(LEDR, LOW);
}
```
