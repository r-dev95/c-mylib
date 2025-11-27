/**
 * エラー処理用関数群。
 */

#include "error_log.h"

#include "utils.h"

/**
 * @brief エラーコードに対応するメッセージを返す。
 * @param code エラーコード。
 * @return エラーメッセージ。
 */
const char* code_to_msg(error_code_e code) {
  switch (code) {
    case ERR_OK: {
      return "No error";
    }
    case ERR_MEM_ALLOC_FAILED: {
      return "Memory allocation failed";
    }
    case ERR_MEM_NULL_POINTER: {
      return "Null pointer";
    }
    case ERR_MEM_OUT_OF_RANGE: {
      return "Memory out of range";
    }
    case ERR_FILE_OPEN_FAILED: {
      return "File open failed";
    }
    case ERR_FILE_READ_FAILED: {
      return "File read failed";
    }
    case ERR_FILE_WRITE_FAILED: {
      return "File write failed";
    }
    case ERR_FILE_SEEK_FAILED: {
      return "File seek failed";
    }
    case ERR_FILE_INVALID_PATH: {
      return "Invalid file path";
    }
    case ERR_IO_ERROR: {
      return "IO error";
    }
    case ERR_IO_TIMEOUT: {
      return "IO timeout";
    }
    case ERR_IO_CLOSED: {
      return "IO closed";
    }
    case ERR_INVALID_ARG: {
      return "Invalid argument";
    }
    case ERR_INVALID_STATE: {
      return "Invalid state";
    }
    case ERR_OUT_OF_RANGE: {
      return "Out of range";
    }
    case ERR_THREAD_CREATE_FAILED: {
      return "Thread create failed";
    }
    case ERR_MUTEX_INIT_FAILED: {
      return "Mutex init failed";
    }
    case ERR_MUTEX_LOCK_FAILED: {
      return "Mutex lock failed";
    }
    case ERR_MUTEX_UNLOCK_FAILED: {
      return "Mutex unlock failed";
    }
    case ERR_CONDITION_INIT_FAILED: {
      return "Condition init failed";
    }
    case ERR_CONDITION_SIGNAL_FAILED: {
      return "Condition signal failed";
    }
    case ERR_CONDITION_WAIT_FAILED: {
      return "Condition wait failed";
    }
    case ERR_NET_CONNECT_FAILED: {
      return "Network connect failed";
    }
    case ERR_NET_SEND_FAILED: {
      return "Network send failed";
    }
    case ERR_NET_RECV_FAILED: {
      return "Network recv failed";
    }
    case ERR_PERMISSION_DENIED: {
      return "Permission denied";
    }
    case ERR_RESOURCE_BUSY: {
      return "Resource busy";
    }
    case ERR_NOT_IMPLEMENTED: {
      return "Not implemented";
    }
    case ERR_UNKNOWN: {
      return "Unknown error";
    }
    default: {
      return "Unrecognized error code";
    }
  }
}

/**
 * @brief エラーログを設定する。
 * @param fpath ファイルパス。
 * @param line 行数。
 * @param fmt フォーマット。
 * @param ... 可変長引数。
 */
void set_error_log(
    const char* fpath, const char* func, const size_t line, error_code_e code,
    const char* fmt, ...
) {
  error_log_t log;
  log.code = code;
  log.line = line;
  log.tm = get_current_time();
  strncpy(log.fpath, fpath ? fpath : "", ERRLOG_FPATH_LEN);
  strncpy(log.func, func ? func : "", ERRLOG_FUNC_LEN);

  va_list ap;
  va_start(ap, fmt);

  log.msg[0] = '\0';
  if (fmt) {
    int nsize = vsnprintf(log.msg, ERRLOG_MSG_LEN, fmt, ap);
    if (nsize > 0) {
      log.msg[MIN(nsize, ERRLOG_MSG_LEN - 1)] = '\0';
    } else {
      log.msg[0] = '\0';
    }
  }
  va_end(ap);

  g_error_log.log[g_error_log.head] = log;
  g_error_log.head = (g_error_log.head + 1) % ERRLOG_MAX_NUM;
  if (g_error_log.count < ERRLOG_MAX_NUM) { g_error_log.count++; }
}

/**
 * @brief 最新のエラーログを取得する。
 * @return エラーログ。
 */
error_log_t get_error_log(void) {
  error_log_t log = {0};
  size_t idx = (g_error_log.head + ERRLOG_MAX_NUM - 1) % ERRLOG_MAX_NUM;
  if (g_error_log.count != 0) { log = g_error_log.log[idx]; }
  return log;
}

/**
 * @brief すべてのエラーログを取得する。
 * @return エラーログリスト。
 */
error_log_list_t get_error_log_all(void) { return g_error_log; }
