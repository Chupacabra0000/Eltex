
CC = gcc
AR = ar

CFLAGS = -Wall -Wextra -std=c11 -pedantic -g

TARGET = phonebook
LIB = liblist.a
LIB_OBJ = list.o

APP_OBJECTS = main.o model.o view.o

all: $(TARGET)

# Compile list.c into an object file
list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c -o list.o

# Create the static library
$(LIB): $(LIB_OBJ)
	$(AR) rcs $(LIB) $(LIB_OBJ)

# Compile application files
main.o: main.c model.h view.h
	$(CC) $(CFLAGS) -c main.c -o main.o

model.o: model.c model.h list.h
	$(CC) $(CFLAGS) -c model.c -o model.o

view.o: view.c view.h model.h
	$(CC) $(CFLAGS) -c view.c -o view.o

# Link application with the static library
$(TARGET): $(APP_OBJECTS) $(LIB)
	$(CC) $(CFLAGS) $(APP_OBJECTS) -L. -llist -o $(TARGET)

clean:
	rm -f $(TARGET) *.o $(LIB)

rebuild: clean all

.PHONY: all clean rebuild

