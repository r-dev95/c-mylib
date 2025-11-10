/**
 * Iniファイルパーサー関数群。
 */

#include "ini_parser.h"

#include "utils.h"

/**
 * @brief キー・バリューデータのメモリを確保する。
 * @return キー・バリューデータのポインタ。
 */
static IniKV* ini_kv_init(void) {
  IniKV* self = calloc(1, sizeof(*self));
  if (!self) return NULL;

  return self;
}

/**
 * @brief キー・バリューデータのメモリを解放する。
 * @param self キー・バリューデータのポインタ。
 */
static void ini_kv_destroy(IniKV* self) {
  if (!self) return;

  free(self->key);
  free(self->value);
  free(self);
  self = NULL;
}

/**
 * @brief セクションデータのメモリを確保する。
 * @return セクションデータのポインタ。
 */
static IniSection* ini_section_init(void) {
  IniSection* self = calloc(1, sizeof(*self));
  if (!self) return NULL;

  return self;
}

/**
 * @brief セクションデータおよびキー・バリューデータのメモリを解放する。
 * @param self セクションデータのポインタ。
 */
static void ini_section_destroy(IniSection* self) {
  if (!self) return;

  for (IniKV* kv = self->kv; kv;) {
    IniKV* del_kv = kv;
    kv = kv->next;
    ini_kv_destroy(del_kv);
  }

  free(self->name);
  free(self);
  self = NULL;
}

/**
 * @brief Iniデータのメモリを確保する。
 * @return Iniデータのポインタ。
 */
static Ini* ini_init(void) {
  Ini* self = calloc(1, sizeof(*self));
  if (!self) return NULL;

  return self;
}

/**
 * @brief Iniデータおよびセクションデータのメモリを解放する。
 * @param self Iniデータのポインタ。
 */
static void ini_destroy(Ini* self) {
  if (!self) return;

  for (IniSection* sec = self->sections; sec;) {
    IniSection* del_sec = sec;
    sec = sec->next;
    ini_section_destroy(del_sec);
  }

  free(self);
  self = NULL;
}

/**
 * @brief 行内のコメントを削除する。
 *
 * - クォーテーション内は削除対象としない。
 * @param str 文字列。
 */
static void remove_inline_comment(char* str) {
  if (!str) return;

  bool in_quote = false;
  char quote_char = 0;
  for (size_t i = 0; str[i]; ++i) {
    char c = str[i];
    if ((c == '"' || c == '\'') && (i == 0 || str[i - 1] != '\\')) {
      if (!in_quote) {
        in_quote = true;
        quote_char = c;
      } else if (c == quote_char) {
        in_quote = false;
        quote_char = 0;
      }
    }
    if (!in_quote && (c == ';' || c == '#')) {
      str[i] = '\0';
      break;
    }
  }
}

/**
 * @brief 同名キーを探索して返す。
 * @param sec セクションデータ。
 * @param key キー名。
 * @return キー・バリューデータ。
 */
static IniKV* find_key(IniSection* sec, const char* key) {
  if (!sec || !key) return NULL;

  IniKV* kv = sec->kv;
  while (kv) {
    if (kv->key && strcmp(kv->key, key) == 0) {
      return kv;
    }
    kv = kv->next;
  }
  return NULL;
}

/**
 * @brief 同名セクションを探索して返す。
 * @param ini Iniデータ。
 * @param name 探索するセクション名。
 * @return 探索されたセクションデータ。
 */
static IniSection* find_section(Ini* ini, const char* name) {
  // グローバルセクション名はNULLのため早期リターンの条件にしない
  if (!ini) return NULL;

  IniSection* sec = ini->sections;
  while (sec) {
    if ((sec->name == NULL && name == NULL) ||
        (sec->name && name && strcmp(sec->name, name) == 0)) {
      return sec;
    }
    sec = sec->next;
  }
  return NULL;
}

/**
 * @brief セクションにキー・バリューデータを作成して追加する。
 *
 * - 既にキーが存在する場合、バリューを上書きする。
 * @param sec セクションデータ。
 * @param key キー名。
 * @param value バリュー名。
 * @return 成功: true, 失敗: false。
 */
static bool make_kv(IniSection* sec, const char* key, const char* value) {
  if (!sec || !key) return false;

  // 既存キーがある場合、バリューを上書き
  IniKV* kv = find_key(sec, key);
  if (kv) {
    char* new_value = my_strdup(value);
    if (!new_value) return false;

    if (kv->value) free(kv->value);
    kv->value = new_value;
    return true;
  }

  // 新規キー・バリューを末尾に追加
  IniKV* new_kv = ini_kv_init();
  if (!new_kv) return false;

  new_kv->key = my_strdup(key);
  new_kv->value = my_strdup(value);
  if (!new_kv->key || !new_kv->value) {
    if (new_kv->key) free(new_kv->key);
    if (new_kv->value) free(new_kv->value);
    free(new_kv);
    return false;
  }

  if (!sec->kv) {
    // リストが空なら先頭に追加
    sec->kv = new_kv;
  } else {
    // 末尾に追加
    IniKV* tail = sec->kv;
    while (tail->next) tail = tail->next;
    tail->next = new_kv;
  }

  return true;
}

