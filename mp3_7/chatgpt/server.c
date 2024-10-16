#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define TFTP_PORT 69          // Default port for TFTP
#define MAX_BUFFER 516        // TFTP packet size (512 bytes data + 4 bytes header)
#define DATA_SIZE 512         // Data size per packet
#define TIMEOUT 5             // Timeout in seconds for client response

// TFTP Opcodes
#define RRQ 1  // Read request
#define DATA 3 // Data packet
#define ACK 4  // Acknowledgment packet

// TFTP Packet Structure
typedef struct {
    uint16_t opcode;
    char data[MAX_BUFFER];
} tftp_packet;

// Function to handle the error and send error packets if necessary
void send_error(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len, uint16_t error_code, const char *error_message) {
    tftp_packet err_packet;
    memset(&err_packet, 0, sizeof(err_packet));
    err_packet.opcode = htons(5); // Error opcode
    err_packet.data[0] = (char)(error_code >> 8);
    err_packet.data[1] = (char)(error_code & 0xFF);
    strncpy(&err_packet.data[2], error_message, sizeof(err_packet.data) - 2);
    
    sendto(sockfd, &err_packet, sizeof(err_packet), 0, (struct sockaddr*) client_addr, addr_len);
}

// Function to handle file reading
void handle_rrq(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len, char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        // Send error if file not found
        send_error(sockfd, client_addr, addr_len, 1, "File not found");
        return;
    }
    
    tftp_packet data_packet;
    int block_num = 1;
    int bytes_read;
    
    while ((bytes_read = fread(data_packet.data, 1, DATA_SIZE, file)) > 0) {
        data_packet.opcode = htons(DATA);  // Set opcode to DATA (3)
        data_packet.data[0] = (char)((block_num >> 8) & 0xFF);
        data_packet.data[1] = (char)(block_num & 0xFF);

        // Send data block
        sendto(sockfd, &data_packet, bytes_read + 4, 0, (struct sockaddr*) client_addr, addr_len);

        // Wait for ACK (block_num)
        fd_set fds;
        struct timeval tv;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        tv.tv_sec = TIMEOUT;
        tv.tv_usec = 0;
        
        int ret = select(sockfd + 1, &fds, NULL, NULL, &tv);
        if (ret == 0) {
            printf("Timeout waiting for ACK\n");
            break;
        } else if (ret > 0) {
            char ack[4];
            recvfrom(sockfd, ack, sizeof(ack), 0, (struct sockaddr*) client_addr, &addr_len);
            uint16_t ack_block = (ack[2] << 8) | ack[3];
            if (ack_block != block_num) {
                printf("ACK block mismatch: Expected %d, got %d\n", block_num, ack_block);
                break;
            }
            block_num++;
        } else {
            perror("Error while waiting for ACK");
            break;
        }
    }
    
    fclose(file);
}

// Main server function to handle requests
void tftp_server(int port) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(1);
    }
    
    printf("TFTP server listening on port %d...\n", port);
    
    while (1) {
        char buffer[MAX_BUFFER];
        int len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &addr_len);
        if (len < 0) {
            perror("Failed to receive data");
            continue;
        }
        
        // Check for Read Request (RRQ)
        if (buffer[0] == 0 && buffer[1] == RRQ) {
            printf("Received RRQ from client: %s\n", inet_ntoa(client_addr.sin_addr));
            
            // Extract the filename
            char *filename = &buffer[2];
            char *file_ext = strchr(filename, 0);
            if (file_ext) {
                *file_ext = '\0'; // Null-terminate the filename
            }
            
            // Handle the read request (RRQ)
            handle_rrq(sockfd, &client_addr, addr_len, filename);
        } else {
            printf("Invalid or unsupported request from client\n");
        }
    }
}

int main() {
    // Run the TFTP server on port 69
    tftp_server(TFTP_PORT);
    return 0;
}

