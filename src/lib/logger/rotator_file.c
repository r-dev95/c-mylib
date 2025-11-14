/**
 * ファイルローテーション処理関数群。
 */

#include "rotator_file.h"

#include "utils.h"

/**
 * @brief ログ出力用ファイルを開く。
 * @param fpath ファイルパス。
 * @return 成功: true, 失敗: false。
 */
static bool fp_init(const char* fpath) {
  if (!fpath) { return false; }

  g_param.fp = fopen(fpath, "a");
  if (!g_param.fp) { return false; }
  return true;
}

/**
 * @brief ログ出力用ファイルをフラッシュし、閉じる。
 */
static void fp_destroy(void) {
  if (!g_param.fp) { return; }

  fflush(g_param.fp);
  fclose(g_param.fp);
  g_param.fp = NULL;
}

/**
 * @brief ファイル情報データのメモリを確保する。
 * @return ファイル情報データのポインタ。
 */
static file_info_t* finfo_init(void) {
  file_info_t* self = calloc(1, sizeof(*self));
  if (!self) { return NULL; }
  return self;
}

/**
 * @brief ファイル情報データのメモリを解放する。
 * @param self ファイル情報データのポインタ。
 */
static void finfo_destroy(file_info_t* self) {
  if (!self) { return; }
  free(self->fpath);
  free(self);
}

/**
 * @brief ファイル情報データを更新する。
 * @param self ファイル情報データのポインタ。
 * @param info 更新用のファイル情報データのポインタ。
 * @return 成功: true, 失敗: false。
 */
static bool finfo_update(file_info_t** self, file_info_t* info) {
  if (!self || !info) { return false; }
  file_info_t* pself = *self;
  finfo_destroy(pself);
  pself = finfo_init();
  pself->fpath = my_strdup(info->fpath);
  pself->fsize = info->fsize;
  pself->mtime = info->mtime;
  *self = pself;
  return true;
}

/**
 * @brief ファイル情報リストのメモリを確保する。
 * @param max_fno ファイル情報データの数。
 * @return ファイル情報リストのポインタ。
 */
static file_list_t* flist_init(size_t max_fno) {
  file_list_t* self = calloc(1, sizeof(*self));
  if (!self) { return NULL; }
  self->finfos = calloc(max_fno, sizeof(*self->finfos));
  if (!self->finfos) {
    free(self);
    return NULL;
  }
  self->max_fno = max_fno;
  return self;
}

/**
 * @brief ファイル情報リストのメモリを解放する。
 * @param self ファイル情報リストのポインタ。
 */
static void flist_destroy(file_list_t* self) {
  if (!self) { return; }
  if (self->finfos) {
    for (size_t i = 0; i < self->cur_fno; ++i) {
      finfo_destroy(self->finfos[i]);
    }
    free(self->finfos);
  }
  free(self);
}

/**
 * @brief ファイル情報リストの末尾に情報データを追加する。
 * @param self ファイル情報リストのポインタ。
 * @param info ファイル情報データのポインタ。
 * @return 成功: 0, 引数がNULL: 1, リストの格納最大数を超える: -1。
 */
static int flist_add_finfo(file_list_t* self, file_info_t* info) {
  if (!self || !info) { return 1; }
  if (self->cur_fno + 1 > self->max_fno) { return -1; }
  self->finfos[self->cur_fno] = finfo_init();
  self->finfos[self->cur_fno]->fpath = my_strdup(info->fpath);
  self->finfos[self->cur_fno]->fsize = info->fsize;
  self->finfos[self->cur_fno]->mtime = info->mtime;
  self->cur_fno++;
  return 0;
}

/**
 * @brief ファイル情報リストの末尾の情報データを削除する。
 * @param self ファイル情報リストのポインタ。
 * @return 成功: true, 失敗: false。
 */
static bool flist_del_last_finfo(file_list_t* self) {
  if (!self) { return false; }
  finfo_destroy(self->finfos[self->cur_fno - 1]);
  self->cur_fno--;
  return true;
}

/**
 * @brief ファイル情報データ配列のメモリを再確保する。
 * @param pself ファイル情報リストのポインタのポインタ。
 * @return 成功: true, 失敗: false。
 */
static bool flist_realloc_finfo(file_list_t** pself) {
  if (!pself || !*pself) { return false; }
  file_list_t* self = *pself;
  self->max_fno = self->max_fno * 2;
  size_t cap = self->max_fno * sizeof(file_info_t*);
  file_info_t** new_infos = (file_info_t**)realloc(self->finfos, cap);
  if (!new_infos) { return false; }
  self->finfos = new_infos;
  *pself = self;
  return true;
}

/**
 * @brief 降順ソート用に更新時間を比較する。
 * @param a 前の要素。
 * @param b 次の要素。
 * @return -1 or 0 or 1。
 */
static int compare_mtime_desc(const void* a, const void* b) {
  const file_info_t* fa = *(const file_info_t**)a;
  const file_info_t* fb = *(const file_info_t**)b;
  if (!fa && !fb) { return 0; }
  if (!fa) { return 1; }
  if (!fb) { return -1; }
  if (fa->mtime > fb->mtime) { return -1; }
  if (fa->mtime < fb->mtime) { return 1; }
  return 0;
}

