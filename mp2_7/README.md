# ECEN-602-Machine-Problem-2: TCP Simple Broadcast Chat Server and Client

## Team Members and Roles ##
- **Isabel Moore**: Developed the core server (server.c) and client (client.c) programs, and extended the sbcp.h header file by defining the structures and functions essential for handling SBCP messages, as part of the original code.

 - **Yiyang Yan**: Developed sbcp.h header file, refactored connection establishment code, added IPv6 support.
- **Shubham Kumar**: Took original code and chatgpt version to provide the optimized code, generated testcases and verified them to generate the report.

## Overview
This project involves a TCP-based chat application that supports multiple clients communicating through a central server. The application uses the Simple Broadcast Chat Protocol (SBCP) to manage chat sessions, handle client messages, and track client states like active, idle, and disconnected.

## Getting Started
### Clone repository
Use following command to clone repository: `git@github.com:isabelmoore/ECEN-602-Machine-Problems.git`
Ensure you have set up an SSH key for your GitHub account to use SSH. 

### Compiling Code
Run `make all` to compile source code and scripts `echos` and `echo` to simplify running server and client programs.

## Running Application
1. **Start server**:
 In server terminal, run following command: `./server 127.0.0.1 12345 10` with `12345` is the port number, and `10` is the maximum number of clients allowed.

2. **Connect with client**:
In client terminal, start your client(s) by connecting to server:  `./client <username> 127.0.0.1 12345`. Replace `<username>` with your desired username.

Once connected, clients can type messages which will be broadcast to all other connected clients. The server handles JOIN, SEND, FWD, and IDLE messages according to the SBCP.

**Using an IPv6 Address**:
User can also use `::1`, which is shorthand for the IPv6 loopback address `0:0:0:0:0:0:0:1`.

## Code Architecture
### Server
- Handles multiple client connections using non-blocking sockets and the select() function to manage I/O multiplexing.
- Maintains a list of connected clients and broadcasts messages to all clients except the sender.
- Detects disconnects and idle clients, sending appropriate notifications to other clients.
### Client
- Connects to the server and handles user input asynchronously.
- Uses `select()` to listen for input from both the standard input (keyboard) and the network socket.
- Displays messages from other clients and handles server notifications.


## Errata & Error Handling
- Redundant condition checks need correction to ensure proper logic and handling.
- Error messages should include more context-specific information to improve debugging.
- String copy operations should use safer functions like `strncpy()` to prevent buffer overflows.
- Proper resource cleanup is essential to avoid memory leaks and resource exhaustion when errors occur or when exiting.

