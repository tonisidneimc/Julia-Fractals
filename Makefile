CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 `sdl2-config --cflags`
LDLIBS = `sdl2-config --libs` -lm

SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = fractal

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

