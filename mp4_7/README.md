# ECEN-602-Machine-Problem-4: Proxy Server

- **Isabel Moore**: Developed the source code for proxy.c, client.c, and proxy.h, optimizing the implementation specifically for Test Case 1. Integrated a ChatGPT-enhanced version and conducted testing for Test Cases 4-6.
- **Yiyang Yan**: Implemented cache-related helper functions, tested the original code, and wrote the report.
- **Shubham Kumar**: Re-ran all the testcases before final closure.

## Overview:
This project implements a **Proxy Server** using TCP. The server handles client requests, caches responses, and blocks certain URLs based on predefined lists. It supports handling of multiple clients using `pthread` for concurrency.

## Getting Started
### Clone repository
Use the following command to clone the repository: `git@github.com:isabelmoore/ECEN-602-Machine-Problems.git`
Ensure you have set up an SSH key for your GitHub account to use SSH. 

### Compiling Code
Run `make clean` to remove any previously compiled binaries.
Then, run `make all` to compile the source code.

## Running Application
1. **Start server**:
In the server terminal, run the following command: `./proxy <ip> <port>` with `<ip>` being the server IP address and `<port>` being the port number.

2. **Connect Using a Client**:
Use the provided client program to connect to the server: `./client <proxy address> <proxy port> <URL to retrieve>`.

## Code Architecture
### Server
- The server listens for incoming client connections and spawns a new thread for each client.
- It checks if the requested URL is blocked or cached.
- If blocked, it sends a "403 Forbidden" response.
- If cached, it serves the cached content.
- If not cached, it fetches the content from the web server, caches it, and then serves it to the client.

### Client
- The client connects to the proxy server and sends an HTTP GET request for the specified URL.
- It receives the response from the proxy server and saves it to a file.

## Errata & Error Handling
- The server currently does not implement a timeout mechanism for client requests.
- Error handling includes sending appropriate HTTP error responses when a URL is blocked or when there are issues fetching content from the web server.
