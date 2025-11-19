# =========================================================
# Usage:
#     make        : 通常ビルド（デバッグ情報付き）
#     make SAN=1  : サニタイザ有効ビルド
#     make release: 最適化されたリリースビルド
#     make clean  : ビルド成果物を削除
#     make dirs   : 必要なディレクトリ作成のみ
#     ---
#     make MAIN=test/test.c: mainファイルを指定
# =========================================================

# -----------------------------------------------
# 設定
# -----------------------------------------------
SRC_DIR   := src
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
BIN_DIR   := $(BUILD_DIR)/bin

MAIN      ?= $(SRC_DIR)/main.c
MAIN_OBJ  := $(MAIN:%.c=$(OBJ_DIR)/%.o)
TARGET    := $(MAIN:%.c=$(BIN_DIR)/%)
SRCS      := $(shell find $(SRC_DIR) -name "*.c" ! -name "_*.c")
OBJS      := $(filter-out $(MAIN_OBJ), $(SRCS:%.c=$(OBJ_DIR)/%.o))

CC        := gcc
CFLAGS    := -std=c11 -Wall -Wextra -Wpedantic \
            -Wconversion -Wshadow -Wcast-align -Wpointer-arith \
			-Wformat -Wmissing-prototypes -Wstrict-prototypes -Werror \
			-g
IFLAGS    := -Isrc/lib
LDFLAGS   :=

ifdef SAN
CFLAGS   += -fsanitize=address -fsanitize=undefined
LDFLAGS  += -fsanitize=address -fsanitize=undefined
endif

ifdef RELEASE
CFLAGS   := -std=c11 -O2 -Wall -Wextra -Wpedantic
endif

# -----------------------------------------------
# ルール
# -----------------------------------------------
# デフォルト
all: dirs $(TARGET)

# コンパイル
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

# リンク
$(TARGET): $(MAIN_OBJ) $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(MAIN_OBJ) $(OBJS) -o $@ $(LDFLAGS)

# クリーンアップ
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# ディレクトリ作成
dirs:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

# -----------------------------------------------
# ビルドモードのエイリアス
# -----------------------------------------------
# デバッグモード
debug:
	make all

# リリースモード
release:
	make RELEASE=1 all

.PHONY: all clean dirs debug release