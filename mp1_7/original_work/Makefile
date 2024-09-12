CC = gcc
CFLAGS = -Wall -g

SERVER = echos
CLIENT = echo

SERVER_SRC = server.c
CLIENT_SRC = client.c

# Build server and client
all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

# Clean built files
clean:
	rm -f $(SERVER) $(CLIENT)

# Run server
run_server:
	./$(SERVER) $(TARGET)

# Run client
run_client:
	./$(CLIENT) 127.0.0.1 $(TARGET)

