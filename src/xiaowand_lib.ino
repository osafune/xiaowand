// ------------------------------------------------------------------------------ //
//    XIAO WAND電源コントロールライブラリ
//
//      Author  : Shun OSAFUNE (s.osafune@j7system.jp)
//      Release : 2022/07/03 Version 0.93
//              : 2023/06/13 Version 0.94
// ------------------------------------------------------------------------------ //
//
// The MIT License
// Copyright (c) 2022 Shun OSAFUNE / J-7SYSTEM WORKS LIMITED.
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

const int xiaowand_mml_tempo_min = 30;    // MMLで設定可能な最小テンポ数(30)
const int xiaowand_mml_tempo_max = 280;   // MMLで設定可能な最大テンポ数(280)
const int xiaowand_mml_divnote_max = 96;  // MMLで最大音符分割数(96分音符)
const int xaiowand_mml_loop_max = 9;      // MMLの最大ループ回数(9回)

#if defined(XIAOWAND_MODULE_XIAO) || defined(XIAOWAND_MODULE_XIAO_USE_TCC0)
const int xiaowand_pwrsw_pin = 1;       // XIAO WANDの電源ボタン入力ピン(PIN_D1)
const int xiaowand_pwren_pin = 3;       // XIAO WANDの電源制御出力ピン(PIN_D3)
const int xiaowand_buzzer_pin = 0;			// XIAO WANDの圧電ブザーピン(PIN_D0)
#else
const int xiaowand_pwrsw_pin = D1;      // XIAO WANDの電源ボタン入力ピン(PIN_D1)
const int xiaowand_pwren_pin = D3;      // XIAO WANDの電源制御出力ピン(PIN_D3)
const int xiaowand_buzzer_pin = D0;			// XIAO WANDの圧電ブザーピン(PIN_D0)
#endif

#define XIAOWAND_REVISION_B             // XIAO WAND Rev.B/Cの指示


// ------------------------------------------------------------------------------ //
//    電源コントロールライブラリ (xiaowand_power_lib)
// ------------------------------------------------------------------------------ //

// 制御変数
enum xiaowand_power_state_type {STARTUP, ACTIVE, SHUTDOWN, HALT};
volatile enum xiaowand_power_state_type xiaowand_state = STARTUP;

volatile bool xiaowand_event_startup = false;
volatile bool xiaowand_event_shutdown = false;
volatile bool xiaowand_event_press = false;
volatile bool xiaowand_event_release = false;
volatile bool xiaowand_event_longpush = false;
volatile bool xiaowand_event_click = false;
static bool xiaowand_event_exec = false;
static void (*xiaowand_startup_cb)(void) = NULL;
static void (*xiaowand_shutdown_cb)(void) = NULL;
static void (*xiaowand_press_cb)(void) = NULL;
static void (*xiaowand_release_cb)(void) = NULL;
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

// スタートアップイベントが起こったかどうかをチェック（システム側でのみ使用）
static bool xiaowand_check_startup(void) {
  bool res = xiaowand_event_startup;
  xiaowand_event_startup = false;
  return res;
}

