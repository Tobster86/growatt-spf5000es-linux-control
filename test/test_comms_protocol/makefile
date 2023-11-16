CC = gcc
CFLAGS = -I../../common -I../

COMMON_DIR = ../../common

SOURCES = $(wildcard *.c) $(wildcard $(COMMON_DIR)/*.c) $(wildcard ../*.c)
HEADERS = $(wildcard $(COMMON_DIR)/*.h) $(wildcard ../*.h)
OBJECTS = $(SOURCES:.c=.o)

EXECUTABLE = test

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

