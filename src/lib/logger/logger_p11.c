/**
 * ログ処理関数群。
 *
 * - C11標準: threads
 */

#include "logger_p11.h"

#include "utils.h"

/**
 * @brief ログデータのメモリを確保する。
 * @return ログデータのポインタ。
 */
static log_item_t* log_item_init(void) {
  log_item_t* self = (log_item_t*)calloc(1, sizeof(*self));
  if (!self) return NULL;

  return self;
}

/**
 * @brief ログデータのメモリを解放する。
 * @param self ログデータのポインタ。
 */
static void log_item_destroy(log_item_t* self) {
  if (!self) return;

  free(self->fname);
  free(self->func);
  free(self->msg);
  free(self);
}

/**
 * @brief ログフォーマットのメモリを確保し、文字列を格納する。
 * @param fmt ログフォーマット。
 * @return 成功: true, 失敗: false。
 */
static bool format_init(const char* fmt) {
  if (!fmt) return false;

  if (g_format) free(g_format);
  g_format = my_strdup(fmt);
  if (!g_format) return false;

  return true;
}

/**
 * @brief ログフォーマットのメモリを解放する。
 */
static void format_destroy(void) {
  if (!g_format) return;

  free(g_format);
  g_format = NULL;
}

/**
 * @brief ログ出力用ファイルを開く。
 * @param fpath ファイルパス。
 * @return 成功: true, 失敗: false。
 */
static bool fp_init(const char* fpath) {
  if (!fpath) return false;

  g_fp = fopen(fpath, "a");
  if (!g_fp) return false;

  return true;
}

/**
 * @brief ログ出力用ファイルをフラッシュし、閉じる。
 */
static void fp_destroy(void) {
  if (!g_fp || g_fp == stderr) return;

  fflush(g_fp);
  fclose(g_fp);
  g_fp = NULL;
}

/**
 * @brief ログ出力用ファイルのメモリを確保する。
 * @param bufsize バッファサイズ。
 * @return 成功: true, 失敗: false。
 */
static bool fp_setvbuf(const size_t bufsize) {
  if (setvbuf(g_fp, NULL, _IOFBF, bufsize) != 0) return false;

  return true;
}

/**
 * @brief 非同期モード用のキューのメモリを確保する。
 * @param nqueue 非同期モード用のキューの数。
 * @return 成功: true, 失敗: false。
 */
static bool queue_init(const size_t nqueue) {
  g_queue = (log_item_t**)calloc(nqueue, sizeof(log_item_t*));
  if (!g_queue) return false;

  return true;
}

/**
 * @brief 非同期モード用のキューのメモリを解放する。
 */
static void queue_destroy(void) {
  if (!g_queue) return;

  free(g_queue);
  g_queue = NULL;
}

/**
 * @brief mtx_lockをエラーハンドリングのためラップしている。
 * @param mutex 排他制御用mutex
 * @return 成功: true, 失敗: false。
 */
static bool mutex_lock(mtx_t* mutex) {
  if (mtx_lock(mutex) != 0) {
    fprintf(stderr, "mutexをロックできません。\n");
    return false;
  }
  return true;
}

/**
 * @brief mtx_unlockをエラーハンドリングのためラップしている。
 * @param mutex 排他制御用mutex
 * @return 成功: true, 失敗: false。
 */
static bool mutex_unlock(mtx_t* mutex) {
  if (mtx_unlock(mutex) != 0) {
    fprintf(stderr, "mutexをアンロックできません。\n");
    return false;
  }
  return true;
}

/**
 * @brief cnd_signalをエラーハンドリングのためラップしている。
 * @param cond 排他制御用cond
 * @return 成功: true, 失敗: false。
 */
static bool cond_signal(cnd_t* cond) {
  if (cnd_signal(cond) != 0) {
    fprintf(stderr, "condシグナルを送信できません。\n");
    return false;
  }
  return true;
}

/**
 * @brief cnd_waitをエラーハンドリングのためラップしている。
 * @param cond 排他制御用cond
 * @param mutex 排他制御用mutex
 * @return 成功: true, 失敗: false。
 */
static bool cond_wait(cnd_t* cond, mtx_t* mutex) {
  if (cnd_wait(cond, mutex) != 0) {
    fprintf(stderr, "condシグナルを待ち受けできません。\n");
    return false;
  }
  return true;
}

/**
 * @brief ログレベル名を取得する。
 * @param level ログレベル。
 * @return ログレベル名。
 */
