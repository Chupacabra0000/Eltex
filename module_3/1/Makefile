
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic

TARGET = copy
SOURCE = main.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET) *.copy

.PHONY: all clean
