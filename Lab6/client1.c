/*
 * Abrudan Paul - Andrei
 * IA 2024, subgrupa 1
 * Tema 6.1b
 */

#include <stdio.h>      //perror
#include <stdlib.h>     //exit
#include <string.h>     //string operations
#include <sys/socket.h> //socket operations
#include <netinet/in.h> //sockaddr_in, htons
#include <arpa/inet.h>  //inet_addr
#include <unistd.h>     //close
#include <signal.h>     //signals

#define SERVER_PORT 1312
#define SERVER_IP   "127.0.0.1"
#define BUFFER_SIZE 1024

int                SOCKET;
struct sockaddr_in server_address;

void handle_server_resp(int sockfd, struct sockaddr_in* servaddr)
{
    char      buffer[BUFFER_SIZE];
    int       n;
    socklen_t len = sizeof(*servaddr);

    n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)servaddr, &len);
    if(n < 0)
    {
        perror("recvfrom");
        return;
    }
    buffer[n] = 0; //Manually null-terminate string
    printf("Server: %s\n", buffer);
}

//Handle signals sent to process
void handle_signal(int sig)
{
    //We only care for SIGINT here
    if(sig == SIGINT)
    {
        const char* msg = "client_disconnect";
        sendto(SOCKET, msg, strlen(msg), 0, (const struct sockaddr*)&server_address, sizeof(server_address));
        close(SOCKET);
        printf("Client disconnected.\n");
        exit(0);
    }
}

int main()
{
    SOCKET = socket(AF_INET, SOCK_DGRAM, 0);

    if(SOCKET == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    //Set the server address and port
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family      = AF_INET;
    server_address.sin_port        = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);

    //Register the signal handler
    signal(SIGINT, handle_signal);

    while(1)
    {
        char buffer[BUFFER_SIZE];
        printf("$ ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; //Remove trailing newline
        //Check if the user wants to quit
        if(strcmp(buffer, "quit") == 0)
        {
            sendto(SOCKET, buffer, strlen(buffer), 0, (const struct sockaddr*)&server_address, sizeof(server_address));
            break;
        }
        sendto(SOCKET, buffer, strlen(buffer), 0, (const struct sockaddr*)&server_address, sizeof(server_address));
        handle_server_resp(SOCKET, &server_address);
    }

    close(SOCKET);
    return 0;
}


/*>
   ./client
   $ help
   Server: Invalid command
   $ time
   Server: Invalid command
   $ uptime
   Server: Server is up for 0d 1h 4min
   $ stats
   Server: Load Avg: 1.85, 1.94, 2.79  CPU Usage: 4%  Memory Usage: Total: 32736572 kB, Used: 8802112 kB
   $ ls
   Server: Invalid command
   $ cmd: ls
   Server: client
   client1.c
   server
   server1.c

   $ quit
 */
