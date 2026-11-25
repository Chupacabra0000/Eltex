
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

TARGET = priority_queue

SOURCES = main.c model.c view.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

main.o: main.c model.h view.h
	$(CC) $(CFLAGS) -c main.c

model.o: model.c model.h
	$(CC) $(CFLAGS) -c model.c

view.o: view.c view.h model.h
	$(CC) $(CFLAGS) -c view.c

clean:
	rm -f $(OBJECTS) $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild

