/**
 * ファイルローテーション処理関数群。
 */

#include "rotator_file.h"

#include "error/error.h"
#include "utils.h"

/**
 * @brief ファイルを開く。
 * @param fpath ファイルパス。
 * @return ファイルストリーム。
 */
static FILE* fp_init(const char* fpath) {
  if (!fpath) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return NULL;
  }

  FILE* self = fopen(fpath, "a");
  if (!self) {
    SET_ERR_LOG_AUTO(ERR_FILE_OPEN_FAILED);
    return NULL;
  }

  return self;
}

/**
 * @brief ファイルをフラッシュして閉じる。
 * @param self ファイルストリーム。
 */
static void fp_destroy(FILE** self) {
  if (!self || !*self) { return; }

  fflush(*self);
  fclose(*self);
  *self = NULL;
}

/**
 * @brief ファイル情報データのメモリを確保する。
 * @return ファイル情報データ。
 */
static file_info_t* finfo_init(void) {
  file_info_t* self = calloc(1, sizeof(*self));
  if (!self) {
    SET_ERR_LOG_AUTO(ERR_MEM_ALLOC_FAILED);
    return NULL;
  }

  return self;
}

/**
 * @brief ファイル情報データのメモリを解放する。
 * @param self ファイル情報データ。
 */
static void finfo_destroy(file_info_t** self) {
  if (!self || !*self) { return; }

  if ((*self)->fpath) { free((*self)->fpath); }
  free(*self);
  *self = NULL;
}

/**
 * @brief ファイル情報データを更新する。
 * @param self 更新先のファイル情報データ。
 * @param info 更新元のファイル情報データ。
 * @return 成功: true, 失敗: false。
 */
static bool finfo_update(file_info_t** self, file_info_t* info) {
  if (!self || !*self || !info) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return false;
  }

  finfo_destroy(&*self);
  *self = finfo_init();
  if (!*self) { return false; }

  (*self)->fpath = my_strdup(info->fpath);
  if (!(*self)->fpath) { return false; }

  (*self)->fsize = info->fsize;
  (*self)->mtime = info->mtime;

  return true;
}

/**
 * @brief ファイル情報リストのメモリを確保する。
 * @param max_fno ファイル情報データの数。
 * @return ファイル情報リスト。
 */
static file_list_t* flist_init(size_t max_fno) {
  if (max_fno < 1) {
    SET_ERR_LOG(
        ERR_INVALID_ARG,
        "The number of file infomation data must be set to 1 or more. [%zu]",
        max_fno
    );
    return NULL;
  }

  file_list_t* self = calloc(1, sizeof(*self));
  if (!self) {
    SET_ERR_LOG_AUTO(ERR_MEM_ALLOC_FAILED);
    return NULL;
  }

  self->finfos = calloc(max_fno, sizeof(*self->finfos));
  if (!self->finfos) {
    SET_ERR_LOG_AUTO(ERR_MEM_ALLOC_FAILED);
    free(self);
    return NULL;
  }

  self->max_fno = max_fno;

  return self;
}

/**
 * @brief ファイル情報リストのメモリを解放する。
 * @param self ファイル情報リスト。
 */
static void flist_destroy(file_list_t** self) {
  if (!self || !*self) { return; }

  if ((*self)->finfos) {
    for (size_t i = 0; i < (*self)->cur_fno; i++) {
      finfo_destroy(&(*self)->finfos[i]);
    }
    free((*self)->finfos);
  }
  free(*self);
  *self = NULL;
}

/**
 * @brief ファイル情報リストの末尾に情報データを追加する。
 * @param self ファイル情報リスト。
 * @param info ファイル情報データ。
 * @return 成功: 0, 引数がNULL: 1, リストの格納最大数を超える: -1。
 */
static int flist_add_finfo(file_list_t* self, file_info_t* info) {
  if (!self || !info) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return 1;
  }

  if (self->cur_fno + 1 > self->max_fno) {
    SET_ERR_LOG_AUTO(ERR_MEM_OUT_OF_RANGE);
    return -1;
  }

  self->finfos[self->cur_fno] = finfo_init();
  if (!self->finfos[self->cur_fno]) { return false; }

  self->finfos[self->cur_fno]->fpath = my_strdup(info->fpath);
  if (!self->finfos[self->cur_fno]->fpath) { return false; }

  self->finfos[self->cur_fno]->fsize = info->fsize;
  self->finfos[self->cur_fno]->mtime = info->mtime;
  self->cur_fno++;

  return 0;
}

/**
 * @brief ファイル情報リストの末尾の情報データを削除する。
 * @param self ファイル情報リスト。
 * @return 成功: true, 失敗: false。
 */