static char* get_level_name(const log_level_t level) {
  switch (level) {
    case LOG_LEVEL_ERROR:
      return "ERROR";
    case LOG_LEVEL_WARN:
      return "WARN";
    case LOG_LEVEL_INFO:
      return "INFO";
    case LOG_LEVEL_DEBUG:
      return "DEBUG";
    default:
      return "UNK";
  }
}

/**
 * @brief 作成したログを格納するメモリを再確保する。
 *
 * - 使用したいメモリサイズが使用可能なメモリサイズを超えた場合、
 *   使用可能なメモリサイズを2倍にして、メモリを再確保する。
 * @param pout ログ格納ポインタのポインタ。
 * @param cap 使用可能なメモリサイズ。
 * @param needed_size 使用したいメモリサイズ。
 * @return 成功: true, 失敗: false。
 */
static bool realloc_format_line(
    char** pout, size_t* cap, const size_t needed_size
) {
  if (needed_size > *cap) {
    *cap = (needed_size) * 2;
    char* new_out = (char*)realloc(*pout, *cap);
    if (!new_out) {
      return false;
    }
    *pout = new_out;
    return true;
  }

  return true;
}

/**
 * @brief フォーマットに応じたログを作成する。
 * @param item ログデータ。
 * @return 作成したログ。
 */
static char* format_line(const log_item_t* item) {
  size_t cap = MIN_LOG_SIZE;
  char* out = (char*)malloc(cap);
  if (!out) return NULL;

  out[0] = '\0';
  size_t len = 0;
  const char* fmt = g_format ? g_format : DEFAULT_FORMAT;
  for (const char* ptr = fmt; *ptr; ++ptr) {
    if (*ptr == '%' && *(ptr + 1)) {
      char ch = *(ptr + 1);
      ptr++;

      char buff[MAX_CONV_SPEC_SIZE];
      switch (ch) {
        case 'T': {  // タイムスタンプ
          struct tm tm = get_current_time();
          snprintf(
              buff, sizeof(buff), "%04d-%02d-%02d %02d:%02d:%02d",
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
              tm.tm_min, tm.tm_sec
          );
          break;
        }
        case 'l': {  // ログレベル
          snprintf(buff, sizeof(buff), "%-5s", get_level_name(item->level));
          break;
        }
        case 'F': {  // ファイル名
          snprintf(buff, sizeof(buff), "%s", item->fname ? item->fname : "");
          break;
        }
        case 'L': {  // 行数
          snprintf(buff, sizeof(buff), "%d", item->line);
          break;
        }
        case 'f': {  // 関数名
          snprintf(buff, sizeof(buff), "%s", item->func ? item->func : "");
          break;
        }
        case 'm': {  // メッセージ
          snprintf(buff, sizeof(buff), "%s", item->msg ? item->msg : "");
          break;
        }
        default: {
          buff[0] = '%';
          buff[1] = ch;
          buff[2] = '\0';
          break;
        }
      }

      size_t add_size = strlen(buff);
      if (!realloc_format_line(&out, &cap, len + add_size + 2)) {
        free(out);
        return NULL;
      }
      memcpy(out + len, buff, add_size);
      len += add_size;
      out[len] = '\0';
    } else {
      if (!realloc_format_line(&out, &cap, len + 2)) {
        free(out);
        return NULL;
      }
      out[len++] = *ptr;
      out[len] = '\0';
    }
  }

  // 終端処理
  if (len == 0 || out[len - 1] != '\n') {
    if (!realloc_format_line(&out, &cap, len + 2)) {
      free(out);
      return NULL;
    }
    out[len++] = '\n';
    out[len] = '\0';
  }

  return out;
}

/**
 * @brief ログを出力する。
 * @param item ログデータのポインタ。
 */
static void output_line(const log_item_t* item) {
  char* line = format_line(item);
  if (!line) {
    fprintf(stderr, "ログを作成できません。\n");
    return;
  }

  if ((g_out & LOG_STD_OUT) == LOG_STD_OUT) {
    printf("%s", line);
  }

  if ((g_out & LOG_FILE_OUT) == LOG_FILE_OUT) {
    fputs(line, g_fp ? g_fp : stderr);
  }
  free(line);
}

/**
 * @brief キューにログデータを追加する。
 * @param item ログデータ。
 * @return 成功: true, 失敗: false。
 */
static bool enqueue_item(log_item_t* item) {
  if (!g_async || !item) return false;

  mutex_lock(&g_mutex);

  bool ok = false;
  if (g_q_count < g_nqueue) {
    // キューに空きがある場合、末尾に追加
    g_queue[g_q_tail] = item;
    g_q_tail = (g_q_tail + 1) % g_nqueue;
    g_q_count++;
    ok = true;
  } else {
    // キューに空きがない場合、先頭（古い）データを削除して追加
    log_item_destroy(g_queue[g_q_head]);
    g_queue[g_q_head] = item;
    g_q_head = (g_q_head + 1) % g_nqueue;
    g_q_tail = (g_q_tail + 1) % g_nqueue;
    ok = true;
  }
  if (!cond_signal(&g_cond)) return false;
  mutex_unlock(&g_mutex);
  return ok;
}

