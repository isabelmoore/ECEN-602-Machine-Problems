#include "proxy.h"
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <netdb.h>  

void list_blocked_sites();
void handle_http_request(int client_socket, const char *url);
void fetch_from_server(int client_socket, const char *url, const char *hostname, const char *path);
void parse_url(const char *url, char *hostname, char *path);

// global variables
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t block_mutex = PTHREAD_MUTEX_INITIALIZER;
CachedSite *cachedSites = NULL;
BlockedSite *blockedSites = NULL;
CachedSite *cache_head = NULL;
int serverRunning = 1;
int server_socket;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    load_cache();
    load_blocked_sites();

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CACHE_SIZE) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Proxy server listening on IP %s and port %d\n", ip, port);

    pthread_t console_thread;
    if (pthread_create(&console_thread, NULL, console_listener, NULL) != 0) {
        perror("Failed to create console listener thread");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    fd_set read_fds;
    int max_fd = server_socket;

    while (serverRunning) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("Select error");
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
            
            if (!serverRunning) break;

            if (client_socket < 0) {
                perror("Failed to accept connection");
                continue;
            }

            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_socket) != 0) {
                perror("Failed to create client thread");
                close(client_socket);
                continue;
            }

            pthread_detach(thread_id);
        }
    }

    pthread_join(console_thread, NULL);
    close_server(server_socket);
    return 0;
}

// client request handler
void *handle_client(void *client_socket_ptr) {
    int client_socket = *(int *)client_socket_ptr;
    char buffer[BUFFER_SIZE];

    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read < 0) {
        perror("Failed to read from client");
        close(client_socket);
        pthread_exit(NULL);
    }
    buffer[bytes_read] = '\0';

    char method[BUFFER_SIZE], url[BUFFER_SIZE];
    sscanf(buffer, "%s %s", method, url);

    if (strcmp(method, "GET") == 0) {
        pthread_mutex_lock(&block_mutex);
        int blocked = is_blocked(url);
        pthread_mutex_unlock(&block_mutex);

        if (blocked) {
            send_blocked_message(client_socket);
        } else {
            pthread_mutex_lock(&cache_mutex);
            CachedSite *cached_site = check_cache(url);
            if (cached_site && is_fresh(cached_site)) {
                send_cached_page(client_socket, cached_site->filename);
            } else {
                handle_http_request(client_socket, url);
            }
            pthread_mutex_unlock(&cache_mutex);
        }
    }

    close(client_socket);
    pthread_exit(NULL);
}

// handles http request
void handle_http_request(int client_socket, const char *url) {
    char hostname[BUFFER_SIZE], path[BUFFER_SIZE];
    parse_url(url, hostname, path);
    fetch_from_server(client_socket, url, hostname, path);
}

// fetches content from server
void fetch_from_server(int client_socket, const char *url, const char *hostname, const char *path) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);

    struct hostent *host = gethostbyname(hostname);
    if (!host) {
        perror("Failed to resolve hostname");
        close(server_socket);
        return;
    }

    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to web server");
        close(server_socket);
        return;
    }

    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, hostname);
    if (send(server_socket, request, strlen(request), 0) < 0) {
        perror("Failed to send request to web server");
        close(server_socket);
        return;
    }

    char response[BUFFER_SIZE];
    ssize_t bytes_received;
    FILE *cache_file = NULL;
    char cache_filename[BUFFER_SIZE];

    snprintf(cache_filename, sizeof(cache_filename), "%s/%ld.html", CACHE_DIR, time(NULL));
    cache_file = fopen(cache_filename, "w");

    while ((bytes_received = recv(server_socket, response, BUFFER_SIZE - 1, 0)) > 0) {
        response[bytes_received] = '\0';
        send(client_socket, response, bytes_received, 0);
        if (cache_file) {
            fwrite(response, 1, bytes_received, cache_file);
        }
    }

    if (cache_file) {
        fclose(cache_file);
        add_cache_entry(url, cache_filename);
    }

    close(server_socket);
}

// parses url into hostname and path
void parse_url(const char *url, char *hostname, char *path) {
    if (sscanf(url, "http://%99[^/]/%199[^\n]", hostname, path) == 1) {
        // If no path is specified, use "/"
        strcpy(path, "/");
    }
}

// console listener for server commands
void *console_listener(void *arg) {
    char command[100];
    while (1) {
        if (fgets(command, sizeof(command), stdin) != NULL) {
            command[strcspn(command, "\n")] = 0;
            if (strcmp(command, "blocked") == 0) {
                list_blocked_sites();
            } else if (strcmp(command, "cached") == 0) {
                list_cached_sites();
            } else if (strcmp(command, "close") == 0) {
                printf("Shutting down server...\n");
                serverRunning = 0;
                break;
            } else {
                printf("Unknown command: %s\n", command);
            }
        }
    }
    return NULL;
}

