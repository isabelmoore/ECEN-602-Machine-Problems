#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#define TFTP_PORT 1024             // TFTP port number
#define TFTP_DIR "tftp_files"    // Directory where the files are stored

// Define the size of TFTP data packets (512 bytes is the default block size in TFTP)
#define TFTP_PACKET_SIZE 516
#define DATA_SIZE 512

// Function to handle read request (RRQ)
void handle_rrq(int client_socket, sockaddr_in &client_address, const std::string &filename) {
    // Create the full file path by concatenating the directory and filename
    std::string file_path = std::string(TFTP_DIR) + "/" + filename;

    // Open the requested file in binary mode
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "File not found: " << file_path << std::endl;
        return;
    }

    // Read and send the file in blocks to the client
    char buffer[DATA_SIZE];
    int block_num = 1;
    while (file.read(buffer, DATA_SIZE)) {
        // Prepare TFTP Data packet
        std::vector<char> packet;
        packet.push_back(0);  // TFTP opcode (Data)
        packet.push_back(3);  // Data opcode
        packet.push_back(block_num >> 8);  // Block number high byte
        packet.push_back(block_num & 0xFF);  // Block number low byte

        // Append file data to packet
        packet.insert(packet.end(), buffer, buffer + DATA_SIZE);

        // Send the packet to the client
        if (sendto(client_socket, packet.data(), packet.size(), 0,
                   (struct sockaddr*)&client_address, sizeof(client_address)) == -1) {
            std::cerr << "Failed to send data block" << std::endl;
            return;
        }

        // Move to the next block
        block_num++;
    }

    // Handle any remaining data
    if (file.gcount() > 0) {
        std::vector<char> packet;
        packet.push_back(0);  // TFTP opcode (Data)
        packet.push_back(3);  // Data opcode
        packet.push_back(block_num >> 8);  // Block number high byte
        packet.push_back(block_num & 0xFF);  // Block number low byte

        // Append the remaining bytes to the packet
        packet.insert(packet.end(), buffer, buffer + file.gcount());

        // Send the final packet
        if (sendto(client_socket, packet.data(), packet.size(), 0,
                   (struct sockaddr*)&client_address, sizeof(client_address)) == -1) {
            std::cerr << "Failed to send final data block" << std::endl;
            return;
        }
    }

    // Close the file after sending
    file.close();
    std::cout << "File sent successfully!" << std::endl;
}

// Main server function
int main() {
    // Set up the UDP socket
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        return 1;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);  // Bind to all available interfaces
    server_address.sin_port = htons(TFTP_PORT);         // Use the default TFTP port

    // Bind the socket to the server address
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Failed to bind socket");
        close(server_socket);
        return 1;
    }

    std::cout << "TFTP server listening on port " << TFTP_PORT << "...\n";

    // Buffer for receiving requests
    char buffer[TFTP_PACKET_SIZE];
    sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);

    while (true) {
        // Receive request from a client
        int bytes_received = recvfrom(server_socket, buffer, TFTP_PACKET_SIZE, 0,
                                      (struct sockaddr*)&client_address, &client_len);
        if (bytes_received < 0) {
            perror("Failed to receive data");
            continue;
        }

        // Extract the opcode from the request
        unsigned short opcode = (buffer[0] << 8) | buffer[1];

        // Process a read request (RRQ)
        if (opcode == 1) {  // RRQ (Read Request)
            std::string filename;
            bool is_octet_mode = false;
            for (int i = 2; i < bytes_received; i++) {
                if (buffer[i] == 0) {
                    // Null-terminated string (filename) found
                    if (filename.empty()) {
                        filename = std::string(buffer + 2, buffer + i);  // Extract filename
                    } else {
                        // We are now reading the mode field
                        is_octet_mode = (std::string(buffer + i + 1) == "octet");
                    }
                }
            }

            // Only proceed if mode is "octet" (TFTP standard)
            if (is_octet_mode) {
                std::cout << "Read request for file: " << filename << "\n";
                handle_rrq(server_socket, client_address, filename);
            } else {
                std::cerr << "Unsupported mode requested.\n";
            }
        }
        // You can handle other request types (e.g., WRQ) here as well.
    }

    // Clean up and close the server socket when done
    close(server_socket);
    return 0;
}

