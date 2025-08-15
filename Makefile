CC       ?= gcc
CSTD     ?= -std=c11
CPPFLAGS ?= -Iinclude
CFLAGS   ?= $(CSTD) -Wall -Wextra -Wpedantic -g -O0 -DDEBUG -fno-omit-frame-pointer -fno-inline
LDFLAGS  ?=
LDLIBS   ?=

SRC_DIR   := src
BUILD_DIR := build
BIN       := c

VPATH := \
  $(SRC_DIR)/core:\
  $(SRC_DIR)/core/internal:\
  $(SRC_DIR)/hardware_sim:\
  $(SRC_DIR)/machine:\
  $(SRC_DIR)/tables

SRCS := \
  $(wildcard $(SRC_DIR)/core/*.c) \
  $(wildcard $(SRC_DIR)/core/internal/*.c) \
  $(wildcard $(SRC_DIR)/hardware_sim/*.c) \
  $(wildcard $(SRC_DIR)/machine/*.c) \
  $(wildcard $(SRC_DIR)/tables/*.c)

OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.c=.o)))

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN)