/**
 * @brief ファイルの更新時間によって降順ソートする。
 * @param flist ファイル情報リストのポインタ。
 */
static void sort_file_list_desc(file_list_t* flist) {
  if (!flist || flist->cur_fno == 0) { return; }
  qsort(
      flist->finfos, flist->cur_fno, sizeof(file_info_t*), compare_mtime_desc
  );
}

/**
 * @brief ファイルパスの末尾に現在の日時を付与する。
 * @param new_fpath 日時を付与されたファイルパスのポインタ。
 * @param fpath 元のファイルパスのポインタ。
 */
static void make_fpath(char* new_fpath, const char* fpath) {
  struct tm tm = get_current_time();
  snprintf(
      new_fpath, FPATH_SIZE * sizeof(new_fpath), "%s.%04d%02d%02d-%02d%02d%02d",
      fpath, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
      tm.tm_min, tm.tm_sec
  );
}

/**
 * @brief ファイルの情報を取得する。
 * @param fpath ファイルパスのポインタ。
 * @return ファイル情報データのポインタ。
 */
static file_info_t* get_file_info(const char* fpath) {
  if (!fpath) { return NULL; }

  struct stat st;
  file_info_t* info = finfo_init();
  if (!info) {
    fprintf(stderr, "❌ファイル情報データのメモリ確保に失敗。\n");
    return NULL;
  }

  if (stat(fpath, &st) != 0) {
    fprintf(stderr, "❌ファイルがありません。[%s]\n", fpath);
    return NULL;
  }
  if (S_ISDIR(st.st_mode)) {
    fprintf(stderr, "❌ファイルではなくディレクトリです。[%s]\n", fpath);
    return NULL;
  }

  info->fpath = my_strdup(fpath);
  info->fsize = (size_t)st.st_size;
  info->mtime = st.st_mtime;
  return info;
}

/**
 * @brief ディレクトリにあるすべてのファイルの情報を取得する。
 *
 * - すべてのファイルといっても厳密には、ファイル情報リストに入る数のみ。
 * @param flist ファイル情報リストのポインタ。
 * @param dpath ディレクトリパス。
 * @param search 探索するファイルの共通文字列のポインタ。
 * @return 成功: true, 失敗: false。
 */
static bool get_all_file_info(
    file_list_t* flist, const char* dpath, const char* search
) {
  int res;
  char fpath[FPATH_SIZE];
  file_info_t* info;

  DIR* dir = opendir(dpath);
  if (!dir) {
    fprintf(stderr, "❌ディレクトリオープンに失敗。[%s]\n", dpath);
    return false;
  }

  for (struct dirent* dp = readdir(dir); dp != NULL; dp = readdir(dir)) {
    if (!joinstr(fpath, dpath, "/", dp->d_name)) {
      fprintf(stderr, "❌パスの結合に失敗。[%s][%s]\n", dpath, dp->d_name);
      return false;
    }

    if (strstr(fpath, search)) {
      info = get_file_info(fpath);
      if (!info) {
        fprintf(stderr, "❌ファイル情報の取得に失敗。[%s]\n", fpath);
        return false;
      }

      if ((res = flist_add_finfo(flist, info)) != 0) {
        if (res == -1) {
          // ファイル情報データ配列のメモリを再確保して再度データを追加
          if (!flist_realloc_finfo(&flist)) {
            fprintf(
                stderr, "❌ファイル情報データ配列のメモリの再確保に失敗。\n"
            );
            finfo_destroy(info);
            return false;
          }
          if ((res = flist_add_finfo(flist, info)) != 0) {
            fprintf(stderr, "❌リストにファイル情報データの追加失敗。\n");
            finfo_destroy(info);
            return false;
          }
        } else {
          fprintf(stderr, "❌リストにファイル情報データの追加失敗。\n");
          finfo_destroy(info);
          return false;
        }
      }
      finfo_destroy(info);
    }
  }
  closedir(dir);
  sort_file_list_desc(flist);
  return true;
}

/**
 * @brief 最大ファイルサイズを設定する。
 * @param size ファイルの最大サイズ。
 */
static void rotator_set_max_fsize(size_t size) { g_param.max_fsize = size; }

/**
 * @brief 最大ファイルアーカイブ数を設定する。
 *
 * - ファイルのアーカイブ数 + 1（書き込み対象）を設定する。
 * @param no ファイルの最大アーカイブ数。
 */
static void rotator_set_max_fno(size_t no) { g_param.max_fno = no + 1; }

/**
 * @brief ベースのファイルパスを設定する。
 * @param dpath ディレクトリパスのポインタ。
 * @param fname ファイル名のポインタ。（拡張子含まないこと）
 * @param extension 拡張子のポインタ。（ドットを含むこと）
 * @return 成功: true, 失敗: false。
 */
