
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -g

TARGET = phonebook

SOURCES = main.c model.c view.c list.c
HEADERS = model.h view.h list.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild

