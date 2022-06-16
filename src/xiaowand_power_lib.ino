// ------------------------------------------------------------------------------ //
//    XIAO WAND電源コントロールライブラリ
//
//      Author  : Shun OSAFUNE (s.osafune@j7system.jp)
//      Release : 2022/06/16 Version 0.9
// ------------------------------------------------------------------------------ //
//
// The MIT License
// Copyright (c) 2022 Shun OSAFUNE/J-7SYSTEM WORKS LIMITED
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is furnished
// to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ------------------------------------------------------------------------------ //

// *** 使い方 ***
// このinoファイルを対象のスケッチのフォルダにコピーしてください


// 定数
const int xiaowand_pswcount_max = 100;  // 長押し検出の最大時間(0.1秒単位で10秒まで)
const int xiaowand_startup_hold = 50;   // 電源ONから5秒間長押し(STARTUP)
const int xiaowand_shutdown_hold = 50;  // 5秒間長押しで電源OFF(SHUTDOWN)
const int xiaowand_longpush_hold = 20;  // 2秒で長押し検出(LONGPUSH)
const int xiaowand_click_hold = 5;      // 0.5秒以下でクリック検出(CLICK)

#define XIAOWAND_REVISION_B             // XIAO WAND Rev.Bの指示

#if defined(XIAOWAND_MODULE_XIAO) || defined(XIAOWAND_MODULE_XIAO_USE_TCC0)
const int xiaowand_pwrsw_pin = 1;       // XIAO WANDの電源ボタン入力ピン(PIN_D1)
const int xiaowand_pwren_pin = 3;       // XIAO WANDの電源制御出力ピン(PIN_D3)
#else
const int xiaowand_pwrsw_pin = D1;      // XIAO WANDの電源ボタン入力ピン(PIN_D1)
const int xiaowand_pwren_pin = D3;      // XIAO WANDの電源制御出力ピン(PIN_D3)
#endif

// 制御変数
enum xiaowand_power_state_type {STARTUP, ACTIVE, SHUTDOWN, HALT};
volatile enum xiaowand_power_state_type xiaowand_state = STARTUP;

volatile bool xiaowand_event_press = false;
volatile bool xiaowand_event_release = false;
volatile bool xiaowand_event_startup = false;
volatile bool xiaowand_event_shutdown = false;
volatile bool xiaowand_event_longpush = false;
volatile bool xiaowand_event_click = false;
static bool xiaowand_event_exec = false;
static void (*xiaowand_press_cb)(void) = NULL;
static void (*xiaowand_release_cb)(void) = NULL;
static void (*xiaowand_startup_cb)(void) = NULL;
static void (*xiaowand_shutdown_cb)(void) = NULL;
static void (*xiaowand_longpush_cb)(void) = NULL;
static void (*xiaowand_click_cb)(void) = NULL;

volatile uint32_t xiaowand_blink_pattern = 0;
volatile int xiaowand_blink_cycle = 0;
volatile bool xiaowand_blink_init = false;
static void (*xiaowand_blinkon_cb)(void) = NULL;
static void (*xiaowand_blinkoff_cb)(void) = NULL;


///// XIAO WANDイベントポーリング関数 /////
// XIAO WANDが使える状態かどうかを返す
bool xiaowand_is_active(void) {
  return (xiaowand_state == ACTIVE);
}

// ボタン押下イベントが起こったかどうかをチェック
bool xiaowand_check_press(void) {
  bool res = xiaowand_event_press;
  xiaowand_event_press = false;
  return res;
}

// ボタンリリースイベントが起こったかどうかをチェック
bool xiaowand_check_release(void) {
  bool res = xiaowand_event_release;
  xiaowand_event_release = false;
  return res;
}

// スタートアップイベントが起こったかどうかをチェック
bool xiaowand_check_startup(void) {
  bool res = xiaowand_event_startup;
  xiaowand_event_startup = false;
  return res;
}

// シャットダウンイベントが起こったかどうかをチェック
bool xiaowand_check_shutdown(void) {
  bool res = xiaowand_event_shutdown;
  xiaowand_event_shutdown = false;
  return res;
}

// ボタン長押しイベントが起こったかどうかをチェック
bool xiaowand_check_longpush(void) {
  bool res = xiaowand_event_longpush;
  xiaowand_event_longpush = false;
  return res;
}

