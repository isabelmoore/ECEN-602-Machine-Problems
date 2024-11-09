CFLAGS = -Wall -g -lpthread
LDFLAGS = -lresolv
PORT = 6969
CACHE_DIR = cache

all: Proxy RequestHandler

Proxy: Proxy.c
	$(CC) $(CFLAGS) -o Proxy Proxy.c $(LDFLAGS)

RequestHandler: RequestHandler.c
	$(CC) $(CFLAGS) -o RequestHandler RequestHandler.c $(LDFLAGS)

clean:
	rm -f Proxy RequestHandler
