#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sbcp.h"

int clientCount = 0; // tracks number of clients connected to server
struct infoClient *clients; // array to store information about each client

// prototypes for functions handling SBCP messages
void NAK(int clientSocketFD, int code);
void ACK(int clientSocketFD);
void ONLINE(fd_set master, int serverSocketFD, int clientSocketFD, int maxFD);
void OFFLINE(fd_set master, int fdIndex, int serverSocketFD, int clientSocketFD, int maxFD, int clientCount);
int isClientValid(int clientSocketFD, int maxClients);

/*
check for duplicate usernames
*/
int checkUsername(char username[]) {
    // loop through list of clients to see if username is already taken
    for (int fdIndex = 0; fdIndex < clientCount; fdIndex++) {
        if (!strcmp(username, clients[fdIndex].username)) {
            printf("Client attempt with duplicate username '%s' detected - rejecting connection.\n", username);
            return 1; // if username is found
        }
    }
    return 0; // if username is not found
}


/*
send ACK message to new client, confirming connection
*/
void ACK(int clientSocketFD) {
    Message Message_ACK;
    Header Header_ACK;
    MessageAttribute Attribute_ACK;
    char tempUsername[180];

    // initializing message structures with protocol-specific values
    Header_ACK.version = 3;
    Header_ACK.type = 7; 
    Attribute_ACK.type = 4; 

    // constructing string with count and list of usernames of connected clients
    tempUsername[0] = (char)(((int)'0') + clientCount);
    tempUsername[1] = ' ';
    tempUsername[2] = '\0';

    for (int counter = 0; counter < clientCount; counter++) {
        strcat(tempUsername, clients[counter].username);
        if (counter < clientCount - 1) {
            strcat(tempUsername, ", ");
        }
    }

    // setting payload for message attribute
    Attribute_ACK.length = strlen(tempUsername) + 1;
    strcpy(Attribute_ACK.payload, tempUsername);

    // composing message
    Message_ACK.header = Header_ACK;
    Message_ACK.attribute[0] = Attribute_ACK;

    // sending ACK message to client
    write(clientSocketFD, (void *)&Message_ACK, sizeof(Message_ACK));
}

/*
send NAK message to client when connection request is rejected
*/
void NAK(int clientSocketFD, int code) {
    Message Message_NAK;
    Header Header_NAK;
    MessageAttribute MessageAttribute_NAK;
    char tempUsername[32];

    // initializing message structures with NAK-specific details
    Header_NAK.version = 3;
    Header_NAK.type = 5; // NAK type

    MessageAttribute_NAK.type = 1; // Attribute type for reason of rejection

    // setting appropriate error message based on rejection code
    if (code == 1) {
        strcpy(tempUsername, "Username is incorrect");
    } else if (code == 2) {
        strcpy(tempUsername, "Client count exceeded request");
    }

    // setting payload for message attribute
    MessageAttribute_NAK.length = strlen(tempUsername);
    strcpy(MessageAttribute_NAK.payload, tempUsername);

    
    Message_NAK.header = Header_NAK;
    Message_NAK.attribute[0] = MessageAttribute_NAK;

    // sending NAK message and closing connection
    write(clientSocketFD, (void *)&Message_NAK, sizeof(Message_NAK));
    close(clientSocketFD);
}

/*
broadcast to all clients when new client comes online
*/
void ONLINE(fd_set master, int serverSocketFD, int clientSocketFD, int maxFD) {
    Message forwardMessage;
    forwardMessage.header.version = 3;
    forwardMessage.header.type = 8; 
    forwardMessage.attribute[0].type = 2; 

    // setting username of newly connected client in message payload
    strcpy(forwardMessage.attribute[0].payload, clients[clientCount - 1].username);

    // broadcasting message to all clients except newly connected one and server itself
    for (int clientFD = 0; clientFD <= maxFD; clientFD++) {
        if (FD_ISSET(clientFD, &master) && clientFD != serverSocketFD && clientFD != clientSocketFD) {
            write(clientFD, (void *)&forwardMessage, sizeof(forwardMessage));
        }
    }
    printf("Server accepted Client %s\n", clients[clientCount-1].username);
}

