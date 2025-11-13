OS := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(OS), Linux)
    PLATFORM = linux
	CC = gcc
endif

ifeq ($(OS), Darwin)
    PLATFORM = macos
	CC = clang
endif

ifeq ($(OS), Windows)
	CC = gcc
    ifdef ComSpec
        PLATFORM = windows-cmd
    endif
    ifdef PSModulePath
        PLATFORM = windows-powershell
    endif
endif

ifneq (,$(findstring MINGW, $(OS)))
    PLATFORM = windows-gitbash
endif

CFLAGS := -Iinclude
SRC_DIR := src
OBJ_DIR := obj
TARGET := sackServ

OBJ_FILES = $(OBJDIR)/*.o

$(OBJ_DIR)/*.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

all: platform clean mkdir $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $@ 

.PHONY: platform clean mkdir

platform:
	@echo "Detected PLATFORM: $(PLATFORM)"

mkdir:
	@mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)