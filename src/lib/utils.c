/**
 * ユーティリティ関数群。
 */

#include "utils.h"

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
 * @param out 結合した文字列を出力するポインタ。
 * @param lstr 左側の文字列。
 * @param cstr 真中の文字列。
 * @param rstr 右側の文字列。
 * @return 成功: true, 失敗: false。
 */
bool joinstr(char* out, const char* lstr, const char* cstr, const char* rstr) {
  if (!out || !lstr || !cstr || !rstr) { return false; }
  strcpy(out, lstr);
  strcat(out, cstr);
  strcat(out, rstr);
  if (!out) { return false; }
  return true;
}

/**
 * @brief 文字列を新たなメモリに格納してポインタを返す。（自作strdup）
 * @param str 文字列。
 * @return 文字列を格納したポインタ。
 */
char* my_strdup(const char* str) {
  size_t len = strlen(str) + 1;
  char* copy = malloc(len);
  if (copy) { memcpy(copy, str, len); }
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