/**
 * @brief セクションが存在しなければ作成して返す。
 * @param ini Iniデータ。
 * @param name セクション名。
 * @return セクションデータ。
 */
static IniSection* make_section(Ini* ini, const char* name) {
  IniSection* sec = find_section(ini, name);
  if (sec) return sec;

  IniSection* new_sec = ini_section_init();
  if (!new_sec) return NULL;

  new_sec->name = name ? my_strdup(name) : NULL;

  if (!ini->sections) {
    // リストが空なら先頭に追加
    ini->sections = new_sec;
  } else {
    // 末尾に追加
    IniSection* tail = ini->sections;
    while (tail->next) tail = tail->next;
    tail->next = new_sec;
  }
  return new_sec;
}

static bool ini_parse(FILE* fp, Ini* ini, IniSection* cur_sec) {
  size_t cap = 0;
  size_t len;
  char* line = NULL;
  while ((len = my_getline(&line, &cap, fp)) != 0) {
    remove_newline(line, len);
    remove_inline_comment(line);
    remove_spaces(line);
    if (*line == '\0') continue;                 // 空行
    if (*line == ';' || *line == '#') continue;  // 行頭コメント

    // セクションを探索して追加
    if (line[0] == '[') {
      char* end = strchr(line, ']');
      if (end) {
        size_t nlen = (size_t)(end - (line + 1));
        char* secname = malloc(nlen + 1);
        if (!secname) return false;

        memcpy(secname, line + 1, nlen);
        secname[nlen] = '\0';
        remove_spaces(secname);
        IniSection* new_sec = make_section(ini, secname);
        if (!new_sec) {
          fprintf(stderr, "セクションが作成できません。\n");
          return false;
        }

        cur_sec = new_sec;
        free(secname);
        continue;
      } else {
        fprintf(stderr, "セクションの閉じ括弧\"]\"がありません。\n");
        return false;
      }
    }

    // キー・バリューを探索して追加
    char* eq = strchr(line, '=');
    if (!eq) continue;

    *eq = '\0';
    char* key = line;
    char* value = eq + 1;
    remove_spaces(key);
    remove_spaces(value);
    remove_quotes(value);
    if (!make_kv(cur_sec, key, value)) {
      fprintf(stderr, "キー・バリューが設定できません。\n");
      return false;
    }
  }

  free(line);
  return true;
}

// ----------------------------------------------------------------------------
// 以降、公開関数
// ----------------------------------------------------------------------------

/**
 * @brief iniファイルを読み込み、データを取得する。
 * @param fpath iniファイルパス。
 * @return Iniデータ。
 */
Ini* ini_load(const char* fpath) {
  FILE* fp = fopen(fpath, "r");
  if (!fp) {
    fprintf(stderr, "iniファイルを開けません。[%s]\n", fpath);
    return NULL;
  }

  Ini* ini = ini_init();
  if (!ini) {
    fprintf(stderr, "Iniデータのメモリを確保できません。\n");
    fclose(fp);
    return NULL;
  }

  // グローバルセクションを作成
  IniSection* cur_sec = make_section(ini, NULL);
  if (!cur_sec) {
    fprintf(stderr, "グローバルセクションが作成できません。\n");
    ini_destroy(ini);
    fclose(fp);
    return NULL;
  }

  // iniファイルを解析してデータを取得
  if (!ini_parse(fp, ini, cur_sec)) {
    fprintf(stderr, "iniファイルを解析できません。\n");
    ini_destroy(ini);
    fclose(fp);
    return NULL;
  }

  fclose(fp);
  return ini;
}

/**
 * @brief iniファイル操作を終了する。
 * @param ini Iniデータのポインタ。
 */
void ini_close(Ini* ini) {
  if (!ini) return;

  ini_destroy(ini);
}

/**
 * @brief Iniデータからセクションとキーに対応するバリューを取得する。
 *
 * - 対応するセクションとキーが存在しない場合、デフォルトのバリューを返す。
 * @param ini Iniデータのポインタ。
 * @param section セクション名。
 * @param key キー名。
 * @param default_value デフォルトバリュー。
 * @return セクションとキーに対応するバリューまたはデフォルトバリュー。
 */
const char* ini_get(
    Ini* ini, const char* section, const char* key, const char* default_value
) {
  if (!ini || !key) return default_value;

  IniSection* sec = find_section(ini, section);
  if (!sec) return default_value;

  IniKV* kv = find_key(sec, key);
  if (!kv) return default_value;

  return kv->value;
}