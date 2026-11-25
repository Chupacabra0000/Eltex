
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -pthread

TARGETS = producer consumer

all: $(TARGETS)

producer: producer.c common.h
	$(CC) $(CFLAGS) producer.c -o producer $(LDFLAGS)

consumer: consumer.c common.h
	$(CC) $(CFLAGS) consumer.c -o consumer $(LDFLAGS)

clean:
	rm -f $(TARGETS)

rebuild: clean all

.PHONY: all clean rebuild
