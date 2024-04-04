/*
 * Abrudan Paul - Andrei
 * IA 2024, subgrupa 1
 * Tema 5.1b
 * Client-ul pentru primul server
 */

#include <stdio.h>      //IO
#include <stdlib.h>
#include <string.h>     //strlen
#include <sys/socket.h> //socket
#include <netinet/in.h> //INET
#include <unistd.h>     //close()
#include <arpa/inet.h>  //inet_pton

#define SERVER_PORT 1312
#define SERVER_IP   "127.0.0.1"
#define BUFFER_SIZE 2048

int main()
{
    int                sock;
    struct sockaddr_in server_addr;
    char               receive_buffer[BUFFER_SIZE];
    char               send_buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1)
    {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        return EXIT_FAILURE;
    }

    //comunicarea cu server-ul
    while(1)
    {
        printf(">>> ");
        scanf("%s", send_buffer);

        if(send(sock, send_buffer, strlen(send_buffer), 0) == -1)
        {
            perror("send");
            break;
        }

        //Instead of overwriting the entire buffer we only null terminate after the read bytes
        int read_bytes = recv(sock, receive_buffer, BUFFER_SIZE, 0);
        if(read_bytes < 1)
        {
            perror("recv");
            break;
        }
        receive_buffer[read_bytes] = 0;

        printf("::: %s :::\n", receive_buffer);

        if(strcmp(send_buffer, "exit") == 0)
        {
            break;
        }
    }

    close(sock);

    return EXIT_SUCCESS;
}

/*
   >>> user
   ::: admin :::
   >>> time
   ::: Thu Apr  4 13:27:02 2024 :::
   >>> dasdas
   ::: echo :: dasdas :::
 */
