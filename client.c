#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>


#define PORT 8080
#define IP_SERVER "127.0.0.1"
#define MSG_SIZE 1024
#define BUFFER_SIZE 2048
#define USERNAME_SIZE 255

int sock_fd = 0;
int disconnection_flag = 0;
char username[USERNAME_SIZE];

void remove_carriage_return_char (char* message) {
    int message_length = strlen(message) - 1;
    if (message[message_length] == '\n') {
      message[message_length] = '\0';
    }
}

void send_message() {
    char message[MSG_SIZE];
    char buffer[BUFFER_SIZE];
    time_t current_time;

    char message_prefix[USERNAME_SIZE + 30];

    int should_run = 1;
    while (should_run) {

        fgets(message, MSG_SIZE, stdin);
        printf("\033[1A\033[2K\r");
        fflush(stdout);
        remove_carriage_return_char(message);
    
        if (strcmp(message, "exit") == 0) {
            sprintf(buffer, "%s", message);
            send(sock_fd, buffer, strlen(buffer), 0);
            disconnection_flag = 1;
        } else if (strlen(message) > 0) {
            time(&current_time);
            struct tm *local_time = localtime(&current_time);
            strftime(message_prefix, sizeof(message_prefix), "(%d-%m-%Y/%H:%M) ", local_time);
            strcat(message_prefix, username);

            snprintf(buffer, sizeof(buffer), "%s : %s\n", message_prefix, message);

            send(sock_fd, buffer, strlen(buffer), 0);
        }

        bzero(message, MSG_SIZE);
        bzero(buffer, MSG_SIZE);
    }
}

void receive_message() {
    
    char message[MSG_SIZE];
    char buffer[BUFFER_SIZE];

    int should_run = 1;
    while (should_run) {
        
        int received = recv(sock_fd, message, MSG_SIZE, 0);
        if (received > 0) {
            printf("%s", message);
            fflush(stdout);
        } else if (received == 0) {
            break;
        }
        memset(message, 0, sizeof(message));
    }

}

int main(int argc, char const* argv[]) {
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = inet_addr(IP_SERVER);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);


    printf("CLIENT: Enter your username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        fprintf(stderr, "CLIENT: Error reading input.\n");
        return EXIT_FAILURE;
    }
    if (strlen(username) > USERNAME_SIZE && username[strlen(username) - 1] != '\n') {
        fprintf(stderr, "CLIENT-ERROR: Username is too long. Maximum length is %d characters.\n", (USERNAME_SIZE-1));
        return EXIT_FAILURE;
    }
    username[strlen(username) - 1] = '\0';


    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        printf("CLIENT-ERROR: socket: could not open the socket\n");
        return EXIT_FAILURE;
    }

    if (inet_pton(AF_INET, IP_SERVER, &server_addr.sin_addr)<= 0) {
        printf("ERROR: inet_pton: invalid address or address not supported\n");
        return EXIT_FAILURE;
    }

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("ERROR: connect: connection Failed\n");
        return EXIT_FAILURE;
    }

    send(sock_fd, username, USERNAME_SIZE, 0);

    fflush(stdout);
    printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
    printf("≈≈≈≈≈≈≈≈≈ FISA3 - CHAT ROOM ≈≈≈≈≈≈≈≈≈\n");
    printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
    printf("CLIENT-INFO: You have joined the chat room\n");

    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, (void *) send_message, NULL) != 0){
        printf("CLIENT-ERROR: pthread: could not create thread\n");
        return EXIT_FAILURE;
    }

    pthread_t receive_msg_thread;
    if(pthread_create(&receive_msg_thread, NULL, (void *) receive_message, NULL) != 0){
        printf("CLIENT-ERROR: pthread: could not create thread\n");
        return EXIT_FAILURE;
    }
    
    int should_run = 1;
    while (should_run) {
        if (disconnection_flag) {
            printf("CLIENT-INFO: You have been successfuly disconnected from the room\n");
            break;
        }
    }
    
    close(sock_fd);

    return EXIT_SUCCESS;

}