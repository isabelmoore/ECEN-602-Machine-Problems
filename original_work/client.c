#include <errno.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    // check commandline args
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_port = atoi(argv[2]);

    // create socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Creating Socket Failed");
        exit(EXIT_FAILURE);
    }

    // setup server address
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(server_port);
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

    // connect to server
    if (connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("Connecting to Server Failed");
        exit(EXIT_FAILURE);
    }
    puts("Connection Established");

    // buffers for communication
    char send_buffer[100];
    char receive_buffer[100];

    // communication loop
    while (1) {
        printf("Enter Message: ");
        if (fgets(send_buffer, sizeof(send_buffer), stdin) == NULL || feof(stdin)) {
            puts("EOF Encountered - Exiting");
            break;
        }

        if (send(socket_fd, send_buffer, strlen(send_buffer), 0) == -1) {
            perror("Sending Message Failed");
            continue;
        }

        int bytes_received = recv(socket_fd, receive_buffer, sizeof(receive_buffer) - 1, 0);
        if (bytes_received > 0) {
            receive_buffer[bytes_received] = '\0';
            printf("Received: %s", receive_buffer);
        } else if (bytes_received == 0) {
            puts("Server Closed the Connection");
            break;
        } else {
            perror("Receiving Message Failed");
        }

        memset(send_buffer, 0, sizeof(send_buffer));
        memset(receive_buffer, 0, sizeof(receive_buffer));
    }

    // close socket
    close(socket_fd);
    puts("Connection Closed");

    return 0;
}
