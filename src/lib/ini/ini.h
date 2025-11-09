/**
 * Iniファイルパーサー用公開ヘッダ。
 */

#pragma once

// キー・バリューデータ
typedef struct IniKV {
  char* key;
  char* value;
  struct IniKV* next;
} IniKV;

// セクションデータ
typedef struct IniSection {
  char* name;
  IniKV* kv;
  struct IniSection* next;
} IniSection;

// Iniデータ
typedef struct Ini {
  IniSection* sections;
} Ini;

Ini* ini_load(const char* fpath);
void ini_close(Ini* ini);
const char* ini_get(
    Ini* ini, const char* section, const char* key, const char* default_value
);