CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g

TARGET = myshell
SRC = main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

.PHONY: clean

clean:
	rm -f $(TARGET)