/* 
broadcast to all clients when client goes offline
*/
void OFFLINE(fd_set master, int fdIndex, int serverSocketFD, int clientSocketFD, int maxFD, int clientCount) {
    Message Message_OFFLINE;
    for (int clientIndex = 0; clientIndex < clientCount; clientIndex++) {
        if (clients[clientIndex].fd == fdIndex) {
            Message_OFFLINE.attribute[0].type = 2; 
            strcpy(Message_OFFLINE.attribute[0].payload, clients[clientIndex].username);
        }
    }

    // logging disconnection event
    printf("Socket %d belonging to User '%s' has disconnected\n", fdIndex, Message_OFFLINE.attribute[0].payload);
    Message_OFFLINE.header.version = 3;
    Message_OFFLINE.header.type = 6; 

    // broadcasting offline message to all clients except disconnected one and server
    for (int clientFD = 0; clientFD <= maxFD; clientFD++) {
        if (FD_ISSET(clientFD, &master) && clientFD != fdIndex && clientFD != serverSocketFD) {
            write(clientFD, (void *)&Message_OFFLINE, sizeof(Message_OFFLINE));
        }
    }
}

/* validates new clients connection request */

int isClientValid(int clientSocketFD, int maxClients) {
    Message Message_join;
    MessageAttribute MessageAttribute_join;
    char tempUsername[30];

    // reading join message from client
    read(clientSocketFD, (struct Message *)&Message_join, sizeof(Message_join));
    MessageAttribute_join = Message_join.attribute[0];
    strcpy(tempUsername, MessageAttribute_join.payload);

    // checking if maximum client limit has been reached
    if (clientCount == maxClients) {
        printf("New client tries to connect, but Client count exceeded - rejecting\n");
        NAK(clientSocketFD, 2); // Rejecting due to max client count exceeded
        return 2;
    }

    // checking for username availability
    int status  = checkUsername(tempUsername);
    if (status  == 1) {
        NAK(clientSocketFD, 1); // username already exists, rejecting connection
    } else {
        // adding new client to client list
        strcpy(clients[clientCount].username, tempUsername);
        clients[clientCount].fd = clientSocketFD;
        clientCount++;
        ACK(clientSocketFD); // sending ACK to newly accepted client
    }

    return status;
}

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <hostname> <port> <max_clients>\n", argv[0]);
        exit(1);
    }

    // server and client management variables
    Message forwardMessage, clientMessage;
    MessageAttribute clientAttribute;
    int serverSocketFD, clientSocketFD, removeIndex, currClientIndex;
    unsigned int clientAddrLen ;
    int clientStatus = 0;
    struct sockaddr_in serverAddr, *clientAddresses;
    struct hostent* serverHost;

    fd_set master; 
    fd_set readyFDs; 
    int maxFD, tempUsername, fdIndex = 0, clientFD = 0, shiftIndex = 0, clientIndex, bytesReceived, maxClients = 0;

    // creating server socket
    serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFD == -1) {
        perror("Server: socket creation failed");
        exit(1);
    } else {
        printf("Server socket created successfully.\n");
    }

    // set socket options
    bzero(&serverAddr, sizeof(serverAddr));
    int enable = 1;
    
    // setsockopt() allocates buffer space, control timeouts, or permit socket data broadcasts
    if (setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("Server: socktet option failed"); 
        exit(1);
    }

    // setting server address
    serverAddr.sin_family = AF_INET;
    serverHost = gethostbyname(argv[1]);
    memcpy(&serverAddr.sin_addr.s_addr, serverHost->h_addr, serverHost->h_length);
    serverAddr.sin_port = htons(atoi(argv[2]));
    maxClients = atoi(argv[3]);

    // allocating memory for clients and setting up bind
    clients = (struct infoClient *)malloc(maxClients * sizeof(struct infoClient));
    clientAddresses = (struct sockaddr_in *)malloc(maxClients * sizeof(struct sockaddr_in));
    if (bind(serverSocketFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0) {
        perror("Server: socket bind error");
        exit(1);
    } else {
        printf("Socket successfully bounded.\n");
    }

    // start listening for client connections on server socket
    if (listen(serverSocketFD, maxClients) != 0) {
        perror("Server: listening error");
        exit(1);
    } else {
        printf("Server is now listening on port.\n");
    }

    // initialize master file descriptor set and add server socket to it
    FD_SET(serverSocketFD, &master);
    maxFD = serverSocketFD; // keep track of largest file descriptor

    // server loop
    for (;;) {
        readyFDs = master; 
        // wait for event on one of sockets
        if (select(maxFD + 1, &readyFDs, NULL, NULL, NULL) == -1) {
            perror("Server: select failed");
            exit(4);
        }

        // handle data from clients
        for (fdIndex = 0; fdIndex <= maxFD; fdIndex++) {
            if (FD_ISSET(fdIndex, &readyFDs)) { // check if fd is part of set
                if (fdIndex == serverSocketFD) {

                    // handle new connections
                    clientAddrLen  = sizeof(clientAddresses[clientCount]);
                    clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddresses[clientCount], &clientAddrLen);
                    if (clientSocketFD < 0) {
                        perror("Server: aacept failed");
                        exit(1);
                    } 
                    
                    else {
                        printf("New connection from client.\n");
                        tempUsername = maxFD; // store current maxFD
                        FD_SET(clientSocketFD, &master); // add new socket to master set

                        // update maxFD if necessary
                        if (clientSocketFD > maxFD) {
                            maxFD = clientSocketFD;
                        }

                        // validate new client
                        clientStatus = isClientValid(clientSocketFD, maxClients);
                        if (clientStatus == 0) {
                            ONLINE(master, serverSocketFD, clientSocketFD, maxFD); // client is valid, announce online
                        } else {
                            maxFD = tempUsername; // restore old maxFD
                            FD_CLR(clientSocketFD, &master); // remove client from master set if not valid
                        }
                    }
                } else {
                    // handle data from existing clients
                    if ((bytesReceived = read(fdIndex, (struct Message *)&clientMessage, sizeof(clientMessage))) <= 0) {
                        if (bytesReceived == 0) {
                            // client has closed connection
                            OFFLINE(master, fdIndex, serverSocketFD, clientSocketFD, maxFD, clientCount); // announce client offline
                        } else {
                            perror("Server: receive failed");
                        }
                        close(fdIndex); // close client socket
                        FD_CLR(fdIndex, &master); // remove from master set

                        // remove client from client array
                        for (currClientIndex = 0; currClientIndex < clientCount; currClientIndex++) {
                            if (clients[currClientIndex].fd == fdIndex) {
                                removeIndex = currClientIndex; // find client's index
                                break;
                            }
                        }
                        for (shiftIndex = removeIndex; shiftIndex < clientCount - 1; shiftIndex++) {
                            clients[shiftIndex] = clients[shiftIndex + 1]; // shift clients down in array
                        }
                        clientCount--; // decrement count of clients
                    } else {
                        // handle messages from client
                        clientAttribute = clientMessage.attribute[0]; // get first attribute
                        forwardMessage = clientMessage; // prepare forward message

                        // echo message back to other clients
                        for (clientIndex = 0; clientIndex < clientCount; clientIndex++) {
                            if (clients[clientIndex].fd == fdIndex) {
                                strcpy(forwardMessage.attribute[1].payload, clients[clientIndex].username); // set username in message
                            }
                        }
                        printf("Received message from '%s': %s\n", forwardMessage.attribute[1].payload, clientAttribute.payload);

                        // broadcast message to all clients except sender
                        for (clientFD = 0; clientFD <= maxFD; clientFD++) {
                            if (FD_ISSET(clientFD, &master) && clientFD != fdIndex && clientFD != serverSocketFD) {
                                if (write(clientFD, (void *)&forwardMessage, bytesReceived) == -1) {
                                    perror("Message send failed");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // cleanup on server shutdown
    close(serverSocketFD); // close server socket
    return 0;
}
