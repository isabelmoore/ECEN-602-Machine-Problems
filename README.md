# ECEN-602-Machine-Problem-1: TCP Echo Server and Client
**Authors**: Isabel Moore, Yiyang Yan, and Shubham Kumar  
**Date**: August 30, 2024

## Overview
To run the TCP server-client application on Ubuntu 22.04, this server will handle incoming connections and echo back any messages received from the client.

## Getting Started
### Clone the repository
Use the following command to clone the repository: 
`git clone git@github.com/ECEN-602-Machine-Problem-1.git`
 Ensure you have set up an SSH key for your GitHub account to use SSH. If not, use the command `ssh-keygen`

### Compiling the Code
Within the repository folder, compile the code using the following commands:
- In one terminal for the server: `gcc -o server server.c -Wall -Wextra`
- In another terminal for the client: `gcc -o client client.c -Wall -Wextra`

These commands compile the `server.c` and `client.c` files into executables named `server` and `client` respectively, enabling all warnings (`-Wall`) and extra warnings (`-Wextra`).

## Running the Application
1. **Start the server**:
 In the server terminal, run the following command: `./server 12345` with `12345` as an example port number.
2. **Connect with the client**:
In the client terminal, start your client(s) by connecting to the server:  `./client 127.0.0.1 12345`

A prompt will be enabled in the clients for you to type and press enter once your message is finished. These messages should be echoed back by the server, indicating that the communication is successful.

### Expected Server Outputs
The echoed messages in the server terminal should be as follows:
- Starting connection to socket
- Listening to the desired port
- Accepted connection at IP address
- New child process ID is spawned to handle communication with the client
- Received message
- Echoed messages
