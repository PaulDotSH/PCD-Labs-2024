/*
 * Abrudan Paul - Andrei
 * IA 2024, subgrupa 1
 * Tema 6.1a
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT        1312
#define BUFFER_SIZE 2048

volatile sig_atomic_t KEEP_RUNNING = 1;

void sig_handler(int signal)
{
    (void)signal; //Get rid of unused parameter warning
    KEEP_RUNNING = 0;
}

//Wrapper for sendto
void send_response(int sockfd, struct sockaddr_in* cliaddr, socklen_t clilen, const char* message)
{
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)cliaddr, clilen);
}

//Custom system function...
int custom_system(int sockfd, struct sockaddr_in* cliaddr, socklen_t clilen, const char* command)
{
    int pipefds[2];
    pipe(pipefds);

    pid_t pid = fork();
    if(pid == -1)
    {
        perror("fork");
        return -1;
    }
    else if(pid == 0)
    {
        //Redirect stdout and stderr to pipe
        close(pipefds[0]);
        dup2(pipefds[1], STDOUT_FILENO);
        dup2(pipefds[1], STDERR_FILENO);
        close(pipefds[1]);

        //Execute the command in the child process
        execlp("/bin/sh", "sh", "-c", command, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {
        close(pipefds[1]);

        //Read the command output from the child process
        char    buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(pipefds[0], buffer, sizeof(buffer) - 1);
        if(bytes_read > 0)
        {
            buffer[bytes_read] = 0;                         //Null-terminate the string
            send_response(sockfd, cliaddr, clilen, buffer); //Send output to client
        }
        else
        {
            send_response(sockfd, cliaddr, clilen, "Failed to get output");
        }
        close(pipefds[0]);

        int status;
        waitpid(pid, &status, 0);

        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
}

void handler_uptime(int sockfd, struct sockaddr_in* cliaddr, socklen_t clilen)
{
    double seconds;
    FILE*  uptime_file = fopen("/proc/uptime", "r");
    if(uptime_file == NULL)
    {
        send_response(sockfd, cliaddr, clilen, "fopen error");
        return;
    }
    //Easily convert from the file
    fscanf(uptime_file, "%lf", &seconds);
    fclose(uptime_file);

    int days = (int)seconds / (24 * 3600);
    seconds -= days * (24 * 3600);
    int hours = (int)seconds / 3600;
    seconds -= hours * 3600;
    int minutes = (int)seconds / 60;
    //Create the uptime info string
    char uptime_info[BUFFER_SIZE];
    snprintf(uptime_info, BUFFER_SIZE, "Server is up for %dd %dh %dmin", days, hours, minutes);

    send_response(sockfd, cliaddr, clilen, uptime_info);
}

void handler_stats(int sockfd, struct sockaddr_in* cliaddr, socklen_t clilen)
{
    char response[BUFFER_SIZE];

    float load1, load5, load15;
    FILE* fp = fopen("/proc/loadavg", "r");
    if(fp == NULL)
    {
        strcpy(response, "Error on averages");
        send_response(sockfd, cliaddr, clilen, response);
        return;
    }

    fscanf(fp, "%f %f %f", &load1, &load5, &load15);
    fclose(fp);

    //CPU usage
    long double first[4], second[4], load_average;
    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &first[0], &first[1], &first[2], &first[3]);
    fclose(fp);
    sleep(1); //Wait a second
    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &second[0], &second[1], &second[2], &second[3]);
    fclose(fp);
    //calculate
    load_average = ((second[0] + second[1] + second[2]) - (first[0] + first[1] + first[2])) / ((second[0] + second[1] + second[2] + second[3]) - (first[0] + first[1] + first[2] + first[3]));
    int cpu_usage = (int)(load_average * 100); //CPU usage in percentages

    //Memory usage
    int total_memory, free_memory, buffers, cached, sreclaimable, shmem;
    fp = fopen("/proc/meminfo", "r");
    char line[BUFFER_SIZE];

    //Parse
    while(fgets(line, sizeof(line), fp))
    {
        if(strncmp(line, "MemTotal:", 9) == 0)
        {
            sscanf(line, "MemTotal: %d kB", &total_memory);
        }
        else if(strncmp(line, "MemFree:", 8) == 0)
        {
            sscanf(line, "MemFree: %d kB", &free_memory);
        }
        else if(strncmp(line, "Buffers:", 8) == 0)
        {
            sscanf(line, "Buffers: %d kB", &buffers);
        }
        else if(strncmp(line, "Cached:", 7) == 0)
        {
            sscanf(line, "Cached: %d kB", &cached);
        }
        else if(strncmp(line, "SReclaimable:", 13) == 0)
        {
            sscanf(line, "SReclaimable: %d kB", &sreclaimable);
        }
        else if(strncmp(line, "Shmem:", 6) == 0)
        {
            sscanf(line, "Shmem: %d kB", &shmem);
        }
    }

    fclose(fp);

    //Approximate calculation
    int used_memory = total_memory - free_memory - buffers - cached - sreclaimable + shmem;

    //Format
    snprintf(response, sizeof(response)
        , "Load Avg: %.2f, %.2f, %.2f  CPU Usage: %d%%  Memory Usage: Total: %d kB, Used: %d kB"
        , load1, load5, load15, cpu_usage, total_memory, used_memory);

    send_response(sockfd, cliaddr, clilen, response);
}
//
void handler_command_exec(int sockfd, struct sockaddr_in* cliaddr, socklen_t clilen, const char* command)
{
    //The command execution function is updated to capture output
    custom_system(sockfd, cliaddr, clilen, command);
}


int main()
{
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int                sockfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(PORT);

    if(bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while(KEEP_RUNNING)
    {
        char      buffer[BUFFER_SIZE];
        socklen_t len = sizeof(cliaddr);

        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&cliaddr, &len);
        if(n < 0)
        {
            perror("recvfrom");
            if(!KEEP_RUNNING)
            {
                break;
            }
            continue;
        }
        buffer[n] = 0; //Null terminate the string

        if(strcmp(buffer, "client_disconnect") == 0)
        {
            printf("Client disconnected.\n");
            break;
        }
        else if(strcmp(buffer, "quit") == 0)
        {
            printf("Quit command received, shutting down.\n");
            break;
        }


        if(strcmp(buffer, "uptime") == 0)
        {
            handler_uptime(sockfd, &cliaddr, len);
            printf("::: Sending current up time to client...\n");
        }
        else if(strcmp(buffer, "stats") == 0)
        {
            handler_stats(sockfd, &cliaddr, len);
            printf("::: Sending current stats to client...\n");
        }
        else if(strncmp(buffer, "cmd:", 4) == 0)
        {
            //If a command is actually sent
            if(strlen(buffer) > 4)
            {
                handler_command_exec(sockfd, &cliaddr, len, buffer + 4);
                printf("::: Sending some command output to client...\n");
            }
        }
        else
        {
            //Send an error message for invalid commands
            char error_msg[BUFFER_SIZE] = "Invalid command";
            sendto(sockfd, error_msg, strlen(error_msg), 0, (struct sockaddr*)&cliaddr, len);
        }
    }

    //Close the socket and exit
    printf("Shutting down...\n");
    close(sockfd);
    return 0;
}

/*
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