// クリックイベントが起こったかどうかをチェック
bool xiaowand_check_click(void) {
  bool res = xiaowand_event_click;
  xiaowand_event_click = false;
  return res;
}


///// イベントコールバック登録 /////
// 呼び出し元がイベントコールバックかどうかを返す
bool xiaowand_is_eventcb(void) {
  return xiaowand_event_exec;
}

// ボタン押下イベントのコールバックを登録
void xiaowand_attach_press(void (*cb_func)(void)) {
  xiaowand_press_cb = cb_func;
}

// ボタンリリースイベントのコールバックを登録
void xiaowand_attach_release(void (*cb_func)(void)) {
  xiaowand_release_cb = cb_func;
}

// スタートアップイベントのコールバックを登録
void xiaowand_attach_startup(void (*cb_func)(void)) {
  xiaowand_startup_cb = cb_func;
}

// シャットダウンイベントのコールバックを登録
void xiaowand_attach_shutdown(void (*cb_func)(void)) {
  xiaowand_shutdown_cb = cb_func;
}

// ボタン長押しイベントのコールバックを登録
void xiaowand_attach_longpush(void (*cb_func)(void)) {
  xiaowand_longpush_cb = cb_func;
}

// クリックイベントのコールバックを登録
void xiaowand_attach_click(void (*cb_func)(void)) {
  xiaowand_click_cb = cb_func;
}


///// LED点滅サービス /////
// LED点滅が動作中かどうかを確認
bool xiaowand_check_blink(void) {
  return (xiaowand_blink_pattern != 0 && xiaowand_blink_cycle != 0);
}

// LED点滅サービスのコールバックを登録
void xiaowand_attach_blink(void (*cb_on_func)(void), void (*cb_off_func)(void)) {
  xiaowand_blinkon_cb = cb_on_func;
  xiaowand_blinkoff_cb = cb_off_func;
}

// LED点滅サービスの点滅パターンと回数を登録
void xiaowand_blink(uint32_t pattern, int cycle) {
  if (cycle == 0) {
    if (pattern) {
      if (xiaowand_blinkon_cb) (*xiaowand_blinkon_cb)();
    } else {
      if (xiaowand_blinkoff_cb) (*xiaowand_blinkoff_cb)();
    }
    xiaowand_blink_pattern = 0;
    xiaowand_blink_cycle = 0;
    xiaowand_blink_init = false;
  } else {
    xiaowand_blink_pattern = pattern;
    xiaowand_blink_cycle = cycle;
    xiaowand_blink_init = true;
  }
}

// LED点滅サーピスのインターバル処理
static void xiaowand_blink_interval(void) {
  static int counter = 0, nibble = 0, nibble_old = 7;

  if (xiaowand_check_blink()) {
    if (xiaowand_blink_init) {
      counter = 0;
      nibble = 0;
      nibble_old = 7;
      xiaowand_blink_init = false;
    }

    if (counter) {
      counter--;
    } else {
      while (counter == 0) {
        nibble = (nibble - 1) & 7;
        counter = (xiaowand_blink_pattern >> (nibble * 4)) & 15;
      }
      if (nibble > nibble_old && xiaowand_blink_cycle > 0) xiaowand_blink_cycle--;

      if (xiaowand_blink_cycle) {
        if (nibble & 1) {
          if (xiaowand_blinkon_cb) (*xiaowand_blinkon_cb)();
        } else {
          if (xiaowand_blinkoff_cb) (*xiaowand_blinkoff_cb)();
        }
        nibble_old = nibble;
      }
    }
  }
}

// デフォルトのLED ON/OFF処理
#ifndef XIAOWAND_BLINK_LED_PIN
# define XIAOWAND_BLINK_LED_PIN LED_BUILTIN
#endif
#if (XIAOWAND_BLINK_LED_PIN >= 0)
static void xiaowand_blink_on_builtin(void) {
  pinMode(XIAOWAND_BLINK_LED_PIN, OUTPUT);
  digitalWrite(XIAOWAND_BLINK_LED_PIN, LOW);
}
static void xiaowand_blink_off_builtin(void) {
  pinMode(XIAOWAND_BLINK_LED_PIN, OUTPUT);
  digitalWrite(XIAOWAND_BLINK_LED_PIN, HIGH);
}
#endif


