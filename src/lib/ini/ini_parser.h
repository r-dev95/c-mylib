/**
 * Iniファイルパーサー用ヘッダ。
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ini.h"

static IniKV* ini_kv_init(void);
static void ini_kv_destroy(IniKV* self);
static IniSection* ini_section_init(void);
static void ini_section_destroy(IniSection* self);
static Ini* ini_init(void);
static void ini_destroy(Ini* self);
static void remove_inline_comment(char* str);
static IniKV* find_key(IniSection* sec, const char* key);
static IniSection* find_section(Ini* ini, const char* name);
static bool make_kv(IniSection* sec, const char* key, const char* value);
static IniSection* make_section(Ini* ini, const char* name);
static bool ini_parse(FILE* fp, Ini* ini, IniSection* cur_sec);