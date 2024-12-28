/*SERVER*/

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

// Définitions de constantes :
// - PORT : Port d'écoute du serveur.
// - BUFFER_SIZE : Taille maximale des messages échangés.
// - CHAT_ROOM_SIZE : Limite du nombre d'utilisateurs connectés simultanément.
// - USERNAME_LENGTH : Taille maximale du nom d'utilisateur.
// - LOGS_FILE_NAME_SIZE : Taille maximale pour les noms de fichiers journaux.

typedef struct {
    int socket_file_descriptor;    // Descripteur du socket de l'utilisateur.
    int id;                        // Identifiant unique de l'utilisateur.
    char username[USERNAME_LENGTH]; // Nom d'utilisateur.
} User;

// Structure représentant un utilisateur connecté.

int id = 1;                        // Identifiant global incrémental pour chaque utilisateur.
int nb_users = 0;                  // Nombre actuel d'utilisateurs connectés.
User *users[CHAT_ROOM_SIZE];       // Tableau pour stocker les utilisateurs connectés.

char logs_file_name[LOGS_FILE_NAME_SIZE] = "chat-room-logs";
FILE *file_pointer;                // Pointeur vers le fichier de journaux.

pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex pour gérer la liste des utilisateurs.
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex pour l'accès aux journaux.


// Déclaration des fonctions utilisées dans le programme.
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

