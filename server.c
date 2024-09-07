#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>    
#include <sys/wait.h> 

#define MAXLINE 1024  // maximum buffer size

// function to write 'n' bytes to socket
int writen(int fd, const char *vptr, size_t n) {
    size_t nleft = n;  // bytes left to write
    ssize_t nwritten;  // bytes written
    const char *ptr = vptr;  // buffer pointer

    // loop until all data is written
    while (nleft > 0) {
        if ((nwritten = send(fd, ptr, nleft, 0)) <= 0) {  // send data over socket
            if (nwritten < 0 && errno == EINTR) {  //if interrupted by signal
                printf("Server: Write Interrupted - Retrying\n");
                nwritten = 0;  // retry write
            } else {
                perror("Server: Write Error"); 
                return -1;  
            }
        }
        nleft -= nwritten;  // update remaining bytes
        ptr += nwritten;    // move buffer pointer forward
    }
    return n;  // if transmitting message is successful, return num of bytes sent
}

// function to read line from socket
ssize_t readline(int fd, char *vptr, size_t maxlen) {
    ssize_t n, rc;
    char c, *ptr = vptr;  // pointer to buffer

    // loop to read up to maxlen characters or until newline
    for (n = 1; n < maxlen; n++) {
        if ((rc = recv(fd, &c, 1, 0)) == 1) {  // read one byte from socket
            *ptr++ = c;  // store character in buffer
            if (c == '\n') {  // newline encountered
                *ptr = 0;  // null-terminate string
                return n;  // return num of bytes read
            }
        } else if (rc == 0) {  // EOF
            *ptr = 0;  // null-terminate string
            return n - 1;  // return bytes read before EOF
        } else {
            if (errno == EINTR) {  // interrupted by signal
                printf("Server: Read interrupted - Continuing\n");
                continue;  // retry read
            }
            perror("Server: Read Error");  
            return -1;  
        }
    }

    *ptr = 0;  // null-terminate string
    return n;  // return num of bytes
}

// function to echo back received data to client
void echo(int sockfd) {
    char buf[MAXLINE];  // buffer to hold received data
    int n;

    // loop to continuously read data from client
    while ((n = readline(sockfd, buf, MAXLINE)) > 0) {
        printf("Server Received: %s", buf);  
        if (writen(sockfd, buf, n) != n) {  // echo data back to client
            perror("Server: Write Back Error");  
        }
    }

    // check for errors in final read operation
    if (n < 0) {
        perror("Server: Read Final Error");
    }
}

// signal handler for SIGCHLD to prevent zombie processes
void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char **argv) {

    // signal handler for SIGCHLD to prevent zombie processes
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Server: sigaction");
        exit(EXIT_FAILURE);
    }

    if (argc != 2) {  // check if user provided port number as an argument
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);  // convert argument to int

    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;  // client and server address structures

    listenfd = socket(AF_INET, SOCK_STREAM, 0);  // socket for listening - IPV4, TCP
    if (listenfd < 0) {
        perror("Server: Socket Creation Error");
        exit(EXIT_FAILURE);
    }

    // zero out server address structure and set its fields
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;  // IPV4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // listen on any available network interface
    servaddr.sin_port = htons(port);  // set server port from command-line argument

    // bind socket to specified address and port
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Server: Bind Error");
        exit(EXIT_FAILURE);
    }

    // start listening for incoming connections - backlog of 10 connection
    if (listen(listenfd, 10) < 0) {
        perror("Server: Listen Error");
        exit(EXIT_FAILURE);
    }

    printf("Server: Listening on Port %d\n", port);

    // main server loop to accept and handle client connections
    while (1) {
        clilen = sizeof(cliaddr);  // size of client address structure
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);  // accept client connection
        if (connfd < 0) {
            perror("Server: Accept Error");
            continue;  // continue to next iteration
        }

        // fork new process to handle client connection
        if ((childpid = fork()) == 0) {  // child process
            close(listenfd);  // child closes listening socket
            echo(connfd);  // handle client request - echo data back
            exit(0);  // child process exits after handling request
        }
        close(connfd);  // parent closes connected socket - child handles connection
    }
}