static bool flist_del_last_finfo(file_list_t* self) {
  if (!self) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return false;
  }

  finfo_destroy(&self->finfos[self->cur_fno - 1]);
  self->cur_fno--;

  return true;
}

/**
 * @brief ファイル情報データ配列のメモリを再確保する。
 * @param self ファイル情報リスト。
 * @return 成功: true, 失敗: false。
 */
static bool flist_realloc_finfo(file_list_t** self) {
  if (!self || !*self) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return false;
  }

  (*self)->max_fno = (*self)->max_fno + INI_FILE_NUM;
  size_t cap = (*self)->max_fno * sizeof(file_info_t*);
  file_info_t** new_infos = realloc((*self)->finfos, cap);
  if (!new_infos) {
    SET_ERR_LOG_AUTO(ERR_MEM_ALLOC_FAILED);
    return false;
  }

  (*self)->finfos = new_infos;

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
 * @param flist ファイル情報リスト。
 * @return 成功: true, 失敗: false。
 */
static bool sort_file_list_desc(file_list_t* flist) {
  if (!flist || flist->cur_fno == 0) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return false;
  }

  qsort(
      flist->finfos, flist->cur_fno, sizeof(file_info_t*), compare_mtime_desc
  );

  return true;
}

/**
 * @brief ファイルパスの末尾に現在の日時を付与する。
 * @param new_fpath 新規ファイルパス。
 * @param fpath 元のファイルパス。
 * @return 成功: true, 失敗: false。
 */
static bool make_fpath(char* new_fpath, const char* fpath) {
  if (!new_fpath || !fpath) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return false;
  }

  struct tm tm = get_current_time();
  snprintf(
      new_fpath, FPATH_SIZE * sizeof(new_fpath), "%s.%04d%02d%02d-%02d%02d%02d",
      fpath, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
      tm.tm_min, tm.tm_sec
  );

  return true;
}

/**
 * @brief ファイルの情報を取得する。
 * @param fpath ファイルパス。
 * @return ファイル情報データ。
 */
static file_info_t* get_file_info(const char* fpath) {
  if (!fpath) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return NULL;
  }

  struct stat st;
  file_info_t* info = finfo_init();
  if (!info) { return NULL; }

  if (stat(fpath, &st) != 0) {
    SET_ERR_LOG(
        ERR_FILE_INVALID_PATH, "%s: file does not exist. [%s]",
        code_to_msg(ERR_FILE_INVALID_PATH), fpath
    );
    return NULL;
  }
  if (S_ISDIR(st.st_mode)) {
    SET_ERR_LOG(
        ERR_IO_ERROR, "%s: It's a directory, not a file. [%s]",
        code_to_msg(ERR_FILE_INVALID_PATH), fpath
    );
    return NULL;
  }

  info->fpath = my_strdup(fpath);
  if (!info->fpath) { return NULL; }

  info->fsize = (size_t)st.st_size;
  info->mtime = st.st_mtime;

  return info;
}

/**
 * @brief ディレクトリにあるすべてのファイルの情報を取得する。
 *
 * - すべてのファイルといっても厳密には、ファイル情報リストに入る数のみ。
 * @param flist ファイル情報リスト。
 * @param dpath ディレクトリパス。
 * @param search 探索するファイルの共通文字列。
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
    SET_ERR_LOG(
        ERR_IO_ERROR, "%s: Unable to open directory. [%s]",
        code_to_msg(ERR_IO_ERROR), dpath
    );
    return false;
  }

  for (struct dirent* dp = readdir(dir); dp != NULL; dp = readdir(dir)) {
    if (!joinstr(fpath, dpath, "/", dp->d_name)) { return false; }

    if (strstr(fpath, search)) {
      info = get_file_info(fpath);
      if (!info) { return false; }

      if ((res = flist_add_finfo(flist, info)) != 0) {
        if (res == -1) {
          // ファイル情報データ配列のメモリを再確保して再度データを追加
          if (!flist_realloc_finfo(&flist)) {
            finfo_destroy(&info);
            return false;
          }
          if ((res = flist_add_finfo(flist, info)) != 0) {
            finfo_destroy(&info);
            return false;
          }
        } else {
          finfo_destroy(&info);
          return false;
        }
      }
      finfo_destroy(&info);
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
 * @param dpath ディレクトリパス。
 * @param fname ファイル名。（拡張子含まないこと）
 * @param extension 拡張子。（ドットを含むこと）
 * @return 成功: true, 失敗: false。
 */
static bool rotator_set_base_fpath(
    const char* dpath, const char* fname, const char* extension
) {
  if (!dpath || !fname || !extension) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return false;
  }

  char tmp_fpath[FPATH_SIZE] = {0};
  if (!joinstr(tmp_fpath, dpath, "/", fname)) { return false; }
  if (!joinstr(g_param.base_fpath, tmp_fpath, "", extension)) { return false; }

  return true;
}

