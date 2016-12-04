TARGET = discord
LIBS = -lm -lcurl -ljson-c -lwsclient -lpthread
CC = gcc
CFLAGS = -g -O2
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
	$(CC) $(OBJECTS)  $(shell pkg-config --cflags json-c) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)