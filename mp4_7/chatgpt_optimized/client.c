#include "proxy.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <proxy address> <proxy port> <URL to retrieve>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *proxy_address = argv[1];
    int proxy_port = atoi(argv[2]);
    char *url = argv[3];

    int client_socket;
    struct sockaddr_in proxy_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(proxy_port);
    if (inet_pton(AF_INET, proxy_address, &proxy_addr.sin_addr) <= 0) {
        perror("Invalid proxy address");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("Failed to connect to proxy server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", url, proxy_address);
    if (send(client_socket, request, strlen(request), 0) < 0) {
        perror("Failed to send request to proxy server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char response[BUFFER_SIZE];
    ssize_t bytes_received;
    FILE *output_file = fopen("output.html", "w");
    if (!output_file) {
        perror("Failed to open output file");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Response from proxy server:\n");
    while ((bytes_received = recv(client_socket, response, BUFFER_SIZE - 1, 0)) > 0) {
        response[bytes_received] = '\0';
        printf("%s", response);
        fwrite(response, 1, bytes_received, output_file);
    }
    if (bytes_received < 0) {
        perror("Error receiving response");
    }

    fclose(output_file);
    close(client_socket);
    return 0;
}


