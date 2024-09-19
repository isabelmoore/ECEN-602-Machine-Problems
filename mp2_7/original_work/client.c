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
Handle messages received from the server and act accordingly
*/
int MessagefromServer(int clientSocketFD) {

    Message serverMessage;
    int status = 0;
    int bytesReceived = 0;

    // Read the message from the server
    bytesReceived = read(clientSocketFD,(struct Message *) &serverMessage,sizeof(serverMessage));
    if (bytesReceived <= 0) {
        perror("Server disconnected \n");
        kill(0, SIGINT);
        exit(1);
	}

    // FWD message
    if (serverMessage.header.type == 3) {
    	if ((serverMessage.attribute[0].payload != NULL || *(serverMessage.attribute[0].payload)  !=  '\0') && (serverMessage.attribute[1].payload != NULL || *(serverMessage.attribute[1].payload)  !=  '\0') && serverMessage.attribute[0].type == 4 && serverMessage.attribute[1].type == 2) {     
		    printf("%s : %s ", serverMessage.attribute[1].payload, serverMessage.attribute[0].payload);
	    }
        status = 0;
    }

    // NAK message
    if (serverMessage.header.type == 5) {
    	if ((serverMessage.attribute[0].payload != NULL || *(serverMessage.attribute[0].payload)  !=  '\0') && serverMessage.attribute[0].type == 1) {
            printf("Disconnected NAK Message from Server is %s \n",serverMessage.attribute[0].payload);
        }
        status = 1;
    }

   // ACK Message
    if (serverMessage.header.type == 7) {    	
        if ((serverMessage.attribute[0].payload != NULL || *(serverMessage.attribute[0].payload)  !=  '\0') && serverMessage.attribute[0].type == 4) {
            printf("ACK Message from Server is %s \n",serverMessage.attribute[0].payload);
        }
        status = 0;
    }

    // ONLINE Message
    if (serverMessage.header.type == 8) {
        if ((serverMessage.attribute[0].payload != NULL || *(serverMessage.attribute[0].payload)  !=  '\0') && serverMessage.attribute[0].type == 2) {
                    printf("User '%s' is ONLINE \n",serverMessage.attribute[0].payload);
        }
        status = 0;
    }

    // OFFLINE message
    if (serverMessage.header.type == 6) {
        if ((serverMessage.attribute[0].payload != NULL || *(serverMessage.attribute[0].payload)  !=  '\0') && serverMessage.attribute[0].type == 2) {
            printf("User '%s' is OFFLINE \n",serverMessage.attribute[0].payload);
        }
        status = 0;
    }

    // IDLE Message
    if (serverMessage.header.type == 9) {
		if ((serverMessage.attribute[0].payload != NULL || *(serverMessage.attribute[0].payload)  !=  '\0') && serverMessage.attribute[0].type == 2) {
                printf("User '%s' is IDLE \n",serverMessage.attribute[0].payload);
       }
       status = 0;
    }

    return status;
}

/*
Send JOIN message to the server
*/
void JOIN(int clientSocketFD,const char *arg[]) {

    MessageAttribute joinAttribute;
    Header joinHeader;
    Message joinMessage;

    // build JOIN Header
    joinHeader.version = 3;
    joinHeader.type = 2;  
    joinMessage.header = joinHeader;


    // build username attribute
    joinAttribute.type = 2;
    joinAttribute.length = strlen(arg[1]) + 1;
    strcpy(joinAttribute.payload,arg[1]);
    joinMessage.attribute[0] = joinAttribute;

    write(clientSocketFD,(void *) &joinMessage,sizeof(joinMessage));

    // waiting for server's reply
    sleep(1);

    int joinStatus  = MessagefromServer(clientSocketFD); 
    if (joinStatus  ==  1) {
        close(clientSocketFD);
	    exit(0);
	}
	
}


