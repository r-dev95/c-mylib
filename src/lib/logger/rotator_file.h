/**
 * ファイルローテーション用ヘッダ。
 */

#pragma once

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "rotator.h"

// 最小値取得関数マクロ
#define MIN(a, b) (a < b ? a : b)

// ファイルパスのバイトサイズ
#define FPATH_SIZE 256
// 初回読み込みファイル数
#define INI_FILE_NUM 20

// ファイル情報データ
typedef struct {
  char* fpath;   // パス
  size_t fsize;  // バイトサイズ
  time_t mtime;  // 更新時間
} file_info_t;

// ファイル情報リスト
typedef struct file_list_t {
  file_info_t** finfos;  // ファイル情報データの配列
  size_t cur_fno;        // 現在のファイル数
  size_t max_fno;        // 最大ファイル数
} file_list_t;

// パラメータ
typedef struct {
  FILE* fp;                     // ファイルストリームのポインタ
  size_t max_fsize;             // ログ出力ファイルの最大サイズ
  size_t max_fno;               // ログ出力ファイルの最大数
  file_list_t* flist;           // ファイル情報リストのポインタ
  char base_fpath[FPATH_SIZE];  // ベースのファイルパス
} rot_param_t;

// パラメータの初期化
static rot_param_t g_param = {
    .fp = NULL,
    .max_fsize = 0,
    .max_fno = 0,
    .flist = NULL,
    .base_fpath = {0},
};

static bool fp_init(const char* fpath);
static void fp_destroy(void);
static file_info_t* finfo_init(void);
static void finfo_destroy(file_info_t* self);
static bool finfo_update(file_info_t** self, file_info_t* info);
static file_list_t* flist_init(size_t file_num);
static void flist_destroy(file_list_t* self);
static int flist_add_finfo(file_list_t* self, file_info_t* info);
static bool flist_del_last_finfo(file_list_t* self);
static bool flist_realloc_finfo(file_list_t** pself);
static int compare_mtime_desc(const void* a, const void* b);
static void sort_file_list_desc(file_list_t* flist);
static void make_fpath(char* new_fpath, const char* fpath);
static file_info_t* get_file_info(const char* fpath);
static bool get_all_file_info(
    file_list_t* flist, const char* dpath, const char* search
);
static void rotator_set_max_fsize(size_t size);
static void rotator_set_max_fno(size_t no);
static bool rotator_set_base_fpath(
    const char* dpath, const char* fname, const char* extension
);
static bool rotator_set_file_info(const char* dpath, const char* extension);