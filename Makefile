CC = gcc
CFLAGS = -Wall -Wextra -Isrc -std=c11

TARGET = build/main
SRC := $(shell find src -name '*.c')

all:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)