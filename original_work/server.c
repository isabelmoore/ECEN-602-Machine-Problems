#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>

// send messages to the client
int send_to_client(int socket_desc, char *message, int len) {
    int bytes_sent = send(socket_desc, message, len, 0);
    
    // retry if interrupted
    if (bytes_sent == -1) {
        if (errno == EINTR) {
            printf("Server: Interrupted send, retrying.\n");
            return send_to_client(socket_desc, message, len);
        }
        perror("Server: Error while sending.");
        return -1;
    }
    return bytes_sent;
}

// read incomig messages from client
int read_from_client(int socket_desc, char *buffer, int buf_size) {
    int bytes_received = recv(socket_desc, buffer, buf_size, 0);

    // retry if read fails
    if (bytes_received == -1) {
        while (bytes_received == -1) {
            perror("Server: Failed to read, retrying.");
            bytes_received = recv(socket_desc, buffer, buf_size, 0);
        }
    }
    return bytes_received;
}

int main(int argc, char *argv[]) {
    // declarations
    int server_socket;
    struct sockaddr_in server_address, client_address;
    int client_socket;
    char buffer[2048];  // large buffer
    char temp[2048];    // temp buffer
    memset(&temp, '\0', sizeof(temp));  // unnecessary buffer clear

    int addr_len = sizeof(struct sockaddr_in);
    int reuse_port = 1;
    
    // create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Server: Could not Create Socket.");
        exit(1);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_port, sizeof(int)) == -1) {
        perror("Server: Socket Option Setup Failed.");
        exit(1);
    }

    // setup server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[1]));
    server_address.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_address.sin_zero), '\0', 8);

    // bind socket
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Server: Bind Error");
        exit(1);
    }

    // listen for connections
    if (listen(server_socket, 5) == -1) {
        perror("Server: Listening Error");
        exit(1);
    }

    printf("Server is Listening on Port %s...\n", argv[1]);

    // main loop to accept clients
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addr_len);
        if (client_socket == -1) {
            perror("Server: Accept Failed.");
            continue;
        }

        printf("Server: Connection Accepted from %s\n", inet_ntoa(client_address.sin_addr));

        if (fork() == 0) {  // child process for client
            close(server_socket);

            while (1) {
                memset(&buffer, '\0', sizeof(buffer));
                int msg_len = read_from_client(client_socket, buffer, sizeof(buffer));
                
                if (msg_len <= 0) {
                    printf("Server: Client Disconnected.\n");
                    break;
                }

                printf("Server Received: %s", buffer);
                
                // send response back
                int bytes_written = send_to_client(client_socket, buffer, msg_len);
                if (bytes_written == -1) {
                    printf("Server: Failed to Send Message Back.\n");
                }
            }

            close(client_socket);
            exit(0);
        }

        close(client_socket);  // parent closes client socket
    }

    return 0;
}
