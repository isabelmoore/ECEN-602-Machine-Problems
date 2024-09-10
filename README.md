# ECEN-602-Machine-Problem-1: TCP Echo Server and Client

## Team Members and Roles ##
- **Isabel Moore**: Set up the GitHub repository and wrote the core server (`server.c`) and client (`client.c`) programs. Also implemented the `echos` and `echo` aliases in the `Makefile` for easier execution of the server and client.
 - **Yiyang Yan**: Created the project report and conducted all the test cases.
- **Shubham Kumar**: (to be added)

## Overview
To run TCP server-client application on Ubuntu 22.04, this server will handle incoming connections and echo back any messages received from client.

## Getting Started
### Clone repository
Use following command to clone repository: 
`git clone git@github.com/ECEN-602-Machine-Problem-1.git`
 Ensure you have set up an SSH key for your GitHub account to use SSH. 

### Compiling Code
Run `make all` to compile source code and scripts `echos` and `echo` to simplify running server and client programs.

## Running Application
1. **Start server**:
 In server terminal, run following command: `./server 12345` with `12345` as an example port number.
2. **Connect with client**:
In client terminal, start your client(s) by connecting to server:  `./client 127.0.0.1 12345`

The client terminal will prompt you to type a message, which will be echoed back by server.

To streamline your commands, you can use aliases `echos` and `echo` as substitutes for `./server` and `./client` respectively. However, if you've encountered issues with `.bashrc` not effectively adding the current directory to your PATH, you may need to manually execute `export PATH=$PATH:$(pwd)` in your terminal before running `echos 12345` for the server and `echo 127.0.0.1 12345` for the client. Also, when operating on Linux, using the echo command will simply repeat the provided IP address and port number you specify.

## Code Architecture
### Server
- server listens for client connections and forks a child process for each connection, allowing multiple clients to connect simultaneously.
- System calls like `recv()` and `writen()` handle errors, retrying operations if interrupted by signals (`EINTR`).
- A signal handler (`SIGCHLD`) prevents zombie processes by cleaning up terminated child processes.
### Client
- client connects to server using TCP and communicates by sending messages, which server echoes back.


## Errata & Error Handling
- **EINTR**: `writen()` and `readline()` functions retry operations if interrupted by signals.
- **Socket Errors**: Errors from functions like `socket()`, `bind()`, `accept()`, and `recv()` are handled gracefully, printing error messages and exiting as necessary.
- **Zombie Process Prevention**: Server uses `waitpid()` to clean up terminated child processes and avoid zombie processes.
- **Multiple Clients**: Server handles multiple clients, but performance may degrade under heavy load.
- **Client Disconnection**: Server gracefully handles client disconnection by detecting EOF, but unexpected network issues may cause clients to hang.


