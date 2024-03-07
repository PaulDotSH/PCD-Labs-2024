/**
 * Abrudan Paul - Andrei
 * IA3 2023, subgrupa 1
 * Tema 2.1
 * Programul de mai jos primeste ca argument numarul de procese pe care trebuie sa il creeze
 * Acesta creeaza procesele A -> B -> 0 -> (1..=n) unde n este numarul de procese dat ca argument
 * Am tratat urmatoarele situatii limita care pot aparea in
 * contextul programului de mai jos :
 * -- numar argumente incorect
 * -- argumente incorecte
 * -- valori incorecte pentru argumentele date (nu sunt numerice, valoarea nu se conformeaza invariantilor etc.)
 */
#include <stdio.h>    //Used for standard IO operations, fprinf, prinf etc
#include <stdlib.h>   //Used for the exit function
#include <unistd.h>   //Used for fork(), getpid() and getppid() (creating processes and getting data about them)
#include <sys/wait.h> //Used to wait for children processes to die
#include "string.h"   //Strchar for strtol checking
#include <getopt.h>   //Used for argument parsing
#include "errno.h"    //Used for strtol conversion checking

void print_help(char* file_name)
{
    printf("Usage: %s [-p --processes] <count>\n", file_name);
    printf("Options:\n");
    printf("  -p, --processes <count>          Specify the subprocess count to spawn under process 0, total process count is <count> + 3.\n");
    printf("  -h, --help                       Display the help message and exit.\n");
}

void bad_aguments(char* file_name)
{
    print_help(file_name);
    exit(EXIT_FAILURE);
}

static struct option options[] = {
    {"processes", required_argument, 0, 'p'}
    ,{"help",      no_argument,       0, 'h'}
    ,{0,           0,                 0, 0  }        //Marks the end of the struct
};

int main(int argc, char* argv[])
{
    int  opt;
    long proc_count = 0;

    while((opt = getopt_long(argc, argv, "p:h", options, NULL)) != -1) //The documentation says that getopt_long returns -1 when it's "done"
    {
        switch(opt)
        {
            case 'p':
            {
                if(optarg[0] == '-') //strtol converts negative numbers too
                {
                    fprintf(stderr, "The number must be positive\n");
                    exit(EXIT_FAILURE);
                }
                if(!strchr("0123456789", optarg[0])) //Apparently it doesn't check the first character either properly
                {
                    fprintf(stderr, "The provided string \"%s\" is not a valid number\n", optarg);
                    exit(EXIT_FAILURE);
                }
                char* endptr = NULL;
                proc_count = strtol(optarg, &endptr, 10);   //"Safe" convert with error checking
                if((endptr == optarg) || (*endptr != '\0')) //Checks if the string contains an invalid character (not a digit)
                {
                    fprintf(stderr, "The provided string \"%s\" is not a valid number\n", optarg);
                    exit(EXIT_FAILURE);
                }
                if(errno != 0) //Overflow and underflow check
                {
                    fprintf(stderr, "The provided number \"%s\" is too large\n", optarg);
                    exit(EXIT_FAILURE);
                }
                if(proc_count == 0)
                {
                    fprintf(stderr, "The provided number cannot be 0\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 'h':
            {
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
            }
            default:
            {
                bad_aguments(argv[0]);
            }
        }
    }

    if(proc_count == 0)
    {
        bad_aguments(argv[0]);
    }

    pid_t pid_A, pid_B, pid_0, pid_child;

    //There definitely should be an easier way of doing this, but I don't know how
    //Create process A
    pid_A = fork();
    if(pid_A < 0)
    {
        fprintf(stderr, "Failed to fork process A.\n");
        exit(EXIT_FAILURE);
    }
    else if(pid_A == 0)
    {
        //In process A
        printf("Proces[A] PID %d PPID %d\n", getpid(), getppid());
        //Create process B
        pid_B = fork();
        if(pid_B < 0)
        {
            fprintf(stderr, "Failed to fork process B.\n");
            exit(EXIT_FAILURE);
        }
        else if(pid_B == 0)
        {
            //In process B
            printf("Proces[B] PID %d PPID %d\n", getpid(), getppid());
            //Create process 0
            pid_0 = fork();
            if(pid_0 < 0)
            {
                fprintf(stderr, "Failed to fork process 0.\n");
                exit(EXIT_FAILURE);
            }
            else if(pid_0 == 0)
            {
                //In process 0
                printf("Proces[0] PID %d PPID %d\n", getpid(), getppid());

                for(long i = 1; i <= proc_count; i++)
                {
                    pid_child = fork();
                    if(pid_child < 0)
                    {
                        fprintf(stderr, "Failed to fork process %ld.\n", i);
                        exit(EXIT_FAILURE);
                    }
                    else if(pid_child == 0)
                    {
                        //In one of the children process
                        printf("Proces[%ld] PID %d PPID %d\n", i, getpid(), getppid());
                        exit(EXIT_SUCCESS); //Exit child
                    }
                    else
                    {
                        //In process 0, waiting for children processes to finish
                        wait(NULL);
                    }
                }

                exit(0); //Process 0 exits after creating all children and they finish
            }
            else
            {
                //In process B, waiting for process 0 to finish
                wait(NULL);
                exit(EXIT_SUCCESS);
            }
        }
        else
        {
            //In process A, waiting for process B to finish
            wait(NULL);
            exit(EXIT_SUCCESS);
        }
    }
    else
    {
        //In the original process, waiting for process A to finish
        wait(NULL);
    }

    exit(EXIT_SUCCESS);
}
/*> Exemple de compilare si rulare a programului

   gcc app1.c -o app1

   ./app1 -p 3

   Proces[A] PID 70370 PPID 70369
   Proces[B] PID 70371 PPID 70370
   Proces[0] PID 70372 PPID 70371
   Proces[1] PID 70373 PPID 70372
   Proces[2] PID 70374 PPID 70372
   Proces[3] PID 70375 PPID 70372


   ./app1 -p 0
   The provided number cannot be 0

   ./app1 -p -10
   The number must be positive

   ./app1 -p asd
   The provided string "asd" is not a valid number

   ./app1 --processes 3
   Proces[A] PID 85886 PPID 85885
   Proces[B] PID 85887 PPID 85886
   Proces[0] PID 85888 PPID 85887
   Proces[1] PID 85889 PPID 85888
   Proces[2] PID 85890 PPID 85888
   Proces[3] PID 85891 PPID 85888

   ./app1  --processes
   ./app1.out: option '--processes' requires an argument
   Usage: ./app1.out [-p --processes] <count>
   Options:
   -p, --processes <count>          Specify the subprocess count to spawn under process 0, total process count is <count> + 3.
   -h, --help                       Display the help message and exit.

   ./app1 asd
   Usage: ./app1.out [-p --processes] <count>
   Options:
   -p, --processes <count>          Specify the subprocess count to spawn under process 0, total process count is <count> + 3.
   -h, --help                       Display the help message and exit.
 */
