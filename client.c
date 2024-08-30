#include <stdio.h>
#include <string.h>         // memset()
#include <errno.h>          // errno
#include <netdb.h>          // addrinfo struct, getaddrinfo()
#include <sys/socket.h>     // AF_, SOCK_, SHUT_ defines, socket(), connect(), shutdown(), recv()


void main() {
    struct addrinfo hints;
    struct addrinfo *result;
    const char      *host = "192.168.1.123";
    const char      *port = "12345";
    int             status;
    int             sockfd;
    char            msg[100];          

    /* Establish TCP connection with the server */
    // memset(&hints, 0, sizeof(hints));
    // hints.ai_family   = AF_INET;
    // hints.ai_socktype = SOCK_STREAM;

    // status = getaddrinfo(host, port, &hints, &result);
    // if (status != 0) {
    //     printf("getaddrinfo error: %d", errno);
    // }

    // sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    // if (sockfd < 0) {
    //     printf("socket error: %d", errno);
    // }

    // status = connect(sockfd, result->ai_addr, result->ai_addrlen);
    // if (status != 0) {
    //     printf("connect error: %d", errno);
    //     shutdown(sockfd, SHUT_RDWR);
    //     return;
    // } else {
    //     printf("Connected to server.");
    // }

    // freeaddrinfo(result);

    /* TODO: communicate with the server */

    /* Read user's input */
    fgets(msg, 5, stdin);
    printf("%s", msg);

    /*  */
}