// checks if a site is blocked
int is_blocked(const char *url) {
    BlockedSite *curr = blockedSites;
    while (curr) {
        if (strcmp(curr->url, url) == 0) {
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

// sends a "403 forbidden" response to blocked sites
void send_blocked_message(int clientSocket) {
    const char *response = "HTTP/1.0 403 Forbidden\r\n\r\nBlocked by Proxy Server";
    send(clientSocket, response, strlen(response), 0);
}

// checks if a url is cached
CachedSite* check_cache(const char *url) {
    CachedSite *curr = cache_head;
    while (curr) {
        if (strcmp(curr->url, url) == 0) {
            curr->last_accessed = time(NULL);  // update access time for LRU
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// sends cached page to client
void send_cached_page(int client_socket, const char *filePath) {
    int file = open(filePath, O_RDONLY);
    if (file < 0) {
        perror("Failed to open cached file");
        return;
    }

    const char *response = "HTTP/1.0 200 OK\r\n\r\n";
    send(client_socket, response, strlen(response), 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(file, buffer, sizeof(buffer))) > 0) {
        send(client_socket, buffer, bytesRead, 0);
    }

    close(file);
}

// sends a non-cached page to client and caches it
void send_non_cached_page(int client_socket, const char *url) {
    const char *content = "<html><body><h1>Non-cached content</h1></body></html>";
    const char *response = "HTTP/1.0 200 OK\r\n\r\n";
    send(client_socket, response, strlen(response), 0);
    send(client_socket, content, strlen(content), 0);

    // cache content
    char filePath[BUFFER_SIZE];
    snprintf(filePath, sizeof(filePath), "%s/%ld.html", CACHE_DIR, time(NULL));
    int file = open(filePath, O_WRONLY | O_CREAT, 0644);
    if (file >= 0) {
        write(file, content, strlen(content));
        close(file);
    }
}

// lists all cached sites
void list_cached_sites() {
    CachedSite *curr = cachedSites;
    printf("Cached Sites:\n");
    while (curr) {
        printf("URL: %s, File: %s\n", curr->url, curr->filename);
        curr = curr->next;
    }
}

// lists all blocked sites
void list_blocked_sites() {
    BlockedSite *curr = blockedSites;
    printf("Blocked Sites:\n");
    while (curr) {
        printf("URL: %s\n", curr->url);
        curr = curr->next;
    }
}

// loads blocked sites from file
void load_blocked_sites() {
    FILE *file = fopen(BLOCKED_FILE, "r");
    if (!file) return;
    char url[BUFFER_SIZE];
    while (fscanf(file, "%s", url) != EOF) {
        add_blocked_site(url);
    }
    fclose(file);
}

// loads cached sites from file
void load_cache() {
    FILE *file = fopen(CACHE_FILE, "r");
    if (!file) return;
    char url[BUFFER_SIZE], filename[BUFFER_SIZE];
    while (fscanf(file, "%s %s", url, filename) != EOF) {
        add_cache_entry(url, filename);
    }
    fclose(file);
}

// removes the oldest cache entry
void remove_oldest_cache_entry() {
    CachedSite *curr = cache_head, *prev = NULL, *oldest = cache_head, *oldest_prev = NULL;
    time_t oldest_time = cache_head->last_accessed;

    while (curr) {
        if (curr->last_accessed < oldest_time) {
            oldest = curr;
            oldest_prev = prev;
            oldest_time = curr->last_accessed;
        }
        prev = curr;
        curr = curr->next;
    }

    if (oldest_prev) {
        oldest_prev->next = oldest->next;
    } else {
        cache_head = oldest->next;
    }

    remove(oldest->filename); // deketed cached file
    free(oldest);
}

// checks if cached entry is fresh
int is_fresh(const CachedSite *cached_site) {
    return (time(NULL) - cached_site->last_accessed) < CACHE_EXPIRATION_SECONDS;
}

// saves blocked sites to file
void save_blocked_sites() {
    FILE *file = fopen(BLOCKED_FILE, "w");
    if (!file) {
        perror("Failed to open blocked sites file for saving");
        return;
    }

    BlockedSite *curr = blockedSites;
    while (curr) {
        fprintf(file, "%s\n", curr->url);
        curr = curr->next;
    }

    fclose(file);
    printf("Blocked sites saved to %s\n", BLOCKED_FILE);
}

// adds a site to the blocked list
void add_blocked_site(const char *url) {
    BlockedSite *new_site = malloc(sizeof(BlockedSite));
    strcpy(new_site->url, url);
    new_site->next = blockedSites;
    blockedSites = new_site;
}

// adds a new entry to the cache
void add_cache_entry(const char *url, const char *data) {
    CachedSite *new_site = (CachedSite*) malloc(sizeof(CachedSite));
    snprintf(new_site->filename, BUFFER_SIZE, "cached_%ld.html", time(NULL));
    strcpy(new_site->url, url);
    new_site->last_accessed = time(NULL);
    new_site->next = cache_head;
    cache_head = new_site;

    if (MAX_CACHE_SIZE > 0) {
        remove_oldest_cache_entry();
    }
}

// saves the cache to file
void save_cache() {
    FILE *file = fopen(CACHE_FILE, "w");
    if (!file) return;
    CachedSite *curr = cachedSites;
    while (curr) {
        fprintf(file, "%s %s\n", curr->url, curr->filename);
        curr = curr->next;
    }
    fclose(file);
}

// closes the server
void close_server(int server_socket) {
    save_cache();
    save_blocked_sites();
    close(server_socket);
    printf("Server closed.\n");
}
