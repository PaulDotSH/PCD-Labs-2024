/*
 * Abrudan Paul - Andrei
 * IA 2024, subgrupa 1
 * Tema 5.2a
 */

#include <stdio.h>      //IO
#include <string.h>     //strlen
#include <sys/socket.h> //socket
#include <netinet/in.h> //INET
#include <unistd.h>     //close
#include <time.h>       //time
#include <stdlib.h>     //exit
#include <sys/wait.h>   //wait

#define PORT        1312
#define BUFFER_SIZE 2048
#define LOG_FILE    "server_logs.log"
#define BACKLOG     5

void server_log(const char* msg)
{
    FILE* log_file = fopen(LOG_FILE, "a");
    if(log_file == NULL)
    {
        perror("Could not open the log file");
        exit(EXIT_FAILURE);
    }

    time_t now         = time(NULL);
    char*  time_string = ctime(&now);
    time_string[strlen(time_string) - 1] = '\0'; //Remove trailing newline

    fprintf(log_file, "[%s] %s\n", time_string, msg);
    fclose(log_file);
}

char is_login_correct(char* const username, char* const password)
{
    FILE* file = fopen("/tmp/credentials.txt", "r");
    if(!file)
    {
        perror("Cannot open credentials file");
        exit(EXIT_FAILURE);
    }

    char fileLine[512];

    while(fgets(fileLine, sizeof(fileLine), file))
    {
        char* newline = strchr(fileLine, '\n');
        if(newline)
        {
            *newline = 0; //Remove trailing newline

        }
        char* storedUser = strtok(fileLine, ";");
        char* storedPass = strtok(NULL, "\n");

        //Both user and pass are found, and they are the same as expected from stdin
        if(storedUser && storedPass && (strcmp(username, storedUser) == 0) && (strcmp(password, storedPass) == 0))
        {
            printf("Logged in!\n");
            return 1;
        }
    }

    return 0;
}

//Using pipes to talk and set the output buffer
void execute_command(const char* command, char* output)
{
    int pipefds[2];
    if(pipe(pipefds) == -1)
    {
        perror("pipe");
        server_log("Error creating pipe");
        return;
    }

    pid_t pid = fork();
    if(pid == -1)
    {
        perror("fork");
        server_log("Error forking process");
        return;
    }
    else if(pid == 0)
    {
        close(pipefds[0]);
        dup2(pipefds[1], STDOUT_FILENO);
        dup2(pipefds[1], STDERR_FILENO);
        close(pipefds[1]);

        execlp("/bin/sh", "sh", "-c", command, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {
        char log_msg[BUFFER_SIZE];
        snprintf(log_msg, sizeof(log_msg), "Executing command: %s", command);
        server_log(log_msg);

        close(pipefds[1]);
        int bytesRead = read(pipefds[0], output, BUFFER_SIZE - 1);
        close(pipefds[0]);
        wait(NULL);
    }
}

int main()
{
    int                server_fd, client_socket;
    struct sockaddr_in address;
    int                address_length              = sizeof(address);
    char               receive_buffer[BUFFER_SIZE] = {0};
    char               send_buffer[BUFFER_SIZE]    = {0};
    char               logged_in                   = 0;

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket");
        server_log("Socket creation failed");
        return EXIT_FAILURE;
    }

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("bind");
        server_log("Bind failed");
        return EXIT_FAILURE;
    }

    if(listen(server_fd, BACKLOG) < 0)
    {
        perror("listen");
        server_log("Listen failed");
        return EXIT_FAILURE;
    }
    server_log("Server started and listening");

    if((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&address_length)) < 0)
    {
        perror("accept");
        server_log("Accept failed");
        return EXIT_FAILURE;
    }
    server_log("Client connected");

    while(1)
    {
        int bytes_read = read(client_socket, receive_buffer, BUFFER_SIZE - 1);
        if(bytes_read < 1)
        {
            printf("Client closed the connection.\n");
            server_log("Client closed the connection");
            break;
        }

        receive_buffer[bytes_read] = '\0';

        if(!logged_in)
        {
            char* username = strtok(receive_buffer, ";");
            char* password = strtok(NULL, ";");

            if((username != NULL) && (password != NULL) && is_login_correct(username, password))
            {
                logged_in = 1;
                char msg[] = "LOGIN_SUCCESS";
                send(client_socket, msg, strlen(msg), 0);
            }
            else
            {
                char msg[] = "LOGIN_FAILURE";
                send(client_socket, msg, strlen(msg), 0);
            }
            continue;
        }

        printf("%s\n", receive_buffer);

        if(strcmp(receive_buffer, "GET_TIME;") == 0)
        {
            time_t t           = time(NULL);
            char*  time_string = ctime(&t);
            time_string[strlen(time_string) - 1] = '\0';
            send(client_socket, time_string, strlen(time_string), 0);
            server_log("Time");
            continue;
        }

        if(strcmp(receive_buffer, "GET_USER;") == 0)
        {
            char* user = getlogin();
            send(client_socket, user, strlen(user), 0);
            server_log("User");
            continue;
        }

        if(strncmp(receive_buffer, "EXEC;", 5) == 0)
        {
            //send_buffer[0]=0;
            memset(send_buffer, 0, BUFFER_SIZE);
            execute_command(receive_buffer + 5, send_buffer);
            if(strlen(send_buffer) == 0)
            {
                send(client_socket, "Empty response", strlen("Empty response"), 0);
            }
            else
            {
                send(client_socket, send_buffer, strlen(send_buffer), 0);
            }
            continue;
        }

        if(strcmp(receive_buffer, "CLOSE;") == 0)
        {
            printf("Close command received. Shutting down.\n");
            server_log("Close command received. Shutting down");
            break;
        }

        if(strncmp(receive_buffer, "ECHO;", 5) == 0)
        {
            send(client_socket, receive_buffer + 5, strlen(receive_buffer + 5), 0);
        }
    }

    close(client_socket);
    close(server_fd);
    server_log("Server shutdown");
    return EXIT_SUCCESS;
}

/*

   gcc server2.c -o server2 && ./server2
   Logged in!
   ECHO;asdkasdoak
   ECHO;zzzz
   GET_TIME;
   GET_USER;
   EXEC;ls
   EXEC;ls | grep .c
   EXEC;ls | grep asdakozkdoas
   EXEC;asd
   CLOSE;
   Close command received. Shutting down.

 */
