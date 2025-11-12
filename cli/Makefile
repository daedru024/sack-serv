OS := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(OS), Linux)
    PLATFORM = linux
	CC = gcc
	CXX = g++
endif

ifeq ($(OS), Darwin)
    PLATFORM = macos
	CC = clang
	CXX = clang++
endif

ifeq ($(OS), Windows)
	CC = gcc
	CXX = g++
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
CXXFLAGS := -Iinclude -std=c++11
LDFLAGS := -lsfml-graphics -lsfml-window -lsfml-system
SRC_DIR := src
OBJ_DIR := obj
TARGET := sackCli

OBJ_FILES = $(OBJDIR)/cli_ui.o $(OBJDIR)/libcli.o

$(OBJ_DIR)/libcli.o: $(SRC_DIR)/libcli.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/cli_ui.o: $(SRC_DIR)/cli_ui.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: platform clean mkdir $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $@ $(LDFLAGS)

.PHONY: platform clean mkdir

platform:
	@echo "Detected PLATFORM: $(PLATFORM)"

mkdir:
	@mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
