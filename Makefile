TARGET = discord
LIBS = -lm -lcurl -ljson-c
CC = gcc
CFLAGS = -g -Wall
CFLAGS += $(shell pkg-config --cflags json-c)
LDFLAGS += $(shell pkg-config --libs json-c)

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(shell pkg-config --cflags json-c) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)