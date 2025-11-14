
/**
 * ファイルローテーション用公開ヘッダ。
 */

#pragma once

#include <stdbool.h>

bool rotator_init(
    const char* dpath, const char* fname, const char* extension,
    size_t max_fsize, size_t max_fno
);
void rotator_close(void);
bool rotator_rotate(size_t len);
bool rotator_fputs(const char* line);