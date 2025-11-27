/**
 * ユーティリティ関数群。
 */

#include "utils.h"

#include "error/error.h"

/**
 * @brief 現在時刻を取得する。
 * @return 現在時刻データ。
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
  if (backslash > pos) { pos = backslash; }  // より後方の区切り文字を選択

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
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
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
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return NULL;
  }

  size_t len = strlen(str) + 1;
  char* copy = malloc(len);
  if (!copy) {
    SET_ERR_LOG_AUTO(ERR_MEM_ALLOC_FAILED);
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
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return 0;
  }

  int c;
  size_t pos = 0;

  // 初期バッファの確保
  if (*line == NULL || *size == 0) {
    *size = 128;
    *line = malloc(*size);
    if (!*line) {
      SET_ERR_LOG_AUTO(ERR_MEM_ALLOC_FAILED);
      return 0;
    }
  }

  while ((c = fgetc(stream)) != EOF) {
    // バッファ拡張
    if (pos + 1 >= *size) {
      size_t new_size = *size * 2;
      char* new_ptr = realloc(*line, new_size);
      if (!new_ptr) {
        SET_ERR_LOG_AUTO(ERR_MEM_ALLOC_FAILED);
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
    SET_ERR_LOG_AUTO(ERR_IO_ERROR);
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
  if (!str) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return;
  }

  while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
    str[--len] = '\0';
  }
}

/**
 * @brief 文字列前後の空白を削除する。
 * @param str 文字列。
 */
void remove_spaces(char* str) {
  if (!str) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return;
  }

  // 左側
  char* ptr = str;
  while (*ptr && isspace((unsigned char)*ptr)) { ptr++; }
  if (ptr != str) { memmove(str, ptr, strlen(ptr) + 1); }

  // 右側
  size_t len = strlen(str);
  while (len > 0 && isspace((unsigned char)str[len - 1])) { str[--len] = '\0'; }
}

/**
 * @brief 文字列前後のクォーテーションを削除する。
 * @param str 文字列。
 */
void remove_quotes(char* str) {
  if (!str) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return;
  }

  size_t len = strlen(str);
  if (len >= 2 && ((str[0] == '"' && str[len - 1] == '"') ||
                   (str[0] == '\'' && str[len - 1] == '\''))) {
    if (len > 2) { memmove(str, str + 1, len - 2); }
    str[len - 2] = '\0';
  }
}