static bool rotator_set_base_fpath(
    const char* dpath, const char* fname, const char* extension
) {
  if (!dpath || !fname || !extension) { return false; }

  bool res = true;
  char tmp_fpath[FPATH_SIZE] = {0};
  res &= joinstr(tmp_fpath, dpath, "/", fname);
  res &= joinstr(g_param.base_fpath, tmp_fpath, "", extension);
  return res;
}

/**
 * @brief アーカイブ数 + 1の最新ファイル情報をリストに設定する。
 * @param dpath ディレクトリパスのポインタ。
 * @param extension 拡張子のポインタ。
 * @return 成功: true, 失敗: false。
 */
static bool rotator_set_file_info(const char* dpath, const char* extension) {
  size_t loop_num;
  file_list_t* all_flist;
  file_list_t* flist;
  file_info_t* info;

  // 全ファイル情報の取得
  all_flist = flist_init(INI_FILE_NUM);
  if (!all_flist) {
    fprintf(stderr, "❌ファイル情報リストのメモリ確保に失敗。\n");
    return false;
  }
  if (!get_all_file_info(all_flist, dpath, extension)) {
    fprintf(stderr, "❌ファイル情報の取得に失敗。\n");
    return false;
  }

  // 最大でアーカイブ数 + 1（書き込み対象）のファイル情報を取得
  flist = flist_init(g_param.max_fno);
  if (!flist) {
    fprintf(stderr, "❌ファイル情報リストのメモリ確保に失敗。\n");
    return false;
  }
  loop_num = MIN(all_flist->cur_fno, flist->max_fno);
  for (size_t i = 0; i < loop_num; i++) {
    flist_add_finfo(flist, all_flist->finfos[i]);
  }
  flist_destroy(all_flist);

  if (flist->cur_fno == 0) {
    fp_init(g_param.base_fpath);
    info = get_file_info(g_param.base_fpath);
    if (!info) {
      fprintf(stderr, "❌ファイル情報の取得に失敗。[%s]\n", g_param.base_fpath);
    }
    flist_add_finfo(flist, info);
    finfo_destroy(info);
  } else {
    fp_init(flist->finfos[0]->fpath);
  }

  g_param.flist = flist;
  return true;
}

// ----------------------------------------------------------------------------
// 以降、公開関数
// ----------------------------------------------------------------------------

/**
 * @brief ローテーション処理を初期化する。
 * @param dpath ディレクトリパスのポインタ。
 * @param fname ファイル名のポインタ。（拡張子を含まないこと）
 * @param extension 拡張子のポインタ。（ドットを含むこと）
 * @param max_fsize 最大ファイルバイトサイズ。
 * @param max_fno 最大ファイルアーカイブ数。
 * @return 成功: true, 失敗: false。
 */
bool rotator_init(
    const char* dpath, const char* fname, const char* extension,
    size_t max_fsize, size_t max_fno
) {
  // 最大ファイルバイトサイズの設定
  rotator_set_max_fsize(max_fsize);
  // 最大ファイルアーカイブ数の設定
  rotator_set_max_fno(max_fno);
  // ベースファイルパスの設定
  if (!rotator_set_base_fpath(dpath, fname, extension)) {
    fprintf(stderr, "❌ベースファイルパスの設定に失敗。\n");
    return false;
  }
  // ファイル情報リストの設定
  if (!rotator_set_file_info(dpath, extension)) {
    fprintf(stderr, "❌ファイル情報リストの設定に失敗。\n");
    return false;
  }
  return true;
}

/**
 * @brief ローテーション処理を終了する。
 */
void rotator_close(void) {
  fp_destroy();
  flist_destroy(g_param.flist);
}

/**
 * @brief ローテーション処理を実行する。
 * @param len ファイルへの書き込みバイトサイズ。
 * @return 成功: true, 失敗: false。
 */
bool rotator_rotate(size_t len) {
  char new_fpath[FPATH_SIZE];
  file_info_t* info;

  file_list_t* flist = g_param.flist;

  if (g_param.max_fsize != 0 &&
      flist->finfos[0]->fsize + len > g_param.max_fsize) {
    fp_destroy();
    make_fpath(new_fpath, flist->finfos[0]->fpath);
    rename(flist->finfos[0]->fpath, new_fpath);
    info = get_file_info(new_fpath);
    finfo_update(&flist->finfos[0], info);
    finfo_destroy(info);
    sort_file_list_desc(flist);

    if (flist->cur_fno + 1 > flist->max_fno) {
      if (flist->max_fno != 1) {
        remove(flist->finfos[flist->cur_fno - 1]->fpath);
      }
      flist_del_last_finfo(flist);
    }

    fp_init(g_param.base_fpath);
    info = get_file_info(g_param.base_fpath);
    info->mtime += 1;  // ソート対応
    flist_add_finfo(flist, info);
    finfo_destroy(info);
    sort_file_list_desc(flist);
  }
  flist->finfos[0]->fsize += len;

  return true;
}

/**
 * @brief 最新ファイルに書き込む。
 * @param line 書き込み文字列のポインタ。
 * @return 成功: true, 失敗: false。
 */
bool rotator_fputs(const char* line) {
  if (!line) { return false; }
  fputs(line, g_param.fp);
  fflush(g_param.fp);
  return true;
}