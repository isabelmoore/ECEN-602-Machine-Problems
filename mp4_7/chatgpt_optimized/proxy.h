#ifndef PROXY_H
#define PROXY_H

#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 4096
#define CACHE_FILE "cached_sites.txt"
#define BLOCKED_FILE "blocked_sites.txt"
#define CACHE_DIR "cached"
#define MAX_CACHE_SIZE 10
#define CACHE_EXPIRATION_SECONDS 86400 // 24 hours

// data structures for caching and blocking
typedef struct CachedSite {
    char url[BUFFER_SIZE];
    char filename[BUFFER_SIZE];
    time_t last_accessed;
    struct CachedSite *next;
} CachedSite;

typedef struct BlockedSite {
    char url[BUFFER_SIZE];
    struct BlockedSite *next;
} BlockedSite;

// externally defined variables
extern pthread_mutex_t cache_mutex;
extern pthread_mutex_t block_mutex;
extern CachedSite *cachedSites;
extern BlockedSite *blockedSites;
extern CachedSite *cache_head;

// server-related function declarations
void *handle_client(void *client_socket);
void *console_listener(void *arg);
void load_blocked_sites();
void load_cache();
void close_server(int server_socket);
void list_cached_sites();
void add_blocked_site(const char *url);
void save_cache();
void save_blocked_sites();

// cache management functions
CachedSite* check_cache(const char *url);
void add_cache_entry(const char *url, const char *data);
void remove_oldest_cache_entry();
int is_fresh(const CachedSite *cached_site);

// blocking and messaging
int is_blocked(const char *url);
void send_blocked_message(int clientSocket);
void send_cached_page(int clientSocket, const char *filePath);
void send_non_cached_page(int clientSocket, const char *url);

#endif // PROXY_H
