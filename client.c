#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>


#define PORT 8080                                       // Port sur lequel le serveur écoute.
#define IP_SERVER "127.0.0.1"                           // Adresse IP du serveur (ici localhost).
#define MSG_SIZE 1024                                   // Taille maximale d'un message simple.
#define BUFFER_SIZE 2048                                // Taille du tampon utilisé pour envoyer des messages plus complexes.
#define USERNAME_SIZE 255                               // Taille maximale du nom d'utilisateur.

int socket_file_descriptor = 0;                         // Descripteur de fichier de la socket l'utilisateur courant
int disconnection_flag = 0;                             // Flag permettant se signifier la volonté de l'utilisateur courant de se déconnecter
char username[USERNAME_SIZE];                           // Nom d'utilisateur de l'utilisateur courant

/**
 * Supprime le dernier retour chariot en fin de la chaîne de caractère donnée en 
 * paramètre s'il y en a un
 */
void remove_carriage_return_char (char* message);

/**
 * Lit et affecte le nom d'utilisateur entré par l'utilisateur courant depuis l'entrée
 * standard et l'affecte à la chaîne de caractère username
 * 
 * Rtourne 0 si le nom d'utilisateur a été lu et affecté
 * Retourne 1 en cas d'erreur
 */
int read_username();

/**
 * Connecte l'utilisateur courant à la salle de discussion
 * 
 * Retourne 0 si l'utilsateur a été connecté à la salle de discussion
 * Retourne 1 en cas d'erreur
 */
int connect_to_room();

// Envoyer des messages entrés depuis l'entrée standard au serveur de la salle de discussion
void send_message();

// Lire les messages reçus du serveur de la salle de discussion sur la sortie standard
void listen_server_message();

// Fonction princpale permettant d'exécuter le programme server.c
int main(int argc, char const* argv[]);

void remove_carriage_return_char (char* message) {
    int message_length = strlen(message) - 1;
    /**
     * Si le dernier caractères de la chaîne de caractères donnée en paramètre est 
     * le caractère retour chariot ('\n'), remplace ce caractère par le caractère
     * vide ('\0')
     */
    if (message[message_length] == '\n') {
      message[message_length] = '\0';
    }
}

int read_username() {
    printf("CLIENT: Enter your username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        /**
         * Si une erreur survient à la lecture de la chaîne de caractère, 
         * affiche un message d'erreur sur la sortie d'erreur
         */
        fprintf(stderr, "CLIENT-ERROR: Error reading input.\n");
        // Retourne 1 signifiant un échec
        return 1;
    } else if (strlen(username) < 2) {
        /**
         * Si le nombre de caractère du nom d'utilisateur entré est inférieur à 2, affiche un message d'erreur
         * sur la sortie d'erreur
         */
        fprintf(stderr, "CLIENT-ERROR: Username must be at least 1 character long\n");
        // Retourne 1 signifiant un échec
        return 1;
    } else if (strlen(username) > USERNAME_SIZE && username[strlen(username) - 1] != '\n') {
        /**
         * Si le nombre de caractère du nom d'utilisateur entré est supérieur au nombre de caractères maximum autorisé
         * et que le dernier caractère n'est pas le caractère retour chariot ('\n') affiche un message d'erreur sur la sortie d'erreur
         */
        fprintf(stderr, "CLIENT-ERROR: Username is too long. Maximum length is %d characters.\n",(USERNAME_SIZE-1));
        // Retourne 1 signifiant un échec
        return 1;
    }

    // Supprime le retour à la ligne à la fin de l'entrée utilisateur.
    remove_carriage_return_char(username);
    // Retourne 0 signifiant que le nom d'utilisateur a été lu et affecté à username
    return 0;

}

int connect_to_room() {
    // Envoie le nom d'utilisateur de l'utilsiateur courant au serveur de la salle de discussion
    send(socket_file_descriptor, username, USERNAME_SIZE, 0);
    char error_message[MSG_SIZE];
    // Réception un potentiel message d'erreur de connection envoyé par le serveur
    int rcv = recv(socket_file_descriptor, error_message, MSG_SIZE, 0);
    if (rcv == 65) {
        // Si le message d'erreur de connection est réceptionné, affiche un message d'erreur sur la sortie d'erreur
        fprintf(stderr, "%s", error_message);
        // Change la valeur de disconnection_flag signifiant une déconnection
        disconnection_flag = 1;
        // Retourne 1 signifiant un erreur
        return 1;
    }

    // Retourne 0, la connection à la salle de discussion est établie
    return 0;
}