/**
 * @brief キューの先頭からログデータを取得する。（mutexによる排他制御なし）
 * @return ログデータ。
 */
static log_item_t* dequeue_item_none_mutex(void) {
  log_item_t* item = NULL;
  if (g_q_count > 0) {
    item = g_queue[g_q_head];
    g_q_head = (g_q_head + 1) % g_nqueue;
    g_q_count--;
  }
  return item;
}

/**
 * @brief キューの先頭からログデータを取得する。
 * @return ログデータ。
 */
static log_item_t* dequeue_item(void) {
  if (!g_async) return NULL;

  if (!mutex_lock(&g_mutex)) return false;

  log_item_t* item = dequeue_item_none_mutex();
  mutex_unlock(&g_mutex);
  return item;
}

/**
 * @brief
 * キューに追加されたログデータをストリームへ出力する。（スレッド用ワーカー）
 * @param arg パラメータ。（使用しない）
 * @return NULL
 */
static int worker(void* arg) {
  (void)arg;

  while (true) {
    mutex_lock(&g_mutex);

    // キューへのログデータ追加待ち
    while (g_q_count == 0 && g_worker_running) {
      cond_wait(&g_cond, &g_mutex);
    }

    // 無限ループを終了
    if (!g_worker_running && g_q_count == 0) {
      mutex_unlock(&g_mutex);
      break;
    }

    // キューからログデータを取得してストリームに出力
    log_item_t* item = dequeue_item_none_mutex();
    mutex_unlock(&g_mutex);
    if (item) {
      output_line(item);
      log_item_destroy(item);
    }
  }

  return 0;
}

/**
 * @brief ログ出力フラグを設定する。
 * @param out ログ出力フラグ。
 */
static void logger_set_out(const log_out_t out) { g_out = out; }

/**
 * @brief ログレベルを設定する。
 * @param level ログレベル。
 */
static void logger_set_level(const log_level_t level) { g_level = level; }

/**
 * @brief ログストリームを設定する。
 *
 * - ファイルパスが指定されている場合、ファイルストリームを設定する。
 * - ファイルパスが指定されていない場合、"stderr"を設定する。
 * @param fpath ファイルパス。
 * @param bufsize ファイルパスのバッファサイズ。
 * @return 成功: true, 失敗: false。
 */
static bool logger_set_stream(const char* fpath, const size_t bufsize) {
  g_fp = stderr;
  if (!fpath) return true;

  if (!fp_init(fpath)) {
    fprintf(stderr, "ログファイルを開けません。[%s]\n", fpath);
    return false;
  }

  if (!fp_setvbuf(bufsize)) {
    fprintf(
        stderr, "ログストリームのバッファサイズを確保できません。[%zu]\n",
        bufsize
    );
    return false;
  }

  return true;
}

/**
 * @brief 非同期モードを設定する。
 * @param async 非同期モードフラグ。
 * @param nqueue 非同期モード用のキューの数。
 * @return 成功: true, 失敗: false。
 */
static bool logger_set_async(const bool async, const size_t nqueue) {
  g_async = async;
  if (!g_async) return true;

  if (!queue_init(nqueue)) {
    fprintf(stderr, "非同期モード用キューのメモリを確保できません。\n");
    return false;
  }

  g_worker_running = true;
  mtx_init(&g_mutex, mtx_plain);
  cnd_init(&g_cond);
  if (thrd_create(&g_worker, worker, NULL) != thrd_success) {
    g_worker_running = false;
    fprintf(stderr, "非同期モード用スレッドが作成できません。\n");
    return false;
  }

  return true;
}

// ----------------------------------------------------------------------------
// 以降、公開関数
// ----------------------------------------------------------------------------

/**
 * @brief ログ処理を初期化する。
 * @param out ログ出力フラグ。
 * @param fpath ログファイルパス。（NULLの場合、stderr）
 * @param level ログレベル。
 * @param bufsize ログストリームのバッファサイズ。
 * @param async 非同期モードフラグ。
 * @param nqueue 非同期モード用のキューの数。
 * @return 成功: true, 失敗: false。
 */
