
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = chat
SOURCE = chat.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) -pthread -lrt

clean:
	rm -f $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild
