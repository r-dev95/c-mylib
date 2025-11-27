/**
 * エラー処理用ヘッダ。
 */

#pragma once

#include <threads.h>

#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

// エラーログリストの初期化
static thread_local error_log_list_t g_error_log = {0};

#ifdef __cplusplus
}
#endif