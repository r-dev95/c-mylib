/**
 * ログ処理関数群。
 *
 * - Posix (Linux/Mac OS)標準: pthread.h
 */

#include "logger_posix.h"

#include "utils.h"

/**
 * @brief ログデータのメモリを確保する。
 * @return ログデータのポインタ。
 */
static log_item_t* log_item_init(void) {
  log_item_t* self = (log_item_t*)calloc(1, sizeof(*self));
  if (!self) { return NULL; }
  return self;
}

/**
 * @brief ログデータのメモリを解放する。
 * @param self ログデータのポインタ。
 */
static void log_item_destroy(log_item_t* self) {
  if (!self) { return; }
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
  if (!fmt) { return false; }

  if (g_param.format) { free(g_param.format); }
  g_param.format = my_strdup(fmt);
  if (!g_param.format) { return false; }
  return true;
}

/**
 * @brief ログフォーマットのメモリを解放する。
 */
static void format_destroy(void) {
  if (!g_param.format) { return; }

  free(g_param.format);
  g_param.format = NULL;
}

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
 * @brief ログ出力用ファイルのバッファリング方式を設定する。
 * @param bufsize バッファサイズ。
 * @return 成功: true, 失敗: false。
 */
static bool fp_setvbuf(const size_t bufsize) {
  if (setvbuf(g_param.fp, NULL, _IOFBF, bufsize) != 0) { return false; }
  return true;
}

/**
 * @brief 非同期モード用のキューのメモリを確保する。
 * @param nqueue 非同期モード用のキューの数。
 * @return 成功: true, 失敗: false。
 */
static bool queue_init(const size_t nqueue) {
  g_param.queue = (log_item_t**)calloc(nqueue, sizeof(log_item_t*));
  if (!g_param.queue) { return false; }
  return true;
}

/**
 * @brief 非同期モード用のキューのメモリを解放する。
 */
static void queue_destroy(void) {
  if (!g_param.queue) { return; }

  free(g_param.queue);
  g_param.queue = NULL;
}

/**
 * @brief pthread_mutex_lockをエラーハンドリングのためラップしている。
 * @param mutex 排他制御用mutex
 * @return 成功: true, 失敗: false。
 */
static bool mutex_lock(pthread_mutex_t* mutex) {
  if (pthread_mutex_lock(mutex) != 0) {
    fprintf(stderr, "mutexをロックできません。\n");
    return false;
  }
  return true;
}

/**
 * @brief pthread_mutex_unlockをエラーハンドリングのためラップしている。
 * @param mutex 排他制御用mutex
 * @return 成功: true, 失敗: false。
 */
static bool mutex_unlock(pthread_mutex_t* mutex) {
  if (pthread_mutex_unlock(mutex) != 0) {
    fprintf(stderr, "mutexをアンロックできません。\n");
    return false;
  }
  return true;
}

/**
 * @brief pthread_cond_signalをエラーハンドリングのためラップしている。
 * @param cond 排他制御用cond
 * @return 成功: true, 失敗: false。
 */
static bool cond_signal(pthread_cond_t* cond) {
  if (pthread_cond_signal(cond) != 0) {
    fprintf(stderr, "condシグナルを送信できません。\n");
    return false;
  }
  return true;
}

/**
 * @brief pthread_cond_waitをエラーハンドリングのためラップしている。
 * @param cond 排他制御用cond
 * @param mutex 排他制御用mutex
 * @return 成功: true, 失敗: false。
 */
static bool cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
  if (pthread_cond_wait(cond, mutex) != 0) {
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
  if (needed_size <= *cap) { return true; }

  *cap = (needed_size) * 2;
  char* new_out = (char*)realloc(*pout, *cap);
  if (!new_out) { return false; }

  *pout = new_out;
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
  if (!out) { return NULL; }

  out[0] = '\0';
  size_t len = 0;
  const char* fmt = g_param.format ? g_param.format : DEFAULT_FORMAT;
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

  // 標準出力
  if ((g_param.out & LOG_STD_OUT) == LOG_STD_OUT) { printf("%s", line); }
  // ファイル出力
  if ((g_param.out & LOG_FILE_OUT) == LOG_FILE_OUT) {
    if (g_param.fp) { fputs(line, g_param.fp); }
  }
  free(line);
  fflush(g_param.fp);
}

/**
 * @brief キューにログデータを追加する。
 * @param item ログデータ。
 * @return 成功: true, 失敗: false。
 */
static bool enqueue_item(log_item_t* item) {
  if (!g_param.async || !item) { return false; }

  bool res = false;
  if (g_param.q_count < g_param.nqueue) {
    // キューに空きがある場合、末尾に追加
    g_param.queue[g_param.q_tail] = item;
    g_param.q_tail = (g_param.q_tail + 1) % g_param.nqueue;
    g_param.q_count++;
    res = true;
  } else {
    // キューに空きがない場合、先頭（古い）データを削除して追加
    log_item_destroy(g_param.queue[g_param.q_head]);
    g_param.queue[g_param.q_head] = item;
    g_param.q_head = (g_param.q_head + 1) % g_param.nqueue;
    g_param.q_tail = (g_param.q_tail + 1) % g_param.nqueue;
    res = true;
  }

  return res;
}

/**
 * @brief キューの先頭からログデータを取得する。
 * @return ログデータ。
 */
