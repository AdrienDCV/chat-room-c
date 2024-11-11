// Client side C program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


#define PORT 8080
#define IP_SERVER "127.0.0.1"
#define MAXSIZE 1024

int main(int argc, char const* argv[]){
    int status, valread, client_fd;
    struct sockaddr_in serv_addr;
    char buffer[1024] = { 0 };
    time_t current_time;
    char fullMsg[100];

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);


    if (inet_pton(AF_INET, IP_SERVER, &serv_addr.sin_addr)<= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }


    int should_run = 1;

    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    while (should_run){
        if(status == -1){
            printf("Disconnected from server\n");
            close(client_fd);
            if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
                printf("\nReconnection Failed \n");
                return -1;
            }else{
                printf("Connection to server successfully established\n");
            }
        }
        char *message;
        message = (char *)malloc(MAXSIZE * sizeof(char));
        if (message == NULL) {
            printf("Memory allocation failed.\n");
            return 1;
        }
        scanf("%[^\n]%*c", message);
        if(message[0]=='q'){
            should_run = 0;
            close(client_fd);
        }

        time(&current_time);
        struct tm *local_time = localtime(&current_time);
        strftime(fullMsg, sizeof(fullMsg), "(%d-%m/%H:%M) Khalil : ", local_time);

        strcat(fullMsg, message);
        status = send(client_fd, fullMsg, strlen(fullMsg), 0);
        printf("State: %d\n", status);
         if (status > 0){
           printf("\nMessage sent to server successfully\n");
         }else{
           printf("\nMessage sent to server failed\n");
         }
         valread = read(client_fd, buffer,1024 - 1);
         printf("%s\n", buffer);
    }
    return 0;
}