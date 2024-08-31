#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>


int send_message(int descriptor, char data[], int dataLength) {
    int sent = 0; // how much bytes sent

    while (sent < dataLength) {
        int n = send(descriptor, data + sent, dataLength - sent, 0); // bytes successfully sent to buffer 
        if (n == -1) {
            if (errno == EINTR) {
                // Handle interrupt during sending
                printf("server: Write interrupted. Retrying\n");
                continue;
            }
            
            perror("server: write error");  // error of sending
            return -1;
        }

        sent += n;
    }
    return sent;
}

int receive_message(int descriptor, char data[], int maxLength) {
    // Receive data
    int n = recv(descriptor, data, maxLength, 0);
    if (n == -1) {
        perror("server: Read Error");
        return -1;
    }
    // null terminate string
    data[n] = '\0';
    return n;
}

void handle_client(int client_socket) {
    char msg[1024];
    int msg_length;

    // processing client messages
    while ((msg_length = receive_message(client_socket, msg, sizeof(msg))) > 0) {
        printf("server: Received Message: %s\n", msg);
        if (send_message(client_socket, msg, msg_length) == -1) {
            printf("server: Error echoig Message\n"); // if message cant be echoed back
            break;
        }
    }

    printf("server: Client Disconnected\n");
    close(client_socket);
}

void start_server(int port) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int yes = 1;

    // create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("server: Socket Error");
        exit(EXIT_FAILURE);
    }

    // attaching socket to port
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("server: Setsockopt Error");
        exit(EXIT_FAILURE);
    }

    // Settibg server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }

    // for connections
    if (listen(server_socket, 10) == -1) {
        perror("server: listen error");
        exit(EXIT_FAILURE);
    }

    printf("server: listening on port %d\n", port);  // server is ready

    // incoming connections
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket == -1) {
            perror("server: accept error");
            continue;
        }

        int pid = fork();
        if (pid == 0) {  // child process
            close(server_socket);
            handle_client(client_socket);
            exit(0);
        } else if (pid > 0) {
            close(client_socket); // Close client socket in parent
        } else {
            perror("server: Fork Error");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please specify the port number\n");
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    start_server(port);
    return 0;
}
