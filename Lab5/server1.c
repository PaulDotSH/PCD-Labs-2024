/*
 * Abrudan Paul - Andrei
 * IA 2024, subgrupa 1
 * Tema 5.1a
 * Acest program foloseste socket-urile pentru a raspunde unui client cu informatii ca current time, current user, poate fi oprit si executa shell commands.
 */

#include <stdio.h> //Input Output
#include <stdlib.h>
#include <string.h>     //String operations
#include <sys/socket.h> //Socket
#include <netinet/in.h> //INET
#include <unistd.h>     //close
#include <time.h>       //time

#define PORT        1312
#define BUFFER_SIZE 2048
#define BACKLOG_LEN 5 //irrelevant here

int main()
{
    //Init stuff
    int                server_fd, client_socket;
    char               request_buffer[BUFFER_SIZE] = {0}; //Set buffer to zero
    struct sockaddr_in address;

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);

    int address_length = sizeof(address);

    //Create and bind the socket
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
        return EXIT_FAILURE;
    }

    if(bind(server_fd, (struct sockaddr*)&address, address_length) == -1)
    {
        perror("Bind");
        return EXIT_FAILURE;
    }

    if(listen(server_fd, BACKLOG_LEN) == -1)
    {
        perror("Listen");
        return EXIT_FAILURE;
    }

    if((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&address_length)) == -1)
    {
        perror("Accept");
        return EXIT_FAILURE;
    }

    printf("Server started\n");
    char response_buffer[BUFFER_SIZE] = {0};

    while(1)
    {
        //Reset buffer
        int bytes_read = read(client_socket, request_buffer, BUFFER_SIZE - 1);

        if(bytes_read < 1)
        {
            //On connection closed start a new one
            if((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&address_length)) == -1)
            {
                perror("Accept");
                return EXIT_FAILURE;
            }
            continue;
        }

        request_buffer[bytes_read] = '\0'; //Manually null terminate the string

        if(strcmp(request_buffer, "time") == 0)
        {
            time_t t           = time(NULL);
            char*  time_string = ctime(&t);
            time_string[strlen(time_string) - 1] = '\0';              //Remove trailing \n
            send(client_socket, time_string, strlen(time_string), 0); //Send response
            printf("> Sent current time\n");
            continue;
        }

        if(strcmp(request_buffer, "user") == 0)
        {
            char* user = getlogin();
            send(client_socket, user, strlen(user), 0);
            printf("> Sent current user\n");
            continue;
        }

        snprintf(response_buffer, sizeof(response_buffer), "echo :: %.*s", BUFFER_SIZE - 10, request_buffer);
        send(client_socket, response_buffer, strlen(response_buffer), 0);
        memset(response_buffer, 0, BUFFER_SIZE);
        printf("> Sent message to client\n");

        if(strcmp(request_buffer, "exit") == 0)
        {
            break;
        }
    }

    //Close the sockets (not really 100% necessary since we exit anyway)
    close(client_socket);
    close(server_fd);

    return 0;
}

/*
   Server started
   > Sent current user
   > Sent current time
   > Sent message to client
   > Sent message to client
 */
