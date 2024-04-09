/*
 * Abrudan Paul - Andrei
 * IA 2024, subgrupa 1
 * Tema 6.2a
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>


#define SOCKET_PATH "/tmp/server.socket"
#define BUFFER_SIZE 2048

volatile sig_atomic_t KEEP_RUNNING = 1;

void sig_handler(int signal)
{
    (void)signal; //Remove unused parameter warning
    KEEP_RUNNING = 0;
}

//Wrapper
void send_response(int sockfd, const char* message)
{
    send(sockfd, message, strlen(message), 0);
}

int custom_system(int sockfd, const char* command)
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
        close(pipefds[0]);
        dup2(pipefds[1], STDOUT_FILENO);
        dup2(pipefds[1], STDERR_FILENO);
        close(pipefds[1]);

        execlp("/bin/sh", "sh", "-c", command, NULL);
        perror("execlp failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        close(pipefds[1]);

        char    buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(pipefds[0], buffer, sizeof(buffer) - 1);
        if(bytes_read > 0)
        {
            buffer[bytes_read] = 0;
            send_response(sockfd, buffer);
        }
        else
        {
            send_response(sockfd, "Failed to get command output");
        }
        close(pipefds[0]);

        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
}

void handler_uptime(int sockfd)
{
    //Read uptime from /proc/uptime
    double uptime_seconds;
    FILE*  uptime_file = fopen("/proc/uptime", "r");
    if(uptime_file == NULL)
    {
        send_response(sockfd, "fopen");
        return;
    }
    fscanf(uptime_file, "%lf", &uptime_seconds);
    fclose(uptime_file);

    int days = (int)uptime_seconds / (24 * 3600);
    uptime_seconds -= days * (24 * 3600);
    int hours = (int)uptime_seconds / 3600;
    uptime_seconds -= hours * 3600;
    int  minutes = (int)uptime_seconds / 60;
    char uptime_info[BUFFER_SIZE];
    snprintf(uptime_info, BUFFER_SIZE, "Server is up for %dd %dh %dmin", days, hours, minutes);
    send_response(sockfd, uptime_info);
}

void handler_stats(int sockfd)
{
    char response[2048];
    //Load averages
    float load1, load5, load15;
    FILE* fp = fopen("/proc/loadavg", "r");
    if(fp == NULL)
    {
        strcpy(response, "Error reading load averages");
        send_response(sockfd, response);
        return;
    }
    fscanf(fp, "%f %f %f", &load1, &load5, &load15);
    fclose(fp);

    //CPU usage
    long double first[4], second[4], loadavg;
    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &first[0], &first[1], &first[2], &first[3]);
    fclose(fp);
    sleep(1);
    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &second[0], &second[1], &second[2], &second[3]);
    fclose(fp);
    loadavg = ((second[0] + second[1] + second[2]) - (first[0] + first[1] + first[2])) / ((second[0] + second[1] + second[2] + second[3]) - (first[0] + first[1] + first[2] + first[3]));
    int cpu_usage = (int)(loadavg * 100);

    //Memory usage
    int total_memory, free_memory, buffers, cached, sreclaimable, shmem;
    fp = fopen("/proc/meminfo", "r");
    char line[BUFFER_SIZE];

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
    int used_memory = total_memory - free_memory - buffers - cached - sreclaimable + shmem;
    snprintf(response, sizeof(response)
        , "Load Avg: %.2f, %.2f, %.2f  CPU Usage: %d%%  Memory Usage: Total: %d kB, Used: %d kB"
        , load1, load5, load15, cpu_usage, total_memory, used_memory);
    send_response(sockfd, response);
}

void handler_command_exec(int sockfd, const char* command)
{
    const char* cmd_prefix = "cmd: ";
    if(strncmp(command, cmd_prefix, strlen(cmd_prefix)) != 0)
    {
        //Send an error message if prefix isnt set
        send_response(sockfd, "Error: Command must start with 'cmd: '\n");
        return;
    }
    //Extract
    const char* actual_command = command + strlen(cmd_prefix);
    custom_system(sockfd, actual_command);
}


int main()
{
    int                server_sockfd, client_sockfd;
    struct sockaddr_un server_addr, client_addr;
    char               buffer[BUFFER_SIZE];
    signal(SIGINT, sig_handler);

    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    //Setup
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    unlink(SOCKET_PATH);
    if(bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    listen(server_sockfd, 3);
    printf("Server is listening on path %s\n", SOCKET_PATH);

    while(KEEP_RUNNING)
    {
        socklen_t client_len = sizeof(struct sockaddr_un);
        client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_len);
        if(client_sockfd < 0)
        {
            perror("accept");
            continue;
        }
        printf("Client connected!\n");

        while(1)
        {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t num_bytes_read = read(client_sockfd, buffer, BUFFER_SIZE - 1);

            //EOF or error
            if(num_bytes_read <= 0)
            {
                // EOF
                if(num_bytes_read == 0)
                {
                    printf("Client disconnected.\n");
                    exit(EXIT_SUCCESS);
                }
                perror("read failed");
                break;
            }

            buffer[num_bytes_read] = 0; // Terminate buffer

            if(strncmp(buffer, "uptime", 6) == 0)
            {
                handler_uptime(client_sockfd);
            }
            else if(strncmp(buffer, "stats", 5) == 0)
            {
                handler_stats(client_sockfd);
            }
            else
            {
                handler_command_exec(client_sockfd, buffer);
            }
        }

        close(client_sockfd);
    }

    close(server_sockfd);
    unlink(SOCKET_PATH);
    printf("Server exited.\n");
    exit(EXIT_SUCCESS);
}

/*> Exemple de compilare si rulare a programului
 * gcc server.c -o server
 * gcc client.c -o client
 * ./server
 * ./client
 * Enter 'quit' to stop.
 * Enter message: uptime
 * Server response: Server is UP for 0d 6h 8min
 * Enter message: stats
 * Server response: Load Avg: 1.22, 1.25, 1.32  CPU Usage: 0%  Memory Usage: Total: 12331688 kB, Used: 5738116 kB
 * Enter message: cmd: ls
 * Server response: client
 * client.c
 * server
 * server.c
 * Enter message: ls
 * Server response: Error: Command must be prefixed with 'cmd: '
 * Enter message: quit
 */
