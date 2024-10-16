/* I/O */
#include <stdio.h>      // perror

/* Networking */
#include <netdb.h>
#include <sys/socket.h>

/* System */
#include <unistd.h>     // fork

/* Misc. Utilities */
#include <stdint.h>
#include <stdlib.h>     // exit
#include <string.h>     // memset


#define OP_RRQ      1
#define OP_DATA     3
#define OP_ACK      4
#define OP_ERR      5

/**
 * @brief Creates a UDP socket and binds it to the given IP address and port.
 * 
 * @param addr The string containing the IP address
 * @param port The string containing the port number
 * 
 * @return The socket file descriptor
 */
int udp_socket_init(const char *addr, const char *port) {
    /* Obtain address information */
    struct addrinfo server_hints     = {0};
    struct addrinfo *server_addrinfo = NULL;
    server_hints.ai_family   = AF_INET;
    server_hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(addr, port, &server_hints, &server_addrinfo) != 0) {
        perror("getaddrinfo error");
    }

    /* Create a socket */
    int sockfd = 0;
    sockfd = socket(server_addrinfo->ai_family, server_addrinfo->ai_socktype, server_addrinfo->ai_protocol);
    if (sockfd < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    /* Bind the socket */
    if (bind(sockfd, server_addrinfo->ai_addr, server_addrinfo->ai_addrlen) != 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}


/**
 * @brief Sends file to client.
 * 
 * @param server_sockfd   The server's UDP socket descriptor
 * @param client_sockaddr The pointer to the client's UDP socket address information
 * @param client_socklen  The pointer to the client's UDP socket address length
 * @param file_name       The name of the file to be sent
 * @param mode            The mode
 */
void send_file(int server_sockfd, struct sockaddr *client_sockaddr, socklen_t client_socklen, char *file_name, char *mode) {
    uint8_t  send_buf[516] = {0};
    uint8_t  recv_buf[128] = {0};
    uint16_t send_blk_idx  = 1;
    uint16_t recv_blk_idx  = 0;
    FILE     *stream       = NULL;
    size_t   bytes_read    = 0;

    /* Open the file */
    stream = fopen(file_name, "rb");
    if (stream == NULL) {
        perror("fopen error");

        /* Send error packet to the client */
        send_buf[0] = 0;
        send_buf[1] = OP_ERR;
        send_buf[2] = (1 >> 8) & 0xFF;
        send_buf[3] = 1 & 0xFF;
        strcpy(send_buf + 4, "file not found");
        if (sendto(server_sockfd, send_buf, 4 + strlen("file not found") + 1, 0, client_sockaddr, client_socklen) < 0) {
            perror("send error");
            fclose(stream);
            return;
        }

        return;
    }

    bytes_read = fread(send_buf + 4, 1, 512, stream);

    while (bytes_read >= 0) {
        /* Form opcode and block index for data packet */
        send_buf[0] = 0;
        send_buf[1] = OP_DATA;
        send_buf[2] = (send_blk_idx >> 8) & 0xFF;
        send_buf[3] = send_blk_idx & 0xFF;

        /* Send data packet to the client */
        if (bytes_read != 0) {
            if (sendto(server_sockfd, send_buf, bytes_read + 4, 0, client_sockaddr, client_socklen) < 0) {
                perror("send error");
                fclose(stream);
                return;
            }

            if (send_blk_idx < 5 || send_blk_idx > 0xFFFB) {
                printf("Block #%u sent\n", send_blk_idx);
            }
        } else {
            if (sendto(server_sockfd, send_buf, 4, 0, client_sockaddr, client_socklen) < 0) {
                perror("send error");
                fclose(stream);
                return;
            }

            break;
        }
        
        /* Receive acknowledgement from the client */
        if (recvfrom(server_sockfd, recv_buf, sizeof(recv_buf), 0, client_sockaddr, &client_socklen) < 0) {
            perror("recvfrom error");
            fclose(stream);
            return;
        }

        switch(recv_buf[1]) {
            case 1:
                strcpy(file_name, recv_buf + 2);
                strcpy(mode, recv_buf + 2 + strlen(file_name) + 1);
                printf("RRQ: Filename: %s, Mode: %s\n", file_name, mode);
                break;
            case 4:
                recv_blk_idx = (recv_buf[2] << 8) | recv_buf[3];
                if (send_blk_idx < 5 || send_blk_idx > 0xFFFB) {
                    printf("ACK: Block #%d received\n", recv_blk_idx);
                }
                break;
            default:
                printf("Unknown opcode\n");
                break;
        }

        if (recv_blk_idx == send_blk_idx) {
            send_blk_idx++;
            memset(send_buf, 0, sizeof(send_buf));
            bytes_read = fread(send_buf + 4, 1, 512, stream);
        }
    }

    printf("File transfer completed\n");
    fclose(stream);
}

int main(int argc, char const *argv[]) {
    /* Check arguments */
    if (argc != 3) {
        printf("Usage: server <host> <port>");
        exit(EXIT_FAILURE);
    }

    /* Create an UDP socket and bind it to the well-known port */
    int server_sockfd = 0;
    server_sockfd = udp_socket_init(argv[1], argv[2]);

    /* Receive packets from new clients */
    char                    buf[512]        = {0};
    struct sockaddr_storage client_sockaddr = {0};
    socklen_t               client_socklen  = sizeof(struct sockaddr_storage);
    char                    file_name[256]  = {0};
    char                    mode[16]        = {0};
    pid_t                   pid             = 0;
    while (1) {
        if (recvfrom(server_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)(&client_sockaddr), &client_socklen) < 0) {
            perror("recvfrom error");
            exit(EXIT_FAILURE);
        }

        if (buf[1] != 1) {
            printf("Unsuported opcode\n");
            continue;
        }

        pid = fork();
        if (pid < 0) {
            perror("fork error");
            exit(EXIT_FAILURE);
        }

        /* Child process routine begin */
        if (pid == 0) {
            int sockfd = 0;

            /* Create a new UDP socket and bind it to an ephemeral port */
            sockfd = udp_socket_init(argv[1], "0");

            /* Obtain file name and mode */
            strcpy(file_name, buf + 2);
            strcpy(mode, buf + 2 + strlen(file_name) + 1);
            printf("Requesting %s, Mode: %s\n", file_name, mode);

            /* Send the file to the client */
            send_file(sockfd, (struct sockaddr *)(&client_sockaddr), client_socklen, file_name, mode);

            printf("Child process exited\n");
            exit(EXIT_SUCCESS);
        }
        /* Child process routine end */
    }

    exit(EXIT_SUCCESS);
}
