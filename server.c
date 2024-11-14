#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "queue.h"

#define PORT 8080

void init_socket(int *server_fd);

int main(int argc, char const * argv[]);

void init_socket(int *server_fd) {
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("ERROR: socket: could not open the socket");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const* argv[]) {

    int server_fd, new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[MAX_LEN_MSG] = { 0 };

    Queue queue;
    init_messages_queue(&queue);

    init_socket(&server_fd);

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("ERROR: setsockopt: could not set socket options properly");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("ERROR: bind: could not properly bind the socket to IP address");
        exit(EXIT_FAILURE);
        }

    int should_run = 1;
    while (should_run) {

        if (listen(server_fd, 5) < 0) {
            perror("ERROR: listen: could not listen the socket because the queue is full");
            exit(EXIT_FAILURE);
        }

        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("ERROR: accept: could not connect client");
            exit(EXIT_FAILURE);
        }

        // create new thread at new connection

        // read socket content and write it in buffer 1023 bytes max
        read(new_socket, buffer, 1024 - 1);

        put_in_queue(&queue, buffer);

        print_queue(&queue);
    }

    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    close(server_fd);

    return 0;
}
