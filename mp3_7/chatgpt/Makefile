# Makefile for the TFTP server program

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Target binary
TARGET = server

# Source files
SRC = server.c

# Object files
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Build the target binary
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Rule for compiling C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Install rule (optional, if you want to install to /usr/local/bin or similar)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Phony targets
.PHONY: all clean install

