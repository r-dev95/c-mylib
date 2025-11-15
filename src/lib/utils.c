/**
 * ユーティリティ関数群。
 */

#include "utils.h"

/**
 * @brief 可変長引数を標準エラー出力する。
 *
 * - エラーハンドリング時のエラーメッセージ出力用。
 * - エラー出力をしない場合、_ERROUT_OFF_を定義する。
 * @param fmt フォーマット。
 * @param ... 可変長引数。
 */
void errout(const char* fmt, ...) {
#ifndef _ERROUT_OFF_
  int len;
  va_list ap, ap2;
  char* msg = NULL;
  char tmp_buff[128];

  // 可変長引数のフォーマット
  va_start(ap, fmt);
  va_copy(ap2, ap);

  len = vsnprintf(tmp_buff, sizeof(tmp_buff), fmt, ap2);
  va_end(ap2);

  if (len < 0) {
    msg = my_strdup("<format-error>");
  } else if ((size_t)len < sizeof(tmp_buff)) {
    msg = my_strdup(tmp_buff);
  } else {
    size_t size = (size_t)len + 1;
    msg = (char*)malloc(size);
    if (!msg) {
      va_end(ap);
      return;
    }
    if (vsnprintf(msg, size, fmt, ap) < 0) {
      free(msg);
      va_end(ap);
      return;
    }
  }
  va_end(ap);

  fprintf(stderr, "❌ %s", msg);
#endif  // _ERROUT_OFF_
}

/**
 * @brief 現在時刻を取得する。
 * @return tm (ISO C `broken-down time' structure.)
 */
struct tm get_current_time() {
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
    errout(
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
    errout("文字列が設定されていません。\n");
    return NULL;
  }
  size_t len = strlen(str) + 1;
  char* copy = malloc(len);
  if (!copy) {
    errout("文字列のメモリを確保できません。\n");
    return NULL;
  }
  memcpy(copy, str, len);
  return copy;
}

size_t my_getline(char** pline, size_t* size, FILE* stream) {
  if (pline == NULL || size == NULL || stream == NULL) { return 0; }

  int c;
  size_t pos = 0;

  // 初期バッファの確保
  if (*pline == NULL || *size == 0) {
    *size = 128;
    *pline = malloc(*size);
    if (*pline == NULL) { return 0; }
  }

  while ((c = fgetc(stream)) != EOF) {
    // バッファ拡張
    if (pos + 1 >= *size) {
      size_t new_size = *size * 2;
      char* new_ptr = realloc(*pline, new_size);
      if (new_ptr == NULL) { return 0; }

      *pline = new_ptr;
      *size = new_size;
    }

    (*pline)[pos++] = (char)c;

    if (c == '\n') break;  // 行の終わり
  }

  // EOFかつ何も読んでいない場合
  if (pos == 0 && c == EOF) { return 0; }

  (*pline)[pos] = '\0';
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
