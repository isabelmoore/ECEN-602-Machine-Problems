#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 5555
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast(const char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] != sender_socket) {
            send(clients[i], message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int client_id = client_socket;

    // Welcome message
    char welcome_message[BUFFER_SIZE];
    snprintf(welcome_message, sizeof(welcome_message), "WELCOME %d\n", client_id);
    send(client_socket, welcome_message, strlen(welcome_message), 0);

    // Notify others about the new connection
    broadcast("A new user has joined the chat.\n", client_socket);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        if (bytes_received <= 0) {
            break; // Client disconnected
        }

        buffer[bytes_received] = '\0'; // Null-terminate the string

        if (strncmp(buffer, "JOIN", 4) == 0) {
            // Notify all clients about the new member
            char join_msg[BUFFER_SIZE];
            snprintf(join_msg, sizeof(join_msg), "User %d has joined the chat.\n", client_id);
            broadcast(join_msg, client_socket);
        } else if (strncmp(buffer, "SEND", 4) == 0) {
            // Broadcast message to other clients
            char msg[BUFFER_SIZE];
            snprintf(msg, sizeof(msg), "User %d: %s\n", client_id, buffer + 5);
            broadcast(msg, client_socket);
        } else if (strncmp(buffer, "LEAVE", 5) == 0) {
            break; // Client wants to leave
        }
    }

    // Cleanup on exit
    close(client_socket);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == client_socket) {
            clients[i] = clients[client_count - 1]; // Replace with last client
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    broadcast("A user has left the chat.\n", -1);
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return EXIT_FAILURE;
    }

    listen(server_socket, MAX_CLIENTS);
    printf("[SERVER STARTED] Waiting for connections...\n");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENTS) {
            clients[client_count++] = client_socket;
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, (void*)&client_socket);
            pthread_detach(tid); // Automatically reclaim thread resources when done
        } else {
            char *message = "SERVER: Max clients reached. Connection denied.\n";
            send(client_socket, message, strlen(message), 0);
            close(client_socket);
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    close(server_socket);
    return EXIT_SUCCESS;
}

