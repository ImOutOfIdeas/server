#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#define WELCOME_MESSAGE "======================\nWelcome to the server!\n======================\n"
#define MAX_CLIENTS 3

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

    clients[0].fd = server_fd;  // Put server into File Descriptor Array (for client polling)
    clients[0].events = POLLIN; // Detect when there is data sent to the server

    for (;;) {
        // Check sockets for events
        if (-1 == poll(clients, MAX_CLIENTS, -1)) {
            perror("poll");
        }

        // Handle connected socket events
        for (int i = 1; i < MAX_CLIENTS; i++) {
            if (clients[i].revents & POLLIN) { // Client has data to be read
                char buffer[100];

                recv(clients[i].fd, &buffer, 100, 0);


            } else if (clients[i].revents & POLLERR || clients[i].revents & POLLHUP) {
                printf("Client %d disconnected", i);
                close(clients[i].fd);
                clients[i].fd = -1;
            }
            // ... (handle other events like POLLOUT if needed)
        }


        // Connect new clients (if socket is waiting to connect)
        if (clients[0].revents & POLLIN) {
            struct sockaddr_in new_client_addr;
            socklen_t new_client_size = sizeof(new_client_addr);

            int new_client_fd = accept(server_fd, (struct sockaddr*)&new_client_addr, &new_client_size);
            if (-1 == new_client_fd) {
                perror("accept");
                continue;
            }

            printf("Connection from: %s\n", inet_ntoa(new_client_addr.sin_addr));

            // Find spot in file desriptor array for new client
            int new_client_position = -1;
            for (int i = 1; i < MAX_CLIENTS; i++) {
                if (-1 == clients[i].fd) {
                    new_client_position = i;
                    break;
                }
            }

            // Spot for client found!
            if (-1 != new_client_position) {
                printf("Client assigned to id: %d\n", new_client_position);

                clients[new_client_position].fd = new_client_position;
                clients[new_client_position].events = POLLIN;
                clients[new_client_position].revents = 0;

                // TODO: BUG MIGHT BE IN THESE SEND CALLS
                int bytes_sent = send(new_client_fd, welcome_message, strlen(welcome_message), 0);
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

    return 0;
}
