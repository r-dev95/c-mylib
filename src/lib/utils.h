/**
 * ユーティリティ関数用ヘッダ。
 */

#pragma once

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

void set_error_msg(
    const char* fpath, const char* func, const int line, const char* fmt, ...
);
void out_error_msg(void);

struct tm get_current_time(void);

char* get_fname(const char* fpath);

bool joinstr(char* out, const char* lstr, const char* cstr, const char* rstr);

char* my_strdup(const char* str);
size_t my_getline(char** line, size_t* size, FILE* stream);

void remove_newline(char* str, size_t len);
void remove_spaces(char* str);
void remove_quotes(char* str);

#define SET_ERR_MSG(...) \
  set_error_msg(__FILE__, __func__, __LINE__, __VA_ARGS__)

#define CHECK(expr)                   \
  do {                                \
    if (!(expr)) { out_error_msg(); } \
  } while (0)

#define CHECK_RETURN(expr, retval) \
  do {                             \
    if (!(expr)) {                 \
      out_error_msg();             \
      return (retval);             \
    }                              \
  } while (0)

#define CHECK_RETURN_VOID(expr) \
  do {                          \
    if (!(expr)) {              \
      out_error_msg();          \
      return;                   \
    }                           \
  } while (0)

#ifdef __cplusplus
}
#endif
