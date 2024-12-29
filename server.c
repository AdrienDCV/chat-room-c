#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080                       // Port d'écoute du serveur
#define BUFFER_SIZE 2048                // Taille maximale des messages échangés
#define CHAT_ROOM_SIZE 5                // Limite du nombre d'utilisateurs connectés simultanément
#define USERNAME_LENGTH 255             // Taille maximale du nom d'utilisateur
#define LOGS_FILE_NAME_SIZE 128         // Taille maximale pour les noms de fichiers journaux

// Structure représentant un utilisateur connecté.
typedef struct
{
    int socket_file_descriptor;         // Descripteur du socket de l'utilisateur
    int id;                             // Identifiant unique de l'utilisateur
    char username[USERNAME_LENGTH];     // Nom d'utilisateur
} User;

int id = 1;                             // Numéro d'identifiant d'un utilisateur
int nb_users = 0;                       // Nombre d'utilisateurs connectés
User *users[CHAT_ROOM_SIZE];            // Liste des utilisateurs connectés

FILE *file_pointer;                                          // Pointeur vers le fichier des logs

pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;    // Mutex déddié aux utilisateurs, permettant de bloquer l'utilisation d'une ressource
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;    // Mutex déddié au serveur, permettant de bloquer l'utilisation d'une ressource

// Intiliaser la socket ouverte par le serveur
void init_socket(int *server_fd);

/**
 * Ouvrir le fichier des logs
 * 
 * Retourne 0 si le fichier est ouvert correctement
 * Retourne 1 en cas d'erreur
 */
int open_logs_file();

/**
 * Fermer le fichier des logs
 * 
 * Retourne 0 si le fichier est fermé correctement
 * Retourne 1 en cas d'erreur
 */
int close_logs_file();

// Ecrire la chaîne de caaractères line donnée en paramettre dans le fichier des logs
void write_logs(char *line);

// Propager le message donné en paramètre aux utilisateurs connectés
void broadcast_message(char *message);

/**
 * Créer un nouvel utilisateur
 * 
 * Retourne une instance de la structure User correspondant à l'utilisateur
 */
User* create_user(int client_sock_fd, char *username);

/**
 * Ajouter l'utilisateur donné en paramètre dans la liste des utilisateurs
 * 
 * Retourne 0 si l'utilsiateur est ajouté à la liste des utilisateurs connectés
 * Retourne 1 en cas d'erreur
 */
int add_user_in_list(User *user);

/**
 * Ajouter un nouvel utilisateur dans la salle de discussion
 * 
 * Retourne l'utilisateur ajouté à la salle de discussion
 * Retourne null en cas d'erreur
 */
User* add_user_to_room(int client_socket_file_descriptor, char *username);

/**
 * Retirer un utilisateur dans la lsite des utilisateurs connectés à partir d'un identifiant id donné en paramètre
 * 
 * Retourne 0 si l'utilisateur est retiré correctement de la liste des utilisaterus connectés
 * Retourne 1 en cas d'erreur
 */
int remove_user_from_list(int id);

// Retirer l'utilisateur donné en paramètre de la salle de discussion.
void remove_user_from_room(User *user);

/**
 * Ecoute et réceptionne le nom d'utilsiateur correspondant au descripteur de fichier donnée ne paramètre
 * et affecte la chaîne de caractère reçu à la chaîne de caractères username donnée en paramètre
 * 
 * Retourne 0 si le nom d'utilisateur est reçu et affecté correctement
 * Retourne 1 en cas d'erreur
 */
int listen_username(int client_sock_fd, char *username);

// Ecoute les messages envoyés par l'utilsateur courant
void *listen_to_client(void* arg);

// Fonction princpale permettant d'exécuter le programme server.c
int main(int argc, char const* argv[]);

