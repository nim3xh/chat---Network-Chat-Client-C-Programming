#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define NAME_SIZE 64

void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void sendNameChange(int client_socket, const char *oldName, const char *newName) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "%s has changed their name to %s", oldName, newName);
    send(client_socket, message, strlen(message), 0);
}

void sendQuitMessage(int client_socket, const char *name) {
    char quitMessage[BUFFER_SIZE];
    snprintf(quitMessage, sizeof(quitMessage), "quit %s", name);
    send(client_socket, quitMessage, strlen(quitMessage), 0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip_address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];
    char clientName[NAME_SIZE] = "User";  // Default name

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(argv[1]);
    server_address.sin_port = htons(atoi(argv[2]));

    // create a connection with the server
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");

    // Set the socket to non-blocking mode
    setNonBlocking(STDIN_FILENO);
    setNonBlocking(client_socket);

    while (1) {
        // Input a message from the user (if any)
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Null-terminate the received data
            printf("%s\n", buffer);
        } else if (bytes_received == 0) {
            // Server disconnected
            printf("Server disconnected unexpectedly. Exiting...\n");
            break;  // Exit the loop
        } else if (errno != EAGAIN) {
            perror("Receive error");
            printf("Server disconnected unexpectedly. Exiting...\n");
            break;  // Exit the loop on error
        }

        // Send the message to the server (if any)
        memset(buffer, 0, sizeof(buffer));
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
            buffer[strcspn(buffer, "\n")] = '\0';  // Remove the newline character

            // Check if the message is the "quit" command
            if (strcmp(buffer, "quit") == 0) {
                printf("Sent quit message. Exiting...\n");

                 // Notify the server that the client is quitting
                sendQuitMessage(client_socket, clientName);

                break;  // Exit the loop
            } else if (strncmp(buffer, "name", 4) == 0) {
                // Handle "name" command
                char *newName = strtok(buffer, " ");
                newName = strtok(NULL, " ");
                if (newName != NULL) {
                    char oldName[NAME_SIZE];
                    strncpy(oldName, clientName, sizeof(oldName));

                    snprintf(clientName, sizeof(clientName), "%s", newName);
                    printf("%s has changed their name to %s\n", oldName, clientName);

                    // Notify the server about the name change
                    sendNameChange(client_socket, oldName, clientName);
                } else {
                    printf("Invalid name format. Usage: name <new_name>\n");
                }
            } else {
                // Send regular messages to the server
                char fullMessage[BUFFER_SIZE + NAME_SIZE + 3];  // Buffer for name + message + ": "
                snprintf(fullMessage, sizeof(fullMessage), "%s: %s", clientName, buffer);
                send(client_socket, fullMessage, strlen(fullMessage), 0);
            }
        }
    }

    // Close the socket
    close(client_socket);

    return 0;
}
