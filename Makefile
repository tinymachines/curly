CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -I./include
LDFLAGS = -lcurl -ljansson

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests

TARGET = $(BIN_DIR)/curly
TEST_TARGET = $(BIN_DIR)/run_tests

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

TEST_SRC_FILES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ_FILES = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/test_%.o,$(TEST_SRC_FILES))

.PHONY: all clean test memcheck

all: setup $(TARGET)

setup:
	mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: setup $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET) $(TEST)

$(TEST_TARGET): $(filter-out $(BUILD_DIR)/main.o, $(OBJ_FILES)) $(TEST_OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
