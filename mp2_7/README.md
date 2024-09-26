# ECEN-602-Machine-Problem-2: TCP Simple Broadcast Chat Server and Client

## Team Members and Roles ##
- **Isabel Moore**: (to be added)
 - **Yiyang Yan**: Refactored connection establishment code, added IPv6 support
- **Shubham Kumar**: (Took original code and chatgpt version to provide the optimized code, generated testcases and verified them to generate the report.)

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
- (to be added)


