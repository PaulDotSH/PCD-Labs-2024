/**
 * Abrudan Paul - Andrei
 * IA3 2023, subgrupa 1
 * Tema 1.2
 * Acest program are ca scop afisarea unui mesaj, din acesta pot fi taiati un nr de bytes, de la inceput sau final, dar si sa convertim "case"-ul fiecarui
 * caracter, la uppercase, lowercase sau sa il inversam.
 * Am tratat urmatoarele situatii limita care pot aparea in
 * contextul programului de mai jos :
 * -- numar argumente incorect
 * -- valori incorecte pentru argumentele date (argumentele nu sunt numerice, este negativ)
 */

#include <stdio.h>  //IO
#include <stdlib.h> //Used for exit statuses and exit function
#include <getopt.h> //Used for argument parsing
#include <string.h> //Used for the function to handle string operations (i.e. strchr)
#include <errno.h>  //strtol error checking


static struct option options[] = {
    {"message",     required_argument, 0, 'm'}
    ,{"no-newline",  no_argument,       0, 'n'}
    ,{"end",         no_argument,       0, 'e'}
    ,{"start",       no_argument,       0, 's'}
    ,{"upper",       no_argument,       0, 'U'}
    ,{"lower",       no_argument,       0, 'l'}
    ,{"invert-case", no_argument,       0, 'i'}
    ,{"help",        no_argument,       0, 'h'}
    ,{0,             0,                 0, 0  }//Marks the end of the struct
};

//Separated this function to have the --help argument
void print_help(char* file_name)
{
    printf("Usage: %s -m <message> [options]\n", file_name);
    printf("The order in which options are applied to the message is: ");
    printf("Options:\n");
    printf("  -m, --message <message>                 Specify the message to be printed.\n");
    printf("  -n, --no-newline                        Skips the printing of the trailing '\\n'.\n");
    printf("  -s, --start <byte_count>                Specifies how many bytes to cut from \"start\" of the message.\n");
    printf("  -e, --end <byte_count>                  Specifies how many bytes to cut from \"end\" of the message.\n");
    printf("  -U, --upper                             Convert the character to uppercase if possible.\n");
    printf("  -l, --lower                             Convert the character to lowercase if possible.\n");
    printf("  -i, --invert-case                       Invert the character case if possible.\n");
    printf("  -h, --help                              Display the help message and exit.\n");
}

//Used to avoid code duplication
void bad_aguments(char* file_name)
{
    print_help(file_name);
    exit(EXIT_FAILURE);
}

//Couldn't think of a better name
long parse_positive_int_or_exit(char* str)
{
    if(!strchr("0123456789", str[0])) //The reason is also stated in app2, strtol in my case didn't work if the first chr was a non-digit.
    {
        fprintf(stderr, "The provided string \"%s\" is not a valid number\n", str);
        exit(EXIT_FAILURE);
    }

    char* endptr = NULL;
    long  ret    = strtol(str, &endptr, 10);
    if((endptr == str) || (*endptr != '\0')) //Checks if the string contains an invalid character (not a digit)
    {
        fprintf(stderr, "The provided string \"%s\" is not a valid number\n", str);
        return EXIT_FAILURE;
    }

    if(errno != 0) //Overflow and underflow check
    {
        fprintf(stderr, "The provided number \"%s\" is too large or too small\n", str);
        return EXIT_FAILURE;
    }

    if(ret < 0) //For our case the number can't be negative since it doesn't make sense
    {
        fprintf(stderr, "The number must be positive\n");
        return EXIT_FAILURE;
    }

    return ret;
}

//From the ascii table https://www.cs.cmu.edu/~pattis/15-1XX/common/handouts/ascii.html
char to_lower(char c)
{
    if((c >= 'A') && (c <= 'Z'))
    {
        c += 32;
    }
    return c;
}

//From the ascii table https://www.cs.cmu.edu/~pattis/15-1XX/common/handouts/ascii.html
char to_upper(char c)
{
    if((c >= 'a') && (c <= 'z'))
    {
        c -= 32;
    }
    return c;
}

