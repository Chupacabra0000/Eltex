
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

SERVER = server
CLIENT = client

all: $(SERVER) $(CLIENT)

$(SERVER): server.c common.h
	$(CC) $(CFLAGS) server.c -o $(SERVER)

$(CLIENT): client.c common.h
	$(CC) $(CFLAGS) client.c -o $(CLIENT)

clean:
	rm -f $(SERVER) $(CLIENT)

rebuild: clean all

.PHONY: all clean rebuild