void send_message() {
    char message[MSG_SIZE];                     // Tampon mémoire permettant de stocker le message à envoyer
    char buffer[BUFFER_SIZE];                   // Tampon mémoire permettant de stocker le message entré depuis l'entrée standard
    time_t current_time;                        // Temps du système de l'utilisateur courant
    char message_prefix[USERNAME_SIZE + 30];    // Tampon mémoire permettant de stocket le préfix au message à envoyer de la forme "(dd-mm-yy/HH:MM) username"

    int should_run = 1;
    // Boucle infinie permettant de maintenir l'écoute de l'entrée standard
    while (should_run) {
        // Lecture de l'entrée standard
        fgets(buffer, MSG_SIZE, stdin);
        printf("\033[1A\033[2K\r");
        fflush(stdout);
        remove_carriage_return_char(buffer);
    
        if (strcmp(buffer, "exit") == 0) {
            // Si l'utilisateur entre "exit", envoie un message au serveur pour se déconnecter.
            sprintf(message, "%s", buffer);
            send(socket_file_descriptor, message, strlen(message), 0);
            // Passe la valeur du flag de déconnection à 1 signifiant la fin du programme
            disconnection_flag = 1;
        } else if (strlen(buffer) > 0) {
            time(&current_time);
            // Affecte l'heure courant du système à la variable current_time
            struct tm *local_time = localtime(&current_time);
            // Appliquer le format (jour-mois-année/heures:minutes)
            strftime(message_prefix, sizeof(message_prefix), "(%d-%m-%Y/%H:%M) ", local_time);
            // Concatène le préfix du message au nom d'utilisateur 
            strcat(message_prefix, username);

            // Affecte le message formaté à la variabe message
            snprintf(message, sizeof(message), "%s : %s\n", message_prefix, buffer);

            // Envoie le message au serveur
            send(socket_file_descriptor, message, strlen(message), 0);
        }

        // Réinitialise les tampons
        bzero(message, MSG_SIZE);
        bzero(buffer, MSG_SIZE);
    }
}

void listen_server_message() {
    char message[MSG_SIZE];         // Tampon mémoire permettant de stocker le message à envoyer

    int should_run = 1;
    // Boucle infinie permettant de maintenir l'écoute de la connexion vers la salle de discussion
    while (should_run) {
        int received = recv(socket_file_descriptor, message, MSG_SIZE, 0);
        if (received > 0) {
            // Si le message reçu n'est pas vide, le message reçu est affiché sur la sortie standard
            printf("%s", message);
            fflush(stdout);
        } else if (received == 0) {
            // Si le serveur se déconnecte, arrête la boucle.
            break;
        }
        // Réinitialise le tampon.
        memset(message, 0, sizeof(message));
    }
}

int main(int argc, char const* argv[]) {
    struct sockaddr_in server_addr;     // Adresse du serveur

    // Configuration de l'adresse du serveur
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
    if (inet_pton(AF_INET, IP_SERVER, &server_addr.sin_addr)<= 0) {
        // Si une erreur survient, un message d'erreur est affiché sur la sortie d'erreur
        fprintf(stderr, "CLIENT-ERROR: inet_pton: invalid address or address not supported\n");
        // L'exécution du programme est interrompue
        return EXIT_FAILURE;
    }

    // Connexion au serveur.
    if (connect(socket_file_descriptor, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        // Si la connexion au serveur rencontre une erreur, affiche un message d'erreur sur la sortie standard
        fprintf(stderr, "CLIENT-ERROR: connect: connection Failed\n");
        // L'exécution du programme est interrompue
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
        fflush(stdout);
        printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
        printf("≈≈≈≈≈≈≈≈≈ FISA3 - CHAT ROOM ≈≈≈≈≈≈≈≈≈\n");
        printf("≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈\n");
        printf("CLIENT-INFO: You have joined the chat room\n");
    } else {
        // Si la connection n'a pas été établie, un message d'erreur est affiché sur la sortie d'erreur
        fprintf(stderr, "CLIENT-ERROR: Closing connection to server.\n");
        // La connexion à la salle de discussion est fermée
        close(socket_file_descriptor);
        // L'exécution du programme est interrompue
        return EXIT_FAILURE;
    }
    
    // Création du thread pour envoyer des messages.
    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, (void *) send_message, NULL) != 0){
        // Si une erreur est survenue empêchant la création du thread, un message d'erreur est affiché sur la sortie d'erreur
        fprintf(stderr, "CLIENT-ERROR: pthread: could not create thread\n");
        // L'exécution du programme est interrompue
        return EXIT_FAILURE;
    }

    // Création du thread pour recevoir des messages.
    pthread_t receive_msg_thread;
    if(pthread_create(&receive_msg_thread, NULL, (void *) listen_server_message, NULL) != 0){
        // Si une erreur est survenue empêchant la création du thread, un message d'erreur est affiché sur la sortie d'erreur
        fprintf(stderr, "CLIENT-ERROR: pthread: could not create thread\n");
        // L'exécution du programme est interrompue
        return EXIT_FAILURE;
    }
    
    // Vérification continue de l'indicateur de déconnexion.
    int should_run = 1;
    while (should_run) {
        if (disconnection_flag) {
            // Si l'utilsateur se déconnecte, un message d'information est affiché sur la sortie standard
            printf("CLIENT-INFO: You have been successfuly disconnected from the room\n");
            break;
        }
    }
    
    // Fermeture du socket client.
    close(socket_file_descriptor);

    // Si l'exécution de la boucle infinie est interrompue, l'exécution du programme est interrompue
    return EXIT_SUCCESS;
}