#include <stdio.h>
#include <stdlib.h>

#include "error/error.h"

int func(int num);

int func(int num) { return num; }

int main(void) {
  size_t idx;
  error_log_t log;
  int ary[] = {
      ERR_INVALID_ARG,
      ERR_FILE_OPEN_FAILED,
      ERR_IO_ERROR,
  };

  for (size_t i = 0; i < sizeof(ary) / sizeof(ary[0]); i++) {
    int res = func(ary[i]);
    if (res != ERR_OK) { SET_ERR_LOG(res, "%s", code_to_msg(res)); }
  }

  log = get_error_log();
  printf("最新エラーログ:\n");
  printf(
      "[%04d-%02d-%02d %02d:%02d:%02d][%3d][%s:%zu][%s()] - %s\n",
      log.tm.tm_year + 1900, log.tm.tm_mon + 1, log.tm.tm_mday, log.tm.tm_hour,
      log.tm.tm_min, log.tm.tm_sec, log.code, log.fpath, log.line, log.func,
      log.msg
  );

  error_log_list_t list = get_error_log_all();
  printf("全エラーログ:\n");
  for (size_t i = 0; i < list.count; i++) {
    idx = (list.head + ERRLOG_MAX_NUM - 1 - i) % ERRLOG_MAX_NUM;
    log = list.log[idx];
    printf(
        "[%04d-%02d-%02d %02d:%02d:%02d][%3d][%s:%zu][%s()] - %s\n",
        log.tm.tm_year + 1900, log.tm.tm_mon + 1, log.tm.tm_mday,
        log.tm.tm_hour, log.tm.tm_min, log.tm.tm_sec, log.code, log.fpath,
        log.line, log.func, log.msg
    );
  }
  return EXIT_SUCCESS;
}