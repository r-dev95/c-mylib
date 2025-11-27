/**
 * ユーティリティ関数用ヘッダ。
 */

#pragma once

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// 最小値取得関数マクロ
#define MIN(a, b) (a < b ? a : b)

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

#ifdef __cplusplus
}
#endif
