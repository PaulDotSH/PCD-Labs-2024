/*
 * Abrudan Paul - Andrei
 * IA 2024, subgrupa 1
 * Tema 6.2b
 */

#include <stdio.h>      //perror
#include <string.h>     //string operations
#include <stdlib.h>     //exit
#include <sys/socket.h> //socket related stuff
#include <sys/un.h>     //sockaddr_un
#include <signal.h>     //signals
#include <unistd.h>     //close

#define SOCKET_PATH "/tmp/server.socket"
#define BUFFER_SIZE 1024

int SOCKET;
//Cant be named shutdown since we don't have namespaces in C (take a look at zig)
void client_shutdown()
{
    close(SOCKET);
    exit(EXIT_SUCCESS);
}

void handle_signal(int sig)
{
    if(sig == SIGINT)
    {
        printf("Client disconnecting\n");
        client_shutdown();
    }
}

int main()
{
    struct sockaddr_un servaddr;

    signal(SIGINT, handle_signal);

    //Setup stuff
    SOCKET = socket(AF_UNIX, SOCK_STREAM, 0);
    if(SOCKET == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path, SOCKET_PATH, sizeof(servaddr.sun_path) - 1);
    if(connect(SOCKET, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    char input[BUFFER_SIZE];
    printf("Enter 'quit' to stop.\n");

    while(1)
    {
        memset(input, 0, BUFFER_SIZE);
        printf("Enter message: \n");
        //EOF
        if(fgets(input, BUFFER_SIZE, stdin) == NULL)
        {
            break;
        }
        //Remove trailing \n
        input[strcspn(input, "\n")] = 0;
        if(strcmp(input, "quit") == 0)
        {
            break;
        }
        //Send message
        write(SOCKET, input, strlen(input));
        char buffer[BUFFER_SIZE] = {0};
        read(SOCKET, buffer, BUFFER_SIZE);
        printf("Server sent: %s\n", buffer);
    }

    client_shutdown();
    return 0;
}
