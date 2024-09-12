// echo_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 12346  // Port number for the server
#define SERVER_IP "127.0.0.1"  // IP address of the server

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[1024];
    ssize_t n;

    // Create a socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        // Send the data to the server
        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("Write failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Receive the echoed data from the server
        n = read(sockfd, buffer, sizeof(buffer) - 1);
        if (n < 0) {
            perror("Read failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        } else if (n == 0) {
            printf("Server closed connection\n");
            break;
        }

        buffer[n] = '\0';  // Null-terminate the buffer
        printf("Server echoed: %s", buffer);
    }

    close(sockfd);
    return 0;
}
