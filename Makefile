# Use Bash for recipes
SHELL := /usr/bin/bash

# If CC is still Make's built-in default (usually 'cc'), choose a real compiler.
ifeq ($(origin CC), default)
  ifneq ($(shell command -v clang 2>/dev/null),)
    CC := clang
  else ifneq ($(shell command -v gcc 2>/dev/null),)
    CC := gcc
  else
    $(error No C compiler found in PATH. Install one (see notes below) or run: make CC=<path-to-compiler>)
  endif
endif

# Executable suffix on Windows
EXEEXT ?=
ifeq ($(OS),Windows_NT)
  EXEEXT := .exe
endif

# --- Flags (GCC/Clang) ---
CSTD     ?= -std=c11
CPPFLAGS ?= -Iinclude
CFLAGS   ?= $(CSTD) -Wall -Wextra -Wpedantic -g -O0 -DDEBUG -fno-omit-frame-pointer -fno-inline -DNCURSES_ON
LDFLAGS  ?=
LDLIBS   ?= -lncursesw

# --- Paths & target ---
SRC_DIR   := src
BUILD_DIR := build
BIN_NAME  := c
BIN       := $(BIN_NAME)$(EXEEXT)

# Search paths for sources (flat build dir)
VPATH := \
  $(SRC_DIR)/core:\
  $(SRC_DIR)/core/structs:\
  $(SRC_DIR)/core/internal:\
  $(SRC_DIR)/hardware_sim:\
  $(SRC_DIR)/ui:\
  $(SRC_DIR)/utils:\
  $(SRC_DIR)/machine:\
  $(SRC_DIR)/tables

SRCS := \
  $(wildcard $(SRC_DIR)/core/*.c) \
  $(wildcard $(SRC_DIR)/core/internal/*.c) \
  $(wildcard $(SRC_DIR)/core/structs/*.c) \
  $(wildcard $(SRC_DIR)/hardware_sim/*.c) \
  $(wildcard $(SRC_DIR)/machine/*.c) \
  $(wildcard $(SRC_DIR)/ui/*.c) \
  $(wildcard $(SRC_DIR)/utils/*.c) \
  $(wildcard $(SRC_DIR)/tables/*.c)

OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.c=.o)))

RM       := rm -rf
MKDIR_P  := mkdir -p

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR_P) $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	$(RM) $(BUILD_DIR) $(BIN)
