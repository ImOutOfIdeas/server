#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#define WELCOME_MESSAGE "======================\nWelcome to the server!\n======================\n"
#define MAX_CLIENTS 2

struct pollfd clients[MAX_CLIENTS];
char welcome_message[] = WELCOME_MESSAGE;

int main(int argc, char** argv) {
    if (2 != argc) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    
    char *ip = "192.168.1.100";
    int port = atoi(argv[1]);
    if (1024 > port || 65535 < port) {
        printf("Please enter a port between 1024 and 65535\n");
        return -1;
    }

    // Initialize File Descriptor Array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;         // Ignore the fd
        clients[i].events = 0;      // Don't detect anything
        clients[i].revents = 0;     // No return events yet
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == server_fd) {
        perror("socket");
        return -1;
    }
    if (-1 == bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("bind");
        return -1;
    }
    if (-1 == listen(server_fd, 10)) {
        perror("listen");
        return -1;
    }
    printf("Server Listening for Connections on port %d...\n", port);

    clients[0].fd = server_fd;  // Put server into File Descriptor Array (for polling)
    clients[0].events = POLLIN; // Detect when there is data sent to the server

    for (;;) {
        // Check connected sockets for changes
        if (-1 == poll(clients, MAX_CLIENTS, -1)) {
            perror("poll");
        }

        // The POLLIN flag is set (someone is waiting to connect)
        if (clients[0].revents & POLLIN) {
            struct sockaddr_in new_client_addr;
            socklen_t new_client_size = sizeof(new_client_addr);

            int new_client_fd = accept(server_fd, (struct sockaddr*)&new_client_addr, &new_client_size);
            if (-1 == new_client_fd) {
                perror("accept");
            } else {
                printf("Connection from: %s\n", inet_ntoa(new_client_addr.sin_addr));

                int new_client_fd = -1;

                for (int i = 1; i < MAX_CLIENTS; i++) {
                    if (-1 == clients[i].fd) {  // No socket at i-th index
                        new_client_fd = i;
                        break;
                    }
                }

                if (-1 != new_client_fd) {    // Spot for client found! 
                    printf("Client assigned to id: %d\n", new_client_fd);

                    clients[new_client_fd].fd = new_client_fd;
                    clients[new_client_fd].events = POLLIN;
                    clients[new_client_fd].revents = 0;

                    // TODO: BUG MIGHT BE IN THESE SENDS
                    int bytes_sent = send(clients[new_client_fd].fd, welcome_message, strlen(welcome_message), 0);
                    if (-1 == bytes_sent) perror("send");
                } else {
                    char failure_message[] = "The server is full. Connection rejected\n";

                    printf("%s\n", failure_message);

                    int bytes_sent = send(clients[0].fd, failure_message, strlen(failure_message), 0);
                    if (-1 == bytes_sent) perror("send");

                    if (-1 == close(new_client_fd)) perror("close");
                }
            }
        }

        // Handle connected socket events
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1 && clients[i].revents > 0) {
                if (clients[i].revents & POLLIN) { // Client has data to be read
                                                   // TODO: Read users stuff
                } else if (clients[i].revents & POLLERR || clients[i].revents & POLLHUP) {
                    printf("Client %d disconnected", i);
                    close(clients[i].fd);
                    clients[i].fd = -1;
                }
                // ... (handle other events like POLLOUT if needed)
            }
        }
    }

    return 0;
}
