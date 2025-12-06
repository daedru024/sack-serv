OS := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(OS), Linux)
    PLATFORM = linux
	CC = gcc
endif

ifeq ($(OS), Darwin)
    PLATFORM = macos
	CC = clang
endif

CFLAGS := -Iinclude
SRC_DIR := src
OBJ_DIR := obj
OUT_DIR := out
TARGET := sackServ
AUTOPLAY_FILES := $(OBJ_DIR)/apServ.o

C_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
MAIN_FILES = $(filter-out $(AUTOPLAY_FILES),$(OBJ_FILES))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

all: platform clean mkdir $(TARGET) apServ

$(TARGET): $(MAIN_FILES)
	$(CC) $(MAIN_FILES) -o $(OUT_DIR)/$@

apServ: $(AUTOPLAY_FILES)
	$(CC) $(AUTOPLAY_FILES) -o $(OUT_DIR)/$@

.PHONY: platform clean mkdir

platform:
	@echo "Detected PLATFORM: $(PLATFORM)"

mkdir:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OUT_DIR)

clean:
	rm -rf $(OBJ_DIR) $(OUT_DIR)