#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>  // Updated to use getaddrinfo

#define BUFFER_SIZE 4096
#define CACHE_DIR "cached"

// Structure to hold information about the request handler
typedef struct {
    int clientSocket;
} RequestHandler;

// Function prototypes
void *handle_client(void *handler);
void handle_HTTP_request(int clientSocket, const char *requestString);
void handle_HTTPS_request(int clientSocket, const char *url);
void send_cached_page(int clientSocket, const char *filePath);
void send_non_cached_page(int clientSocket, const char *url);
void send_blocked_message(int clientSocket);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // set up server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6969);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // listen for connections
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Proxy server listening on port 6969...\n");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // create a request handler for the client
        RequestHandler *handler = malloc(sizeof(RequestHandler));
        handler->clientSocket = client_socket;

        // create a thread to handle the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)handler) != 0) {
            perror("Thread creation failed");
            free(handler);
        }
    }

    close(server_socket);
    return 0;
}

// thread function to handle each client
void *handle_client(void *handler) {
    RequestHandler *requestHandler = (RequestHandler *)handler;
    int clientSocket = requestHandler->clientSocket;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        perror("Failed to read request from client");
        close(clientSocket);
        free(handler);
        pthread_exit(NULL);
    }

    buffer[bytes_read] = '\0';
    printf("Request received: %s\n", buffer);

    char method[10], url[1024];
    sscanf(buffer, "%s %s", method, url);

    if (strcmp(method, "CONNECT") == 0) {
        handle_HTTPS_request(clientSocket, url);
    } else {
        handle_HTTP_request(clientSocket, url);
    }

    close(clientSocket);
    free(handler);
    pthread_exit(NULL);
}

// handle HTTP requests
void handle_HTTP_request(int clientSocket, const char *url) {
    char filePath[BUFFER_SIZE];
    snprintf(filePath, sizeof(filePath), "%s/%s.html", CACHE_DIR, url + 7);  // path

    // check if page is cached
    if (access(filePath, F_OK) == 0) {
        send_cached_page(clientSocket, filePath);
    } else {
        send_non_cached_page(clientSocket, url);
    }
}

// handle HTTPS request
void handle_HTTPS_request(int clientSocket, const char *url) {
    // hostname and port
    char hostname[1024];
    int port = 443;  // default HTTPS port
    sscanf(url, "%[^:]:%d", hostname, &port);

    int serverSocket;
    // struct sockaddr_in serverAddr;
    struct addrinfo hints, *res;

    // zero out hints struct and set relevant parameters for an IPv4, TCP connection
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // resolve hostname to IP address
    char portStr[10];
    snprintf(portStr, sizeof(portStr), "%d", port);
    if (getaddrinfo(hostname, portStr, &hints, &res) != 0) {
        perror("Failed to resolve hostname");
        return;
    }

    serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (serverSocket < 0) {
        perror("Socket creation failed for HTTPS");
        freeaddrinfo(res);
        return;
    }

    // connect to the server
    if (connect(serverSocket, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Failed to connect to server for HTTPS");
        close(serverSocket);
        freeaddrinfo(res);
        return;
    }

    freeaddrinfo(res);

    // send response to client indicating successful connection
    const char *response = "HTTP/1.0 200 Connection Established\r\n\r\n";
    send(clientSocket, response, strlen(response), 0);

    // tunnel data between client and server
    fd_set fdset;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    while (1) {
        FD_ZERO(&fdset);
        FD_SET(clientSocket, &fdset);
        FD_SET(serverSocket, &fdset);

        if (select((clientSocket > serverSocket ? clientSocket : serverSocket) + 1, &fdset, NULL, NULL, NULL) < 0) {
            break;
        }

        if (FD_ISSET(clientSocket, &fdset)) {
            if ((n = recv(clientSocket, buffer, sizeof(buffer), 0)) <= 0) break;
            send(serverSocket, buffer, n, 0);
        }

        if (FD_ISSET(serverSocket, &fdset)) {
            if ((n = recv(serverSocket, buffer, sizeof(buffer), 0)) <= 0) break;
            send(clientSocket, buffer, n, 0);
        }
    }

    close(serverSocket);
}

// send cached page to client
void send_cached_page(int clientSocket, const char *filePath) {
    int file = open(filePath, O_RDONLY);
    if (file < 0) {
        perror("Failed to open cached file");
        return;
    }

    const char *response = "HTTP/1.0 200 OK\r\n\r\n";
    send(clientSocket, response, strlen(response), 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(file, buffer, sizeof(buffer))) > 0) {
        send(clientSocket, buffer, bytesRead, 0);
    }

    close(file);
}

// send non-cached page to client and cache it
void send_non_cached_page(int clientSocket, const char *url) {
    const char *content = "<html><body><h1>Non-cached content</h1></body></html>";
    const char *response = "HTTP/1.0 200 OK\r\n\r\n";
    send(clientSocket, response, strlen(response), 0);
    send(clientSocket, content, strlen(content), 0);

    // cache content
    char filePath[BUFFER_SIZE];
    snprintf(filePath, sizeof(filePath), "%s/%s.html", CACHE_DIR, url + 7);
    int file = open(filePath, O_WRONLY | O_CREAT, 0644);
    if (file >= 0) {
        write(file, content, strlen(content));
        close(file);
    }
}

// send 403 forbidden message
void send_blocked_message(int clientSocket) {
    const char *response = "HTTP/1.0 403 Forbidden\r\n\r\nBlocked by Proxy Server";
    send(clientSocket, response, strlen(response), 0);
}
