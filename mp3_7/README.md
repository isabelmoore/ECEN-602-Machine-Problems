# ECEN-602-Machine-Problem-3: Trivial File Transfer Protocol (TFTP) Server

## Team Members and Roles ##
- **Isabel Moore**: Created the Makefile and optimized the code using ChatGPT suggestions.
- **Yiyang Yan**: Implemented and tested TFTP server
- **Shubham Kumar**:Implemented basic server code and verified test cases 

## Overview:
This project implements a **Trivial File Transfer Protocol (TFTP) server** using UDP. The server allows multiple clients to request files using Read Request (RRQ) and supports handling of simultaneous clients using `fork()`. The TFTP server is designed for basic file transfers from the server to clients, and adheres to the TFTP protocol described in RFC 1350.

## Getting Started
### Clone repository
Use following command to clone repository: `git@github.com:isabelmoore/ECEN-602-Machine-Problems.git`
Ensure you have set up an SSH key for your GitHub account to use SSH. 

### Compiling Code
Run `make all` to compile source code.

## Running Application
1. **Start server**:
In server terminal, run following command: `./server 127.0.0.1 12345` with `12345` being an example port number.

2. **Connect Using a TFTP Client**:
Use a standard TFTP client to connect to the server `tftp 127.0.0.1 12345` and request a file `tftp> get <filename>`.

## Code Architecture
### Server
- The server spawns a child process whenever the parent process receives a new message (Read Request).
- Each child process handles the file transfer for one client, sending the file in 512-byte blocks.
- Parent process continues listening for additional client requests while child processes perform their tasks concurrently.

## Errata & Error Handling
- Currently, no timeout mechanism is implemented for client acknowledgments or retransmissions.
- Error handling includes sending an error packet (`OP_ERR`) when a file is not found or an invalid request is received.
