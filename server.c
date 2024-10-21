// Server side C program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_LEN 1024

void init_socket(int *server_fd);

void read_arguement();

void init_socket(int *server_fd) {

    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("ERROR: socket: could not open the socket");
        exit(EXIT_FAILURE);
    }

}

void read_argument() {

}


int main(int argc, char const* argv[]) {

    char msg[MAX_LEN];
    int messages_limit = 3;
    char messages[MAX_LEN][MAX_LEN];
    int index = 0;

    int should_run = 1;
    while (should_run) {
        if (!fgets(msg, sizeof(msg), stdin)) {
            perror("ERROR: error while reading line");
            exit(EXIT_FAILURE);
        }

        strcpy(messages[index], msg);
        index++;

        for (int i = 0; i < index; i++) {
            printf("i = %d\n", i);
            printf("content: %s\n", messages[i]);
        }
    }

   /*  int server_fd, new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = { 0 };
    char command[MAX_LEN];


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

        if ((new_socket
            = accept(server_fd, (struct sockaddr*)&address,
                    &addrlen))
            < 0) {
            perror("ERROR: accept: could not connect client");
            exit(EXIT_FAILURE);
            }
        // create new thread at new connection

        // read socket content and write it in buffer 1023 bytes max
        // valread = read(new_socket, buffer, 1024 - 1);


        // terminator at the end
        printf("%s\n", command);
        // send(new_socket, "hello", strlen("hello"), 0);
    }
    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    close(server_fd); */

    return 0;
}
