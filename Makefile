TARGET = crow
LIBS = -ljson-c -lcurl -lssl -lcrypt -lpthread
CC = gcc
CFLAGS = -g -Wall -I/usr/include/json-c

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard src/*.c))
HEADERS = $(wildcard include/*.h)

%.o: %.c $(HEADERS) $(CC) 
	$(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall deps/libwsclient/libwsclient.a deps/librequests/build/librequests.a $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)