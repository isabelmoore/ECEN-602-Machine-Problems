#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>

#include "sbcp.h"

int clientCount = 0; // Track the number of clients connected to the server
struct infoClient *clients; // Array to store information about each client

// Prototypes for functions handling SBCP messages
void NAK(int clientSocketFD, int code);
void ACK(int clientSocketFD);
void ONLINE(fd_set master, int serverSocketFD, int clientSocketFD, int maxFD);
void OFFLINE(fd_set master, int fdIndex, int serverSocketFD, int clientSocketFD, int maxFD, int clientCount);
int checkUsername(const char username[]);
int isClientValid(int clientSocketFD, int maxClients);

// Check for duplicate usernames
int checkUsername(const char username[]) {
    for (int fdIndex = 0; fdIndex < clientCount; fdIndex++) {
        if (strcmp(username, clients[fdIndex].username) == 0) {
            printf("Duplicate username '%s' detected - rejecting connection.\n", username);
            return 1; // Username found
        }
    }
    return 0; // Username not found
}

// Send ACK message to new client
void ACK(int clientSocketFD) {
    Message message_ACK;
    Header header_ACK = {3, 7}; // Version 3, Type 7
    MessageAttribute attribute_ACK = {4, 0, {0}}; // Type 4

    // Construct the usernames list
    char tempUsername[180] = {0};
    tempUsername[0] = (char)('0' + clientCount);
    strcat(tempUsername, " ");

    for (int counter = 0; counter < clientCount; counter++) {
        strcat(tempUsername, clients[counter].username);
        if (counter < clientCount - 1) {
            strcat(tempUsername, ", ");
        }
    }

    // Set payload and send ACK message
    attribute_ACK.length = strlen(tempUsername) + 1;
    strncpy(attribute_ACK.payload, tempUsername, sizeof(attribute_ACK.payload) - 1);
    
    message_ACK.header = header_ACK;
    message_ACK.attribute[0] = attribute_ACK;

    write(clientSocketFD, &message_ACK, sizeof(message_ACK));
}

// Send NAK message to client when connection request is rejected
void NAK(int clientSocketFD, int code) {
    Message message_NAK;
    Header header_NAK = {3, 5}; // Version 3, Type 5
    MessageAttribute attribute_NAK = {1, 0, {0}}; // Type 1

    // Set error message based on rejection code
    const char *errorMessage = (code == 1) ? "Username is incorrect" : "Client count exceeded request";
    strncpy(attribute_NAK.payload, errorMessage, sizeof(attribute_NAK.payload) - 1);
    attribute_NAK.length = strlen(attribute_NAK.payload) + 1;

    message_NAK.header = header_NAK;
    message_NAK.attribute[0] = attribute_NAK;

    write(clientSocketFD, &message_NAK, sizeof(message_NAK));
    close(clientSocketFD);
}

// Broadcast to all clients when a new client comes online
void ONLINE(fd_set master, int serverSocketFD, int clientSocketFD, int maxFD) {
    Message forwardMessage;
    forwardMessage.header = (Header){3, 8}; // Version 3, Type 8
    forwardMessage.attribute[0] = (MessageAttribute){2,0, {0}}; // Type 2

    // Set username of the newly connected client in message payload
    strncpy(forwardMessage.attribute[0].payload, clients[clientCount - 1].username, sizeof(forwardMessage.attribute[0].payload) - 1);

    // Broadcast message to all clients except the newly connected one and the server itself
    for (int clientFD = 0; clientFD <= maxFD; clientFD++) {
        if (FD_ISSET(clientFD, &master) && clientFD != serverSocketFD && clientFD != clientSocketFD) {
            write(clientFD, &forwardMessage, sizeof(forwardMessage));
        }
    }
    printf("Server accepted Client %s\n", clients[clientCount - 1].username);
}

// Broadcast to all clients when a client goes offline
void OFFLINE(fd_set master, int fdIndex, int serverSocketFD, int clientSocketFD, int maxFD, int clientCount) {
    Message message_OFFLINE;
    const char *username = NULL;

    // Find the username of the disconnected client
    for (int clientIndex = 0; clientIndex < clientCount; clientIndex++) {
        if (clients[clientIndex].fd == fdIndex) {
            username = clients[clientIndex].username;
            break;
        }
    }

    if (username) {
        printf("Socket %d belonging to User '%s' has disconnected\n", fdIndex, username);
        message_OFFLINE.header = (Header){3, 6}; // Version 3, Type 6
        message_OFFLINE.attribute[0] = (MessageAttribute){2, 0, {0}};
        strncpy(message_OFFLINE.attribute[0].payload, username, sizeof(message_OFFLINE.attribute[0].payload) - 1);

        // Broadcast offline message to all clients except the disconnected one and the server
        for (int clientFD = 0; clientFD <= maxFD; clientFD++) {
            if (FD_ISSET(clientFD, &master) && clientFD != fdIndex && clientFD != serverSocketFD) {
                write(clientFD, &message_OFFLINE, sizeof(message_OFFLINE));
            }
        }
    }
}