/*
Handle user chat input and send to the server
*/
void handleUserInput(int connect) {

    Message userMessage;
    MessageAttribute userAttribute;

    int bytes_read = 0;
    char temp[600];
    
    struct timeval tv;
    fd_set readfds;
    tv.tv_sec = 10; // idle bounds for 10 sec
    tv.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv);

    if (FD_ISSET(STDIN_FILENO, &readfds)) {
        
        bytes_read = read(STDIN_FILENO, temp, sizeof(temp));
        if (bytes_read > 0) {
            temp[bytes_read] = '\0';

            strcpy(userAttribute.payload, temp);
            userAttribute.type = 4;
            userMessage.attribute[0] = userAttribute;
            write(connect, (void *) &userMessage, sizeof(userMessage));
        }
    }
}

int main(int argc, char const *argv[]) {

    if (argc  !=  4) {
       fprintf(stderr,"Usage: %s <username> <server_ip> <server_port>\n", argv[0]);
       exit(0);
    }

    // client setup
    int clientSocketFD;
    Message clientMessage;
    struct sockaddr_in serverAddress;
    struct hostent* hostret;

    fd_set masterSet; // track file descriptors
    fd_set inputSet; // user input
    fd_set readSet; // read set for select func

    FD_ZERO(&readSet); // clear 
    FD_ZERO(&masterSet); // clear 
    clientSocketFD = socket(AF_INET,SOCK_STREAM,0);

    if (clientSocketFD == -1) {
        perror("Client: socket creation failed.\n");
        exit(0);
    }

    else {
        printf("Client socket created successful\n");
    }

    // server address
    bzero(&serverAddress,sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    hostret = gethostbyname(argv[2]);
    memcpy(&serverAddress.sin_addr.s_addr, hostret->h_addr,hostret->h_length);
    serverAddress.sin_port = htons(atoi(argv[3]));

    // connect to server
    if (connect(clientSocketFD,(struct sockaddr *)&serverAddress,sizeof(serverAddress)) != 0) {
        printf("Client: error connecting to server\n");
        exit(0);
    }

    else {
        JOIN(clientSocketFD, argv);
        printf("Server connection successful \n");
        FD_SET(clientSocketFD, &masterSet);
        FD_SET(STDIN_FILENO, &inputSet);
        struct timeval tv;
        tv.tv_sec = 10; // timeout of 10s
        tv.tv_usec = 0; // microseconds to 0
            
        pid_t chatProcess;
        int secs, usecs;
        chatProcess = fork(); // folk for handling chat


        if (chatProcess == 0) { // child process for chat handeling

            // infinite loop to check for user input
            for(;;) {
                readSet = inputSet;
                tv.tv_sec = 10;
                tv.tv_usec = 0;
                secs = (int) tv.tv_sec;
                usecs = (int) tv.tv_usec;
                
                // wait for user input using select
                if (select(STDIN_FILENO+1, &readSet, NULL, NULL, &tv) ==  -1) {
                    perror("Client: select failed.");
                    exit(4);
                }

                if (FD_ISSET(STDIN_FILENO, & readSet)) { // if user input is available
                    handleUserInput(clientSocketFD);
                    continue; // continue checking input
                }
        
                else if (tv.tv_sec == 0 && tv.tv_usec == 0) {
                    printf("Timimg out! No user input for %d secs %d usecs\n", secs, usecs);
                    clientMessage.header.type = 9;
                    clientMessage.header.version = 3; 
                    tv.tv_sec = 10;
                    tv.tv_usec = 0;
                    readSet = inputSet;

                    // idle message to server
                    write(clientSocketFD,(void *) &clientMessage,sizeof(clientMessage));
                    continue; // continue checking input
                }
            }
            
        }
        else { // parent process

            // infinite loop to handle server response
            for (;;) {
                readSet = masterSet;
                
                // wait for server response using select
                if (select(clientSocketFD+1, &readSet, NULL, NULL, NULL) ==  -1) {
                    perror("Client: select failed");
                    exit(0);
                }
                
                // check if data
                if (FD_ISSET(clientSocketFD, &readSet)) {
                    MessagefromServer(clientSocketFD);
                }
                
            }
        }

        kill(chatProcess, SIGINT); // kill chat when parent process exits

        printf("\nClient connected and ready for communication.\n");

    }
    printf("\n Client connection closed.\n");
    return 0;
}