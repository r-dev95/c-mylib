/**
 * iniファイルを読み込むデモ。
 */

#include <stdio.h>
#include <stdlib.h>

#include "ini/ini.h"

void usage(int argc, char** argv);

/**
 * @brief 本プログラムの使い方を出力する。
 * @param argc コマンドライン引数の数。
 * @param argv コマンドライン引数のポインタのポインタ。
 */
void usage(int argc, char** argv) {
  (void)argc;
  fprintf(stderr, "使い方: %s <ini filefpath>\n", argv[0]);
}

/**
 * @brief テスト用メイン関数。
 * @param argc コマンドライン引数の数。
 * @param argv コマンドライン引数のポインタのポインタ。
 * @return
 */
int main(int argc, char** argv) {
  if (argc < 2) {
    usage(argc, argv);
    return EXIT_FAILURE;
  }

  Ini* ini = ini_load(argv[1]);
  if (!ini) {
    fprintf(stderr, "[ini_load] iniファイルの読み込み失敗。[%s]\n", argv[1]);
    return EXIT_FAILURE;
  }

  // 値を取り出す例
  const char* v1 = ini_get(ini, "section1", "key1", "def1");
  const char* v2 = ini_get(ini, NULL, "globalKey", "gdef");  // グローバル
  printf("section1: key1 = %s\n", v1);
  printf("global: globalKey = %s\n", v2);

  // 全出力（ファイル順で表示されるはず）
  printf("\n-- full dump --\n");
  ini_dump(ini);

  ini_close(ini);

  return EXIT_SUCCESS;
}
