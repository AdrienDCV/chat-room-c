#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 2048
#define CHAT_ROOM_SIZE 5
#define USERNAME_LENGTH 255
#define LOGS_FILE_NAME_SIZE 128
    
typedef struct
{
    int sockfd;
    int uid;
    char username[USERNAME_LENGTH];
} User;

int uid = 1;
int nb_users;
User *users[CHAT_ROOM_SIZE];

char logs_file_name[64] = "chat-room-logs";
FILE *file_pointer;

pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_socket(int *server_fd) {
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("LOG-ERROR : init_socket: could not open the socket\n");
        exit(EXIT_FAILURE);
    }
    printf("LOG-INFO : init_socket : Socket opened\n");
}

int open_logs_file() {
    if (file_pointer != NULL) {
        return 0;
    }

    char log_file_name[LOGS_FILE_NAME_SIZE];
    char log_file_time[64];

    time_t current_time;
    time(&current_time);
    struct tm *local_time = localtime(&current_time);
    strftime(log_file_time, sizeof(log_file_time), "-%Y-%m-%d", local_time);
    snprintf(log_file_name, sizeof(log_file_name), "chat-room-log%s.txt", log_file_time);

    file_pointer = fopen(log_file_name, "a");

    if (file_pointer == NULL) {
        perror("SERVER-ERROR: open_logs: Could not open logs file.");
        return 1;
    }

    return 0;
}

int close_logs_file() {
    if (file_pointer != NULL) {
        if (fclose(file_pointer) == 0) {
            file_pointer = NULL;
            return 0;
        } else {
            return 1;
        }
    }
    return 0;
}

void write_logs(char *line) {
    pthread_mutex_lock(&server_mutex);


    if (open_logs_file() == 0) {
        line[strlen(line) - 1] = '\0';
        fprintf(file_pointer, "%s\n", line);
        close_logs_file();
    } else {
        printf("SERVER-ERROR: write_logs: Could not right in logs file.");
    }

    pthread_mutex_unlock(&server_mutex);
}

void broadcast_message(char *message){
	pthread_mutex_lock(&server_mutex);

	for(int i=0; i<CHAT_ROOM_SIZE; ++i){
		if(users[i]){
			if(write(users[i]->sockfd, message, strlen(message)) < 0){
				perror("LOG-ERROR : send_message : could not send message\n");
				break;
			}
		}
	}

	pthread_mutex_unlock(&server_mutex);

}

void add_user_in_room(User* user) {
    pthread_mutex_lock(&users_mutex);

	for(int i = 0; i < CHAT_ROOM_SIZE; i++){
		if(!users[i]) {
			users[i] = user;
            break;
		}
	}

    pthread_mutex_unlock(&users_mutex);
}

void remove_user_from_list(int uid) {
    pthread_mutex_lock(&users_mutex);

	for(int i = 0; i < CHAT_ROOM_SIZE; i++){
        if (users[i] && users[i]->uid == uid) {
            users[i] = NULL;
            printf("LOG-INFO : remove_user_from_room : user removed\n");
            break;
        }
	}
    pthread_mutex_unlock(&users_mutex);
}

void remove_user_from_room(User *user) {
    close(user->sockfd);
    remove_user_from_list(user->uid);
  	free(user);
  	nb_users--;
}

User* create_user(int client_sock_fd) {
    User *user = (User *)malloc(sizeof(user));
    user->sockfd = client_sock_fd;
    user->uid = uid++;
    nb_users++;

    return user;
}

void listen_username(User *user, char *username, char *buffer) {
    if (recv(user->sockfd, username, USERNAME_LENGTH, 0) > 0) {
        strcpy(user->username, username);
        sprintf(buffer, "SERVER-INFO : %s has joined the chat\n", user->username);
        printf("%s", buffer);
        broadcast_message(buffer);
        write_logs(buffer);
    }
}

void *listen_to_client(void* arg) {
    char buffer[BUFFER_SIZE];
    char username[USERNAME_LENGTH];
    time_t current_time;
    int *client_socket_fd = (int *)arg;

    User* user = create_user(*client_socket_fd);
    add_user_in_room(user);

    listen_username(user, username, buffer);
    bzero(buffer, BUFFER_SIZE);

    int should_run = 1;
    while (should_run) {
        
        int receive = recv(user->sockfd, buffer, BUFFER_SIZE, 0);
        if (receive > 0){
            if (strcmp(buffer, "exit") == 0) {
                sprintf(buffer, "SERVER-INFO : %s has left the chat\n", user->username);
                broadcast_message(buffer);
                should_run = 0;
            } else if (strlen(buffer) > 0){
				broadcast_message(buffer);
				printf("%s", buffer);
			}

            write_logs(buffer);
            bzero(buffer, BUFFER_SIZE);
		}
    }

    remove_user_from_room(user);
 	pthread_detach(pthread_self());

    return NULL;

}

int main(int argc, char const* argv[]) {

    int socket_fd = 0, client_sock_fd = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int opt = 1;
    pthread_t listen_thread, send_thread;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    init_socket(&socket_fd);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("LOG-ERROR: setsockopt: could not set socket options properly");
        return EXIT_FAILURE;
    }
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("LOG-ERROR: bind: could not properly bind the socket to IP address");
        return EXIT_FAILURE;
    }

    if (listen(socket_fd, CHAT_ROOM_SIZE) < 0) {
        perror("LOG-ERROR: listen: Socket listening failed");
        return EXIT_FAILURE;
	}

    if (open_logs_file() == EXIT_FAILURE) {
        printf("SERVER-ERROR: open_logs_file: Could not open logs file.");
        return EXIT_FAILURE;
    }


    printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
    printf("≈≈≈≈≈≈≈≈≈ FISA3 - CHAT ROOM ≈≈≈≈≈≈≈≈≈\n");
    printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
    int should_run = 1;
    while (should_run) {

        socklen_t client_addr_len = sizeof(client_addr);
        client_sock_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);

        if ((nb_users + 1) == CHAT_ROOM_SIZE) {
            printf("SERVER-INFO : The chat room is full. Connection denied.");
            close(client_sock_fd);
            continue;
        }

        pthread_create(&listen_thread, NULL, &listen_to_client, (void*)&client_sock_fd);

        sleep(1);

    }

    return EXIT_SUCCESS;
}
