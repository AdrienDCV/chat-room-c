#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#include "queue.h"
#include "users_list.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define CHAT_ROOM_SIZE 5
    

int new_socket;
int should_run = 1;
Queue queue;
UsersList users_list;

void init_socket(int *server_fd);
void* listen_to_client(void* arg);
void* send_test_message(void* arg);

int main(int argc, char const * argv[]);

void init_socket(int *server_fd) {
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("ERROR: socket: could not open the socket");
        exit(EXIT_FAILURE);
    }
}

void* listen_to_client(void* arg) {
    char buffer[BUFFER_SIZE];
    while (should_run) {
        ssize_t valread = read(new_socket, buffer, BUFFER_SIZE - 1);
        if (valread > 0) {
            buffer[valread] = '\0';
            put_in_queue(&queue, buffer);

            printf("%s\n", buffer);
            send(new_socket, buffer, strlen(buffer), 0);
        } else if (valread == 0) {
            printf("Client disconnected.\n");
            break;
        } else {
            perror("Read error");
            should_run = 0;
            break;
        }
    }
    return NULL;
}

void* send_test_message(void* arg) {
    int i = 0;
    while (should_run) {
        sleep(2);
        char fullMsg[BUFFER_SIZE];
        char test_msg[BUFFER_SIZE];

        snprintf(test_msg, sizeof(test_msg), "Server test message %d", i);

        time_t current_time;
        time(&current_time);
        struct tm* local_time = localtime(&current_time);
        strftime(fullMsg, sizeof(fullMsg), "(%d-%m/%H:%M) Server: ", local_time);
        strcat(fullMsg, test_msg);

        if (send(new_socket, fullMsg, strlen(fullMsg), 0) > 0) {
            printf("%s\n", fullMsg);
        } else {
            perror("Send error |");
            should_run = 0;
            break;
        }
        i++;
    }
    return NULL;
}


int main(int argc, char const* argv[]) {

    int socket_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    pthread_t listen_thread, send_thread;

    init_socket(&socket_fd);
    init_messages_queue(&queue);
    init_users_list(&users_list);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("ERROR: setsockopt: could not set socket options properly");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("ERROR: bind: could not properly bind the socket to IP address");
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, CHAT_ROOM_SIZE) == 0) {
        printf("SERVER: Listenning\n");
    } else {
        perror("ERROR: listen: could not listen the socket because the queue is full\n");
        exit(EXIT_FAILURE);        
    }

    pid_t child_pid;
    while (should_run) {
        new_socket = accept(socket_fd, (struct sockaddr*)&address, &addrlen);
        if (new_socket < 0) {
            exit(1);
        }
        users_list.users[users_list.nb_users] = &new_socket;
        users_list.nb_users++;

        printf("Connection accepted\n");
        
        pthread_create(&listen_thread, NULL, listen_to_client, NULL);

    }

    /*
    pthread_t listen_thread, send_thread;
    pthread_create(&listen_thread, NULL, listen_to_client, NULL);
    pthread_create(&send_thread, NULL, send_test_message, NULL);

    pthread_join(listen_thread, NULL);
    pthread_join(send_thread, NULL);
    */

    // closing the connected socket
    close(new_socket);


    return 0;
}
