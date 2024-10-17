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
#define MAX_DATA_SIZE 512
#define TFTP_HEADER_SIZE 4

/**
 * @brief Creates a UDP socket and binds it to the given IP address and port.
 * 
 * @param addr The string containing the IP address
 * @param port The string containing the port number
 * 
 * @return The socket file descriptor
 */
int udp_socket_init(const char *addr, const char *port) {
    struct addrinfo server_hints = {0}, *server_addrinfo = NULL;
    server_hints.ai_family   = AF_INET;
    server_hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(addr, port, &server_hints, &server_addrinfo) != 0) {
        perror("getaddrinfo error");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(server_addrinfo->ai_family, server_addrinfo->ai_socktype, server_addrinfo->ai_protocol);
    if (sockfd < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    if (bind(sockfd, server_addrinfo->ai_addr, server_addrinfo->ai_addrlen) != 0) {
        perror("bind error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(server_addrinfo); // Avoid memory leak
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
void send_file(int server_sockfd, struct sockaddr *client_sockaddr, socklen_t client_socklen, const char *file_name, const char *mode) {
    uint8_t  send_buf[MAX_DATA_SIZE + TFTP_HEADER_SIZE] = {0};
    uint8_t  recv_buf[128] = {0};
    uint16_t send_blk_idx  = 1;
    FILE *stream = fopen(file_name, "rb");

    if (!stream) {
        perror("fopen error");

        // Send error packet to the client
        send_buf[0] = 0;
        send_buf[1] = OP_ERR;
        send_buf[2] = (1 >> 8) & 0xFF;
        send_buf[3] = 1 & 0xFF;
        snprintf((char *)(send_buf + 4), sizeof(send_buf) - 4, "file not found");
        sendto(server_sockfd, send_buf, 4 + strlen((char *)(send_buf + 4)) + 1, 0, client_sockaddr, client_socklen);
        return;
    }

    size_t bytes_read = fread(send_buf + 4, 1, MAX_DATA_SIZE, stream);

    while (bytes_read > 0) {
        send_buf[0] = 0;
        send_buf[1] = OP_DATA;
        send_buf[2] = (send_blk_idx >> 8) & 0xFF;
        send_buf[3] = send_blk_idx & 0xFF;

        if (sendto(server_sockfd, send_buf, bytes_read + TFTP_HEADER_SIZE, 0, client_sockaddr, client_socklen) < 0) {
            perror("send error");
            break;
        }

        if (recvfrom(server_sockfd, recv_buf, sizeof(recv_buf), 0, client_sockaddr, &client_socklen) < 0) {
            perror("recvfrom error");
            break;
        }

        if (recv_buf[1] == OP_ACK && (((recv_buf[2] << 8) | recv_buf[3]) == send_blk_idx)) {
            send_blk_idx++;
            memset(send_buf + 4, 0, MAX_DATA_SIZE); // Reset buffer for next read
            bytes_read = fread(send_buf + 4, 1, MAX_DATA_SIZE, stream);
        } else {
            break; // Abort if ACK is incorrect or missing
        }
    }

    fclose(stream);
    printf("File transfer completed\n");
}

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: server <host> <port>\n");
        exit(EXIT_FAILURE);
    }

    int server_sockfd = udp_socket_init(argv[1], argv[2]);

    while (1) {
        char buf[512] = {0};
        struct sockaddr_storage client_sockaddr = {0};
        socklen_t client_socklen = sizeof(client_sockaddr);
        char file_name[256] = {0}, mode[16] = {0};

        ssize_t recv_len = recvfrom(server_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)(&client_sockaddr), &client_socklen);
        if (recv_len < 0) {
            perror("recvfrom error");
            continue;
        }

        if (buf[1] != OP_RRQ) {
            fprintf(stderr, "Unsupported opcode\n");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error");
            continue;
        }

        if (pid == 0) {
            int sockfd = udp_socket_init(argv[1], "0");

            strncpy(file_name, buf + 2, sizeof(file_name) - 1);
            strncpy(mode, buf + 2 + strlen(file_name) + 1, sizeof(mode) - 1);
            printf("Requesting %s, Mode: %s\n", file_name, mode);

            send_file(sockfd, (struct sockaddr *)(&client_sockaddr), client_socklen, file_name, mode);

            close(sockfd); // Close socket in child process
            exit(EXIT_SUCCESS);
        }
    }

    close(server_sockfd); // Close main socket
    return 0;
}
