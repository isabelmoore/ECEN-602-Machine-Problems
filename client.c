#include <errno.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>


int create_socket() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0); // socket descriptor
    if (sock_fd == -1) {
        perror("creating socket failed");
        exit(EXIT_FAILURE);
    }
    return sock_fd;
}

void connect_to_server(int socket_fd, const char *server_ip, int port) {
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;  // address family to internet
    serveraddr.sin_port = htons(port);  // port, converting from host to network byte order
    serveraddr.sin_addr.s_addr = inet_addr(server_ip);  // ip address

    if (connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("connecting to server failed");
        exit(EXIT_FAILURE);
    }
    puts("connection established");
}

// messages to/from the server
void send_and_receive_messages(int socket_fd) {
    char send_buffer[100];   
    char receive_buffer[100];  

    while (1) {
        printf("enter message: ");
        if (fgets(send_buffer, sizeof(send_buffer), stdin) == NULL || feof(stdin)) {
            puts("eof encountered, exiting");
            break;
        }

        if (send(socket_fd, send_buffer, strlen(send_buffer), 0) == -1) {
            perror("sending message failed");
            continue;
        }

        int bytes_received = recv(socket_fd, receive_buffer, sizeof(receive_buffer) - 1, 0);
        if (bytes_received > 0) {
            receive_buffer[bytes_received] = '\0';  // null
            printf("received: %s", receive_buffer);
        } else if (bytes_received == 0) {
            puts("server closed the connection");
            break;
        } else {
            perror("receiving message failed");
        }
    }
}

// socket cleaned by closing connection
void cleanup(int socket_fd) {
    close(socket_fd);
    puts("connection closed");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_port = atoi(argv[2]);  // parse port number from command line
    int socket_fd = create_socket();  // create socket

    connect_to_server(socket_fd, argv[1], server_port);  // establish connection to server
    send_and_receive_messages(socket_fd);  
    cleanup(socket_fd);  

    return 0;
}