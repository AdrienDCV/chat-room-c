#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHAT_ROOM_SIZE 5
    
typedef struct
{
    int nb_users;
    int *users[CHAT_ROOM_SIZE];
} UsersList;

void init_users_list(UsersList* users_list);
void print_users(UsersList* users_list);
void add_user(int user_socket_fd, UsersList* users_list);
void remove_user(int user_socket_fd, UsersList* users_list);

void init_users_list(UsersList* users_list) {
    users_list->nb_users = 0;
}

void print_users(UsersList* users_list) {
    int i = 0;
    printf("Users :\n");

    while (1) {
        printf("Index = %d, userId : %d\n", i, *users_list->users[i]);
        if (i == users_list->nb_users) {
            break;
        }
    }
}

void add_user(int user_socket_fd, UsersList* users_list) {
    int i;
    while (i < users_list->nb_users && users_list->users[i] != &user_socket_fd) {
        i++;
    }

    if (i == users_list->nb_users) {
        perror("Error: this user is already connected");
        exit(1);
    } else {
        users_list->users[users_list->nb_users] = &user_socket_fd;
        users_list->nb_users++;
    }
}

void remove_user(int user_socket_fd, UsersList* users_list) {
    int i;
    while (i < users_list->nb_users && users_list->users[i] == &user_socket_fd) {
        i++;
    }

     if (i == users_list->nb_users) {
        perror("Error: user not found");
        exit(1);
    } else {
        users_list->users[users_list->nb_users] = NULL;
        users_list->nb_users--;
    }
}