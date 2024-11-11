# Makefile for Proxy Server and Client

CC = gcc
CFLAGS = -Wall -Wextra -pedantic

# Targets
all: proxy client

# Build the proxy server
proxy: server.o
	$(CC) $(CFLAGS) -o proxy server.o

# Build the client
client: client.o
	$(CC) $(CFLAGS) -o client client.o

# Compile the proxy source file
proxy.o: server.c
	$(CC) $(CFLAGS) -c server.c

# Compile the client source file
client.o: client.c
	$(CC) $(CFLAGS) -c client.c

# Clean up generated files
clean:
	rm -f proxy client *.o
