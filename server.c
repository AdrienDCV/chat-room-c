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
#define BUFFER_SIZE 1024
#define CHAT_ROOM_SIZE 5
#define USERNAME_LENGTH 255
    
typedef struct
{
    int sockfd;
    int uid;
    char username[USERNAME_LENGTH];
} Client;

int uid = 1;
int nb_users;
Client *users[CHAT_ROOM_SIZE];

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

void send_message(char *s, int uid){
	pthread_mutex_lock(&users_mutex);

	for(int i=0; i<CHAT_ROOM_SIZE; ++i){
		if(users[i]){
			if(users[i]->uid != uid){
				if(write(users[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&users_mutex);
}

void *listen_to_client(void* arg) {
    printf("LOG-INFO : HERE\n");
    char buffer[BUFFER_SIZE];
    char username[USERNAME_LENGTH];
    time_t current_time;
    
    nb_users++;
    Client *user = (Client *)arg;

    if (recv(user->sockfd, username, USERNAME_LENGTH, 0) > 0) {
        strcpy(user->username, username);
        sprintf(buffer, "%s has joined the chat\n", user->username);
        printf("%s", buffer);
        send_message(buffer, user->uid);
    }

    bzero(buffer, BUFFER_SIZE);

    while (1) {

        int receive = recv(user->sockfd, buffer, BUFFER_SIZE, 0);
        if (receive > 0){
			if(strlen(buffer) > 0){
				send_message(buffer, user->uid);

				remove_carriage_return_char(buffer, strlen(buffer));
				printf("%s\n", buffer);
			}
		}

        bzero(buffer, BUFFER_SIZE);
    }

    close(user->sockfd);
  	free(user);
  	nb_users--;
 	pthread_detach(pthread_self());

    return NULL;
}

void add_user_in_room(Client* user) {
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
		if(!users[i] && users[i]->uid == uid) {
			users[i] = NULL;
            printf("LOG-INFO : user removed\n");
			break;
		}
        printf("LOG-INFO : HERE");
	}

    printf("LOG-INFO : ICI");
	pthread_mutex_unlock(&users_mutex);
    printf("LOG-INFO : LÀ");
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
        exit(EXIT_FAILURE);
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

        Client *user = (Client *)malloc(sizeof(user));
        user->sockfd = client_sock_fd;
        user->uid = uid++;

        add_user_in_room(user);
        pthread_create(&listen_thread, NULL, &listen_to_client, (void*)user);

        sleep(1);

    }

    return EXIT_SUCCESS;
}
