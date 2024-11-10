CC = gcc
CFLAGS = -Wall -g -pthread

PROXY = proxy
CLIENT = client

all: $(PROXY) $(CLIENT)

$(PROXY): proxy.c
	$(CC) $(CFLAGS) -o $(PROXY) proxy.c

$(CLIENT): client.c
	$(CC) $(CFLAGS) -o $(CLIENT) client.c

clean:
	rm -f $(PROXY) $(CLIENT)
