all: server client install add_path

server: server.o
	gcc server.o -o server -Wall -g -pthread

server.o: server.c
	gcc -Wall -g -pthread -c server.c

client: client.o
	gcc client.o -o client -Wall -g -pthread

client.o: client.c
	gcc -Wall -g -pthread -c client.c

.PHONY: add_path
add_path:
	@echo "Adding current directory to PATH"
	@export PATH=$$PATH:.

.PHONY: install
install:
	echo '#!/bin/sh' > echos
	echo './server $$@' >> echos
	echo '#!/bin/sh' > echo
	echo './client $$@' >> echo
	chmod +x echos echo

.PHONY: clean
clean:
	rm -f *.o server client echos echo *~ core
