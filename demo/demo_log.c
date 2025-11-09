/**
 * ログをファイルに出力するデモ。
 */

#include <stdio.h>
#include <stdlib.h>

#include "logger/logger.h"

int main(void) {
  const char* path = "demo.log";

  if (!logger_init(path, LOG_LEVEL_DEBUG, 16 * 1024, true, 4096)) {
    fprintf(stderr, "ログ出力処理の初期化に失敗しました。\n");
    return EXIT_FAILURE;
  }

  if (!logger_set_format("[%T][%l][%F:%L][%f()] - %m")) {
    fprintf(stderr, "ログフォーマットの設定に失敗しました。\n");
    return EXIT_FAILURE;
  }

  LOG_DEBUG("デバッグ値: x=%d,%d,%d,%d", 42, 2, 3, 4);
  LOG_INFO("アプリ開始: pid=%d", 12345);
  LOG_WARN("メモリが少ないかも");
  LOG_ERROR("致命的エラー: %s", "何か悪いことが起きた");

  logger_close();

  return EXIT_SUCCESS;
}