bool logger_init(
    const log_out_t out, const char* fpath, const log_level_t level,
    const size_t bufsize, const bool async, const size_t nqueue
) {
  // ログ出力フラグを設定
  logger_set_out(out);
  // ログレベルを設定
  logger_set_level(level);
  // ログフォーマットを設定
  if (!logger_set_format(DEFAULT_FORMAT)) {
    fprintf(
        stderr, "ログフォーマットの設定に失敗しました。[%s]\n", DEFAULT_FORMAT
    );
    return false;
  };
  // ログストリームを設定
  if (!logger_set_stream(fpath, bufsize)) {
    fprintf(
        stderr, "ログストリームの設定に失敗しました。[%s][%zu]\n", fpath,
        bufsize
    );
    return false;
  }
  // 非同期モードを設定
  if (!logger_set_async(async, nqueue)) {
    fprintf(
        stderr, "非同期モードの設定に失敗しました。[%s][%zu]\n",
        async ? "true" : "false", nqueue
    );
    return false;
  }

  return true;
}

/**
 * @brief ログ処理を終了する。
 */
void logger_close(void) {
  if (g_async) {
    // スレッドを停止
    mutex_lock(&g_mutex);

    g_worker_running = false;
    cond_signal(&g_cond);
    mutex_unlock(&g_mutex);
    thrd_join(g_worker, NULL);

    // キューに残っているログを出力
    log_item_t* item;
    while ((item = dequeue_item()) != NULL) {
      output_line(item);
      log_item_destroy(item);
    }
    queue_destroy();
    mtx_destroy(&g_mutex);
    cnd_destroy(&g_cond);
  }
  fp_destroy();
  format_destroy();
}

/**
 * @brief ログフォーマットを設定する。
 *
 * デフォルトフォーマット: [%T][%l][%F:%L][%f()] - %m
 *
 * 変換指定子:
 * - %T : タイムスタンプ (YYYY-MM-DD HH:MM:SS)
 * - %l : ログレベル (DEBUG/INFO/WARN/ERROR)
 * - %F : ファイル名
 * - %L : 行番号
 * - %f : 関数名
 * - %m : メッセージ
 *
 * @param fmt ログフォーマット。
 */
bool logger_set_format(const char* fmt) {
  if (!fmt) return false;

  if (!format_init(fmt)) {
    fprintf(stderr, "ログフォーマットのメモリを確保できません。\n");
    return false;
  }
  return true;
}

/**
 * @brief ログを出力する。
 *
 * - 通常は本関数をラップしたマクロを使用する。
 *
 * @param level ログレベル。
 * @param fpath ファイルパス。
 * @param func 関数名。
 * @param line 行番号。
 * @param fmt 可変長メッセージ。
 */
void logger_log(
    const log_level_t level, const char* fpath, const char* func,
    const int line, const char* fmt, ...
) {
  if (!fmt) return;
  if (level < g_level) return;

  // 可変長メッセージをフォーマット
  va_list ap, ap2;
  char* msg = NULL;
  char small_buff[SMALL_VAR_SIZE];
  int needed;

  va_start(ap, fmt);
  va_copy(ap2, ap);

  // 可変長メッセージの書き込みサイズを取得
  needed = vsnprintf(small_buff, sizeof(small_buff), fmt, ap2);
  va_end(ap2);

  // 可変長メッセージの書き込みサイズの確認と埋め込み
  if (needed < 0) {
    msg = my_strdup("<format-error>");
  } else if ((size_t)needed < sizeof(small_buff)) {
    msg = my_strdup(small_buff);
  } else {
    size_t sz = (size_t)needed + 1;
    msg = (char*)malloc(sz);
    if (!msg) {
      va_end(ap);
      return;
    }

    if (vsnprintf(msg, sz, fmt, ap) < 0) {
      free(msg);
      va_end(ap);
      return;
    }
  }

  va_end(ap);

  // ファイルパスからファイル名を取得
  char* fname = get_fname(fpath);

  // 同期モード（直接フォーマットして書き出し）
  if (!g_async) {
    log_item_t item = {
        .level = level,
        .fname = fname,
        .func = (char*)func,
        .line = line,
        .msg = msg,
    };
    output_line(&item);
    free(msg);
    return;
  }

  // 非同期モード（項目を確保してキューに追加）
  log_item_t* item = log_item_init();
  if (!item) {
    fprintf(stderr, "非同期モードのログデータのメモリを確保できません。\n");
    free(msg);
    return;
  }

  item->level = level;
  item->fname = fname ? my_strdup(fname) : my_strdup("");
  item->func = func ? my_strdup(func) : my_strdup("");
  item->line = line;
  item->msg = msg;

  if (!enqueue_item(item)) log_item_destroy(item);
}
