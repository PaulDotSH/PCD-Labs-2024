/**
 * Abrudan Paul - Andrei
 * IA3 2023, subgrupa 1
 * Tema 1.3
 * Acest program are ca scop copierea (sau mutarea) unui fisier
 * Am tratat urmatoarele situatii limita care pot aparea in
 * contextul programului de mai jos :
 * -- numar argumente incorect
 * -- valori incorecte pentru argumentele date (argumentele nu sunt numerice,
 * este negativ)
 */

#include "argp.h"     //Argument parsing
#include "unistd.h"   //For checking if a file exists
#include <errno.h>    //Directory stream and directory entry
#include <stdint.h>   //uint8_t type for type clarity
#include <stdio.h>    //IO, including perror
#include <stdlib.h>   //Used for exit statuses and exit function
#include <string.h>   //Used for the function to handle blacklisted characters in the commented code (read comment) and for strchr (validation of entered number)
#include <sys/stat.h> //For stat, usage is optimization here

struct arguments
{
    char*   src;
    char*   dest;
    uint8_t overwrite;
    long    buffer_size;
    uint8_t move;
};

static struct argp_option options[] = {
    {"source",      's', "PATH", 0, "Specify the source to the file."                                 }
    ,{"destination", 'd', "PATH", 0, "Specify the destination to the file"                             }
    ,{"overwrite",   'o', 0,      0, "Overwrites the file instead of erroring out."                    }
    ,{"move",        'm', 0,      0, "Deletes the original file after copying, acting like a \"move\"."}
    ,{"buffer-size", 'b', "SIZE", 0, "Specifies a specific buffer size in bytes."                      }
    ,{"help",        'h', 0,      0}
    ,{0}
};

static error_t parse_args(int key, char* arg, struct argp_state* state)
{
    struct arguments* arguments = state->input;

    switch(key)
    {
        case 's':
        {
            arguments->src = arg;
            break;
        }
        case 'd':
        {
            arguments->dest = arg;
            break;
        }
        case 'o':
        {
            arguments->overwrite = 1;
            break;
        }
        case 'm':
        {
            arguments->move = 1;
            break;
        }
        case 'b':
        {
            if(!strchr(
                    "-0123456789"
                    , arg[0]))
            { //The reason is also stated in app1 and app2, strtol in
              //my case didn't work if the first chr was a non-digit.
                fprintf(stderr, "The provided string \"%s\" is not a valid number\n", arg);
                exit(EXIT_FAILURE);
            }

            char* endptr = NULL;
            arguments->buffer_size = strtol(arg, &endptr, 10);
            if((endptr == arg) || (*endptr != '\0'))
            { //Checks if the string contains an
              //invalid character (not a digit)
                fprintf(stderr, "The provided string \"%s\" is not a valid number\n", arg);
                exit(EXIT_FAILURE);
            }

            if(errno != 0)
            { //Overflow and underflow check
                fprintf(stderr, "The provided number \"%s\" is too large or too small\n", arg);
                exit(EXIT_FAILURE);
            }
            if(arguments->buffer_size < 1)
            { //For our case the number can't be negative since it doesn't make
              //sense for a buffer size to be less than 1
                fprintf(stderr, "The number must be higher than 0\n");
                exit(EXIT_FAILURE);
            }
        }
        break;
        case 'h':
        {
            argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
            exit(EXIT_SUCCESS);
        }
        case ARGP_KEY_END:
        {
            break;
        }
        default:
        {
            return ARGP_ERR_UNKNOWN;
        }
    }

    return 0;
}

void init_arguments(struct arguments* arguments)
{
    arguments->src         = NULL;
    arguments->dest        = NULL;
    arguments->overwrite   = 0;
    arguments->buffer_size = 0;
    arguments->move        = 0;
}

static struct argp argp = {options, parse_args, "", "Copy command"};

int main(int argc, char** argv)
{
    struct arguments arguments;
    init_arguments(&arguments);

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    //Check if the user provided src
    if(arguments.src == NULL)
    {
        fprintf(stderr
            , "The argument source must not be empty\nSee %s --help for help\n"
            , argv[0]);
        exit(EXIT_FAILURE);
    }

    //Check if the user provided dest
    if(arguments.dest == NULL)
    {
        fprintf(
            stderr
            , "The argument destination must not be empty\nSee %s --help for help\n"
            , argv[0]);
        exit(EXIT_FAILURE);
    }

    //If there is no overwrite, first check if a file exists at the destination
    if(arguments.overwrite == 0)
    { //https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
        if(access(arguments.dest, F_OK) == 0)
        {
            fprintf(stderr
                , "The file already exists at %s, please use --overwrite or "
                  "manually remove the file\n"
                , arguments.dest);
            exit(EXIT_FAILURE);
        }
    }

    //Error handling for source file
    FILE* source_file = fopen(arguments.src, "r");
    if(source_file == NULL)
    {
        perror("Failed: ");
        return EXIT_FAILURE;
    }

    struct stat src_stats;
    if(stat(arguments.src, &src_stats) != 0)
    { //Check if the stat syscall errors
        perror("Failed: ");
        return EXIT_FAILURE;
    }

    //Using ternary doesn't make this very readable, but it shouldn't be that bad
    //in this case, most new-er languages treat ifs as expressions instead of
    //statements so this could've been way more readable, but that's not how it
    //works in c
    const size_t buffer_size = (arguments.buffer_size == 0) ? ((src_stats.st_blksize > 1024) ? src_stats.st_blksize : 1024) : arguments.buffer_size;

    //The buffer should be blksize returned by stat for "optimal" IO, but since
    //this is technically not portable we check if blksize is less than 1024, in
    //the case that the user didn't specify a manual buffer size
    uint8_t* buffer = (uint8_t*)malloc(buffer_size);
    if(buffer == NULL)
    {
        fprintf(stderr, "Could not allocate a buffer with size %zu\n", buffer_size);
        exit(EXIT_FAILURE);
    }

    //Error handling for destination file
    FILE* destination_file = fopen(arguments.dest, "w");
    if(destination_file == NULL)
    {
        perror("Failed: ");
        return EXIT_FAILURE;
    }

    size_t read_bytes;

    //https://man7.org/linux/man-pages/man3/fread.3.html
    //Read the optimal size from a file and write it "stream-like" to the other
    //file
    while((read_bytes = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
    {
        fwrite(buffer, 1, read_bytes, destination_file);
    }

    fclose(source_file);
    fclose(destination_file);
    free(buffer);

    if(arguments.move)
    {
        if(remove(arguments.src) != 0)
        {
            printf("Unable to move the file %s, try without the move argument\n"
                , arguments.src);
            perror("Error");
            remove(arguments.dest); //This shouldn't fail since we created this file
                                    //in this process, perms shouldn't change while
                                    //the program is running
        }
    }

    return EXIT_SUCCESS;
}

/*> Exemple de compilare si rulare a programului

   gcc app3.c -o app3
   SAU
   ./build.sh

   ./app1 -s foo
   The argument destination must not be empty
   See ./app1 --help for help

   ./app1 -s foo -d bar
   The file already exists at bar, please use --overwrite or manually remove the
   file

   ./app1 -s foo -d bar -o

   ./app1 -s foo -d bar -o -b -1
   The number must be higher than 0

   ./app1 -s foo -d bar -o -b 1000000000000000000
   Could not allocate a buffer with size 1000000000000000000

   ./app1 -s foo -d bar -om

 */
