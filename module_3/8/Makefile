
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = udp_sniffer
SOURCE = main.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET) report.txt

run: $(TARGET)
	sudo ./$(TARGET)

.PHONY: all clean run
