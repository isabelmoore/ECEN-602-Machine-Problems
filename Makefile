client.o: client.c
	gcc -c client.c

client: client.o
	gcc client.o -o client

all: client