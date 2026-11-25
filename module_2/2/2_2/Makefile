
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -I.

TARGET = calculator

SRC = main.c \
      calculator.c \
      parser.c \
      operators.c \
      view.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

rebuild: clean all

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean rebuild run

