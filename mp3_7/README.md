# ECEN-602-Machine-Problem-3: TFTP Server

## Team Members and Roles ##
- **Isabel Moore**: Created the Makefile and optimized the code using ChatGPT suggestions.
- **Yiyang Yan**: Implemented and tested TFTP server
- **Shubham Kumar**: 

## Getting Started
### Clone repository
Use following command to clone repository: `git@github.com:isabelmoore/ECEN-602-Machine-Problems.git`
Ensure you have set up an SSH key for your GitHub account to use SSH. 

### Compiling Code
Run `make all` to compile source code.

## Running Application
1. **Start server**:
In server terminal, run following command: `./server 127.0.0.1 12345` with `12345` being the port number.



## Code Architecture
### Server
- Spawn a child process whenever the parent process receives new message.
- Each child process is capable of handling read request from one client.

## Errata & Error Handling
- Currently no timeout mechanism is implemented.
