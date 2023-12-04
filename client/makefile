# Compiler
CC = gcc

# Source files
SRC_DIR = ../common
SRC = $(wildcard *.c) $(wildcard $(SRC_DIR)/*.c) widgets/*.c

# Output binary name
TARGET = client

# Flags for the compiler and linker
CFLAGS = -Wall -I$(SRC_DIR) -I. -Iwidgets -pthread #-g
LIBS = -lSDL2 -lcairo -lm

# Compile the program
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Clean up
clean:
	rm -f $(TARGET)

# Default target
all: $(TARGET)

