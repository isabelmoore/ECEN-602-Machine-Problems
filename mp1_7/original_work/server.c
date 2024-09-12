#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h> 

// send messages to the client
int writen(int socket_desc, char *message, int len) {
    int nsent = send(socket_desc, message, len, 0);
    
    // retry if interrupted
    if (nsent == -1) {
        if (errno == EINTR) {
            printf("Server: Write Interrupted - Retrying\n");
            return writen(socket_desc, message, len);
        }
        perror("Server: Error while sending.");
        return -1;
    }
    return nsent;
}

// read incoming messages from client
int readline(int socket_desc, char *buffer, int buf_size) {
    int nreceived = recv(socket_desc, buffer, buf_size, 0);

    // retry if read fails
    if (nreceived == -1) {
        while (nreceived == -1) {
            perror("Server: Failed to Read - Retrying.");
            nreceived = recv(socket_desc, buffer, buf_size, 0);
        }
    }
    return nreceived;
}

int main(int argc, char *argv[]) {
    // declarations
    int serv_socket;
    struct sockaddr_in serv_address, cli_address;
    int cli_socket;
    char buffer[2048];  // large buffer

    socklen_t addr_len = sizeof(struct sockaddr_in); // Change type to socklen_t
    int reuse_port = 1;
    
    // create socket
    serv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_socket == -1) {
        perror("Server: Socket Creation Error");
        exit(1);
    }

    if (setsockopt(serv_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_port, sizeof(int)) == -1) {
        perror("Server: Socket Option Setup Failed.");
        exit(1);
    }

    // setup server address
    serv_address.sin_family = AF_INET;
    serv_address.sin_port = htons(atoi(argv[1]));
    serv_address.sin_addr.s_addr = INADDR_ANY;
    memset(&(serv_address.sin_zero), '\0', 8);

    // bind socket
    if (bind(serv_socket, (struct sockaddr *)&serv_address, sizeof(serv_address)) == -1) {
        perror("Server: Bind Error");
        exit(1);
    }

    // listen for connections
    if (listen(serv_socket, 5) == -1) {
        perror("Server: Listening Error");
        exit(1);
    }

    printf("Server is Listening on Port %s...\n", argv[1]);

    // main loop to accept clients
    while (1) {
        cli_socket = accept(serv_socket, (struct sockaddr *)&cli_address, &addr_len);
        if (cli_socket == -1) {
            perror("Server: Accept Failed.");
            continue;
        }

        printf("Server: Connection Accepted from %s\n", inet_ntoa(cli_address.sin_addr));

        if (fork() == 0) {  // child process for client
            close(serv_socket);

            while (1) {
                memset(&buffer, '\0', sizeof(buffer));
                int msg_len = readline(cli_socket, buffer, sizeof(buffer));
                
                if (msg_len <= 0) {
                    printf("Server: Client Disconnected.\n");
                    break;
                }

                printf("Server Received: %s", buffer);
                
                // send response back
                int nwritten = writen(cli_socket, buffer, msg_len);
                if (nwritten == -1) {
                    printf("Server: Failed to Send Message Back.\n");
                }
            }

            close(cli_socket);
            exit(0);
        }

        close(cli_socket);  // parent closes client socket
    }

    return 0;
}
