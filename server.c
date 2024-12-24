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
    int socket_file_descriptor;
    int id;
    char username[USERNAME_LENGTH];
} User;

int id = 1;
int nb_users = 0;
User *users[CHAT_ROOM_SIZE];

char logs_file_name[LOGS_FILE_NAME_SIZE] = "chat-room-logs";
FILE *file_pointer;

pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_socket(int *server_fd);
int open_logs_file();
int close_logs_file();
void write_logs(char *line);
void broadcast_message(char *message);
User* create_user(int client_sock_fd, char *username);
int add_user_in_list(User *user);
User* add_user_to_room(int client_socket_file_descriptor, char *username);
int remove_user_from_list(int id);
void remove_user_from_room(User *user);
int listen_username(int client_sock_fd, char *username);
void *listen_to_client(void* arg);
int main(int argc, char const* argv[]);

void init_socket(int *server_file_descriptor) {
    if ((*server_file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "SERVER-ERROR : init_socket: could not open the socket\n");
        exit(EXIT_FAILURE);
    }
    printf("SERVER-INFO : init_socket : Socket opened\n");
}

int open_logs_file() {
    if (file_pointer != NULL) {
        return 0;
    }

    char log_file_name[LOGS_FILE_NAME_SIZE];
    char log_file_time[LOGS_FILE_NAME_SIZE];

    time_t current_time;
    time(&current_time);
    struct tm *local_time = localtime(&current_time);
    strftime(log_file_time, sizeof(log_file_time), "-%Y-%m-%d", local_time);
    snprintf(log_file_name, sizeof(log_file_name), "chat-room-log%s.txt", log_file_time);

    file_pointer = fopen(log_file_name, "a");

    if (file_pointer == NULL) {
        fprintf(stderr, "SERVER-ERROR: open_logs: Could not open logs file.");
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
            fprintf(stderr, "SERVER-ERROR: close_logs_file: Could not close logs file properly.");
            return 1;
        }
    }
    fprintf(stderr, "SERVER-ERROR: close_logs_file: No file descriptor");
    return 1;
}

void write_logs(char *line) {
    pthread_mutex_lock(&server_mutex);

    if (open_logs_file() == 0) {
        line[strlen(line) - 1] = '\0';
        fprintf(file_pointer, "%s\n", line);
        if (close_logs_file() == 1) {
            fprintf(stderr, "SERVER-ERROR: write_logs: An error occured. Could not close logs file");
        }
    } else {
        fprintf(stderr, "SERVER-ERROR: write_logs: Could not write in logs file. Logs file could not be opened");
    }

    pthread_mutex_unlock(&server_mutex);
}

void broadcast_message(char *message) {
	pthread_mutex_lock(&server_mutex);

	for(int i=0; i<CHAT_ROOM_SIZE; ++i){
		if(users[i]){
			if(write(users[i]->socket_file_descriptor, message, strlen(message)) < 0){
				fprintf(stderr, "SERVER-ERROR : send_message : could not send message\n");
				break;
			}
		}
	}

	pthread_mutex_unlock(&server_mutex);
}

User* create_user(int client_sock_fd, char *username) {
    User *user = (User *)malloc(sizeof(user));
    user->socket_file_descriptor = client_sock_fd;
    strcpy(user->username, username);

    return user;
}

int add_user_in_list(User* user) {
    int success = 0;
    pthread_mutex_lock(&users_mutex);

	for(int i = 0; i < CHAT_ROOM_SIZE; i++){
		if(!users[i]) {
			users[i] = user;
            user->id = id++;
            nb_users++;
            break;
		} else {
            success = 1;
        }
	}

    pthread_mutex_unlock(&users_mutex);
    return success;
}

User* add_user_to_room(int client_socket_file_descriptor, char *username) {
    char buffer[BUFFER_SIZE];

    User *user = create_user(client_socket_file_descriptor, username);

    if (add_user_in_list(user) == 0) {
        printf("SERVER-INFO: add_user_in_list: User added to users list\n");
        sprintf(buffer, "SERVER-INFO: %s has joined the chat\n", user->username);
        printf("%s", buffer);
        broadcast_message(buffer);
        write_logs(buffer);

        return user;
    } else {
        strcpy(buffer, "SERVER-ERROR: add_user_in_list: Could not add user to users list\n");
        fprintf(stderr, "%s", buffer);
        pthread_mutex_lock(&users_mutex);

        write(user->socket_file_descriptor, buffer, strlen(buffer));
        write_logs(buffer);

        pthread_mutex_unlock(&users_mutex);

        free(user);
        return NULL;
    }
}

