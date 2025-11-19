/**
 * ユーティリティ関数群。
 */

#include "utils.h"

// エラーメッセージバッファ
static thread_local char* err_msg_buff = NULL;

/**
 * @brief エラーメッセージを設定する。
 * @param fpath ファイルパス。
 * @param line 行数。
 * @param fmt フォーマット。
 * @param ... 可変長引数。
 */
void set_error_msg(
    const char* fpath, const char* func, const int line, const char* fmt, ...
) {
#ifndef _LIB_ERRMSG_OFF_

  if (!fmt) { return; }

  if (err_msg_buff) {
    free(err_msg_buff);
    err_msg_buff = NULL;
  }

  const char* safe_fpath = fpath ? fpath : "";
  const char* safe_func = func ? func : "";

  va_list ap;
  va_start(ap, fmt);

  // 必要サイズの取得
  va_list ap_copy;
  va_copy(ap_copy, ap);
  int needed = vsnprintf(NULL, 0, fmt, ap_copy);
  va_end(ap_copy);
  if (needed < 0) {
    va_end(ap);
    return;
  }

  // フォーマット部分のメモリ確保
  char* formatted = malloc((size_t)needed + 1);
  if (!formatted) {
    va_end(ap);
    return;
  }

  if (vsnprintf(formatted, (size_t)needed + 1, fmt, ap) < 0) {
    free(formatted);
    va_end(ap);
    return;
  }
  va_end(ap);

  // 全体サイズの取得（file:line + formatted）
  int size = snprintf(
      NULL, 0, "%s:%d (%s): %s", safe_fpath, line, safe_func, formatted
  );
  if (size < 0) {
    free(formatted);
    return;
  }

  err_msg_buff = malloc((size_t)size + 1);
  if (!err_msg_buff) {
    free(formatted);
    return;
  }

  snprintf(
      err_msg_buff, (size_t)size + 1, "%s:%d (%s): %s", safe_fpath, line,
      safe_func, formatted
  );
  free(formatted);

#endif  // _LIB_ERRMSG_OFF_
}

/**
 * @brief エラーメッセージを標準エラーに出力する。
 */
void out_error_msg(void) {
#ifndef _LIB_ERRMSG_OFF_

  if (!err_msg_buff) {
    err_msg_buff = my_strdup("エラーメッセージが設定されていません。\n");
  }

  fprintf(stderr, "❌ %s", err_msg_buff);

  free(err_msg_buff);
  err_msg_buff = NULL;

#endif  // _LIB_ERRMSG_OFF_
}

/**
 * @brief 現在時刻を取得する。
 * @return tm (ISO C `broken-down time' structure.)
 */
struct tm get_current_time(void) {
  time_t now = time(NULL);
  struct tm* tm = localtime(&now);
  return *tm;
}

/**
 * @brief ファイルパスからファイル名を取得する。
 * @param path ファイルパス。
 * @return ファイル名。
 */
char* get_fname(const char* fpath) {
  char* slash = strrchr(fpath, '/');       // Posix形式
  char* backslash = strrchr(fpath, '\\');  // Windows形式

  char* pos = slash;
  if (backslash > pos) pos = backslash;  // より後方の区切り文字を選択

  return pos ? pos + 1 : (char*)fpath;
}

/**
 * @brief 3つの文字列を結合する。
 * @param out 結合した文字列の出力先。
 * @param lstr 左側の文字列。
 * @param cstr 真中の文字列。
 * @param rstr 右側の文字列。
 * @return 成功: true, 失敗: false。
 */
bool joinstr(char* out, const char* lstr, const char* cstr, const char* rstr) {
  if (!out || !lstr || !cstr || !rstr) {
    SET_ERR_MSG(
        "結合した文字列の出力先または左側、真中、右側の文字列が設定されていませ"
        "ん。\n"
    );
    return false;
  }

  strcpy(out, lstr);
  strcat(out, cstr);
  strcat(out, rstr);

  return true;
}

/**
 * @brief 文字列を新たなメモリに格納してポインタを返す。（自作strdup）
 * @param str 文字列。
 * @return 文字列を格納したポインタ。
 */
char* my_strdup(const char* str) {
  if (!str) {
    SET_ERR_MSG("文字列が設定されていません。\n");
    return NULL;
  }
  size_t len = strlen(str) + 1;
  char* copy = malloc(len);
  if (!copy) {
    SET_ERR_MSG("文字列のメモリを確保できません。\n");
    return NULL;
  }
  memcpy(copy, str, len);
  return copy;
}

/**
 * @brief ファイルストリームから1行取得する。
 * @param line 出力バッファ。
 * @param size 出力バッファサイズ。
 * @param stream ファイルストリーム。
 * @return 読み込んだ1行分のサイズ。
 */
size_t my_getline(char** line, size_t* size, FILE* stream) {
  // lineはNULLを許容する
  if (!size || !stream) {
    SET_ERR_MSG(
        "出力バッファサイズまたはファイルストリームが設定されていません。\n"
    );
    return 0;
  }

  int c;
  size_t pos = 0;

  // 初期バッファの確保
  if (*line == NULL || *size == 0) {
    *size = 128;
    *line = malloc(*size);
    if (*line == NULL) {
      SET_ERR_MSG("出力バッファのメモリを確保できません。\n");
      return 0;
    }
  }

  while ((c = fgetc(stream)) != EOF) {
    // バッファ拡張
    if (pos + 1 >= *size) {
      size_t new_size = *size * 2;
      char* new_ptr = realloc(*line, new_size);
      if (new_ptr == NULL) {
        SET_ERR_MSG("出力バッファのメモリを再確保できません。\n");
        return 0;
      }

      *line = new_ptr;
      *size = new_size;
    }

    (*line)[pos++] = (char)c;

    if (c == '\n') break;  // 行の終わり
  }

  // EOFかつ何も読んでいない場合
  if (pos == 0 && c == EOF) {
    SET_ERR_MSG("ファイルストリームから何も読み込めていません。\n");
    return 0;
  }

  (*line)[pos] = '\0';
  return pos;
}

/**
 * @brief 文字列末尾の改行を削除する。
 * @param str 文字列。
 * @param len 文字列の長さ。
 */
void remove_newline(char* str, size_t len) {
  if (!str) { return; }

  while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
    str[--len] = '\0';
  }
}

/**
 * @brief 文字列前後の空白を削除する。
 * @param str 文字列。
 */
void remove_spaces(char* str) {
  if (!str) { return; }

  // 左側
  char* ptr = str;
  while (*ptr && isspace((unsigned char)*ptr)) ptr++;
  if (ptr != str) memmove(str, ptr, strlen(ptr) + 1);

  // 右側
  size_t len = strlen(str);
  while (len > 0 && isspace((unsigned char)str[len - 1])) { str[--len] = '\0'; }
}

/**
 * @brief 文字列前後のクォーテーションを削除する。
 * @param str 文字列。
 */
void remove_quotes(char* str) {
  if (!str) { return; }

  size_t len = strlen(str);
  if (len >= 2 && ((str[0] == '"' && str[len - 1] == '"') ||
                   (str[0] == '\'' && str[len - 1] == '\''))) {
    if (len > 2) memmove(str, str + 1, len - 2);
    str[len - 2] = '\0';
  }
}
