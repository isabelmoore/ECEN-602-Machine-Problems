#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define CACHE_SIZE 10

typedef struct {
    char url[256];
    char data[BUFFER_SIZE];
    time_t timestamp;
    time_t expires;
} CacheEntry;

CacheEntry cache[CACHE_SIZE];
int cache_count = 0;

void init_cache() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache[i].url[0] = '\0';
        cache[i].data[0] = '\0';
        cache[i].timestamp = 0;
        cache[i].expires = 0;
    }
}

int find_in_cache(const char *url) {
    for (int i = 0; i < cache_count; i++) {
        if (strcmp(cache[i].url, url) == 0) {
            return i;
        }
    }
    return -1;
}

void add_to_cache(const char *url, const char *data, time_t expires) {
    int index = cache_count < CACHE_SIZE ? cache_count++ : 0;
    strncpy(cache[index].url, url, sizeof(cache[index].url) - 1);
    strncpy(cache[index].data, data, sizeof(cache[index].data) - 1);
    cache[index].timestamp = time(NULL);
    cache[index].expires = expires;
}

int create_server_socket(const char *ip, int port) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        // Parse the HTTP request
        char method[10], url[256], version[10];
        sscanf(buffer, "%s %s %s", method, url, version);

        if (strcmp(method, "GET") == 0) {
            int cache_index = find_in_cache(url);
            if (cache_index != -1 && time(NULL) < cache[cache_index].expires) {
                // Cache hit
                printf("Cache hit for %s\n", url);
                send(client_socket, cache[cache_index].data, strlen(cache[cache_index].data), 0);
            } else {
                // Cache miss or expired, fetch from server
                printf("Cache miss for %s\n", url);
                // TODO: Implement fetching from server and caching
            }
        }
    }
    close(client_socket);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int server_fd = create_server_socket(ip, port);
    init_cache();

    printf("Proxy server running on %s:%d\n", ip, port);

    fd_set readfds;
    int max_sd, activity, new_socket;
    int client_sockets[MAX_CLIENTS] = {0};

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, NULL, NULL)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("New connection, socket fd is %d\n", new_socket);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                handle_client(sd);
                client_sockets[i] = 0;
            }
        }
    }

    return 0;
}
