/**
 * ログ処理用ヘッダ。
 *
 * - Posix (Linux/Mac OS)標準: pthread
 */

#pragma once

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

#ifdef __cplusplus
extern "C" {
#endif

// ログデータ
typedef struct log_item {
  log_level_t level;
  char* fname;
  char* func;
  int line;
  char* msg;
} log_item_t;

// パラメータ
typedef struct {
  log_out_t out;          // ログ出力フラグ
  FILE* fp;               // ログ出力用のファイルポインタ
  log_level_t level;      // ログレベル
  char* format;           // ログフォーマットのポインタ
  bool async;             // 非同期モードフラグ
  pthread_mutex_t mutex;  // 非同期モード: 排他制御用mutex
  pthread_cond_t cond;    // 非同期モード: 排他制御用cond
  pthread_t worker;       // 非同期モード: スレッドID
  bool worker_running;    // 非同期モード: 実行フラグ
  size_t nqueue;          // 非同期モード: キューに格納するログデータの最大数
  log_item_t** queue;     // 非同期モード: キュー
  size_t q_head;          // 非同期モード: キューの先頭番号
  size_t q_tail;          // 非同期モード: キューの末尾番号
  size_t q_count;         // 非同期モード: キューに格納されているログデータの数
} log_param_t;

// デフォルトフォーマット
static const char* DEFAULT_FORMAT = "[%T][%l][%F:%L][%f()] - %m";
// 可変長引数の小さいバッファサイズ
static const size_t SMALL_VAR_SIZE = 256;
// ログの変換指定子1つ分の最大バッファサイズ
static const size_t MAX_CONV_SPEC_SIZE = 256;
// 作成するログ1行分の最小バッファサイズ
static const size_t MIN_LOG_SIZE = 1024;

// パラメータの初期化
static log_param_t g_param = {
    .out = LOG_BOTH_OUT,
    .fp = NULL,
    .level = LOG_LEVEL_INFO,
    .format = NULL,
    .async = true,
    .mutex = {{0}},
    .cond = {{{0}}},
    .worker = 0,
    .worker_running = false,
    .nqueue = 1024,
    .queue = NULL,
    .q_head = 0,
    .q_tail = 0,
    .q_count = 0,
};

static log_item_t* log_item_init(void);
static void log_item_destroy(log_item_t* item);
static bool format_init(const char* fmt);
static void format_destroy(void);
static bool fp_init(const char* fpath);
static void fp_destroy(void);
static bool fp_setvbuf(const size_t bufsize);
static bool queue_init(const size_t nqueue);
static void queue_destroy(void);
static bool mutex_lock(pthread_mutex_t* mutex);
static bool mutex_unlock(pthread_mutex_t* mutex);
static bool cond_signal(pthread_cond_t* cond);
static bool cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
static char* get_level_name(const log_level_t level);
static bool realloc_format_line(
    char** pout, size_t* cap, const size_t needed_size
);
static char* format_line(const log_item_t* item);
static void output_line(const log_item_t* item);
static bool enqueue_item(log_item_t* item);
static log_item_t* dequeue_item_none_mutex(void);
static log_item_t* dequeue_item(void);
static void* worker(void* arg);
static void logger_set_out(const log_out_t out);
static void logger_set_level(const log_level_t level);
static bool logger_set_stream(const char* fpath, const size_t bufsize);
static bool logger_set_async(const bool async, const size_t nqueue);

#ifdef __cplusplus
}
#endif
