# Compiler
CC = gcc

# Source files
SRC = $(wildcard *.c)

# Output binary name
TARGET = client

# Flags for the compiler and linker
CFLAGS = -Wall -I. -pthread -lSDL2  # Update the flags for SDL
LIBS = -lSDL2 -lcairo  # Update the libraries

# Compile the program
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Clean up
clean:
	rm -f $(TARGET)

# Default target
all: $(TARGET)