//From the ascii table https://www.cs.cmu.edu/~pattis/15-1XX/common/handouts/ascii.html
char invert_case(char c)
{
    if((c >= 'A') && (c <= 'Z'))
    {
        c += 32;
    }
    else if((c >= 'a') && (c <= 'z'))
    {
        c -= 32;
    }
    return c;
}

int main(int argc, char** argv)
{
    int opt;

    //Variables that store the values from the arguments passed to the app
    char* msg              = NULL;
    long  start_cut_bytes  = 0, end_cut_bytes = 0;
    char  trailing_newline = 1, upper = 0, lower = 0, invert = 0;

    //p and m require a value, so we add the : character after them
    while((opt = getopt_long_only(argc, argv, "m:ne:s:Ulih", options, NULL)) != -1)     //The documentation says that getopt_long returns -1 when it's "done"
    {
        switch(opt)
        {
            case 'm':
            {
                msg = optarg;
                break;
            }
            case 'n':
            {
                trailing_newline = 0;
                break;
            }
            case 'e':
            {
                //To avoid code duplication, since all the invariants for e s E and S are the same, we can refactor into a separate function
                end_cut_bytes = parse_positive_int_or_exit(optarg);
                break;
            }
            case 's':
            {
                start_cut_bytes = parse_positive_int_or_exit(optarg);
                break;
            }
            case 'U':
            {
                upper = 1;
                break;
            }
            case 'l':
            {
                lower = 1;
                break;
            }
            case 'i':
            {
                invert = 1;
                break;
            }
            case 'h':
            {
                print_help(argv[0]);
                return EXIT_SUCCESS;
            }
            default:
            {
                bad_aguments(argv[0]);
            }
        }
    }

    if(msg == NULL) //required argument
    {
        bad_aguments(argv[0]);
    }

    char case_convert = invert + upper + lower;
    if(case_convert > 1) //"Smart" way to check if multiple case converts are used, without using if "chains"
    {
        fprintf(stderr, "The case converting arguments are exclusive, you must have none, or only one of them.\n");
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "%s -U //ok \n", argv[0]);
        fprintf(stderr, "%s -U -i //not ok \n", argv[0]);
        return EXIT_FAILURE;
    }

    //We can just shift the pointer to do a "cut" since
    msg += start_cut_bytes;

    //Just set the end of the string where needed and printf stops there
    if(end_cut_bytes > 0)
    {
        msg[end_cut_bytes + start_cut_bytes - 1] = 0;
    }

    //It is less work to first cut the characters then do the conversion, with the same result
    unsigned long msg_length = strlen(msg);

    //Parse lines
    if(case_convert == 1)
    {
        for(unsigned long i = 0; i != msg_length; i++)
        {
            if(lower) //Technically it could be faster having the if's outside the for and 3 different fors, however the compiler should be smart enough
            //to optimize this, if not, the branch predictor will
            {
                msg[i] = to_lower(msg[i]);
            }
            else if(upper)
            {
                msg[i] = to_upper(msg[i]);
            }
            else if(invert)
            {
                msg[i] = invert_case(msg[i]);
            }
        }
    }

    printf("%s", msg);

    if(trailing_newline)
    {
        putc('\n', stdout);
    }

    return EXIT_SUCCESS;
}

/*> Exemple de compilare si rulare a programului

   gcc app2.c -o app2
   SAU
   ./build.sh

   ./app2 -m abcdEFGH123 -s 3 -e 3
   dEFGH

   ./app2 -m abcdEFGH123 -U -s 3 -e 3
   DEFGH

   ./app2 -m abcdEFGH123 -l -s 3 -e 3
   defgh

   ./app2 -m abcdEFGH123 -i -s 3 -e 3
   Defgh

   ./app2 -m abcdef123 -U
   ABCDEF123

   ./app2 -m abcdef123 -Ui
   The case converting arguments are exclusive, you must have none, or only one of them.
   Example:
   ./app2 -U //ok
   ./app2 -U -i //not ok

   ./app2 -m "foo" -n
   foo⏎

   ./app2 -m "foo"
   foo

   ./app2 -message foo -invert-case
   FOO


 */
