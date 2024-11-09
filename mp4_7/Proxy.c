#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

#define PORT 6969
#define BUFFER_SIZE 1024
#define CACHE_FILE "cached_sites.txt"
#define BLOCKED_FILE "block_sites.txt"
#define MAX_THREADS 10

typedef struct CachedSite {
    char url[BUFFER_SIZE];
    char filename[BUFFER_SIZE];
    struct CachedSite *next;
} CachedSite;

typedef struct BlockedSite {
    char url[BUFFER_SIZE];
    struct BlockedSite *next;
} BlockedSite;

CachedSite *cachedSites = NULL;
BlockedSite *blockedSites = NULL;
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t block_mutex = PTHREAD_MUTEX_INITIALIZER;
int serverRunning = 1;

// Function prototypes
void *handle_client(void *client_socket);
void load_cache();
void load_blocked_sites();
void save_cache();
void save_blocked_sites();
int is_blocked(const char *url);
void add_blocked_site(const char *url);
void add_cached_site(const char *url, const char *filename);
void list_cached_sites();
void list_blocked_sites();
void close_server(int server_socket);

extern int serverRunning; // this should control the main server loop

void *console_listener(void *arg) {
    char command[100];
    while (1) {
        printf("\nEnter command (blocked, cached, close): ");
        if (fgets(command, sizeof(command), stdin) != NULL) {
            command[strcspn(command, "\n")] = 0;
            printf("Received command: %s\n", command);  
            if (strcmp(command, "blocked") == 0) {
                printf("Executing blocked sites listing...\n");
                list_blocked_sites();
            } else if (strcmp(command, "cached") == 0) {
                printf("Executing cached sites listing...\n");
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


int main() {
    load_cache();
    load_blocked_sites();

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // create and start the console listener thread
    pthread_t console_thread;
    if (pthread_create(&console_thread, NULL, console_listener, NULL) != 0) {
        perror("Failed to create console listener thread");
        exit(EXIT_FAILURE);
    }

    // creat server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_THREADS) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Proxy server listening on port %d\n", PORT);

    while (serverRunning) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Failed to accept connection");
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_socket) != 0) {
            perror("Failed to create thread");
        }
    }

    close_server(server_socket);
    pthread_join(console_thread, NULL); // wait for the console listener to finish
    return 0;
}


void *handle_client(void *client_socket) {
    int socket = *(int *)client_socket;
    char buffer[BUFFER_SIZE];

    // read request from client
    int bytes_read = recv(socket, buffer, sizeof(buffer), 0);
    if (bytes_read < 0) {
        perror("Failed to read from client");
        close(socket);
        pthread_exit(NULL);
    }
    buffer[bytes_read] = '\0';

    printf("Request received: %s\n", buffer);

    // request parsing
    char method[BUFFER_SIZE], url[BUFFER_SIZE];
    sscanf(buffer, "%s %s", method, url);

    if (strcmp(method, "GET") == 0) {
        pthread_mutex_lock(&block_mutex);
        int blocked = is_blocked(url);
        pthread_mutex_unlock(&block_mutex);

        if (blocked) {
            const char *response = "HTTP/1.0 403 Forbidden\r\n\r\nBlocked by Proxy Server";
            send(socket, response, strlen(response), 0);
        } else {
            // check cache for URL
            pthread_mutex_lock(&cache_mutex);
            CachedSite *curr = cachedSites;
            int found = 0;
            while (curr) {
                if (strcmp(curr->url, url) == 0) {
                    FILE *cached_file = fopen(curr->filename, "r");
                    if (cached_file) {
                        char line[BUFFER_SIZE];
                        send(socket, "HTTP/1.0 200 OK\r\n\r\n", 17, 0);
                        while (fgets(line, sizeof(line), cached_file)) {
                            send(socket, line, strlen(line), 0);
                        }
                        fclose(cached_file);
                        found = 1;
                    }
                    break;
                }
                curr = curr->next;
            }
            pthread_mutex_unlock(&cache_mutex);

            if (!found) {
                // simulate fetching from remote and caching
                const char *content = "<html><body><h1>Fetched Content</h1></body></html>";
                const char *response_header = "HTTP/1.0 200 OK\r\n\r\n";
                send(socket, response_header, strlen(response_header), 0);
                send(socket, content, strlen(content), 0);

                // cache the content
                char filename[BUFFER_SIZE];
                snprintf(filename, sizeof(filename), "cache_%ld.html", time(NULL));
                FILE *cache_file = fopen(filename, "w");
                if (cache_file) {
                    fputs(content, cache_file);
                    fclose(cache_file);
                    pthread_mutex_lock(&cache_mutex);
                    add_cached_site(url, filename);
                    pthread_mutex_unlock(&cache_mutex);
                }
            }
        }
    }

    close(socket);
    pthread_exit(NULL);
}

void load_cache() {
    FILE *file = fopen(CACHE_FILE, "r");
    if (!file) return;
    char url[BUFFER_SIZE], filename[BUFFER_SIZE];
    while (fscanf(file, "%s %s", url, filename) != EOF) {
        add_cached_site(url, filename);
    }
    fclose(file);
}

void load_blocked_sites() {
    FILE *file = fopen(BLOCKED_FILE, "r");
    if (!file) return;
    char url[BUFFER_SIZE];
    while (fscanf(file, "%s", url) != EOF) {
        add_blocked_site(url);
    }
    fclose(file);
}

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

void save_blocked_sites() {
    FILE *file = fopen(BLOCKED_FILE, "w");
    if (!file) return;
    BlockedSite *curr = blockedSites;
    while (curr) {
        fprintf(file, "%s\n", curr->url);
        curr = curr->next;
    }
    fclose(file);
}

void close_server(int server_socket) {
    save_cache();
    save_blocked_sites();
    close(server_socket);
    printf("Server closed.\n");
}

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

void add_blocked_site(const char *url) {
    BlockedSite *new_site = malloc(sizeof(BlockedSite));
    strcpy(new_site->url, url);
    new_site->next = blockedSites;
    blockedSites = new_site;
}

void add_cached_site(const char *url, const char *filename) {
    CachedSite *new_site = malloc(sizeof(CachedSite));
    strcpy(new_site->url, url);
    strcpy(new_site->filename, filename);
    new_site->next = cachedSites;
    cachedSites = new_site;
}

void list_cached_sites() {
    // printf("Listing cached sites...\n"); /
    CachedSite *curr = cachedSites;
    if (!curr) {
        printf("No cached sites available.\n");
    }
    while (curr) {
        printf("URL: %s, File: %s\n", curr->url, curr->filename);
        curr = curr->next;
    }
}

void list_blocked_sites() {
    // printf("Listing blocked sites...\n"); 
    BlockedSite *curr = blockedSites;
    if (!curr) {
        printf("No blocked sites available.\n");
    }
    while (curr) {
        printf("URL: %s\n", curr->url);
        curr = curr->next;
    }
}
