/**
 * Abrudan Paul - Andrei
 * IA3 2023, subgrupa 1
 * Tema 3.2.2
 * Programul implementeaza o varianta de baza a unui shell, cu cerinta adaugata
 */

#include <stdio.h>    //input output
#include <stdlib.h>   //getenv, exit statuses
#include <unistd.h>   //exec chdir and other commands
#include <string.h>   //String operations
#include <sys/wait.h> //For waiting for parent on fork
#include <signal.h>   //CTRL+D handling

#define MAX_ARGS     32
#define MAX_ARG_LEN  256
#define MAX_COMMANDS 10


//Use to parse the command into a char** by splitting on the space character
void parse_command(char* command, char** argv)
{
    char* token;
    int   argc = 0;

    token = strtok(command, " ");

    while(token != NULL)
    {
        argv[argc++] = token;
        token        = strtok(NULL, " ");
    }

    argv[argc] = NULL; //Terminate argument array with null
}

//Custom implementation for running a command
int custom_system(char* command)
{
    //Forking so we can use exec command family properly
    pid_t pid = fork();
    char* argv[MAX_ARGS];
    int   status;

    parse_command(command, argv);

    if(pid == -1)
    {
        perror("Fork failed");
        return -1;
    }
    else if(pid == 0)
    {
        //Forward signal
        signal(SIGINT, SIG_DFL);
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        waitpid(pid, &status, 0);
        if(WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }
        else if(WIFSIGNALED(status))
        {
            return WTERMSIG(status);
        }
        else
        {
            return status; //Other case, return status
        }
    }
}


void adapted_system(char* commandLine)
{
    char* commands[MAX_COMMANDS];
    char* command;
    int   i = 0;

    //Parse commands on ";"
    command = strtok(commandLine, ";");

    while(command != NULL && i < MAX_COMMANDS)
    {
        commands[i++] = command;
        command       = strtok(NULL, ";");
    }

    //Run all commands
    for(int j = 0; j < i; j++)
    {
        int status = custom_system(commands[j]);
        printf("Status: %d\n", status);
    }
}

void login(char* const username, char* const password)
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
            return;
        }
    }

    printf("Login failed.\n");
    exit(EXIT_FAILURE);
}

void shell()
{
    char command[256];

    while(1)
    {
        printf("<> ");
        //fgets returns NULL when EOF is detected or when there is an error
        //CTRL+D sends an EOF so we can use this to close the shell
        if(fgets(command, sizeof(command), stdin) == NULL)
        {
            //CTRL+D pressed
            printf("\nExiting shell.\n");
            exit(EXIT_SUCCESS);
        }

        command[strcspn(command, "\n")] = 0; //Remove trailing newline

        //Command is cd
        if((strncmp(command, "cd", 2) == 0) && ((command[2] == ' ') || (command[2] == '\0')))
        {
            //Offset is strlen("cd")
            char* path = command + 2;

            //Handle additional spaces after cd command
            while(*path == ' ')
            {
                path++;
            }

            if(*path == '\0')
            {
                //Change to HOME (~) if no path is specified
                path = getenv("HOME");
                if(path == NULL)
                {
                    printf("$HOME is not declared.\n");
                    continue;
                }
            }

            if(chdir(path) != 0)
            {
                perror("chdir");
            }

            continue;
        }

        if(strcmp(command, "exit") == 0)
        {
            break;
        }

        adapted_system(command);
    }
}



int main()
{
    //Allow children to be interrupted
    signal(SIGINT, SIG_IGN);

    char username[48], password[48];
    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; //Remove newline

    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0; //Remove newline

    login(username, password);
    shell();

    return 0;
}
