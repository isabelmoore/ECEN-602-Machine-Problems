# Makefile for TCP echo server and client

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Target executable names
SERVER = echo_server
CLIENT = echo_client

# Source files
SERVER_SRC = echo_server.c
CLIENT_SRC = echo_client.c

# Default target
all: $(SERVER) $(CLIENT)

# Build server executable
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)

# Build client executable
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

# Clean up build artifacts
clean:
	rm -f $(SERVER) $(CLIENT)

# Phony targets
.PHONY: all clean

