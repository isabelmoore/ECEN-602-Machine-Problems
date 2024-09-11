#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

// function to create socket and return file descriptor
int create_socket() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);  // create socket using TCP (sock_stream)
    if (sock_fd == -1) {  // if socket creation fails
        perror("Creating Socket Failed");  
        exit(EXIT_FAILURE);  
    }
    return sock_fd;  // return socket file descriptor
}

// function to connect to server using provided IP and port
void connect_to_server(int socket_fd, const char *server_ip, int port) {
    struct sockaddr_in serveraddr;  // define server's address structure

    serveraddr.sin_family = AF_INET;  // set address family to IPV4
    serveraddr.sin_port = htons(port);  // convert port number to network byte order
    serveraddr.sin_addr.s_addr = inet_addr(server_ip);  // convert IP address to binary form

    // attempt to connect to server
    if (connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("Connecting to Server Failed");  // print error message if connection fails
        exit(EXIT_FAILURE);  // exit program
    }
    printf("Connection Established\n");  // print success message when connection is established
}

void send_receive_messages(int socket_fd) {
    char send_buffer[100];  // buffer to hold messages to send
    char receive_buffer[100];  // buffer to hold messages received from server

    // loop to send and receive messages
    while (1) {
        printf("Enter Message: ");  // prompt user for input
        if (fgets(send_buffer, sizeof(send_buffer), stdin) == NULL || feof(stdin)) {  // if EOF encountered, break loop
            puts("EOF Encountered - Exiting");  
            break;
        }

        // send message to server
        if (send(socket_fd, send_buffer, strlen(send_buffer), 0) == -1) { // if sending fails
            perror("Sending Message Failed");  
            continue;  // continue to next loop iteration
        }

        // receive server's response
        int bytes_received = recv(socket_fd, receive_buffer, sizeof(receive_buffer) - 1, 0);
        if (bytes_received > 0) {  // if data is received
            receive_buffer[bytes_received] = '\0';  // nullterminate received string
            printf("Received: %s", receive_buffer); 
        } else if (bytes_received == 0) {  // if server closes connection
            puts("Server Closed Connection");  
            break;
        } else {
            perror("Receiving Message Failed");  // error if receiving fails
        }
    }
}

// clean up and close socket
void cleanup(int socket_fd) {
    close(socket_fd);  
    puts("Connection Closed"); 
}

int main(int argc, char* argv[]) {
    // check if user has provided server IP and port
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);  
        exit(EXIT_FAILURE);  // exit program if args missing
    }

    int server_port = atoi(argv[2]); // convert port to integer
    int socket_fd = create_socket();
    connect_to_server(socket_fd, argv[1], server_port); 
    send_receive_messages(socket_fd);
    cleanup(socket_fd);

    return 0;  // success execution
}
