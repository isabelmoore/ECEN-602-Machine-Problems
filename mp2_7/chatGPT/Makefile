CC = gcc
CFLAGS = -Wall -g

SERVER = server
CLIENT = client

SERVER_SRC = echo_server.c
CLIENT_SRC = echo_client.c

.PHONY: all clean echos echo

# Build server and client
all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

# Clean built files
clean:
	rm -f $(SERVER) $(CLIENT)

# Run server with command-line arguments
echos:
	./$(SERVER) $(PORT)

# Run client with command-line arguments
echo:
	./$(CLIENT) $(SERVER_ADDR) $(PORT)
