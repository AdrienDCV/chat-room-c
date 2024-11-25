#ifndef USERS_LIST_H
#define USERS_LIST_H

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

#endif