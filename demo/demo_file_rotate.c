#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logger/rotator.h"

int main() {
  int len = 0;
  int count = 0;
  int fin_flag = 0;
  size_t loop_num = 128;
  char* line = calloc(128, sizeof(line));

  const char* dpath = "demo";
  const char* fname = "sample";
  const char* extension = ".log";

  if (!rotator_init(dpath, fname, extension, 1024, 5)) {
    fprintf(stderr, "❌ファイルローテーションの初期化に失敗。\n");
    return EXIT_FAILURE;
  }

  while (true) {
    count += 1;
    len = 0;
    for (size_t i = 0; i < loop_num; i++) {
      len += snprintf(line, 128 * sizeof(line), "[%5d]\n", count);
    }
    sleep(5);
    fin_flag += 1;

    if (!rotator_rotate((size_t)len)) {
      fprintf(stderr, "❌ファイルローテーションに失敗。\n");
      return EXIT_FAILURE;
    }

    for (size_t i = 0; i < loop_num; i++) { rotator_fputs(line); }
  }
  free(line);

  rotator_close();

  return EXIT_SUCCESS;
}