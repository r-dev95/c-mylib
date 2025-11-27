/**
 * エラー処理用公開ヘッダ。
 */

#pragma once

#include <stdarg.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// [ユーザが設定変更可能] エラーログの最大数
#ifndef ERRLOG_MAX_NUM
#define ERRLOG_MAX_NUM 128
#endif

// [ユーザが設定変更可能] エラーログのファイルパスバイト数
#ifndef ERRLOG_FPATH_LEN
#define ERRLOG_FPATH_LEN 128
#endif

// [ユーザが設定変更可能] エラーログの関数名バイト数
#ifndef ERRLOG_FUNC_LEN
#define ERRLOG_FUNC_LEN 128
#endif

// [ユーザが設定変更可能] エラーログのメッセージバイト数
#ifndef ERRLOG_MSG_LEN
#define ERRLOG_MSG_LEN 128
#endif

/**
 * @brief エラーコード。
 */
typedef enum {
  ERR_OK = 0,
  // メモリ関連
  ERR_MEM_ALLOC_FAILED = 1,  // malloc/calloc/realloc が失敗
  ERR_MEM_NULL_POINTER = 2,  // NULL ポインタが渡された／不正利用
  ERR_MEM_OUT_OF_RANGE = 3,  // 領域外アクセスやサイズ異常
  // ファイル関連
  ERR_FILE_OPEN_FAILED = 10,   // fopen 失敗
  ERR_FILE_READ_FAILED = 11,   // fread 失敗
  ERR_FILE_WRITE_FAILED = 12,  // fwrite 失敗
  ERR_FILE_SEEK_FAILED = 13,   // fseek/ftell 失敗
  ERR_FILE_INVALID_PATH = 14,  // パスが不正
  // IO関連
  ERR_IO_ERROR = 20,    // 何らかの入出力エラー
  ERR_IO_TIMEOUT = 21,  // デバイス応答なし
  ERR_IO_CLOSED = 22,   // 閉じたハンドルを操作
  // パラメータ関連
  ERR_INVALID_ARG = 30,    // 不正なパラメータ
  ERR_INVALID_STATE = 31,  // オブジェクトの状態が不正
  ERR_OUT_OF_RANGE = 32,   // 配列添字や範囲外の値
  // スレッド・同期関連
  ERR_THREAD_CREATE_FAILED = 40,     // pthread_create 失敗
  ERR_MUTEX_INIT_FAILED = 41,        // pthread_mutex_init 失敗
  ERR_MUTEX_LOCK_FAILED = 42,        // pthread_mutex_lock 失敗
  ERR_MUTEX_UNLOCK_FAILED = 43,      // pthread_mutex_unlock 失敗
  ERR_CONDITION_INIT_FAILED = 44,    // pthread_cond_init 失敗
  ERR_CONDITION_SIGNAL_FAILED = 45,  // pthread_cond_signal 失敗
  ERR_CONDITION_WAIT_FAILED = 46,    // pthread_cond_wait 失敗
  // ネットワーク関連
  ERR_NET_CONNECT_FAILED = 50,  // 接続失敗
  ERR_NET_SEND_FAILED = 51,     // 送信失敗
  ERR_NET_RECV_FAILED = 52,     // 受信失敗
  // 汎用
  ERR_PERMISSION_DENIED = 60,  // 権限不足
  ERR_RESOURCE_BUSY = 61,      // リソースがビジー状態
  ERR_NOT_IMPLEMENTED = 62,    // 未実装機能
  ERR_UNKNOWN = 99             // 予期しないエラー
} error_code_e;

/**
 * @brief エラーログデータ。
 */
typedef struct {
  struct tm tm;                  // 記録時刻
  char fpath[ERRLOG_FPATH_LEN];  // ファイルパス
  char func[ERRLOG_FUNC_LEN];    // 関数名
  size_t line;                   // 行数
  error_code_e code;             // エラーコード
  char msg[ERRLOG_MSG_LEN];      // メッセージ
} error_log_t;

/**
 * @brief エラーログデータリスト。
 */
typedef struct {
  error_log_t log[ERRLOG_MAX_NUM];  // エラーログデータ
  size_t head;                      // データの先頭インデックス
  size_t count;                     // データ数
} error_log_list_t;

const char* code_to_msg(error_code_e code);
void set_error_log(
    const char* fpath, const char* func, const size_t line, error_code_e code,
    const char* fmt, ...
);
error_log_t get_error_log(void);
error_log_list_t get_error_log_all(void);

#define SET_ERR_LOG(code, ...) \
  set_error_log(__FILE__, __func__, __LINE__, code, __VA_ARGS__)

#define SET_ERR_LOG_AUTO(code) \
  set_error_log(__FILE__, __func__, __LINE__, code, "%s", code_to_msg(code))

#ifdef __cplusplus
}
#endif