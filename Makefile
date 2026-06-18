CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGET = build/main
SRC = src/main.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)