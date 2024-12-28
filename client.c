/*CLIENT*/

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

// Définition de constantes utilisées dans l'application :
// - PORT : Port sur lequel le serveur écoute.
// - IP_SERVER : Adresse IP du serveur (ici localhost).
// - MSG_SIZE : Taille maximale d'un message simple.
// - BUFFER_SIZE : Taille du tampon utilisé pour envoyer des messages plus complexes.
// - USERNAME_SIZE : Taille maximale du nom d'utilisateur.

int socket_file_descriptor = 0;
int disconnection_flag = 0;
char username[USERNAME_SIZE];

void remove_carriage_return_char (char* message);
// Fonction pour supprimer le caractère de retour à la ligne à la fin d'une chaîne.

int read_username();
// Fonction pour lire et valider le nom d'utilisateur.

int connect_to_room();
// Fonction pour établir une connexion avec le serveur de chat.

void send_message();
// Fonction pour envoyer des messages au serveur.

void listen_server_message();
// Fonction pour écouter et afficher les messages envoyés par le serveur.

int main(int argc, char const* argv[]);
// Fonction principale qui initialise et orchestre le fonctionnement du programme.

void remove_carriage_return_char (char* message) {
    int message_length = strlen(message) - 1;
    if (message[message_length] == '\n') {
        message[message_length] = '\0';
        // Supprime le caractère de retour à la ligne (\n) s'il est présent à la fin de la chaîne.
    }
}

int read_username() {
    printf("CLIENT: Enter your username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        // Vérifie si l'entrée utilisateur est vide ou invalide.
        printf("CLIENT: Error reading input.\n");
        return 1;
    } else if (strlen(username) < 2) {
        // Vérifie si le nom d'utilisateur est trop court.
        printf("CLIENT-ERROR: Username must be at least 1 character long\n");
        return 1;
    } else if (strlen(username) > USERNAME_SIZE && username[strlen(username) - 1] != '\n') {
        // Vérifie si le nom d'utilisateur dépasse la taille maximale autorisée.
        fprintf(stderr, "CLIENT-ERROR: Username is too long. Maximum length is %d characters.\n", (USERNAME_SIZE - 1));
        return 1;
    }

    username[strlen(username) - 1] = '\0';
    // Supprime le retour à la ligne à la fin de l'entrée utilisateur.
    return 0;
}

int connect_to_room() {
    send(socket_file_descriptor, username, USERNAME_SIZE, 0);
    // Envoie le nom d'utilisateur au serveur.

    char error_message[MSG_SIZE];
    int rcv = recv(socket_file_descriptor, error_message, MSG_SIZE, 0);
    // Reçoit une réponse du serveur. Si la réponse indique une erreur, retourne 1.
    if (rcv == 65) {
        printf("%s", error_message);
        disconnection_flag = 0;
        return 1;
    }

    return 0; // Connexion réussie.
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
            // Si l'utilisateur entre "exit", envoie un message au serveur pour se déconnecter.
            sprintf(buffer, "%s", message);
            send(socket_file_descriptor, buffer, strlen(buffer), 0);
            disconnection_flag = 1;
        } else if (strlen(message) > 0) {
            // Prépare un message avec un horodatage et le nom d'utilisateur.
            time(&current_time);
            struct tm *local_time = localtime(&current_time);
            strftime(message_prefix, sizeof(message_prefix), "(%d-%m-%Y/%H:%M) ", local_time);
            strcat(message_prefix, username);

            snprintf(buffer, sizeof(buffer), "%s : %s\n", message_prefix, message);
            send(socket_file_descriptor, buffer, strlen(buffer), 0);
        }

        bzero(message, MSG_SIZE); // Réinitialise les tampons.
        bzero(buffer, BUFFER_SIZE);
    }
}

void listen_server_message() {
    char message[MSG_SIZE];

    int should_run = 1;
    while (should_run) {
        int received = recv(socket_file_descriptor, message, MSG_SIZE, 0);
        if (received > 0) {
            // Affiche les messages reçus du serveur.
            printf("%s", message);
            fflush(stdout);
        } else if (received == 0) {
            // Si le serveur se déconnecte, arrête la boucle.
            break;
        }
        memset(message, 0, sizeof(message)); // Réinitialise le tampon.
    }
}

int main(int argc, char const* argv[]) {
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = inet_addr(IP_SERVER);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Création du socket client.
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) {
        printf("CLIENT-ERROR: socket: could not open the socket\n");
        return EXIT_FAILURE;
    }

    // Conversion de l'adresse IP en format réseau.
    if (inet_pton(AF_INET, IP_SERVER, &server_addr.sin_addr) <= 0) {
        printf("CLIENT-ERROR: inet_pton: invalid address or address not supported\n");
        return EXIT_FAILURE;
    }

    // Connexion au serveur.
    if (connect(socket_file_descriptor, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("CLIENT-ERROR: connect: connection Failed\n");
        return EXIT_FAILURE;
    }

    // Demande du nom d'utilisateur jusqu'à ce qu'il soit valide.
    int valid_username = 1;
    while (valid_username != 0) {
        if (read_username() == 0) {
            valid_username = 0;
        }
    }

    // Connexion à la chat room.
    int connected_to_room = connect_to_room();
    if (connected_to_room == 0) {
        printf("CLIENT-INFO: You have joined the chat room\n");
    } else {
        printf("CLIENT-ERROR: Closing connection to server.\n");
        close(socket_file_descriptor);
        return EXIT_FAILURE;
    }

    // Création du thread pour envoyer des messages.
    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *) send_message, NULL) != 0) {
        printf("CLIENT-ERROR: pthread: could not create thread\n");
        return EXIT_FAILURE;
    }

    // Création du thread pour recevoir des messages.
    pthread_t receive_msg_thread;
    if (pthread_create(&receive_msg_thread, NULL, (void *) listen_server_message, NULL) != 0) {
        printf("CLIENT-ERROR: pthread: could not create thread\n");
        return EXIT_FAILURE;
    }

    // Vérification continue de l'indicateur de déconnexion.
    int should_run = 1;
    while (should_run) {
        if (disconnection_flag) {
            printf("CLIENT-INFO: You have been successfully disconnected from the room\n");
            break;
        }
    }

    close(socket_file_descriptor); // Fermeture du socket client.

    return EXIT_SUCCESS; // Fin normale du programme.
}
