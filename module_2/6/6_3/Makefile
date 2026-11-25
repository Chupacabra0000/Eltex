
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDLIBS = -ldl -lm

TARGET = calculator

SOURCES = main.c calculator.c parser.c operators.c view.c
OBJECTS = $(SOURCES:.c=.o)

PLUGIN_DIR = plugins

PLUGIN_NAMES = add subtract multiply divide

PLUGIN_LIBS = $(addprefix $(PLUGIN_DIR)/,$(addsuffix .so,$(PLUGIN_NAMES)))

.PHONY: all clean plugins

all: $(TARGET) plugins

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

plugins: $(PLUGIN_LIBS)

$(PLUGIN_DIR)/%.so: $(PLUGIN_DIR)/%.c
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) $(PLUGIN_LIBS)
