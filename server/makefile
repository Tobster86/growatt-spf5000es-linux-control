# Compiler
CC = gcc

# Source files
SRC = $(wildcard *.c)

# Output binary name
TARGET = server

# Directories for libmodbus headers and libraries
MODBUS_INCLUDE = /usr/include/modbus/
MODBUS_LIB = /usr/lib/arm-linux-gnueabihf

# Flags for the compiler and linker
CFLAGS = -Wall -I$(MODBUS_INCLUDE) -I. -pthread
LDFLAGS = -L$(MODBUS_LIB) -lmodbus -pthread

# Compile the program
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Clean up
clean:
	rm -f $(TARGET)

# Default target
all: $(TARGET)
