#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define NAME_SIZE 64

int userCounter = 0;

void broadcast(int sender, int client_sockets[], fd_set *active_fds, const char *message) {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (client_sockets[i] != -1 && client_sockets[i] != sender && FD_ISSET(client_sockets[i], active_fds)) {
		send(client_sockets[i], message, strlen(message), 0);
	}
    }
}

void sendWelcomeMessage(int new_client, const char *name) {
    char welcomeMessage[128];
    snprintf(welcomeMessage, sizeof(welcomeMessage), "Welcome, %s!", name);
    send(new_client, welcomeMessage, strlen(welcomeMessage), 0);
}

void broadcastNameChange(int sender, int client_sockets[], fd_set *active_fds, const char *oldName, const char *newName) {
    char message[128];
    snprintf(message, sizeof(message), "%s has changed their name to %s", oldName, newName);
    broadcast(sender, client_sockets, active_fds, message);
}

void generateDefaultName(char *name, int index) {
    snprintf(name, NAME_SIZE, "User%d", index);
     // Increment the userCounter for the next connected client
     userCounter++;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_socket, client_sockets[MAX_CLIENTS];
    struct sockaddr_in server_address, client_address;
    socklen_t addr_len = sizeof(client_address);
    char buffer[BUFFER_SIZE];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1]));

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %s...\n", argv[1]);

    fd_set active_fds, read_fds;
    FD_ZERO(&active_fds);
    FD_SET(server_socket, &active_fds);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        client_sockets[i] = -1;
    }

    

    while (1) {
        read_fds = active_fds;

        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Select failed");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket) {
                    // New client connected
                    int new_client = accept(server_socket, (struct sockaddr*)&client_address, &addr_len);
                    if (new_client != -1) {
                        printf("New client connected\n");

                        // Assign a default name to the new client
                        char defaultName[NAME_SIZE];
                        generateDefaultName(defaultName, userCounter);

                        // Broadcast the connection message to other clients
                        char connectMessage[128];
                        snprintf(connectMessage, sizeof(connectMessage), "%s has connected", defaultName);
                        broadcast(new_client, client_sockets, &active_fds, connectMessage);

                       

                        // Send welcome message to the new client
                        sendWelcomeMessage(new_client, defaultName);

                        for (int j = 0; j < MAX_CLIENTS; ++j) {
                            if (client_sockets[j] == -1) {
                                client_sockets[j] = new_client;
                                FD_SET(new_client, &active_fds);
                                break;
                            }
                        }
                    }
                } else {
                    ssize_t bytes_received = recv(i, buffer, sizeof(buffer), 0);
                    if (bytes_received > 0) {
                        buffer[bytes_received] = '\0';
                        printf("Received from client %d: %s\n", i, buffer);

                        if (strncmp(buffer, "quit", 4) == 0) {
                           // Extract the name from the "quit" message
                            char *name = strtok(buffer + 5, " ");  // Skip "quit " and get the name

                            // Notify other clients that the user has quit
                            char quitMessage[128];
                            snprintf(quitMessage, sizeof(quitMessage), "%s has quit", name);
                            broadcast(i, client_sockets, &active_fds, quitMessage);

                            printf("Client %d disconnected.\n", i);
                            close(i);
                            FD_CLR(i, &active_fds);

                            for (int j = 0; j < MAX_CLIENTS; ++j) {
                                if (client_sockets[j] == i) {
                                    client_sockets[j] = -1;
                                    break;
                                }
                            }
                        } else if (strncmp(buffer, "name", 4) == 0) {
                            char *newName = strtok(buffer, " ");
                            newName = strtok(NULL, " ");
                            if (newName != NULL) {
                                char oldName[NAME_SIZE];
                                snprintf(oldName, sizeof(oldName), "User%d", i);

                                // Broadcast the name change to other clients
                                broadcastNameChange(i, client_sockets, &active_fds, oldName, newName);
                            } else {
                                printf("Invalid name format. Usage: name <new_name>\n");
                            }
                        } else {
                            broadcast(i, client_sockets, &active_fds, buffer);
                        }
                    } else if (bytes_received == 0) {
                        printf("User %d disconnected.\n", userCounter);
                        close(i);
                        FD_CLR(i, &active_fds);

                        for (int j = 0; j < MAX_CLIENTS; ++j) {
                            if (client_sockets[j] == i) {
                                client_sockets[j] = -1;
                                break;
                            }
                        }
                    } else {
                        perror("Receive error");
                        close(i);
                        FD_CLR(i, &active_fds);
                    }
                }
            }
        }
    }

	return 0;
}
