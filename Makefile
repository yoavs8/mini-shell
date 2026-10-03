CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g

TARGET = myshell
SRC = main.c parser.c executor.c
HEADERS = parser.h executor.h

$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

.PHONY: clean

clean:
	rm -f $(TARGET)