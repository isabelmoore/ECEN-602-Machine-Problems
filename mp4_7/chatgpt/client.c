#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

void download_file(const char *proxy_address, int proxy_port, const char *url) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(proxy_port);
    
    if (inet_pton(AF_INET, proxy_address, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return;
    }
    
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", url, url);
    send(sock, request, strlen(request), 0);
    printf("Request sent to proxy\n");
    
    int valread = read(sock, buffer, BUFFER_SIZE);
    printf("Response received\n");
    
    // Extract filename from URL
    const char *filename = strrchr(url, '/');
    if (filename == NULL) {
        filename = "index.html";
    } else {
        filename++;
    }
    
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("Error opening file for writing\n");
        close(sock);
        return;
    }
    
    // Skip HTTP headers
    char *body = strstr(buffer, "\r\n\r\n");
    if (body != NULL) {
        body += 4;
        fwrite(body, 1, valread - (body - buffer), fp);
    }
    
    while ((valread = read(sock, buffer, BUFFER_SIZE)) > 0) {
        fwrite(buffer, 1, valread, fp);
    }
    
    fclose(fp);
    close(sock);
    
    printf("File '%s' downloaded successfully.\n", filename);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <proxy_address> <proxy_port> <URL_to_retrieve>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *proxy_address = argv[1];
    int proxy_port = atoi(argv[2]);
    const char *url = argv[3];

    download_file(proxy_address, proxy_port, url);

    return 0;
}
