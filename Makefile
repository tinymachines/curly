CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -I./include
LDFLAGS = -lcurl -ljansson -lpthread

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests

# Installation paths
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1
DOCDIR = $(PREFIX)/share/doc/curly

TARGET = $(BIN_DIR)/curly
PARALLEL_TARGET = $(BIN_DIR)/curly_parallel
TEST_TARGET = $(BIN_DIR)/run_tests

# Standard (non-main) source files
CORE_SRC_FILES = $(filter-out $(SRC_DIR)/main%.c, $(wildcard $(SRC_DIR)/*.c))
CORE_OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(CORE_SRC_FILES))

# Main object files
MAIN_OBJ = $(BUILD_DIR)/main.o
PARALLEL_MAIN_OBJ = $(BUILD_DIR)/main_parallel.o

# Test source files
TEST_SRC_FILES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ_FILES = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/test_%.o,$(TEST_SRC_FILES))

.PHONY: all parallel clean test memcheck install uninstall

all: setup $(TARGET) $(PARALLEL_TARGET)

setup:
	mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(CORE_OBJ_FILES) $(MAIN_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(PARALLEL_TARGET): $(CORE_OBJ_FILES) $(PARALLEL_MAIN_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: setup $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET) $(TEST)

$(TEST_TARGET): $(CORE_OBJ_FILES) $(TEST_OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./$(TARGET)

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)
	install -m 755 $(PARALLEL_TARGET) $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(DOCDIR)
	install -m 644 README.md LICENSE $(DESTDIR)$(DOCDIR)
	cp -r examples $(DESTDIR)$(DOCDIR)
	@echo "Installation completed to $(DESTDIR)$(PREFIX)"
	@echo "You can now run: curly, curly_parallel"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/curly
	rm -f $(DESTDIR)$(BINDIR)/curly_parallel
	rm -rf $(DESTDIR)$(DOCDIR)
	@echo "Uninstallation completed"

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
