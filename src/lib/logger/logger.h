/**
 * ログ処理用公開ヘッダ。
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ログレベル
typedef enum {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR,
} log_level_t;

bool logger_init(
    const char* fpath, const log_level_t level, const size_t bufsize,
    const bool async, const size_t nqueue
);
void logger_close(void);
bool logger_set_format(const char* fmt);
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
