/**
 * ログ処理用公開ヘッダ。
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ログ出力フラグ
typedef enum {
  LOG_STD_OUT = 1,  // 標準出力
  LOG_FILE_OUT,     // ファイル出力
  LOG_BOTH_OUT,     // 両方
} log_out_t;

// ログレベル
typedef enum {
  LOG_LEVEL_DEBUG = 0,  // デバッグ
  LOG_LEVEL_INFO,       // 情報
  LOG_LEVEL_WARN,       // 警告
  LOG_LEVEL_ERROR,      // エラー
} log_level_t;

bool logger_init(
    const log_out_t out, const log_level_t level, const char* fmt,
    const bool async, const char* fpath
);
void logger_close(void);
void logger_log(
    const log_level_t level, const char* fpath, const char* func,
    const int line, const char* fmt, ...
);

/**
 * @brief ログ出力用マクロ。（デバッグ）
 */
#define LOG_DEBUG(...) \
  logger_log(LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__)
/**
 * @brief ログ出力用マクロ。（情報）
 */
#define LOG_INFO(...) \
  logger_log(LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, __VA_ARGS__)
/**
 * @brief ログ出力用マクロ。（警告）
 */
#define LOG_WARN(...) \
  logger_log(LOG_LEVEL_WARN, __FILE__, __func__, __LINE__, __VA_ARGS__)
/**
 * @brief ログ出力用マクロ。（エラー）
 */
#define LOG_ERROR(...) \
  logger_log(LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
