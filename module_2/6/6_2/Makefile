
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic

TARGET = phonebook
LIB = liblist.dylib
OBJECT = list.o

SOURCES = main.c model.c view.c

all: $(TARGET)

# Compile linked-list source
$(OBJECT): list.c list.h
	$(CC) $(CFLAGS) -fPIC -c list.c -o $(OBJECT)

# Create dynamic library
$(LIB): $(OBJECT)
	$(CC) -dynamiclib -install_name @rpath/$(LIB) -o $(LIB) $(OBJECT)

# Compile and link application
$(TARGET): $(SOURCES) model.h view.h list.h $(LIB)
	$(CC) $(CFLAGS) $(SOURCES) -L. -llist \
	-Wl,-rpath,@executable_path -o $(TARGET)

# Run application
run: $(TARGET)
	./$(TARGET)

# Remove generated files
clean:
	rm -f $(OBJECT) $(LIB) $(TARGET)

.PHONY: all run clean

