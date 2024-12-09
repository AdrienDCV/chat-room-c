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
    
typedef struct
{
    int sockfd;
    int uid;
    char username[USERNAME_LENGTH];
} User;

int uid = 1;
int nb_users;
User *users[CHAT_ROOM_SIZE];

pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_socket(int *server_fd) {
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("ERROR: socket: could not open the socket");
        exit(EXIT_FAILURE);
    }
    printf("LOG-INFO : Socket opened\n");
}

void remove_carriage_return_char (char* msg, int length) {
  int i;
  for (i = 0; i < length; i++) {
    if (msg[i] == '\n') {
      msg[i] = '\0';
      break;
    }
  }
}

void send_message(char *s){
	pthread_mutex_lock(&users_mutex);

	for(int i=0; i<CHAT_ROOM_SIZE; ++i){
		if(users[i]){
			if(write(users[i]->sockfd, s, strlen(s)) < 0){
				perror("ERROR: write to descriptor failed");
				break;
			}
		}
	}

	pthread_mutex_unlock(&users_mutex);
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

void remove_user_froom_room(int uid) {
	pthread_mutex_lock(&users_mutex);

	for(int i = 0; i < CHAT_ROOM_SIZE; i++){
        if (users[i] && users[i]->uid == uid) {
            users[i] = NULL;
            printf("LOG-INFO : user removed\n");
            break;
        }
	}

	pthread_mutex_unlock(&users_mutex);
}

User* create_user(int client_sock_fd) {
    User *user = (User *)malloc(sizeof(user));
    user->sockfd = client_sock_fd;
    user->uid = uid++;

    return user;
}

void *listen_to_client(void* arg) {

    char buffer[BUFFER_SIZE];
    char username[USERNAME_LENGTH];
    time_t current_time;
    
    nb_users++;
    User *user = (User *)arg;

    if (recv(user->sockfd, username, USERNAME_LENGTH, 0) > 0) {
        strcpy(user->username, username);
        sprintf(buffer, "%s has joined the chat\n", user->username);
        printf("%s", buffer);
        send_message(buffer);
    }

    bzero(buffer, BUFFER_SIZE);

    int should_stop = 0;
    int should_run = 1;
    while (should_run) {

        if (should_stop) {
            break;
        }
        
        int receive = recv(user->sockfd, buffer, BUFFER_SIZE, 0);
        if (receive > 0){
            if (strcmp(buffer, "exit") == 0) {
                sprintf(buffer, "%s has left the chat\n", user->username);
                send_message(buffer);
                should_stop = 1;
            } else if (strlen(buffer) > 0){
                printf("ne devrait pas être là\n");
				send_message(buffer);

				remove_carriage_return_char(buffer, strlen(buffer));
				printf("%s\n", buffer);
			}

            bzero(buffer, BUFFER_SIZE);

		}

    }

    close(user->sockfd);
    remove_user_froom_room(user->uid);
  	free(user);
  	nb_users--;
 	pthread_detach(pthread_self());

    return NULL;

}

int main(int argc, char const* argv[]) {

    int socket_fd = 0, client_sock_fd = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int opt = 1;
    pthread_t listen_thread, send_thread;

    init_socket(&socket_fd);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("ERROR: setsockopt: could not set socket options properly");
        return EXIT_FAILURE;
    }
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR: bind: could not properly bind the socket to IP address");
        return EXIT_FAILURE;
    }

    if (listen(socket_fd, CHAT_ROOM_SIZE) < 0) {
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
	}

    printf("≈≈≈≈≈≈≈≈≈ FISA3 - CHAT ROOM ≈≈≈≈≈≈≈≈≈\n");
    int should_run = 1;
    while (should_run) {

        socklen_t client_addr_len = sizeof(client_addr);
        client_sock_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);

        if ((nb_users + 1) == CHAT_ROOM_SIZE) {
            printf("The chat room is full. Connection denied.");
            close(client_sock_fd);
            continue;
        }

        User* user = create_user(client_sock_fd);
        add_user_in_room(user);

        pthread_create(&listen_thread, NULL, &listen_to_client, (void*)user);

        sleep(1);

    }

    return EXIT_SUCCESS;
}
