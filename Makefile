CC := gcc

OPENSSL_DIR := $(shell brew --prefix openssl@3)
CURL_DIR := $(shell brew --prefix curl)

SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build

CFLAGS := -Wall -Wextra -Werror -std=c11 -g -MMD -MP
CFLAGS += -I$(SRC_DIR)
CFLAGS += -I$(OPENSSL_DIR)/include
CFLAGS += -I$(CURL_DIR)/include

LDFLAGS := -L$(OPENSSL_DIR)/lib
LDFLAGS += -L$(CURL_DIR)/lib

LDLIBS := -lcrypto
LDLIBS += -lcurl

TARGET := $(BUILD_DIR)/main

SRC := $(shell find $(SRC_DIR) -name '*.c')
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

TEST_SRC := $(wildcard $(TEST_DIR)/test_*.c)
TEST_TARGETS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SRC))


# ----------------
# Application
# ----------------

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(OBJ:.o=.d)


# ----------------
# Tests
# ----------------

$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.c $(filter-out $(BUILD_DIR)/main.o,$(OBJ))
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< \
		$(filter-out $(BUILD_DIR)/main.o,$(OBJ)) \
		-o $@ $(LDFLAGS) $(LDLIBS)

tests: $(TEST_TARGETS)
	@for test in $(TEST_TARGETS); do \
		echo "Running $$test"; \
		./$$test || exit 1; \
	done


# ----------------
# Run
# ----------------

run: all
	./$(TARGET) $(ARGS)


# ----------------
# Clean
# ----------------

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all tests run clean