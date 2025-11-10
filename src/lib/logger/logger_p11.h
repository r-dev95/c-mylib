/**
 * ログ処理用ヘッダ。
 *
 * - C11標準: threads
 */

#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

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

static log_item_t* log_item_init(void);
static void log_item_del(log_item_t* item);
static bool format_init(const char* fmt);
static void format_del(void);
static bool fp_init(const char* fpath);
static void fp_del(void);
static bool fp_setvbuf(const size_t bufsize);
static bool queue_init(const size_t nqueue);
static void queue_del(void);
static bool mutex_lock(mtx_t* mutex);
static bool mutex_unlock(mtx_t* mutex);
static bool cond_signal(cnd_t* cond);
static bool cond_wait(cnd_t* cond, mtx_t* mutex);
static char* get_level_name(const log_level_t level);
static bool realloc_format_line(
    char** pout, size_t* cap, const size_t needed_size
);
static char* format_line(const log_item_t* item);
static void output_line(const log_item_t* item);
static bool enqueue_item(log_item_t* item);
static log_item_t* dequeue_item_none_mutex(void);
static log_item_t* dequeue_item(void);
static int worker(void* arg);
static void logger_set_level(const log_level_t level);
static bool logger_set_stream(const char* fpath, const size_t bufsize);
static bool logger_set_async(const bool async, const size_t nqueue);

// デフォルトフォーマット
static const char* DEFAULT_FORMAT = "[%T][%l][%F:%L][%f()] - %m";
// 可変長引数の小さいバッファサイズ
static const int SMALL_VAR_SIZE = 256;
// ログの変換指定子1つ分の最大バッファサイズ
static const int MAX_CONV_SPEC_SIZE = 256;
// 作成するログ1行分の最小バッファサイズ
static const size_t MIN_LOG_SIZE = 1024;

// ログ出力用のファイルポインタ
static FILE* g_fp = NULL;
// ログレベル
static log_level_t g_level = LOG_LEVEL_INFO;
// ログフォーマットのポインタ
static char* g_format = NULL;
// 非同期モードフラグ
static bool g_async = false;
// 非同期モード用: 排他制御用mutex
static mtx_t g_mutex;
// 非同期モード用: 排他制御用cond
static cnd_t g_cond;
// 非同期モード用: スレッドID
static thrd_t g_worker;
// 非同期モード用: 実行フラグ
static bool g_worker_running = false;
// 非同期モード用: キューに格納するログデータの最大数
static size_t g_nqueue = 1024;
// 非同期モード用: キュー
static log_item_t** g_queue = NULL;
// 非同期モード用: キューの先頭番号
static size_t g_q_head = 0;
// 非同期モード用: キューの末尾番号
static size_t g_q_tail = 0;
// 非同期モード用: キューに格納されているログデータの数
static size_t g_q_count = 0;

#ifdef __cplusplus
}
#endif