int remove_user_from_list(int id) {
    int success = 0;
    pthread_mutex_lock(&users_mutex);

	for(int i = 0; i < CHAT_ROOM_SIZE; i++){
        if (users[i] && users[i]->id == id) {
            users[i] = NULL;
            break;
        } else {
            success = 1;
        }
	}

    pthread_mutex_unlock(&users_mutex);

    return success;
}

void remove_user_from_room(User *user) {
    char buffer[BUFFER_SIZE];

    if (remove_user_from_list(user->id) == 0) {
        close(user->socket_file_descriptor);
        free(user);
  	    nb_users--;
        printf("SERVER-INFO : remove_user_from_room: User removed from chat room.\n");
        sprintf(buffer, "SERVER-INFO : %s has left the chat\n", user->username);
        broadcast_message(buffer);
        pthread_detach(pthread_self());
    } else {
        fprintf(stderr, "%s", " ERROR: remove_user_from_room: User could not be removed from chat room.\n");
    }
}

int listen_username(int client_sock_fd, char *username) {
    char buffer[BUFFER_SIZE];

    if (recv(client_sock_fd, username, USERNAME_LENGTH, 0) > 0 && strlen(username) > 0) {
        return 0;
    }
    return 1;
}

void *listen_to_client(void* arg) {
    char buffer[BUFFER_SIZE];
    char username[USERNAME_LENGTH];
    User *user;
    time_t current_time;
    int *client_socket_file_descriptor = (int *)arg;

    if (listen_username(*client_socket_file_descriptor, username) != 0) {
        fprintf(stderr, "SERVER-ERROR: listen_username: Could not retrieve username.\n");
    }

    user = add_user_to_room(*client_socket_file_descriptor, username);
    if (user == NULL) {
        close(*client_socket_file_descriptor);
        pthread_detach(pthread_self());

        return NULL;
    }

    bzero(buffer, BUFFER_SIZE);
    int should_run = 1;
    while (should_run) {
        
        int receive = recv(user->socket_file_descriptor, buffer, BUFFER_SIZE, 0);
        if (receive > 0){
            if (strcmp(buffer, "exit") == 0) {
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

    return NULL;
}

int main(int argc, char const* argv[]) {
    int socket_file_descriptor = 0, client_sock_file_descriptor = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int opt = 1;
    pthread_t listen_thread, send_thread;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    init_socket(&socket_file_descriptor);

    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "SERVER-ERROR: setsockopt: could not set socket options properly");
        return EXIT_FAILURE;

    }
    if (bind(socket_file_descriptor, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "SERVER-ERROR: bind: could not properly bind the socket to IP address");
        return EXIT_FAILURE;
    }

    if (listen(socket_file_descriptor, CHAT_ROOM_SIZE) < 0) {
        fprintf(stderr, "SERVER-ERROR: listen: Socket listening failed");
        return EXIT_FAILURE;
	}

    if (open_logs_file() == EXIT_FAILURE) {
        fprintf(stderr, "SERVER-ERROR: open_logs_file: Could not open logs file.");
        return EXIT_FAILURE;
    }

    printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
    printf("≈≈≈≈≈≈≈≈≈ FISA3 - CHAT ROOM ≈≈≈≈≈≈≈≈≈\n");
    printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
    
    int should_run = 1;
    while (should_run) {

        socklen_t client_addr_len = sizeof(client_addr);
        client_sock_file_descriptor = accept(socket_file_descriptor, (struct sockaddr*)&client_addr, &client_addr_len);

        if ((nb_users + 1) == CHAT_ROOM_SIZE) {;
            char *error_message = "SERVER-INFO : The chat room is full. Connection denied.";
            write(client_sock_file_descriptor, error_message, strlen(error_message));
            close(client_sock_file_descriptor);
            continue;
        }

        pthread_create(&listen_thread, NULL, &listen_to_client, (void*)&client_sock_file_descriptor);

    }

    exit(EXIT_SUCCESS);
}
