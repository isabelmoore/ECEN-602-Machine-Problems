
client.o: client.c
	gcc -c client.c

server.o: server.c
	gcc -c server.c

client: client.o
	gcc client.o -o client

server: server.o
	gcc server.o -o server

all: client server

.PHONY: clean
clean:
	rm -f *.o