// Validate new client's connection request
int isClientValid(int clientSocketFD, int maxClients) {
    Message message_join;
    MessageAttribute messageAttribute_join;
    char tempUsername[30];

    // Read join message from client
    if (read(clientSocketFD, &message_join, sizeof(message_join)) <= 0) {
        return 2; // Connection error
    }
    
    messageAttribute_join = message_join.attribute[0];
    strncpy(tempUsername, messageAttribute_join.payload, sizeof(tempUsername) - 1);
    tempUsername[sizeof(tempUsername) - 1] = '\0'; // Null-terminate

    // Check if maximum client limit has been reached
    if (clientCount == maxClients) {
        printf("Client count exceeded - rejecting connection.\n");
        NAK(clientSocketFD, 2); // Reject due to max client count exceeded
        return 2;
    }

    // Check for username availability
    if (checkUsername(tempUsername) == 1) {
        NAK(clientSocketFD, 1); // Username already exists, rejecting connection
        return 1;
    }

    // Add new client to client list
    strcpy(clients[clientCount].username, tempUsername);
    clients[clientCount].fd = clientSocketFD;
    clientCount++;
    ACK(clientSocketFD); // Send ACK to newly accepted client

    return 0; // Valid connection
}

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <hostname> <port> <max_clients>\n", argv[0]);
        exit(1);
    }

    int serverSocketFD, clientSocketFD, maxFD;
    unsigned int clientAddrLen;
    int clientStatus = 0;
    fd_set master, readyFDs;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(argv[1], argv[2], &hints, &res) != 0) {
        perror("getaddrinfo failed");
        exit(1);
    }

    if ((serverSocketFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        perror("Socket creation failed");
        freeaddrinfo(res);
        exit(1);
    }

    int enable = 1;
    if (setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt error");
        close(serverSocketFD);
        exit(1);
    }

    if (bind(serverSocketFD, res->ai_addr, res->ai_addrlen) != 0) {
        perror("Bind error");
        close(serverSocketFD);
        freeaddrinfo(res);
        exit(1);
    }
    freeaddrinfo(res);

    int maxClients = atoi(argv[3]);
    clients = (struct infoClient *)malloc(maxClients * sizeof(struct infoClient));
    if (!clients) {
        perror("Failed to allocate memory for clients");
        close(serverSocketFD);
        exit(1);
    }

    if (listen(serverSocketFD, maxClients) != 0) {
        perror("Listening error");
        free(clients);
        close(serverSocketFD);
        exit(1);
    }

    FD_SET(serverSocketFD, &master);
    maxFD = serverSocketFD; // Keep track of largest file descriptor

    // Server loop
    for (;;) {
        readyFDs = master; 
        if (select(maxFD + 1, &readyFDs, NULL, NULL, NULL) == -1) {
            perror("Select failed");
            free(clients);
            close(serverSocketFD);
            exit(4);
        }

        for (int fdIndex = 0; fdIndex <= maxFD; fdIndex++) {
            if (FD_ISSET(fdIndex, &readyFDs)) {
                if (fdIndex == serverSocketFD) {
                    // Handle new connections
                    struct sockaddr_storage clientAddresses;
                    socklen_t clientAddrLen = sizeof(clientAddresses);
                    clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddresses, &clientAddrLen);

                    if (clientSocketFD < 0) {
                        perror("Accept failed");
                        continue;
                    }

                    printf("New connection from client.\n");
                    FD_SET(clientSocketFD, &master);
                    maxFD = (clientSocketFD > maxFD) ? clientSocketFD : maxFD;

                    clientStatus = isClientValid(clientSocketFD, maxClients);
                    if (clientStatus != 0) {
                        FD_CLR(clientSocketFD, &master); // Remove invalid client
                    } else {
                        ONLINE(master, serverSocketFD, clientSocketFD, maxFD); // Announce online
                    }
                } else {
                    // Handle data from existing clients
                    Message clientMessage;
                    ssize_t bytesReceived = read(fdIndex, &clientMessage, sizeof(clientMessage));

                    if (bytesReceived <= 0) {
                        if (bytesReceived == 0) {
                            OFFLINE(master, fdIndex, serverSocketFD, clientSocketFD, maxFD, clientCount); // Announce client offline
                        } else {
                            perror("Receive failed");
                        }
                        close(fdIndex); // Close client socket
                        FD_CLR(fdIndex, &master); // Remove from master set

                        // Remove client from client array
                        for (int currClientIndex = 0; currClientIndex < clientCount; currClientIndex++) {
                            if (clients[currClientIndex].fd == fdIndex) {
                                for (int shiftIndex = currClientIndex; shiftIndex < clientCount - 1; shiftIndex++) {
                                    clients[shiftIndex] = clients[shiftIndex + 1]; // Shift clients down in array
                                }
                                clientCount--; // Decrement count of clients
                                break;
                            }
                        }
                    } else {
                        // Handle messages from clients
                        Message forwardMessage = clientMessage;
                        MessageAttribute clientAttribute = clientMessage.attribute[0];

                        // Set the username in the message
                        for (int clientIndex = 0; clientIndex < clientCount; clientIndex++) {
                            if (clients[clientIndex].fd == fdIndex) {
                                strncpy(forwardMessage.attribute[1].payload, clients[clientIndex].username, sizeof(forwardMessage.attribute[1].payload) - 1);
                                break;
                            }
                        }
                        printf("Received message from '%s': %s\n", forwardMessage.attribute[1].payload, clientAttribute.payload);

                        // Broadcast message to all clients except the sender
                        for (int clientFD = 0; clientFD <= maxFD; clientFD++) {
                            if (FD_ISSET(clientFD, &master) && clientFD != fdIndex && clientFD != serverSocketFD) {
                                write(clientFD, &forwardMessage, bytesReceived);
                            }
                        }
                    }
                }
            }
        }
    }

    // Cleanup on server shutdown
    free(clients); // Free allocated memory for clients
    close(serverSocketFD); // Close server socket
    return 0;
}

