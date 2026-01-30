SHELL := /bin/bash

UNAME_S := $(shell uname -s 2>/dev/null)

EXEEXT ?=
ifeq ($(OS),Windows_NT)
  EXEEXT := .exe
endif

# If CC not explicitly set by user, pick a working compiler.
ifeq ($(origin CC), default)
  CC := $(firstword \
        $(shell command -v clang 2>/dev/null) \
        $(shell command -v gcc   2>/dev/null))
  ifeq ($(strip $(CC)),)
    $(error No C compiler found in PATH. Install Xcode CLT (mac) or build-essential (Linux) or set CC=<path-to-compiler>)
  endif
endif

CSTD     ?= -std=c11
CPPFLAGS ?= -Iinclude
CFLAGS   ?= $(CSTD) -Wall -Wextra -Wpedantic -g -O0 -DDEBUG -fno-omit-frame-pointer -fno-inline -DNCURSES_ON
LDFLAGS  ?=
LOG      ?= log.txt

# macOS: -lncurses
# Linux/WSL: -lncursesw
LDLIBS   ?=
ifeq ($(UNAME_S),Darwin)
  LDLIBS += -lncurses
else
  LDLIBS += -lncursesw
endif

SRC_DIR   := src
BUILD_DIR := build
BIN_NAME  := c
BIN       := $(BIN_NAME)$(EXEEXT)

VPATH := \
  $(SRC_DIR):\
  $(SRC_DIR)/core:\
  $(SRC_DIR)/core/structs:\
  $(SRC_DIR)/core/internal:\
  $(SRC_DIR)/core/interrupt:\
  $(SRC_DIR)/hardware_sim:\
  $(SRC_DIR)/ui:\
  $(SRC_DIR)/utils:\
  $(SRC_DIR)/machine:\
  $(SRC_DIR)/tables

SRCS := \
  $(wildcard $(SRC_DIR)/*.c) \
  $(wildcard $(SRC_DIR)/core/*.c) \
  $(wildcard $(SRC_DIR)/core/internal/*.c) \
  $(wildcard $(SRC_DIR)/core/structs/*.c) \
  $(wildcard $(SRC_DIR)/core/interrupt/*.c) \
  $(wildcard $(SRC_DIR)/hardware_sim/*.c) \
  $(wildcard $(SRC_DIR)/machine/*.c) \
  $(wildcard $(SRC_DIR)/ui/*.c) \
  $(wildcard $(SRC_DIR)/utils/*.c) \
  $(wildcard $(SRC_DIR)/tables/*.c)

OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.c=.o)))

RM      := rm -rf
MKDIR_P := mkdir -p

.PHONY: all clean run

all: $(LOG) $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) 2>>$(LOG)

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR_P) $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ 2>>$(LOG)

$(LOG):
	@ : >$(LOG)

run: $(BIN)
	./$(BIN)

clean:
	$(RM) $(BUILD_DIR) $(BIN)