// Fonction pour initialiser un socket serveur.
void init_socket(int *server_file_descriptor) {
    if ((*server_file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // Vérifie si la création du socket a échoué.
        fprintf(stderr, "SERVER-ERROR : init_socket: could not open the socket\n");
        exit(EXIT_FAILURE);
    }
    printf("SERVER-INFO : init_socket : Socket opened\n");
}

// Fonction pour ouvrir un fichier de journaux avec un horodatage unique.
int open_logs_file() {
    if (file_pointer != NULL) {
        // Si le fichier est déjà ouvert, ne rien faire.
        return 0;
    }

    char log_file_name[LOGS_FILE_NAME_SIZE];
    char log_file_time[LOGS_FILE_NAME_SIZE];

    // Génère un horodatage pour inclure dans le nom du fichier.
    time_t current_time;
    time(&current_time);
    struct tm *local_time = localtime(&current_time);
    strftime(log_file_time, sizeof(log_file_time), "-%Y-%m-%d", local_time);
    snprintf(log_file_name, sizeof(log_file_name), "chat-room-log%s.txt", log_file_time);

    // Ouvre le fichier en mode "ajout".
    file_pointer = fopen(log_file_name, "a");

    if (file_pointer == NULL) {
        // Gère les erreurs d'ouverture de fichier.
        fprintf(stderr, "SERVER-ERROR: open_logs: Could not open logs file.");
        return 1;
    }

    return 0;
}

// Fonction pour fermer le fichier de journaux.
int close_logs_file() {
    if (file_pointer != NULL) {
        // Si un fichier est ouvert, le fermer.
        if (fclose(file_pointer) == 0) {
            file_pointer = NULL;
            return 0;
        } else {
            // Gérer les erreurs lors de la fermeture.
            fprintf(stderr, "SERVER-ERROR: close_logs_file: Could not close logs file properly.");
            return 1;
        }
    }
    // Si aucun fichier n'est ouvert.
    fprintf(stderr, "SERVER-ERROR: close_logs_file: No file descriptor");
    return 1;
}

// Fonction pour écrire un message dans le fichier de journaux.
void write_logs(char *line) {
    pthread_mutex_lock(&server_mutex); // Verrouillage pour un accès sécurisé.

    if (open_logs_file() == 0) {
        // Supprime le retour à la ligne final pour éviter les doublons.
        line[strlen(line) - 1] = '\0';
        fprintf(file_pointer, "%s\n", line);

        if (close_logs_file() == 1) {
            // Gestion des erreurs de fermeture.
            fprintf(stderr, "SERVER-ERROR: write_logs: An error occured. Could not close logs file");
        }
    } else {
        // Gestion des erreurs d'ouverture.
        fprintf(stderr, "SERVER-ERROR: write_logs: Could not write in logs file. Logs file could not be opened");
    }

    pthread_mutex_unlock(&server_mutex); // Déverrouillage.
}

// Fonction pour diffuser un message à tous les utilisateurs connectés.
void broadcast_message(char *message) {
    pthread_mutex_lock(&server_mutex); // Verrouillage pour éviter les conflits.

    for (int i = 0; i < CHAT_ROOM_SIZE; ++i) {
        if (users[i]) {
            // Vérifie si un utilisateur est connecté à l'index.
            if (write(users[i]->socket_file_descriptor, message, strlen(message)) < 0) {
                // Gestion des erreurs d'envoi.
                fprintf(stderr, "SERVER-ERROR : send_message : could not send message\n");
                break;
            }
        }
    }

    pthread_mutex_unlock(&server_mutex); // Déverrouillage.
}

// Fonction pour créer un utilisateur.
User* create_user(int client_sock_fd, char *username) {
    User *user = (User *)malloc(sizeof(User)); // Alloue dynamiquement un utilisateur.
    user->socket_file_descriptor = client_sock_fd; // Associe le socket à l'utilisateur.
    strcpy(user->username, username); // Copie le nom d'utilisateur.

    return user;
}

// Fonction pour ajouter un utilisateur dans la liste des utilisateurs connectés.
int add_user_in_list(User *user) {
    pthread_mutex_lock(&users_mutex); // Verrouillage de la liste des utilisateurs.

    for (int i = 0; i < CHAT_ROOM_SIZE; i++) {
        if (!users[i]) {
            // Trouve un emplacement libre et ajoute l'utilisateur.
            users[i] = user;
            user->id = id++; // Associe un identifiant unique.
            nb_users++; // Incrémente le compteur.
            pthread_mutex_unlock(&users_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&users_mutex); // Déverrouillage si la liste est pleine.
    return 1;
}

// Fonction principale pour gérer le serveur.
int main(int argc, char const* argv[]) {
    int socket_file_descriptor = 0, client_sock_file_descriptor = 0;
    struct sockaddr_in server_addr, client_addr;
    int opt = 1;
    pthread_t listen_thread;

    // Configure les informations du serveur.
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    init_socket(&socket_file_descriptor); // Initialise le socket serveur.

    // Configure les options du socket pour réutiliser le port.
    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "SERVER-ERROR: setsockopt: could not set socket options properly");
        return EXIT_FAILURE;
    }

    // Associe le socket à l'adresse et au port.
    if (bind(socket_file_descriptor, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "SERVER-ERROR: bind: could not properly bind the socket to IP address");
        return EXIT_FAILURE;
    }

    // Démarre l'écoute sur le port défini.
    if (listen(socket_file_descriptor, CHAT_ROOM_SIZE) < 0) {
        fprintf(stderr, "SERVER-ERROR: listen: Socket listening failed");
        return EXIT_FAILURE;
    }

    printf("SERVER-INFO: Chat room is open and listening on port %d\n", PORT);

    // Boucle principale pour accepter les connexions des clients.
    while (1) {
        socklen_t client_addr_len = sizeof(client_addr);
        client_sock_file_descriptor = accept(socket_file_descriptor, (struct sockaddr*)&client_addr, &client_addr_len);

        if (client_sock_file_descriptor < 0) {
            fprintf(stderr, "SERVER-ERROR: accept: Failed to accept connection");
            continue;
        }

        pthread_create(&listen_thread, NULL, &listen_to_client, (void*)&client_sock_file_descriptor);
        // Crée un thread pour chaque nouveau client.
    }

    close(socket_file_descriptor); // Ferme le socket serveur à la fin.
    return EXIT_SUCCESS;
}
