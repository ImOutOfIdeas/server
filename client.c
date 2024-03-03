#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_INPUT_SIZE 512

void print_addr(struct sockaddr_in addr);

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

    // Setup server info
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // Create client socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == client_fd) {
        perror("socket");
        return -1;
    }

    // Connect to server
    if (-1 == connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("connect");
        return -1;
    }

    struct pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN; // Monitor for server data

    char buffer[MAX_INPUT_SIZE];
    int message_size;
    int bytes_received;
    while (1) {
        int poll_ret = poll(&client_pollfd, 1, -1); // Monitor the client socket
        if (poll_ret == -1) {
            perror("poll");
            break;
        }

        memset(buffer, 0, MAX_INPUT_SIZE);

        if (client_pollfd.revents & POLLIN) { // Read if server has sent data
            bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

            if (bytes_received == -1) {
                perror("recv");
                break;
            } else if (bytes_received == 0) {
                printf("Server disconnected.\n");
                break;
            }

            buffer[bytes_received] = '\0'; // Null-terminate the received data

            printf("Received from server: %s\n", buffer);
        }

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            perror("fgets");
            continue;
        }
        message_size = strcspn(buffer, "\n");
        buffer[message_size] = '\0';

        send(client_fd, &buffer, message_size, 0);
    }

    return 0;
}