///// 電源コントロール /////
// コールバック呼び出しのマクロ
#define XIAOWAND_DO_EVENTCALLBACK(_func) if (_func) {xiaowand_event_exec=true; (*(_func))(); xiaowand_event_exec=false;}

// 電源ボタンイベント検出のインターバル処理
static void xiaowand_power_interval(void) {
  static int pwrsw_count = 1;

  // ボタンの状態遷移とイベント判定
  if (digitalRead(xiaowand_pwrsw_pin) == LOW) {

    // ボタン押下イベントの判定
    if (xiaowand_state != HALT && pwrsw_count == 0) {
      xiaowand_event_press = true;
      XIAOWAND_DO_EVENTCALLBACK(xiaowand_press_cb);
    }

    // スタートアップイベントの判定
    if (xiaowand_state == STARTUP && pwrsw_count == xiaowand_startup_hold) {
      xiaowand_event_startup = true;
      XIAOWAND_DO_EVENTCALLBACK(xiaowand_startup_cb);
    }

    // ボタン長押しイベントの判定
    if (xiaowand_state == ACTIVE && pwrsw_count == xiaowand_longpush_hold) {
      xiaowand_event_longpush = true;
      XIAOWAND_DO_EVENTCALLBACK(xiaowand_longpush_cb);
    }

    // シャットダウンイベントの判定
    if ((xiaowand_state == ACTIVE || xiaowand_state == HALT) && pwrsw_count == xiaowand_shutdown_hold) {
      xiaowand_state = SHUTDOWN;
      xiaowand_event_shutdown = true;
      XIAOWAND_DO_EVENTCALLBACK(xiaowand_shutdown_cb);
    }

    if (pwrsw_count < xiaowand_pswcount_max) pwrsw_count++;
  } else {

    // ボタンリリースイベントの判定
    if (xiaowand_state != HALT && pwrsw_count > 0) {
      xiaowand_event_release = true;
      XIAOWAND_DO_EVENTCALLBACK(xiaowand_release_cb);
    }

    // クリックイベントの判定
    if (xiaowand_state == ACTIVE && (pwrsw_count > 0 && pwrsw_count <= xiaowand_click_hold)) {
      xiaowand_event_click = true;
      XIAOWAND_DO_EVENTCALLBACK(xiaowand_click_cb);
    }

    // ボタンリリースでACTIVEステートへ遷移
    if (xiaowand_state == STARTUP) {
      xiaowand_state = ACTIVE;
    }

    pwrsw_count = 0;
  }

  // LED点滅サービスの処理
  xiaowand_blink_interval();
}

// XIAOによって異なるインターバルタイマーのラッピング
// XIAOWAND_MODULE_xxxによる定義名でインターバル実装部分を切り替え
#if defined(XIAOWAND_MODULE_XIAO_BLE)
# if defined(SOFTWARETIMER_H_)
#  define _XIAOWAND_USE_SOFTWARETIMER
# else
#  define _XIAOWAND_USE_SCHEDULER
# endif
#elif defined(XIAOWAND_MODULE_XIAO_RP2040)
# define _XIAOWAND_USE_SCHEDULER
#elif defined(XIAOWAND_MODULE_XIAO)
# define _XIAOWAND_USE_TC3
#elif defined(XIAOWAND_MODULE_XIAO_USE_TCC0)
# define _XIAOWAND_USE_TCC0
#else
# error There is no XIAO module name or interval timer resource indication.
#endif

// XIAO BLEでRTOSソフトウェアタイマーを使う時
#ifdef _XIAOWAND_USE_SOFTWARETIMER
SoftwareTimer TickerTimer;
static void xiaowand_interval_loop(TimerHandle_t xTimerID) {
  (void) xTimerID;
  xiaowand_power_interval();
}
static void xiaowand_inverval_begin(void) {
  TickerTimer.begin(100, xiaowand_interval_loop);
  TickerTimer.start();
}
static void xiaowand_interval_end(void) {
  TickerTimer.stop();
}
#undef _XIAOWAND_USE_SOFTWARETIMER
#endif