void init_socket(int *server_file_descriptor) {
    // Vérifie que la socket a bien été créée
    if ((*server_file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // En cas d'erreur, affiche un message d'erreur sur la sortie d'erreur
        fprintf(stderr, "SERVER-ERROR : init_socket: could not create the socket\n");
        // Stoppe l'exécutiin du programme
        exit(EXIT_FAILURE);
    }
    // Affiche un message d'information sur la sortie standard
    printf("SERVER-INFO : init_socket : Socket created\n");
}

int open_logs_file() {
    if (file_pointer != NULL) {
        // Si le fichier existe déjà, c'est que le fichier est déjà ouvert.
        return 0;
    }
    char logs_file_name[LOGS_FILE_NAME_SIZE];
    char logs_file_time[LOGS_FILE_NAME_SIZE];

    // Récupère l'heure courante du système et la concatène à la variable logs_file_name
    time_t current_time;
    time(&current_time);
    struct tm *local_time = localtime(&current_time);
    strftime(logs_file_time, sizeof(logs_file_time), "%Y-%m-%d-", local_time);
    snprintf(logs_file_name, sizeof(logs_file_name), "%schat-room-logs.txt", logs_file_time);

    // Ouvrre le fichier des logs en mode "append" pour ajouter du contenu
    file_pointer = fopen(logs_file_name, "a");

    if (file_pointer == NULL) {
        // Si le pointeur vers le fichier des logs est null, affiche un message d'erreur sur la sortie d'erreur
        fprintf(stderr, "SERVER-ERROR: open_logs: Could not open logs file.");
        // Retourne 1 signifiant qu'une erreur est survenue
        return 1;
    }

    // Retourne 0 pour signifier un succès. Le fichier a bien été ouvert
    return 0;
}

int close_logs_file() {
    if (file_pointer != NULL) {
        if (fclose(file_pointer) == 0) {
            // Si le fichier a été fermé correctement, le pointeur vers le fichier des logs est réinitialisé
            file_pointer = NULL;
            // Retourne 0, signifiant un succès. Le fichier a correctement été fermé
            return 0;
        } else {
            fprintf(stderr, "SERVER-ERROR: close_logs_file: Could not close logs file properly.");
            return 1;
        }
    }
    // Si le pointeur du fichier des logs est null, un message d'erreur est affiché sur la sortie d'erreur
    fprintf(stderr, "SERVER-ERROR: close_logs_file: No such file");
    // Retourne 1, signifiant qu'une erreur est survenue
    return 1;
}

void write_logs(char *line) {
    // Appose un mutex afin de réserver la manipulation de la ressource : le fichier des logs
    pthread_mutex_lock(&server_mutex);

    if (open_logs_file() == 0) {
        // Si le fichier des logs a été ouvert correctement, la chaîne de caractères line, donnée en paramètre, est ajoutée au fichier des logs
        line[strlen(line) - 1] = '\0';
        fprintf(file_pointer, "%s\n", line);
        if (close_logs_file() == 1) {
            // Si le fichier n'a pas été fermé correctement, affiche un message d'erreur sur la sortie d'erreur
            fprintf(stderr, "SERVER-ERROR: write_logs: An error occured. Could not close logs file");
        }
    } else {
        // Si le fichier n'a pas été ouvert correctement, affiche un message d'erreur sur la sortie d'erreur
        fprintf(stderr, "SERVER-ERROR: write_logs: Could not write in logs file. Logs file could not be opened");
    }

    // Levée du mutex apposé
    pthread_mutex_unlock(&server_mutex);
}

void broadcast_message(char *message) {
    // Appose un mutex afin de réserver la manipulation des ressources : user->socket_file_descriptor
	pthread_mutex_lock(&server_mutex);

    // Le message donné en paramètre est propagé à l'ensemble des utilisateurs connectés à la salle de discussion
	for(int i=0; i<CHAT_ROOM_SIZE; ++i){
		if(users[i]){
			if(write(users[i]->socket_file_descriptor, message, strlen(message)) < 0){
                // Si une erreur est survenue, affiche un message d'erreur sur la sortie d'erreur
				fprintf(stderr, "SERVER-ERROR : send_message : could not send message\n");
                // Sort de la boucle
				break;
			}
		}
	}

    // Levée du mutex apposé
	pthread_mutex_unlock(&server_mutex);
}

User* create_user(int client_sock_fd, char *username) {
    // Allocation de l'espace mémoire le nouvel utilsiateur connecté
    User *user = (User *)malloc(sizeof(user));
    user->socket_file_descriptor = client_sock_fd;
    strcpy(user->username, username);

    return user;
}

int add_user_in_list(User* user) {
    // Appose un mutex pour pouvoir assurer la manipulation de la liste des utilisateurs connectés
    pthread_mutex_lock(&users_mutex);

	for(int i = 0; i < CHAT_ROOM_SIZE; i++){
        // Si l'emplacement à l'indice i de la liste des utilisateurs connectés est libre, on y insère l'utilisateur donné en paramètre
		if(!users[i]) {
			users[i] = user;
            user->id = id++;
            nb_users++;
            // Levée du mutex apposé
            pthread_mutex_unlock(&users_mutex);
            // Retourne 0 signifiant un succès. L'utilisateur a été ajouté à la liste des utilisateurs connectés
            return 0;
		}
	}

    // Levée du mutex apposé
    pthread_mutex_unlock(&users_mutex);
    // Retourne 1 signifiant qu'une erreur est survenue. L'utilisateur n'a pas été ajouté à la liste des utilisateurs connectés car la liste est pleine
    return  1;
}

User* add_user_to_room(int client_socket_file_descriptor, char *username) {
    char buffer[BUFFER_SIZE]; // Espace tampon permettant de stocker le nom d'utilisateur reçu

    User *user = create_user(client_socket_file_descriptor, username);

    if (add_user_in_list(user) == 0) {
        // Si l'utilisateur a été ajouté à la liste des utilisateurs connectés, un message d'info est propoagé aux utilisateurs connectés
        printf("SERVER-INFO: add_user_in_list: User added to users list\n");
        sprintf(buffer, "SERVER-INFO: %s has joined the chat\n", user->username);
        printf("%s", buffer);
        broadcast_message(buffer);
        write_logs(buffer);

        // Retourne l'utilisateur nouvellement connecté à la salle de discussion
        return user;
    } else {
        // Si l'utilisateur n'a pas été ajouté à la liste des utilisateurs connectés, un message d'erreur est envoyé à l'utilisateur courant
        strcpy(buffer, "SERVER-ERROR: add_user_in_list: Could not add user to users list\n");
        fprintf(stderr, "%s", buffer);
        // Apposé un mutex pour assurer la manipulation exclusive du descripteur de fichier de la socket de l'utilisateur courant
        pthread_mutex_lock(&users_mutex);

        write(user->socket_file_descriptor, buffer, strlen(buffer));
        write_logs(buffer);

        // Levée du mutex apposé
        pthread_mutex_unlock(&users_mutex);

        // Libération de l'espace resérvé pour l'utilisateur courant
        free(user);
        return NULL;
    }
}

int remove_user_from_list(int id) {
    // Appose un mutex pour assusre l'exclusivité sur la manipulation de la liste des utilisateurs connectés
    pthread_mutex_lock(&users_mutex);

	for(int i = 0; i < CHAT_ROOM_SIZE; i++){
        if (users[i] && users[i]->id == id) {
            // Si un utilisateur possédant l'identifiant donné en paramètre, il est retiré de la liste des utilisateurs connectés
            users[i] = NULL;
            // Levée du mutex apposé
            pthread_mutex_unlock(&users_mutex);
            // Retourne 0 signifiant un succès. L'utilisateur a été trouvé et a été retiré de la liste des utilisateurs connectés
            return 0;
        }
	}

    // Levée du mutex aposé
    pthread_mutex_unlock(&users_mutex);
    // Retourne 1 signifiant qu'une erreur est survenue. L'utilisateur n'a pas été trouvé et/ou n'a pas été retiré de la liste des utilisateurs connectés correctement
    return 1;
}

void remove_user_from_room(User *user) {
    char buffer[BUFFER_SIZE]; // Espace tampon permettant de stocker un message

    if (remove_user_from_list(user->id) == 0) {
        // Si l'utilisateur a été retiré de la liste des utilisateurs connectés, sa connexion au serveur est fermée
        close(user->socket_file_descriptor);
        // L'espace alloué à l'utilisateur est libéré
        free(user);
        // Le nombre d'utilisateurs connectés est décrémenté de 1
  	    nb_users--;
        // Un message informant de la déconnexion de l'utilisateur est propagé à tous les utilisateurs connectés à la salle de discussion
        printf("SERVER-INFO : remove_user_from_room: User removed from chat room.\n");
        sprintf(buffer, "SERVER-INFO : %s has left the chat\n", user->username);
        broadcast_message(buffer);
        // Le thread déddié à l'utilisateur est détaché
        pthread_detach(pthread_self());
    } else {
        // Si l'utilisateur n'a pas été retiré de la liste des utilisateurs conectés, affiche un message dans la sortie d'erreur
        fprintf(stderr, "%s", " ERROR: remove_user_from_room: User could not be removed from chat room.\n");
    }
}

int listen_username(int client_sock_fd, char *username) {
    char buffer[BUFFER_SIZE]; // Espace tampon permettant de stocker un message

    if (recv(client_sock_fd, username, USERNAME_LENGTH, 0) > 0 && strlen(username) > 0) {
        // Retourne 0 signifiant un succès. Un nom d'utilisateur a été reçu et affecté à la variable username
        return 0;
    }
    // Retourne 1 signifiant qu'une erreur est survenue. Aucun nom d'utilisateur n'a été reçu et/ou n'a pas été affecté à la variable username
    return 1;
}

void *listen_to_client(void* arg) {
    char buffer[BUFFER_SIZE];                           // Espace tampon permettant de stocker un message
    char username[USERNAME_LENGTH];                     // Espace tampon permettant de stocker un nom d'utilisateur
    User *user;                                         // Utilisateur courant
    int *client_socket_file_descriptor = (int *)arg;    // Descripteur de fichier de la socket de l'utilisateur courant

    if (listen_username(*client_socket_file_descriptor, username) != 0) {
        // Affiche un message d'erreur sur la sortie d'erreur si aucun nom d'utilisateur n'est reçu
        fprintf(stderr, "SERVER-ERROR: listen_username: Could not retrieve username.\n");
    }

    user = add_user_to_room(*client_socket_file_descriptor, username);
    if (user == NULL) {
        // Si l'utlisateur courant n'a pas été ajouté à la salle de discussion, sa connexion au serveur est fermée
        close(*client_socket_file_descriptor);
        // Le thread déddié à l'utilisateur est détaché
        pthread_detach(pthread_self());
        // Arrêt de l'exécution de la fonction
        return NULL;
    }

    bzero(buffer, BUFFER_SIZE);
    // Boucle infinie permettant d'écouter les messages envoyés par l'utilisateur courant
    int should_run = 1;
    while (should_run) {
        
        int receive = recv(user->socket_file_descriptor, buffer, BUFFER_SIZE, 0);
        if (receive > 0){
            if (strcmp(buffer, "exit") == 0) {
                // Si le message reçu est "exit", cela signifie que l'utilisateur souhaite se deconnecter. L'exécution de la boucle est interrompue
                should_run = 0;
            } else if (strlen(buffer) > 0){
                // Si la zone tampon contient un message, il est propagé à l'ensemble des utlisateurs connectés
				broadcast_message(buffer);
				printf("%s", buffer);
                // Le message reçu est archivé dans le fichier des logs du serveur
                write_logs(buffer);
			}

            // Le contenu du buffer est réinitialisé
            bzero(buffer, BUFFER_SIZE);
		}
    }

    // Si l'exécution des la boucle est interrompue, cela signifie que l'utilisateur souhaite se déconnecter
    remove_user_from_room(user);

    return NULL;
}

int main(int argc, char const* argv[]) {
    int socket_file_descriptor = 0, client_sock_file_descriptor = 0;    // descirpteurs de fichiers rattaché au serveur et à l'utilisateur se connectant
    struct sockaddr_in server_addr;                                     // adresse du serveur
    struct sockaddr_in client_addr;                                     // adresse de l'utilsateur se connectant
    int opt = 1;                                                        // Flag d'activation des options appliquées à la socket
    pthread_t listen_thread;                                            // Threads déddié à l'écoute des messages envoyé par les utilisateurs

    // Configuration de l'adresse du serveur
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // initialisation de la socket du serveur
    init_socket(&socket_file_descriptor);

    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        // Si les options n'ont pas été correctement appliquées, un message d'erreur es affiché sur la sortie d'erreur
        fprintf(stderr, "SERVER-ERROR: setsockopt: could not set socket options properly");
        // L'exécution du programme est interrompue
        return EXIT_FAILURE;

    }
    if (bind(socket_file_descriptor, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        // Si l'adresse du serveur n'a pas été associé à la socket, un message d'erreur est affiché sur la sortie d'erreur
        fprintf(stderr, "SERVER-ERROR: bind: could not properly bind the socket to IP address");
        // L'exécution du programme est interrompue
        return EXIT_FAILURE;
    }

    if (listen(socket_file_descriptor, CHAT_ROOM_SIZE) < 0) {
        // Si la socket n'est pas mise en écoute, un message d'erreur est affiché sur la sortie d'erreur
        fprintf(stderr, "SERVER-ERROR: listen: Socket listening failed");
        // L'exécution du programme est interrompue
        return EXIT_FAILURE;
	}

    if (open_logs_file() == EXIT_FAILURE) {
        // Si le fichier des logs n'a pas été ouvert correctement, un message d'erreur est affiché sur la sortie d'erreur
        fprintf(stderr, "SERVER-ERROR: open_logs_file: Could not open logs file.");
        // L'exécution du programme est interrompue
        return EXIT_FAILURE;
    }

    printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
    printf("≈≈≈≈≈≈≈≈≈ FISA3 - CHAT ROOM ≈≈≈≈≈≈≈≈≈\n");
    printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
    
    // Bonclue infinie permettant de maintenir l'exécution du programme
    int should_run = 1;
    while (should_run) {

        // Récupération des informations de la connexion de l'utilisateur courant
        socklen_t client_addr_len = sizeof(client_addr);
        client_sock_file_descriptor = accept(socket_file_descriptor, (struct sockaddr*)&client_addr, &client_addr_len);

        if ((nb_users + 1) == CHAT_ROOM_SIZE) {
            // Si la salle d'utilisateur est pleine, un message d'information est envoyé à l'utilisateur courant
            char *error_message = "SERVER-INFO : The chat room is full. Connection denied.";
            write(client_sock_file_descriptor, error_message, strlen(error_message));
            close(client_sock_file_descriptor);
            continue;
        }

        // Création d'un thread déddié à l'écoute des messages envoyés par l'utilsateur courant
        pthread_create(&listen_thread, NULL, &listen_to_client, (void*)&client_sock_file_descriptor);

    }

    // Si l'exécution de la boucle infinie est interrompue, l'exécution du programme est interrompue
    exit(EXIT_SUCCESS);
}
