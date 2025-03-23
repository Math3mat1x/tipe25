CC = gcc
CFLAGS = -I include -Wall -ggdb
LDFLAGS = -lm

SOURCES = $(wildcard src/*.c) $(wildcard lib/*.c)
OBJECTS = $(SOURCES:.c=.o)

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
