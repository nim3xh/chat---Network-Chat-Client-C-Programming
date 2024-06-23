# Chat Server

A basic multi-client chat server in C using sockets, supporting non-blocking I/O, and client commands for name changes and quitting.

## Features

- **Multi-client support**: Handles up to 10 clients.
- **Non-blocking I/O**: Efficient handling of multiple clients with `select`.
- **Client commands**: Supports "name" to change usernames and "quit" to disconnect.
- **Message broadcasting**: Sends messages from one client to all other connected clients.

## Setup

### Prerequisites

- C compiler (e.g., `gcc`)
- Unix-like environment (Linux, macOS, or WSL for Windows)

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/nim3xh/chat---Network-Chat-Client-C-Programming.git
   ```

2. **Build the server and client**:
   ```bash
       gcc -o server server.c
       gcc -o client client.c
   ```

## Usage
### Starting the Server
  ```bash
          ./server <port>
```

- Replace <port> with the desired port number, e.g., 8080.

### Connecting Clients
 ```bash
          ./client <server_ip> <port>
```

### Commands
- name <new_name>: Change your username.
- quit: Disconnect from the server.

