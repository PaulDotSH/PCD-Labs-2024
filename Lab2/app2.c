/**
 * Abrudan Paul - Andrei
 * IA3 2023, subgrupa 1
 * Tema 2.2
 * Programul de mai jos primeste ca argument numarul de procese pe care trebuie sa il creeze
 * Acesta creeaza procesele A -> B -> 0 -> (1..=n) -> (0..=m)
 * unde n este numarul de procese dat ca argument, la fel si m, si fiecare proces este un
 * copil al procesului parinte m
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
    printf("Usage: %s [-p --processes] <count> [-s --subprocesses] <count>\n", file_name);
    printf("Options:\n");
    printf("  -p, --processes <count>          Specify the children count to spawn under process 0.\n");
    printf("  -s, --subprocesses <count>       Specify the subprocess count to spawn under each process.\n");
    printf("  -h, --help                       Display the help message and exit.\n");
}

void bad_aguments(char* file_name)
{
    print_help(file_name);
    exit(EXIT_FAILURE);
}

static struct option options[] = {
    {"processes",    required_argument, 0, 'p'}
    ,{"subprocesses", no_argument,       0, 's'}
    ,{"help",         no_argument,       0, 'h'}
    ,{0,              0,                 0, 0  }         //Marks the end of the struct
};

long convert_positive_strtol_panic(char* str)
{
    long ret;
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
    ret = strtol(str, &endptr, 10);          //"Safe" convert with error checking
    if((endptr == str) || (*endptr != '\0')) //Checks if the string contains an invalid character (not a digit)
    {
        fprintf(stderr, "The provided string \"%s\" is not a valid number\n", optarg);
        exit(EXIT_FAILURE);
    }
    if(errno != 0) //Overflow and underflow check
    {
        fprintf(stderr, "The provided number \"%s\" is too large\n", optarg);
        exit(EXIT_FAILURE);
    }
    if(ret == 0)
    {
        fprintf(stderr, "The provided number cannot be 0\n", optarg);
        exit(EXIT_FAILURE);
    }
    return ret;
}

int main(int argc, char* argv[])
{
    int  opt;
    long proc_count = 0, subproc_count = 0;

    while((opt = getopt_long(argc, argv, "p:hs:", options, NULL)) != -1) //The documentation says that getopt_long returns -1 when it's "done"
    {
        switch(opt)
        {
            case 'p':
            {
                proc_count = convert_positive_strtol_panic(optarg);
                break;
            }
            case 's':
            {
                subproc_count = convert_positive_strtol_panic(optarg);
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

    pid_t pid_A, pid_B, pid_0, pid_child, pid_sub;

    //There definitely should be an easier way of doing this, but I don't know how
    //Create process A
    pid_A = fork();
    if(pid_A < 0)
    {
        perror("Failed to fork process A"); //This example uses perror instead of fprintf used in app1.c
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
            perror("Failed to fork process B");
            exit(EXIT_FAILURE);
        }
        else if(pid_B == 0)
        {
            //Create process 0
            pid_0 = fork();
            if(pid_0 < 0)
            {
                perror("Failed to fork process C");
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
                        perror("Failed to fork process child");
                        exit(EXIT_FAILURE);
                    }
                    else if(pid_child == 0)
                    {
                        //In one of the children process
                        printf("Proces[%ld] PID %d PPID %d\n", i, getpid(), getppid());

                        long j = 1;
                        if (subproc_count != 0) {
                        while(j <= subproc_count)
                        {
                            pid_sub = fork();
                            if(pid_sub < -1)
                            {
                                //Fork failed
                                perror("Failed to fork subprocess");
                                exit(EXIT_FAILURE);
                            }
                            else if(pid_sub == 0)
                            {
                                //Subproc
                                printf("Proces[%ld.%ld] PID %d PPID %d\n", i, j++, getpid(), getppid());
                            }
                            else
                            {
                                //Parent
                                wait(NULL); //Wait for the subproc to complete
                                exit(EXIT_SUCCESS);
                            }
                        }
                        }

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

./app2 
Usage: ./app2 [-p --processes] <count> [-s --subprocesses] <count>
Options:
  -p, --processes <count>          Specify the children count to spawn under process 0.
  -s, --subprocesses <count>       Specify the subprocess count to spawn under each process.
  -h, --help                       Display the help message and exit.

./app2 -p 3 -s 4
Proces[A] PID 117550 PPID 117549
Proces[0] PID 117552 PPID 117551
Proces[1] PID 117553 PPID 117552
Proces[1.1] PID 117554 PPID 117553
Proces[1.2] PID 117555 PPID 117554
Proces[1.3] PID 117556 PPID 117555
Proces[1.4] PID 117557 PPID 117556
Proces[2] PID 117558 PPID 117552
Proces[2.1] PID 117559 PPID 117558
Proces[2.2] PID 117560 PPID 117559
Proces[2.3] PID 117561 PPID 117560
Proces[2.4] PID 117562 PPID 117561
Proces[3] PID 117563 PPID 117552
Proces[3.1] PID 117564 PPID 117563
Proces[3.2] PID 117565 PPID 117564
Proces[3.3] PID 117566 PPID 117565
Proces[3.4] PID 117567 PPID 117566

./app2 -p 3
Proces[A] PID 118778 PPID 118777
Proces[0] PID 118780 PPID 118779
Proces[1] PID 118781 PPID 118780
Proces[2] PID 118782 PPID 118780
Proces[3] PID 118783 PPID 118780

./app2 -p 3 -s -10
The number must be positive

./app2 -p 3 -s asd
The provided string "asd" is not a valid number

*/