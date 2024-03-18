/**
 * Abrudan Paul - Andrei
 * IA3 2023, subgrupa 1
 * Tema 3.1.2-3.1.6
 * Programul curent este doar un exempul care executa alte procese deoarece am demonstrat
 * ca am inteles cum functioneaza familia exec in Linux, acestea sunt niste exemple de folosire
 * mult mai simple
 */

#include <stdio.h> //Operatii basic de IO
#include <sys/wait.h>
#include <unistd.h> //comenzi din familia exec si sleep
#include <stdlib.h> //malloc si altele

void example_execl()
{
    switch(fork())
    {
        //Fork failed for whatever reason
        case -1:
        {
            perror("Fork");
            exit(EXIT_FAILURE);
        }

        case 0: //Child process
        {
            execl("/bin/ls" //Absolute path is needed
                , "Ignored argument"
                , "-iahl"
                , NULL); //We must end the argument list with a 0

            perror("execl");

            exit(EXIT_FAILURE); //In case exec fails
        }

        default: //Parrent process
        {
            wait(NULL);
        }
    }
}

void example_execlp()
{
    switch(fork())
    {
        //Fork failed for whatever reason
        case -1:
        {
            perror("Fork");
            exit(EXIT_FAILURE);
        }

        case 0:
        {
            execlp("ls" //Uses PATH variable to find the binary file location (path)
                , ""    //Ignored argument
                , "-l"
                , NULL);

            perror("execlp");

            exit(EXIT_FAILURE); //In case exec fails
        }

        default: //Parrent process
        {
            wait(NULL);
        }
    }
}


void example_execle()
{
    switch(fork())
    {
        //Fork failed for whatever reason
        case -1:
        {
            perror("Fork");
            exit(EXIT_FAILURE);
        }

        case 0: /* child */
        {
            execle("/usr/bin/env" //Absolute path
                , ""              //Ignored argument
                , NULL            //Mark the end of argument list (char**)
                , (char* []) { //Set environment variables
                "BIND_ADDRESS=127.0.0.1:8080"
                , "THREAD_COUNT=16"
                , "OVERWRITE=false"
                , NULL //Mark the end of the char**
            });

            perror("execle");

            exit(EXIT_FAILURE); //In case exec fails
        }

        default: //Parrent process
        {
            wait(NULL);
        }
    }
}

void example_execvp()
{
    switch(fork())
    {
        //Fork failed for whatever reason
        case -1:
        {
            perror("Fork");
            exit(EXIT_FAILURE);
        }

        case 0:
        {
            execvp("echo" // Uses PATH too
                , (char* []) {
                ""             // Ignored argument
                , "-e"
                , "Test1234"
                , NULL
                ,
            });

            perror("execvp");

            exit(EXIT_FAILURE); //In case exec fails
        }

        default: //Parrent process
        {
            wait(NULL);
        }
    }
}

void example_execve()
{
    switch(fork())
    {
        //Fork failed for whatever reason
        case -1:
        {
            perror("Fork");
            exit(EXIT_FAILURE);
        }

        case 0:
        {
            execve("/usr/bin/env"
                , (char* []) {
                "" //Ignored argument
                , NULL
            }
                , (char* []) {
                "BIND_ADDRESS=127.0.0.1:8080"
                , "THREAD_COUNT=16"
                , NULL
            });

            perror("execve");

            exit(EXIT_FAILURE); //In case exec fails
        }

        default: //Parrent process
        {
            wait(NULL);
        }
    }
}

int main(int argc, char * argv[])
{
    printf("execl:\n");

    example_execl();

    printf("execlp:\n");

    example_execlp();

    printf("execle:\n");

    example_execle();

    printf("execvp:\n");

    example_execvp();

    printf("execve:\n");

    example_execve();

    exit(EXIT_SUCCESS);

}

/*>
execl:
total 28K
11410171 drwxr-xr-x 1 admin admin   46 Mar 14 16:04 .
10619349 drwxr-xr-x 1 admin admin   80 Mar 12 12:02 ..
11727319 -rwxr-xr-x 1 admin admin  16K Mar 14 16:04 a.out
11410176 -rw-r--r-- 1 admin admin 4.3K Mar 12 12:13 app1.1.c
11717624 -rw-r--r-- 1 admin admin 4.0K Mar 14 15:52 app1.2-6.c
execlp:
total 28
-rwxr-xr-x 1 admin admin 16128 Mar 14 16:04 a.out
-rw-r--r-- 1 admin admin  4369 Mar 12 12:13 app1.1.c
-rw-r--r-- 1 admin admin  4078 Mar 14 15:52 app1.2-6.c
execle:
BIND_ADDRESS=127.0.0.1:8080
THREAD_COUNT=16
OVERWRITE=false
execvp:
Test1234
execve:
BIND_ADDRESS=127.0.0.1:8080
THREAD_COUNT=16
*/