/*
 * Abrudan Paul - Andrei
 * IA 2024, subgrupa 1
 * Tema 5.2b
 */

#include <stdio.h>      //pentru printf și scanf
#include <string.h>     //pentru strlen
#include <sys/socket.h> //pentru socket
#include <netinet/in.h> //pentru structura INET
#include <unistd.h>     //pentru close
#include <arpa/inet.h>  //pentru inet_pton
#include <stdlib.h>     //pentru wait

#define SERVER_PORT 1312
#define SERVER_IP   "127.0.0.1"
#define BUFFER_SIZE 2048

//functia de afisare meniu
void print_menu()
{
    printf(">>> 1. Send Message\n");
    printf(">>> 2. Request Time\n");
    printf(">>> 3. Request User\n");
    printf(">>> 4. Execute Command\n");
    printf(">>> 5. Close Connection\n");
}

int main()
{
    int                sock;
    struct sockaddr_in server_addr;
    char               receive_buffer[BUFFER_SIZE];
    char               send_buffer[BUFFER_SIZE];
    char               authed = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1)
    {
        perror("Could not create socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Connect");
        return 1;
    }

    while(1)
    {
        char username[256], password[256];
        printf("Login: ");
        scanf("%255s", username);
        printf("Password: ");
        scanf("%255s", password);

        char login_buff[BUFFER_SIZE] = {0};

        snprintf(send_buffer, BUFFER_SIZE, "%s;%s", username, password);

        if(send(sock, send_buffer, strlen(send_buffer), 0) < 0)
        {
            perror("send");
            continue;
        }

        if(recv(sock, receive_buffer, BUFFER_SIZE, 0) < 0)
        {
            perror("recv");
            continue;
        }

        if(strcmp(receive_buffer, "LOGIN_SUCCESS") == 0)
        {
            break;
        }
        printf("%s\n", receive_buffer);
        printf("Bad USER/PASS\n");
    }

    print_menu();
    int option = 0;

    char user_input[BUFFER_SIZE];

    while(1)
    {
        printf("Select: ");
        scanf("%d", &option);
        getchar();

start:

        switch(option)
        {
            case 1:
            {
                printf("Enter message: ");
                fgets(user_input, BUFFER_SIZE, stdin);
                snprintf(send_buffer, BUFFER_SIZE, "ECHO;%.*s", BUFFER_SIZE - 18, user_input);
                send_buffer[strcspn(send_buffer, "\n")] = 0;
                break;
            }
            case 2:
            {
                strcpy(send_buffer, "GET_TIME;");
                break;
            }
            case 3:
            {
                strcpy(send_buffer, "GET_USER;");
                break;
            }
            case 4:
            {
                printf("Enter command: ");
                fgets(user_input, BUFFER_SIZE, stdin);
                user_input[strcspn(user_input, "\n")] = 0;
                snprintf(send_buffer, BUFFER_SIZE, "EXEC;%.*s", BUFFER_SIZE - 18, user_input);
                break;
            }
            case 5:
            {
                strcpy(send_buffer, "CLOSE;");
                break;
            }
            default:
            {
                printf("Invalid option\n");
                goto start;
            }
        }

        if(send(sock, send_buffer, strlen(send_buffer), 0) == -1)
        {
            perror("send");
            continue;
        }

        if(option == 5)
        {
            close(sock);
            break;
        }

        int recv_bytes = recv(sock, receive_buffer, BUFFER_SIZE, 0);
        if(recv_bytes == -1)
        {
            perror("recv");
            continue;
        }
        receive_buffer[recv_bytes] = 0;
        printf("::: %s :::\n", receive_buffer);

        print_menu();
    }

    return 0;
}

/*

   gcc client2.c -o client2 && ./client2
   Login: foo
   Password: zar
   LOGIN_FAILURE
   Bad USER/PASS
   Login: foo
   Password: bar
   >>> 1. Send Message
   >>> 2. Request Time
   >>> 3. Request User
   >>> 4. Execute Command
   >>> 5. Close Connection
   Select: 1
   Enter message: asdkasdoak
   ::: asdkasdoak :::
   >>> 1. Send Message
   >>> 2. Request Time
   >>> 3. Request User
   >>> 4. Execute Command
   >>> 5. Close Connection
   Select: 1
   Enter message: zzzz
   ::: zzzz :::
   >>> 1. Send Message
   >>> 2. Request Time
   >>> 3. Request User
   >>> 4. Execute Command
   >>> 5. Close Connection
   Select: 2
   ::: Thu Apr  4 14:55:04 2024 :::
   >>> 1. Send Message
   >>> 2. Request Time
   >>> 3. Request User
   >>> 4. Execute Command
   >>> 5. Close Connection
   Select: 3
   ::: admin :::
   >>> 1. Send Message
   >>> 2. Request Time
   >>> 3. Request User
   >>> 4. Execute Command
   >>> 5. Close Connection
   Select: 4
   Enter command: ls
   ::: client1.c
   client2
   client2.c
   server1.c
   server2
   server2.c
   server_logs.log
   :::
   >>> 1. Send Message
   >>> 2. Request Time
   >>> 3. Request User
   >>> 4. Execute Command
   >>> 5. Close Connection
   Select: 4
   Enter command: ls | grep .c
   ::: client1.c
   client2.c
   server1.c
   server2.c
   :::
   >>> 1. Send Message
   >>> 2. Request Time
   >>> 3. Request User
   >>> 4. Execute Command
   >>> 5. Close Connection
   Select: 4
   Enter command: ls | grep asdakozkdoas
   ::: Empty response :::
   >>> 1. Send Message
   >>> 2. Request Time
   >>> 3. Request User
   >>> 4. Execute Command
   >>> 5. Close Connection
   Select: 4
   Enter command: asd
   ::: sh: line 1: asd: command not found
   :::
   >>> 1. Send Message
   >>> 2. Request Time
   >>> 3. Request User
   >>> 4. Execute Command
   >>> 5. Close Connection
   Select: 5

 */