static log_item_t* dequeue_item(void) {
  if (!g_param.async) { return NULL; }

  log_item_t* item = NULL;
  if (g_param.q_count > 0) {
    item = g_param.queue[g_param.q_head];
    g_param.q_head = (g_param.q_head + 1) % g_param.nqueue;
    g_param.q_count--;
  }
  return item;
}

/**
 * @brief
 * キューに追加されたログデータをストリームへ出力する。（スレッド用ワーカー）
 * @param arg パラメータ。（使用しない）
 * @return NULL
 */
static void* worker(void* arg) {
  (void)arg;

  while (true) {
    mutex_lock(&g_param.mutex);

    // キューへのログデータ追加待ち
    while (g_param.worker_running && g_param.q_count == 0) {
      cond_wait(&g_param.cond, &g_param.mutex);
    }
    // 無限ループを終了
    if (!g_param.worker_running && g_param.q_count == 0) {
      mutex_unlock(&g_param.mutex);
      break;
    }
    // キューからログデータを取得してストリームに出力
    log_item_t* item = dequeue_item();

    mutex_unlock(&g_param.mutex);

    if (item) {
      output_line(item);
      log_item_destroy(item);
    }
  }

  return NULL;
}

/**
 * @brief ログ出力フラグを設定する。
 * @param out ログ出力フラグ。
 */
static void logger_set_out(const log_out_t out) { g_param.out = out; }

/**
 * @brief ログレベルを設定する。
 * @param level ログレベル。
 */
static void logger_set_level(const log_level_t level) { g_param.level = level; }

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
static bool logger_set_format(const char* fmt) {
  fmt = fmt ? fmt : DEFAULT_FORMAT;

  if (!format_init(fmt)) {
    fprintf(stderr, "ログフォーマットのメモリを確保できません。\n");
    return false;
  }
  return true;
}

/**
 * @brief ログストリームを設定する。
 * @param fpath ファイルパス。
 * @param bufsize ファイルパスのバッファサイズ。
 * @return 成功: true, 失敗: false。
 */
static bool logger_set_stream(const char* fpath) {
  if (!fpath) { return false; }

  if (!fp_init(fpath)) {
    fprintf(stderr, "ログファイルを開けません。[%s]\n", fpath);
    return false;
  }

  if (!fp_setvbuf(STREAM_BUF_SIZE)) {
    fprintf(
        stderr, "ログストリームのバッファリング方式を設定できません。[%zu]\n",
        STREAM_BUF_SIZE
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
static bool logger_set_async(const bool async) {
  g_param.async = async;
  if (!g_param.async) { return true; }

  if (!queue_init(MAX_QUEUE_NO)) {
    fprintf(
        stderr, "非同期モード用キューのメモリを確保できません。[%zu]\n",
        MAX_QUEUE_NO
    );
    return false;
  }

  g_param.worker_running = true;
  pthread_mutex_init(&g_param.mutex, NULL);
  pthread_cond_init(&g_param.cond, NULL);
  if (pthread_create(&g_param.worker, NULL, worker, NULL) != 0) {
    g_param.worker_running = false;
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
 * @param level ログレベル。
 * @param fmt ログフォーマット。
 * @param async 非同期モードフラグ。
 * @param fpath ログファイルパス。（NULLの場合、stderr）
 * @param max_fsize 最大ファイルバイトサイズ。
 * @param max_fno 最大アーカイブファイル数。
 * @return 成功: true, 失敗: false。
 */
bool logger_init(
    const log_out_t out, const log_level_t level, const char* fmt,
    const bool async, const char* fpath
) {
  // ログ出力フラグを設定
  logger_set_out(out);
  // ログレベルを設定
  logger_set_level(level);
  // ログフォーマットを設定
  if (!logger_set_format(fmt)) {
    fprintf(stderr, "ログフォーマットの設定に失敗しました。[%s]\n", fmt);
    return false;
  };
  // ログストリームを設定
  if (!logger_set_stream(fpath)) {
    fprintf(stderr, "ログストリームの設定に失敗しました。[%s]\n", fpath);
    return false;
  }
  // 非同期モードを設定
  if (!logger_set_async(async)) {
    fprintf(
        stderr, "非同期モードの設定に失敗しました。[%s]\n",
        async ? "true" : "false"
    );
    return false;
  }

  return true;
}

/**
 * @brief ログ処理を終了する。
 */
void logger_close(void) {
  if (g_param.async) {
    // スレッドを停止
    mutex_lock(&g_param.mutex);

    g_param.worker_running = false;
    cond_signal(&g_param.cond);
    mutex_unlock(&g_param.mutex);
    pthread_join(g_param.worker, NULL);

    // キューに残っているログを出力
    log_item_t* item;
    while ((item = dequeue_item()) != NULL) {
      output_line(item);
      log_item_destroy(item);
    }
    queue_destroy();
    pthread_mutex_destroy(&g_param.mutex);
    pthread_cond_destroy(&g_param.cond);
  }
  fp_destroy();
  format_destroy();
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
  if (!fmt) { return; }
  if (level < g_param.level) { return; }

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
  if (!g_param.async) {
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

  mutex_lock(&g_param.mutex);
  if (enqueue_item(item)) {
    cond_signal(&g_param.cond);
  } else {
    log_item_destroy(item);
  }
  mutex_unlock(&g_param.mutex);
}
