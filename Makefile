CC := gcc

OPENSSL_DIR := $(shell brew --prefix openssl@3)

CFLAGS := -Wall -Wextra -Werror -std=c11 -g -MMD -MP
CFLAGS += -I$(OPENSSL_DIR)/include

LDFLAGS := -L$(OPENSSL_DIR)/lib
LDLIBS := -lcrypto

SRC_DIR := src
BUILD_DIR := build

TARGET := $(BUILD_DIR)/main

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(TARGET) $(ARGS)

.PHONY: all clean run