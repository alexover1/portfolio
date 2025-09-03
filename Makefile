CC ?= gcc
CFLAGS ?= -Wno-macro-redefined

SRC := $(wildcard src/*.c)
HEADERS := $(wildcard src/*.h)

BUILD := build
TARGET := $(BUILD)/main

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SRC)

$(BUILD):
	mkdir -p $(BUILD)

run: $(TARGET)
	./$(TARGET)

.PHONY: all run clean

clean:
	rm -rf $(BUILD)
