CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2

BUILD_DIR := build
TARGET := $(BUILD_DIR)/test_multicore_reference
SOURCES := \
	src/multicore_frame.c \
	src/cpu1_reference.c \
	src/cpu2_reference.c \
	tests/test_multicore_reference.c

.PHONY: all test clean

all: test

$(TARGET): $(SOURCES) include/multicore_reference.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude $(SOURCES) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
