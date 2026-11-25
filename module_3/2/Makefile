
CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L

TARGET = publisher_broker_subscriber

SOURCES = main.c broker.c publisher.c subscriber.c

OBJECTS = $(SOURCES:.c=.o)


all: $(TARGET)


$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJECTS) $(TARGET)


.PHONY: all clean
