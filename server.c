#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int listen_socket;
int should_run = 1;

void* listen_to_client(void* arg) {
    char buffer[BUFFER_SIZE];
    while (should_run) {
        ssize_t valread = read(listen_socket, buffer, BUFFER_SIZE - 1);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("%s\n", buffer);
            send(listen_socket, buffer, strlen(buffer), 0);
        } else if (valread == 0) {
            printf("Client disconnected.\n");
            should_run = 0;
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

        if (send(listen_socket, fullMsg, strlen(fullMsg), 0) > 0) {
            printf("%s\n", fullMsg);
        } else {
            perror("Send error");
            should_run = 0;
            break;
        }
        i++;
    }
    return NULL;
}

int main(int argc, char const* argv[]) {
    int send_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    if ((send_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(send_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(send_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(send_socket, 3) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    if ((listen_socket = accept(send_socket, (struct sockaddr*)&address, &addrlen)) < 0) {
        perror("Accept");
        exit(EXIT_FAILURE);
    }
    printf("Client connected.\n");

    pthread_t listen_thread, send_thread;
    pthread_create(&listen_thread, NULL, listen_to_client, NULL);
    pthread_create(&send_thread, NULL, send_test_message, NULL);

    pthread_join(listen_thread, NULL);
    pthread_join(send_thread, NULL);

    close(listen_socket);
    close(send_socket);

    return 0;
}