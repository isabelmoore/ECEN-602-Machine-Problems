#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 5555
#define BUFFER_SIZE 1024

void *receive_messages(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Connection closed.\n");
            break;
        }
        printf("%s", buffer);
    }
    return NULL;
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return EXIT_FAILURE;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, receive_messages, (void*)&client_socket);

    while (1) {
        char command[BUFFER_SIZE];
        fgets(command, BUFFER_SIZE, stdin);
        send(client_socket, command, strlen(command), 0);
        
        if (strncmp(command, "LEAVE", 5) == 0) {
            break; // Exit loop if LEAVE command is sent
        }
    }

    close(client_socket);
    return EXIT_SUCCESS;
}

