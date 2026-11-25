
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

TARGETS = producer consumer

all: $(TARGETS)

producer: producer.c common.h
	$(CC) $(CFLAGS) producer.c -o producer

consumer: consumer.c common.h
	$(CC) $(CFLAGS) consumer.c -o consumer

clean:
	rm -f $(TARGETS)

rebuild: clean all

.PHONY: all clean rebuild
