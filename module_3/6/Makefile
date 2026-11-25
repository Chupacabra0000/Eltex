
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = udp_chat
SOURCE = main.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) -pthread

clean:
	rm -f $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild
