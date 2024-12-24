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

int socket_file_descriptor = 0;
int disconnection_flag = 0;
char username[USERNAME_SIZE];

void remove_carriage_return_char (char* message);
int read_username();
int connect_to_room();
void send_message();
void listen_server_message();
int main(int argc, char const* argv[]);

void remove_carriage_return_char (char* message) {
    int message_length = strlen(message) - 1;
    if (message[message_length] == '\n') {
      message[message_length] = '\0';
    }
}

int read_username() {
    printf("CLIENT: Enter your username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        printf("CLIENT: Error reading input.\n");
        return 1;
    } else if (strlen(username) < 2) {
        printf("CLIENT-ERROR: Username must be at least 1 character long\n");
        return 1;
    } else if (strlen(username) > USERNAME_SIZE && username[strlen(username) - 1] != '\n') {
        fprintf(stderr, "CLIENT-ERROR: Username is too long. Maximum length is %d characters.\n",(USERNAME_SIZE-1));
        return 1;
    }

    username[strlen(username) - 1] = '\0';
    return 0;

}

int connect_to_room() {
    send(socket_file_descriptor, username, USERNAME_SIZE, 0);
    char error_message[MSG_SIZE];
    int rcv = recv(socket_file_descriptor, error_message, MSG_SIZE, 0);
    if (rcv == 65) {
        printf("%s", error_message);
        disconnection_flag = 0;
        return 1;
    }

    return 0;
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
            send(socket_file_descriptor, buffer, strlen(buffer), 0);
            disconnection_flag = 1;
        } else if (strlen(message) > 0) {
            time(&current_time);
            struct tm *local_time = localtime(&current_time);
            strftime(message_prefix, sizeof(message_prefix), "(%d-%m-%Y/%H:%M) ", local_time);
            strcat(message_prefix, username);

            snprintf(buffer, sizeof(buffer), "%s : %s\n", message_prefix, message);

            send(socket_file_descriptor, buffer, strlen(buffer), 0);
        }

        bzero(message, MSG_SIZE);
        bzero(buffer, MSG_SIZE);
    }
}

void listen_server_message() {
    
    char message[MSG_SIZE];
    char buffer[BUFFER_SIZE];

    int should_run = 1;
    while (should_run) {
        
        int received = recv(socket_file_descriptor, message, MSG_SIZE, 0);
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

    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) {
        printf("CLIENT-ERROR: socket: could not open the socket\n");
        return EXIT_FAILURE;
    }

    if (inet_pton(AF_INET, IP_SERVER, &server_addr.sin_addr)<= 0) {
        printf("CLIENT-ERROR: inet_pton: invalid address or address not supported\n");
        return EXIT_FAILURE;
    }

    if (connect(socket_file_descriptor, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("CLIENT-ERROR: connect: connection Failed\n");
        return EXIT_FAILURE;
    }

    int valid_username = 1;
    while (valid_username != 0) {
        if (read_username() == 0) {
            valid_username = 0;
        }
    }

    int connected_to_room = connect_to_room();
    if (connected_to_room == 0) {
        fflush(stdout);
        printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
        printf("≈≈≈≈≈≈≈≈≈ FISA3 - CHAT ROOM ≈≈≈≈≈≈≈≈≈\n");
        printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
        printf("CLIENT-INFO: You have joined the chat room\n");
    } else {
        printf("CLIENT-ERROR: Closing connection to server.\n");
        close(socket_file_descriptor);
        return EXIT_FAILURE;
    }
    
    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, (void *) send_message, NULL) != 0){
        printf("CLIENT-ERROR: pthread: could not create thread\n");
        return EXIT_FAILURE;
    }

    pthread_t receive_msg_thread;
    if(pthread_create(&receive_msg_thread, NULL, (void *) listen_server_message, NULL) != 0){
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
    
    close(socket_file_descriptor);

    return EXIT_SUCCESS;
}