// XIAO BLE/XIAO RP2040でタスクスケジューラーを使う時
#ifdef _XIAOWAND_USE_SCHEDULER
#include <Scheduler.h>
volatile bool xiaowand_interval_enable = false;
static void xiaowand_interval_loop(void) {
  delay(100);
  if (xiaowand_interval_enable) xiaowand_power_interval();
}
static void xiaowand_inverval_begin(void) {
  xiaowand_interval_enable = true;
  Scheduler.startLoop(xiaowand_interval_loop);
}
static void xiaowand_interval_end(void) {
  xiaowand_interval_enable = false;
}
#undef _XIAOWAND_USE_SCHEDULER
#endif

// XIAOでTC3/TCC0タイマーを使う時
#if defined(_XIAOWAND_USE_TC3) || defined(_XIAOWAND_USE_TCC0)
static void (*xiaowand_interval_cb)(void) = NULL;
static void xiaowand_interval_attach(void (*cb_func)(void)) {
  xiaowand_interval_cb = cb_func;
}
static void xiaowand_interval_loop(void) {
  static int power_interval_count = 0;

  if (power_interval_count) {
    power_interval_count--;
  } else {
    xiaowand_power_interval();
    power_interval_count = 100 - 1;
  }

  if (xiaowand_interval_cb) (*xiaowand_interval_cb)();
}
#endif
#ifdef _XIAOWAND_USE_TC3
#include <TimerTC3.h>
static void xiaowand_inverval_begin(void) {
  TimerTc3.initialize(1000);    // 1msごとにループを起床
  TimerTc3.attachInterrupt(xiaowand_interval_loop);
}
static void xiaowand_interval_end(void) {
  TimerTc3.detachInterrupt();
}
#undef _XIAOWAND_USE_TC3
#endif
#ifdef _XIAOWAND_USE_TCC0
#include <TimerTCC0.h>
static void xiaowand_inverval_begin(void) {
  TimerTcc0.initialize(1000);   // 1msごとにループを起床
  TimerTcc0.attachInterrupt(xiaowand_interval_loop);
}
static void xiaowand_interval_end(void) {
  TimerTcc0.detachInterrupt();
}
#undef _XIAOWAND_USE_TCC0
#endif


///// Arduinoユーザーランド用関数 /////
// 電源ON処理（初期化処理）
void xiaowand_power_begin() {
  xiaowand_state = STARTUP;
  xiaowand_event_press = false;
  xiaowand_event_release = false;
  xiaowand_event_startup = false;
  xiaowand_event_shutdown = false;
  xiaowand_event_longpush = false;
  xiaowand_event_click = false;

  xiaowand_event_exec = false;
  xiaowand_attach_press(NULL);
  xiaowand_attach_release(NULL);
  xiaowand_attach_startup(NULL);
  xiaowand_attach_shutdown(NULL);
  xiaowand_attach_longpush(NULL);
  xiaowand_attach_click(NULL);

  xiaowand_blink(0, -1);
#if (XIAOWAND_BLINK_LED_PIN >= 0)
  xiaowand_attach_blink(xiaowand_blink_on_builtin, xiaowand_blink_off_builtin);
#else
  xiaowand_attach_blink(NULL, NULL);
#endif

  pinMode(xiaowand_pwrsw_pin, INPUT_PULLUP);
  pinMode(xiaowand_pwren_pin, OUTPUT);
#if defined(XIAOWAND_REVISION_B)
  digitalWrite(xiaowand_pwren_pin, HIGH);
#else
  digitalWrite(xiaowand_pwren_pin, LOW);
#endif

  xiaowand_inverval_begin();
}

// 電源OFF処理
void xiaowand_power_end(void) {
  xiaowand_state = SHUTDOWN;
  xiaowand_interval_end();
  xiaowand_blink(0, 0);     // 回数が0の時はON/OFFを即時反映

#if defined(XIAOWAND_REVISION_B)
  digitalWrite(xiaowand_pwren_pin, LOW);
#else
  pinMode(xiaowand_pwren_pin, INPUT);
#endif
}

// システム停止（シャットダウンのみ受け付ける）
void xiaowand_halt(void) {
  xiaowand_state = HALT;
  xiaowand_attach_shutdown(xiaowand_power_end);

  while (true) yield();
}

// メインループ（ポーリング）
void xiaowand_polling(void) {
  if (xiaowand_is_active()) {
    xiaowand_loop();

  } else if (xiaowand_check_startup()) {
    xiaowand_startup();

  } else if (xiaowand_check_shutdown()) {
    xiaowand_shutdown();
    xiaowand_power_end();
  }
}