/**
 * @brief アーカイブ数 + 1の最新ファイル情報をリストに設定する。
 * @param dpath ディレクトリパス。
 * @param extension 拡張子。
 * @return 成功: true, 失敗: false。
 */
static bool rotator_set_file_info(const char* dpath, const char* extension) {
  // 全ファイル情報の取得
  file_list_t* all_flist = flist_init(INI_FILE_NUM);
  if (!all_flist) { return false; }

  if (!get_all_file_info(all_flist, dpath, extension)) { return false; }

  // 最大でアーカイブ数 + 1（書き込み対象）の最新ファイル情報を取得
  file_list_t* flist = flist_init(g_param.max_fno);
  if (!flist) { return false; }

  size_t loop_num = MIN(all_flist->cur_fno, flist->max_fno);
  for (size_t i = 0; i < loop_num; i++) {
    if (flist_add_finfo(flist, all_flist->finfos[i]) != 0) {
      flist_destroy(&all_flist);
      return false;
    }
  }
  flist_destroy(&all_flist);

  // 書き込み（最新）ファイルのオープン
  if (flist->cur_fno == 0) {
    g_param.fp = fp_init(g_param.base_fpath);
    if (!g_param.fp) { return false; }

    file_info_t* info = get_file_info(g_param.base_fpath);
    if (!info) { return false; }

    if (flist_add_finfo(flist, info) != 0) {
      finfo_destroy(&info);
      return false;
    }

    finfo_destroy(&info);
  } else {
    g_param.fp = fp_init(flist->finfos[0]->fpath);
    if (!g_param.fp) { return false; }
  }

  g_param.flist = flist;

  return true;
}

// ----------------------------------------------------------------------------
// 以降、公開関数
// ----------------------------------------------------------------------------

/**
 * @brief ローテーション処理を初期化する。
 * @param dpath ディレクトリパス。
 * @param fname ファイル名。（拡張子を含まないこと）
 * @param extension 拡張子。（ドットを含むこと）
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
  if (!rotator_set_base_fpath(dpath, fname, extension)) { return false; }
  // ファイル情報リストの設定
  if (!rotator_set_file_info(dpath, extension)) { return false; }

  return true;
}

/**
 * @brief ローテーション処理を終了する。
 */
void rotator_close(void) {
  fp_destroy(&g_param.fp);
  flist_destroy(&g_param.flist);
}

/**
 * @brief ローテーション処理を実行する。
 * @param len ファイルへの書き込みバイトサイズ。
 * @return 成功: true, 失敗: false。
 */
bool rotator_rotate(size_t len) {
  char new_fpath[FPATH_SIZE];

  file_list_t* flist = g_param.flist;

  // 書き込みサイズの確認
  if (g_param.max_fsize != 0 &&
      flist->finfos[0]->fsize + len > g_param.max_fsize) {
    // 最新ファイルを閉じてリネーム
    fp_destroy(&g_param.fp);
    if (!make_fpath(new_fpath, flist->finfos[0]->fpath)) { return false; }
    rename(flist->finfos[0]->fpath, new_fpath);

    // ファイル情報データを更新
    file_info_t* info = get_file_info(new_fpath);
    if (!info) { return false; }

    if (!finfo_update(&flist->finfos[0], info)) { return false; }

    finfo_destroy(&info);
    sort_file_list_desc(flist);

    // アーカイブファイル数の確認
    if (flist->cur_fno + 1 > flist->max_fno) {
      if (flist->max_fno != 1) {
        remove(flist->finfos[flist->cur_fno - 1]->fpath);
      }
      if (!flist_del_last_finfo(flist)) { return false; }
    }

    // 次の書き込みファイルをオープンしてファイル情報リストを更新
    g_param.fp = fp_init(g_param.base_fpath);
    if (!g_param.fp) { return false; }

    info = get_file_info(g_param.base_fpath);
    if (!info) { return false; }

    info->mtime += 1;  // ソート対応
    if (flist_add_finfo(flist, info) != 0) {
      finfo_destroy(&info);
      return false;
    }
    finfo_destroy(&info);
    sort_file_list_desc(flist);
  }
  flist->finfos[0]->fsize += len;

  return true;
}

/**
 * @brief 最新ファイルに書き込む。
 * @param line 書き込み文字列。
 * @return 成功: true, 失敗: false。
 */
bool rotator_fputs(const char* line) {
  if (!line) {
    SET_ERR_LOG_AUTO(ERR_INVALID_ARG);
    return false;
  }

  fputs(line, g_param.fp);
  fflush(g_param.fp);

  return true;
}