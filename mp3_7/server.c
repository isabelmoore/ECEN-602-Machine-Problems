#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define TFTP_PORT 5000           // The default port number for TFTP
#define TFTP_DIR "./"            // Directory where files are served from
#define BUFSIZE 512              // Maximum data size for TFTP packets
#define RRQ 1                    // Read request (RRQ)
#define DATA 3                   // Data packet (DATA)
#define ACK 4                    // Acknowledgment (ACK)
#define ERROR 5                  // Error packet (ERROR)

// Function to handle the RRQ (Read Request)
void handle_rrq(int sockfd, struct sockaddr_in client_addr, char *filename) {
    char buffer[BUFSIZE];
    socklen_t client_len = sizeof(client_addr);
    char file_path[BUFSIZE];
    int file_fd;
    int block_num = 1;
    ssize_t bytes_read;

    // Construct the full file path for the requested file
    snprintf(file_path, sizeof(file_path), "%s/%s", TFTP_DIR, filename);

    // Open the requested file for reading
    file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        perror("Error: File not found!");
        return;
    }

    printf("Sending file: %s to client\n", filename);

    // Sending data blocks to the client
    while ((bytes_read = read(file_fd, buffer + 4, BUFSIZE - 4)) > 0) {
        buffer[0] = 0;                // Opcode high byte
        buffer[1] = DATA;             // Opcode for DATA packet
        buffer[2] = (block_num >> 8) & 0xFF;  // Block number high byte
        buffer[3] = block_num & 0xFF;        // Block number low byte

        // Send the data packet to the client
        if (sendto(sockfd, buffer, bytes_read + 4, 0, (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("Error sending data packet");
            close(file_fd);
            return;
        }

        printf("Sent block %d with %zd bytes\n", block_num, bytes_read);

        // Receive ACK from client for the current block
        if (recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&client_addr, &client_len) < 0) {
            perror("Error receiving ACK from client");
            close(file_fd);
            return;
        }

        // Check if the received ACK is for the correct block number
        if (buffer[1] != ACK || (buffer[2] << 8 | buffer[3]) != block_num) {
            fprintf(stderr, "Error: Incorrect ACK received\n");
            close(file_fd);
            return;
        }

        block_num++;
    }

    printf("File transfer complete!\n");
    close(file_fd);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFSIZE];

    // Check for the correct number of arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create the socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error: Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));  // Get port from argument

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("TFTP Server listening on port %s...\n", argv[1]);

    while (1) {
        // Receive a packet from the client
        int received_len = recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (received_len < 0) {
            perror("Error receiving packet");
            continue;
        }

        // Fork a new process to handle the client request
        pid_t pid = fork();

        if (pid == -1) {
            // Fork failed
            perror("Error: Fork failed");
            continue;
        } else if (pid == 0) {
            // Child process: handle the request
            if (buffer[1] == RRQ) {  // Read request received
                char filename[BUFSIZE];
                int i = 2;

                // Extract filename from the request
                while (buffer[i] != 0) {
                    filename[i - 2] = buffer[i];
                    i++;
                }
                filename[i - 2] = '\0';

                // Handle the RRQ request
                handle_rrq(sockfd, client_addr, filename);
            } else {
                fprintf(stderr, "Invalid TFTP packet received\n");
            }

            // Child process is done, exit
            exit(0);
        }
        // Parent process continues to listen for new requests
    }

    close(sockfd);
    return 0;
}

