
CC = gcc

CFLAGS = -std=c11 -Wall -Wextra -pedantic

INCLUDES = -Imodel -Iview -Ilist

LIB_DIR = list
LIB_NAME = liblist.dylib
LIB = $(LIB_DIR)/$(LIB_NAME)

OBJECT = $(LIB_DIR)/list.o
TARGET = phonebook

APP_SOURCES = main.c model/model.c view/view.c

all: $(TARGET)

# Compile linked-list source into position-independent object code
$(OBJECT): list/list.c list/list.h
	$(CC) $(CFLAGS) -fPIC -c list/list.c -o $(OBJECT)

# Build dynamic library
$(LIB): $(OBJECT)
	$(CC) -dynamiclib -install_name @rpath/$(LIB_NAME) -o $(LIB) $(OBJECT)

# Build application and link dynamic library
$(TARGET): $(APP_SOURCES) model/model.h view/view.h list/list.h $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $(APP_SOURCES) \
	-L$(LIB_DIR) -llist \
	-Wl,-rpath,@executable_path/$(LIB_DIR) \
	-o $(TARGET)

# Run application
run: $(TARGET)
	./$(TARGET)

# Remove generated files
clean:
	rm -f $(OBJECT) $(LIB) $(TARGET)

.PHONY: all run clean

