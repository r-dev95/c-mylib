/**
 * ユーティリティ関数用ヘッダ。
 */

#pragma once

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tm get_current_time();
char* get_fname(const char* fpath);
char* my_strdup(const char* str);
size_t my_getline(char** pline, size_t* size, FILE* stream);
void remove_newline(char* str, size_t len);
void remove_spaces(char* str);
void remove_quotes(char* str);

#ifdef __cplusplus
}
#endif