// シャットダウンイベントが起こったかどうかをチェック（システム側でのみ使用）
static bool xiaowand_check_shutdown(void) {
  bool res = xiaowand_event_shutdown;
  xiaowand_event_shutdown = false;
  return res;
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

// スタートアップイベントのコールバックを登録
void xiaowand_attach_startup(void (*cb_func)(void)) {
  xiaowand_startup_cb = cb_func;
}

// シャットダウンイベントのコールバックを登録
void xiaowand_attach_shutdown(void (*cb_func)(void)) {
  xiaowand_shutdown_cb = cb_func;
}

// ボタン押下イベントのコールバックを登録
void xiaowand_attach_press(void (*cb_func)(void)) {
  xiaowand_press_cb = cb_func;
}

// ボタンリリースイベントのコールバックを登録
void xiaowand_attach_release(void (*cb_func)(void)) {
  xiaowand_release_cb = cb_func;
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
  while (xiaowand_interval_enable) {
    delay(100);
    xiaowand_power_interval();
  }
}
static void xiaowand_inverval_begin(void) {
  xiaowand_interval_enable = true;
  Scheduler.start(xiaowand_interval_loop);
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
#if defined(XIAOWAND_REVISION_A)
  digitalWrite(xiaowand_pwren_pin, LOW);
#else
  digitalWrite(xiaowand_pwren_pin, HIGH);
#endif

  xiaowand_inverval_begin();
  xiaowand_mml_begin();
}

// 電源OFF処理
void xiaowand_power_end(void) {
  xiaowand_state = SHUTDOWN;
  xiaowand_mml_end();
  xiaowand_interval_end();
  xiaowand_blink(0, 0);     // 回数が0の時はON/OFFを即時反映

#if defined(XIAOWAND_REVISION_A)
  pinMode(xiaowand_pwren_pin, INPUT);
#else
  digitalWrite(xiaowand_pwren_pin, LOW);
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


// ------------------------------------------------------------------------------ //
//    MML演奏ライブラリ (xiaowand_mml_lib)
// ------------------------------------------------------------------------------ //

// 制御変数
enum xiaowand_mml_state_type {
  STOP,
  PLAYING,
  ABORT,
  INIT,
};

enum xiaowand_mml_error_type {
  MML_OK = 0,
  SYNTAX_ERROR = -1,
  OUT_OF_RANGE = -2,
  LOOP_OVERFLOW = -3,
  LOOP_UNDERFLOW = -4,
};

#ifndef XIAOWAND_MML_LOOPNEST_MAX
# define XIAOWAND_MML_LOOPNEST_MAX (10)		// MMLのループの最大ネスト数(10回)
#endif

struct xiaowand_mml_resource_type {
  volatile enum xiaowand_mml_state_type state;
  enum xiaowand_mml_error_type err_code;
  int err_pos;
  volatile bool stop_request;
  volatile bool play_request;
  const char *playmml;
  bool playloop;
  int tempo, octave, volume, panning, length, quantize;
  const char *s, *s_top;
  bool was_note_on;
  int loopstack;
  int loopnum[XIAOWAND_MML_LOOPNEST_MAX];
  const char *s_loop[XIAOWAND_MML_LOOPNEST_MAX];
  int note_ch, note_freq, note_time, note_wait;
};

struct xiaowand_mml_resource_type xiaowand_mml = {
  STOP, MML_OK, 0,
  false, false, NULL, false,
  120, 5, 8, 64, 4, 8,
  NULL, NULL, false,
  -1, {0}, {NULL},
  0, -1, 0, 0
};


///// MMLパーサー /////
// 音源制御ルーチン差し替え用マクロ
#ifndef XIAOWAND_MML_NOTEON
# define XIAOWAND_MML_NOTEON(_p)	tone(xiaowand_buzzer_pin, (_p)->note_freq)
#endif
#ifndef XIAOWAND_MML_NOTEOFF
# define XIAOWAND_MML_NOTEOFF(_p)	noTone(xiaowand_buzzer_pin)
#endif
#ifndef XIAOWAND_MML_SOUNDSTOP
# define XIAOWAND_MML_SOUNDSTOP(_p)	noTone(xiaowand_buzzer_pin)
#endif

// MML文字列の中の数字を取得するサブルーチン
static int xiaowand_mml_get_number(const char *s, int *num) {
  const char *s_top = s;
  int res = 0;

  while (*s >= '0' && *s <= '9') {
    res = res * 10 + (*s - '0');
    s++;
  }
  *num = res;

  return (int)(s - s_top);
}

// MML文字列の中の休符・音符情報を取得するサブルーチン
static int xiaowand_mml_get_tone(struct xiaowand_mml_resource_type *p) {
  const char note_char[] = {'R', 'C', 'c', 'D', 'd', 'E', 'F', 'f', 'G', 'g', 'A', 'a', 'B'};
  const int freq_table[] = {
    // ノート0:休符
    0,
    // ノート1～12:O1
    33,  35,  37,  39,  41,  44,  46,  49,  52,  55,  58,  62,
    // ノート13～24:O2
    65,  69,  73,  78,  82,  87,  92,  98, 104, 110, 117, 123,
    // ノート25～36:O3
    131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
    // ノート37～48:O4
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
    // ノート49～60:O5
    523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
    // ノート61～72:O6
    1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
    // ノート73～84:O7
    2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
    // ノート85～96:O8
    4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
    // ノート97:O8B+
    8372
  };
  const char *s = p->s;
  char c;
  int note_num, note_len = 0, n, len;

  while (1) {
    // 音階を取得
    c = *s;
    if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
    for (note_num = 0; note_num < 13; note_num++) if (c == note_char[note_num]) break;

    if (note_num == 13) {	// 休符でも音符でもない
      note_num = -1;
      break;
    }
    s++;
    if (note_num) {			// 音符
      note_num += (p->octave - 1) * 12;
      if (*s == '+' || *s == '#') {
        note_num++;
        s++;
      } else if (*s == '-') {
        note_num--;
        s++;
      }
    }

    // 音長を取得
    len = xiaowand_mml_get_number(s, &n);
    if (!len) n = p->length;
    if (n < 1 || n > xiaowand_mml_divnote_max) {	// 音長の長さ指定範囲外
      note_len = 0;
      break;
    }
    s += len;
    if (*s == '.') {
      note_len += (60 * 1000 * 6 / (p->tempo * n));	// 付点音符(1.5倍)
      s++;
    } else {
      note_len += (60 * 1000 * 4 / (p->tempo * n));
    }

    // タイの判定
    if (*s != '&') break;
    s++;
  }

  if (note_num >= 0 && note_num <= 97) {
    p->note_freq = freq_table[note_num];
    p->note_time = note_len;
#ifdef XAIOWAND_MML_DEBUG_SAVESTR
    str_debug = p->s;
    str_debug_len = (int)(s - p->s);
#endif
  } else {
    p->note_freq = -1;
  }

  return (int)(s - p->s);
}

// MML文字列を解析して演奏するハンドラ
//	result	< 0 : エラーアボート
//			= 0 : 再生終了
//			> 0 : 次に呼び出されるまでの待ち時間(ms)
static int xiaowand_mml_parse(struct xiaowand_mml_resource_type *p) {
  int	n, len, d;
  int result = 0;

  // 再生開始リクエストの処理
  if (p->play_request) {
    noTone(xiaowand_buzzer_pin);
    p->s_top = p->playmml;
    p->s = p->s_top;
    p->was_note_on = false;
    p->loopstack = -1;
    p->note_freq = -1;
    p->note_time = 0;
    p->err_code = MML_OK;
    p->err_pos = 0;
    p->play_request = false;
    p->state = PLAYING;
  }

  // 再生中のMML解析処理
  while (true) {
    if (p->note_wait) {					// クォンタイズの消音時間
      XIAOWAND_MML_NOTEOFF(p);
      result = p->note_wait;
      p->note_wait = 0;
      break;

    } else if (p->stop_request) {		// 再生停止要求があった
      result = 0;
      p->stop_request = false;
      p->state = STOP;
      XIAOWAND_MML_SOUNDSTOP(p);
      break;

    } else if (!*(p->s)) {
      if (p->loopstack >= 0) {		// 最後まで再生したが閉じられてないループがある
        p->s = p->s_loop[p->loopstack] - 1;
        p->err_code = LOOP_UNDERFLOW;
        p->state = ABORT;
        break;
      } else if (p->was_note_on && p->playloop) {	// 発音がある楽譜ならループ再生指示有効
        p->s = p->s_top;
      } else {						// 再生終了
        p->note_time = 0;
        XIAOWAND_MML_NOTEOFF(p);
        result = 0;
        p->state = STOP;
        break;
      }
    }

    if (*(p->s) == 'T' || *(p->s) == 't') {
      // テンポ設定（xiaowand_mml_tempo_min～xiaowand_mml_tempo_max）
      p->s++;
      len = xiaowand_mml_get_number(p->s, &n);
      if (!len) {
        p->err_code = SYNTAX_ERROR;
        p->state = ABORT;
        break;
      }
      if (n < xiaowand_mml_tempo_min || n > xiaowand_mml_tempo_max) {
        p->err_code = OUT_OF_RANGE;
        p->state = ABORT;
        break;
      }
      p->tempo = n;
      p->s += len;

    } else if (*(p->s) == 'O' || *(p->s) == 'o') {
      // オクターブ設定（1～8）
      p->s++;
      len = xiaowand_mml_get_number(p->s, &n);
      if (!len) {
        p->err_code = SYNTAX_ERROR;
        p->state = ABORT;
        break;
      }
      if (n < 1 || n > 8) {
        p->err_code = OUT_OF_RANGE;
        p->state = ABORT;
        break;
      }
      p->octave = n;
      p->s += len;

    } else if (*(p->s) == '>') {
      // １オクターブ上げ
      p->s++;
      if (p->octave < 8) p->octave++;

    } else if (*(p->s) == '<') {
      // １オクターブ下げ
      p->s++;
      if (p->octave > 1) p->octave--;

    } else if (*(p->s) == 'V' || *(p->s) == 'v') {
      // ボリューム設定（0～15）
      p->s++;
      len = xiaowand_mml_get_number(p->s, &n);
      if (!len) {
        p->err_code = SYNTAX_ERROR;
        p->state = ABORT;
        break;
      }
      if (n < 0 || n > 15) {
        p->err_code = OUT_OF_RANGE;
        p->state = ABORT;
        break;
      }
      p->volume = n;
      p->s += len;

    } else if (*(p->s) == ']') {
      // １ボリューム上げ
      p->s++;
      if (p->volume < 15) p->volume++;

    } else if (*(p->s) == '[') {
      // １ボリューム下げ
      p->s++;
      if (p->volume > 0) p->volume--;

    } else if (*(p->s) == 'P' || *(p->s) == 'p') {
      // パンニング設定（1～127）
      p->s++;
      len = xiaowand_mml_get_number(p->s, &n);
      if (!len) {
        p->err_code = SYNTAX_ERROR;
        p->state = ABORT;
        break;
      }
      if (n < 1 || n > 127) {
        p->err_code = OUT_OF_RANGE;
        p->state = ABORT;
        break;
      }
      p->panning = n;
      p->s += len;

    } else if (*(p->s) == 'L' || *(p->s) == 'l') {
      // 基本音長設定（1～xiaowand_mml_divnote_max）
      p->s++;
      len = xiaowand_mml_get_number(p->s, &n);
      if (!len) {
        p->err_code = SYNTAX_ERROR;
        p->state = ABORT;
        break;
      }
      if (n < 1 || n > xiaowand_mml_divnote_max) {
        p->err_code = OUT_OF_RANGE;
        p->state = ABORT;
        break;
      }
      p->length = n;
      p->s += len;

    } else if (*(p->s) == 'Q' || *(p->s) == 'q') {
      // クオンタイズ設定（1～8）
      p->s++;
      len = xiaowand_mml_get_number(p->s, &n);
      if (!len) {
        p->err_code = SYNTAX_ERROR;
        p->state = ABORT;
        break;
      }
      if (n < 1 || n > 8) {
        p->err_code = OUT_OF_RANGE;
        p->state = ABORT;
        break;
      }
      p->quantize = n;
      p->s += len;

    } else if (*(p->s) == '(') {
      // ループ先頭
      if (p->loopstack >= XIAOWAND_MML_LOOPNEST_MAX - 1) {
        p->err_code = LOOP_OVERFLOW;
        p->state = ABORT;
        break;
      }
      p->s++;
      p->loopstack++;
      p->s_loop[p->loopstack] = p->s;
      p->loopnum[p->loopstack] = 1;

    } else if (*(p->s) == ')') {
      // ループ末尾（回数省略時は2）
      if (p->loopstack < 0) {
        p->err_code = LOOP_UNDERFLOW;
        p->state = ABORT;
        break;
      }
      p->s++;
      len = xiaowand_mml_get_number(p->s, &n);
      if (!len) n = 2;
      if (n < 2 || n > xaiowand_mml_loop_max) {
        p->err_code = OUT_OF_RANGE;
        p->state = ABORT;
        break;
      }
      p->s += len;
      if (p->loopnum[p->loopstack] < n) {
        p->loopnum[p->loopstack]++;
        p->s = p->s_loop[p->loopstack];
      } else {
        p->loopstack--;
      }

    } else {
      // 休符または音符
      p->s += xiaowand_mml_get_tone(p);
      if (p->note_freq < 0) {
        p->err_code = SYNTAX_ERROR;
        p->state = ABORT;
        break;
      }
      if (p->note_time == 0) {
        p->err_code = OUT_OF_RANGE;
        p->state = ABORT;
        break;
      }

      // 発音
      if (p->note_freq) {
        if (p->quantize < 8) {
          d = (p->note_time * p->quantize + 7) / 8;
          p->note_wait = p->note_time - d;
          p->note_time = d;
        } else {
          p->note_wait = 0;
        }
        XIAOWAND_MML_NOTEON(p);
        p->was_note_on = true;
      } else {
        XIAOWAND_MML_NOTEOFF(p);
      }
      result = p->note_time;
      break;
    }
  }

  // エラー中断処理
  if (p->state == ABORT) {
    p->err_pos = (int)(p->s - p->s_top) + 1;
    result = (int)p->err_code;
    XIAOWAND_MML_SOUNDSTOP(p);
  }

  return result;
}

// MML演奏スレッド処理のラッピング
// XIAO WAND電源コントロールで使用しているリソースにあわせる
#if defined(XIAOWAND_MML_THREADS_DISABLE)
# define _XIAOWAND_MML_NO_THREADS
#elif defined(XIAOWAND_MODULE_XIAO_BLE)
# if defined(SOFTWARETIMER_H_)
#  define _XIAOWAND_MML_USE_SOFTWARETIMER
# else
#  define _XIAOWAND_MML_USE_SCHEDULER
# endif
#elif defined(XIAOWAND_MODULE_XIAO_RP2040)
# define _XIAOWAND_MML_USE_SCHEDULER
#elif defined(XIAOWAND_MODULE_XIAO) || defined(XIAOWAND_MODULE_XIAO_USE_TCC0)
# define _XIAOWAND_MML_USE_INTERVAL
#else
# error There is no XIAO module name or interval timer resource indication.
#endif

// XIAO BLEでRTOSソフトウェアタイマーを使っている時
#ifdef _XIAOWAND_MML_USE_SOFTWARETIMER
SoftwareTimer MMLtimer;
static void xiaowand_mml_handler(void) {
  MMLtimer.stop();
  if (xiaowand_mml.state == PLAYING || xiaowand_mml.play_request) {
    int res = xiaowand_mml_parse(&xiaowand_mml);
    if (res > 0) {
      MMLtimer.setPeriod(res);
      MMLtimer.start();
    }
  }
}
static void xiaowand_mml_loop(TimerHandle_t xTimerID) {
  (void) xTimerID;
  xiaowand_mml_handler();
}
static void xiaowand_mml_handler_begin(void) {
  MMLtimer.begin(100, xiaowand_mml_loop);
  MMLtimer.stop();
}
static void xiaowand_mml_handler_end(void) {
  MMLtimer.stop();
}
#undef _XIAOWAND_MML_USE_SOFTWARETIMER
#endif

// XIAO BLE/XIAO RP2040でタスクスケジューラーを使っている時
#ifdef _XIAOWAND_MML_USE_SCHEDULER
#include <Scheduler.h>
volatile bool xiaowand_mml_handler_enable = false;
static void xiaowand_mml_handler(void) {
}
static void xiaowand_mml_loop(void) {
  while (xiaowand_mml_handler_enable) {
    if (xiaowand_mml.state == PLAYING || xiaowand_mml.play_request) {
      int res = xiaowand_mml_parse(&xiaowand_mml);
      if (res > 0) delay(res);
    } else {
      yield();
    }
  }
}
static void xiaowand_mml_handler_begin(void) {
  xiaowand_mml_handler_enable = true;
  Scheduler.start(xiaowand_mml_loop);
}
static void xiaowand_mml_handler_end(void) {
  xiaowand_mml_handler_enable = false;
}
#undef _XIAOWAND_MML_USE_SCHEDULER
#endif

// XIAOでTC3/TCC0タイマー使っている時
#ifdef _XIAOWAND_MML_USE_INTERVAL
static int xiaowand_mml_delaycounter = 0;
static void xiaowand_mml_handler(void) {
  xiaowand_mml_delaycounter = 0;
}
static void xiaowand_mml_loop(void) {
  if (xiaowand_mml_delaycounter) {
    xiaowand_mml_delaycounter--;
  } else if (xiaowand_mml.state == PLAYING || xiaowand_mml.play_request) {
    int res = xiaowand_mml_parse(&xiaowand_mml);
    if (res > 0) xiaowand_mml_delaycounter = res - 1;
  }
}
static void xiaowand_mml_handler_begin(void) {
  xiaowand_interval_attach(xiaowand_mml_loop);
}
static void xiaowand_mml_handler_end(void) {
  xiaowand_interval_attach(NULL);
}
#undef _XIAOWAND_MML_USE_INTERVAL
#endif

// スレッド処理しない時
#ifdef _XIAOWAND_MML_NO_THREADS
static void xiaowand_mml_handler(void) {
  xiaowand_mml.playloop = false;		// シングルスレッド時はループ再生はできない

  if (xiaowand_is_eventcb()) return;	// コールバックの中で呼ばれた場合はそのまま戻る
  while (true) {
    int res = xiaowand_mml_parse(&xiaowand_mml);
    if (res < 1) break;
    delay(res);
  }
}
static void xiaowand_mml_handler_begin(void) {
}
static void xiaowand_mml_handler_end(void) {
  xiaowand_mml.play_request = false;
  xiaowand_mml.stop_request = true;
}
#undef _XIAOWAND_MML_NO_THREADS
#endif


///// MMLサービスAPI /////
// MMLサービスの初期化
void xiaowand_mml_begin(void) {
  struct xiaowand_mml_resource_type *p = &xiaowand_mml;

  p->state = INIT;
  p->err_code = MML_OK;
  p->err_pos = 0;
  p->stop_request = false;
  p->play_request = false;
  p->playmml = NULL;
  p->playloop = false;

  p->tempo = 120;
  p->octave = 5;
  p->volume = 8;
  p->panning = 64;
  p->length = 4;
  p->quantize = 8;

  p->s_top = NULL;
  p->s = NULL;
  p->was_note_on = false;
  p->loopstack = -1;
  p->note_ch = 0;
  p->note_freq = -1;
  p->note_time = 0;
  p->note_wait = 0;

  // noToneだけを先に呼ぶとハングアップするのを防止するワークアラウンド
  tone(xiaowand_buzzer_pin, 440);
  noTone(xiaowand_buzzer_pin);
  //XIAOWAND_MML_SOUNDSTOP(p);
  p->state = STOP;

  xiaowand_mml_handler_begin();
}

// MMLサービスの終了
void xiaowand_mml_end(void) {
  xiaowand_mml_handler_end();
  XIAOWAND_MML_SOUNDSTOP(&xiaowand_mml);
}

// MMLが再生中かどうか確認
bool xiaowand_is_mmlplay(void) {
  return (xiaowand_mml.state == PLAYING);
}

// MML演奏停止のリクエスト
void xiaowand_mml_stop(void) {
  if (xiaowand_is_mmlplay()) xiaowand_mml.stop_request = true;
}

// MML演奏開始のリクエスト
void xiaowand_mml_play(const char *mml_str, const bool loop) {
  xiaowand_mml.playmml = mml_str;
  xiaowand_mml.playloop = loop;
  xiaowand_mml.play_request = true;

  xiaowand_mml_handler();
}

// MMLパーサーのエラーステータスを取得
void xiaowand_mml_status(int *err_code, int *err_pos) {
  *err_code = (int)xiaowand_mml.err_code;
  *err_pos = (int)xiaowand_mml.err_pos;

#ifdef XAIOWAND_MML_DEBUG_SAVESTR
  switch (xiaowand_mml.err_code) {
    case MML_OK:
      break;
    case SYNTAX_ERROR:
      printf("[!] MML Syntax error.\n");
      break;
    case OUT_OF_RANGE:
      printf("[!] Parameter out of range.\n");
      break;
    case LOOP_OVERFLOW:
      printf("[!] Too many nested loops.\n");
      break;
    case LOOP_UNDERFLOW:
      printf("[!] No corresponding loop.\n");
      break;
    default:
      printf("[!] Unknown error.\n");
  }
#endif
}
