#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "sbcp.h"

/*
Handle messages received from the server and act accordingly.
*/
int MessagefromServer(int clientSocketFD) {
    Message serverMessage;
    int bytesReceived = read(clientSocketFD, &serverMessage, sizeof(serverMessage));

    if (bytesReceived <= 0) {
        perror("Server disconnected");
        kill(0, SIGINT);
        exit(1);
    }

    switch (serverMessage.header.type) {
        case 3: // FWD message
            if (serverMessage.attribute[0].payload && serverMessage.attribute[1].payload &&
                serverMessage.attribute[0].type == 4 && serverMessage.attribute[1].type == 2) {
                printf("%s : %s\n", serverMessage.attribute[1].payload, serverMessage.attribute[0].payload);
            }
            break;

        case 5: // NAK message
            if (serverMessage.attribute[0].payload && serverMessage.attribute[0].type == 1) {
                printf("Disconnected NAK Message from Server: %s\n", serverMessage.attribute[0].payload);
            }
            return 1;

        case 7: // ACK message
            if (serverMessage.attribute[0].payload && serverMessage.attribute[0].type == 4) {
                printf("ACK Message from Server: %s\n", serverMessage.attribute[0].payload);
            }
            break;

        case 8: // ONLINE message
            if (serverMessage.attribute[0].payload && serverMessage.attribute[0].type == 2) {
                printf("User '%s' is ONLINE\n", serverMessage.attribute[0].payload);
            }
            break;

        case 6: // OFFLINE message
            if (serverMessage.attribute[0].payload && serverMessage.attribute[0].type == 2) {
                printf("User '%s' is OFFLINE\n", serverMessage.attribute[0].payload);
            }
            break;

        case 9: // IDLE message
            if (serverMessage.attribute[0].payload && serverMessage.attribute[0].type == 2) {
                printf("User '%s' is IDLE\n", serverMessage.attribute[0].payload);
            }
            break;

        default:
            fprintf(stderr, "Unknown message type: %d\n", serverMessage.header.type);
            break;
    }
    return 0;
}

/*
Send JOIN message to the server.
*/
void JOIN(int clientSocketFD, const char *username) {
    Message joinMessage;
    joinMessage.header.version = 3;
    joinMessage.header.type = 2;

    MessageAttribute joinAttribute;
    joinAttribute.type = 2;
    joinAttribute.length = strlen(username) + 1;
    strncpy(joinAttribute.payload, username, sizeof(joinAttribute.payload) - 1);
    joinAttribute.payload[sizeof(joinAttribute.payload) - 1] = '\0'; // Ensure null termination

    joinMessage.attribute[0] = joinAttribute;

    if (write(clientSocketFD, &joinMessage, sizeof(joinMessage)) < 0) {
        perror("Failed to send JOIN message");
        close(clientSocketFD);
        exit(1);
    }

    // Waiting for server's reply
    sleep(1);
    if (MessagefromServer(clientSocketFD) == 1) {
        close(clientSocketFD);
        exit(0);
    }
}

/*
Handle user chat input and send to the server.
*/
void handleUserInput(int clientSocketFD) {
    Message userMessage;
    MessageAttribute userAttribute;
    char temp[600];
    
    struct timeval tv;
    tv.tv_sec = 10; // idle timeout
    tv.tv_usec = 0;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0) {
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            ssize_t bytesRead = read(STDIN_FILENO, temp, sizeof(temp) - 1);
            if (bytesRead > 0) {
                temp[bytesRead] = '\0'; // Null terminate the string
                userAttribute.type = 4;
                strncpy(userAttribute.payload, temp, sizeof(userAttribute.payload) - 1);
                userAttribute.payload[sizeof(userAttribute.payload) - 1] = '\0'; // Ensure null termination
                userMessage.attribute[0] = userAttribute;
                write(clientSocketFD, &userMessage, sizeof(userMessage));
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <username> <server_ip> <server_port>\n", argv[0]);
        exit(1);
    }

    int clientSocketFD;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(argv[2], argv[3], &hints, &res) != 0) {
        perror("getaddrinfo failed");
        exit(1);
    }

    clientSocketFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (clientSocketFD < 0) {
        perror("Socket creation error");
        exit(1);
    }
    
    if (connect(clientSocketFD, res->ai_addr, res->ai_addrlen) != 0) {
        perror("Connect error");
        close(clientSocketFD);
        exit(1);
    }

    freeaddrinfo(res);
    JOIN(clientSocketFD, argv[1]);
    printf("Server connection successful\n");

    fd_set masterSet, inputSet, readSet;
    FD_ZERO(&masterSet);
    FD_ZERO(&inputSet);
    FD_SET(clientSocketFD, &masterSet);
    FD_SET(STDIN_FILENO, &inputSet);

    pid_t chatProcess = fork();
    if (chatProcess == 0) { // Child process for handling user input
        for (;;) {
            readSet = inputSet;
            struct timeval tv = {10, 0}; // timeout of 10 seconds
            if (select(STDIN_FILENO + 1, &readSet, NULL, NULL, &tv) == -1) {
                perror("Select failed");
                exit(1);
            }
            if (FD_ISSET(STDIN_FILENO, &readSet)) {
                handleUserInput(clientSocketFD);
            } else {
                Message idleMessage;
                idleMessage.header.version = 3;
                idleMessage.header.type = 9; // IDLE message
                write(clientSocketFD, &idleMessage, sizeof(idleMessage));
            }
        }
    } else { // Parent process for handling server response
        for (;;) {
            readSet = masterSet;
            if (select(clientSocketFD + 1, &readSet, NULL, NULL, NULL) == -1) {
                perror("Select failed");
                exit(1);
            }
            if (FD_ISSET(clientSocketFD, &readSet)) {
                MessagefromServer(clientSocketFD);
            }
        }
    }

    kill(chatProcess, SIGINT); // Terminate chat process on exit
    close(clientSocketFD);
    return 0;
}

