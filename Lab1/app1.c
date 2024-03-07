/**
 * Abrudan Paul - Andrei
 * IA3 2023, subgrupa 1
 * Tema 1.1
 * Programul urmator primeste pe linia de comanda diferite argumente si incearca se emuleze o varianta basic a comenzii ls implementata in shell-uri
 * Are un singur argument necesar (-p sau --path) care ii arata path-ul folder-ului de unde sa caute fisierele
 * Acest program are si argumentele -d (--detailed) care afiseaza diverse informatii in plus despre fisiere
 * -a (--all) pentru a afisa si fisierele ascunse
 * -h (--help) pentru a arata utilizatorului cum se foloseste programul
 * -m <numar> (--max) pentru a afisa cel mult X fisiere.
 * Am tratat urmatoarele situatii limita care pot aparea in
 * contextul programului de mai jos :
 * -- numar argumente incorect
 * -- valori incorecte pentru argumentele date (max nu este numeric, este 0 sau negativ)
 */

#include <stdio.h>    //IO, including perror
#include <stdlib.h>   //Used for exit statuses and exit function
#include <getopt.h>   //Used for argument parsing
#include <string.h>   //Used for the function to handle blacklisted characters in the commented code (read comment) and for strchr (validation of entered number)
#include <dirent.h>   //Directory stream and directory entry
#include <sys/stat.h> //stat function for file information
#include <time.h>     //For time formatting (ctime function)
#include <pwd.h>      //Get username from uid
#include <errno.h>    //strtol error checking

static struct option options[] = {
    {"path",     required_argument, 0, 'p'}
    ,{"all",      no_argument,       0, 'a'}
    ,{"detailed", no_argument,       0, 'd'}
    ,{"help",     no_argument,       0, 'h'}
    ,{"max",      optional_argument, 0, 'm'}
    ,{0,          0,                 0, 0  } //Marks the end of the struct
};

//Separated this function to have the --help argument
void print_help(char* file_name)
{
    printf("Usage: %s -p <path> [options]\n", file_name);
    printf("Options:\n");
    printf("  -p, --path <path>                Specify path where to list the files.\n");
    printf("  -a, --all                        Shows all files (including hidden ones).\n");
    printf("  -d, --detailed                   Sets the output mode to \"detailed\", having more information.\n");
    printf("  -h, --help                       Display the help message and exit.\n");
    printf("  -m, --max <number>               Sets the maximum number of entries to show, defaulting to 1000.\n");
}

//Used to avoid code duplication
void bad_aguments(char* file_name)
{
    print_help(file_name);
    exit(EXIT_FAILURE);
}

//https://man7.org/linux/man-pages/man3/readdir.3.html
char parse_ftype(unsigned char ftype)
{
    switch(ftype)
    {
        case DT_BLK: //Block device.
        {
            return 'b';
        }
        case DT_CHR: //Character device.
        {
            return 'c';
        }
        case DT_DIR: //Directory
        {
            return 'D';
        }
        case DT_FIFO: //Named pipe
        {
            return 'p';
        }
        case DT_LNK:
        {
            return 'L'; //Symlink
        } case DT_REG:
        {
            return 'F'; //Regular file
        } case DT_SOCK:
        {
            return 's'; //Socket
        } case DT_UNKNOWN:
        default:
        {
            return '?';
        }
    }
}

//https://man7.org/linux/man-pages/man3/getpwuid.3p.html
//Gets the username from a uid
char* get_user(uid_t uid)
{
    struct passwd* pws;
    pws = getpwuid(uid);
    return pws->pw_name;
}

int main(int argc, char** argv)
{
    int    opt;
    char   all  = 0, detailed = 0;
    size_t max  = 1000; //Default value
    char*  path = NULL;

    //p and m require a value, so we add the : character after them
    while((opt = getopt_long(argc, argv, "p:adhm::", options, NULL)) != -1) //The documentation says that getopt_long returns -1 when it's "done"
    {
        switch(opt)
        {
            case 'p':
            {
                path = optarg;
                break;
            }
            case 'a':
            {
                all = 1;
                break;
            }
            case 'd':
            {
                detailed = 1;
                break;
            }
            case 'h':
            {
                print_help(argv[0]);
                return EXIT_SUCCESS;
            }
            case 'm':
            {
                if(optarg == NULL)
                {
                    break;
                }
                if(optarg[0] == '-') //strtol converts negative numbers too
                {
                    fprintf(stderr, "The number must be positive\n");
                    return EXIT_FAILURE;
                }
                if(!strchr("0123456789", optarg[0])) //Apparently it doesn't check the first character either properly
                {
                    fprintf(stderr, "The provided string \"%s\" is not a valid number\n", optarg);
                    return EXIT_FAILURE;
                }
                char* endptr = NULL;
                max = strtol(optarg, &endptr, 10);          //"Safe" convert with error checking
                if((endptr == optarg) || (*endptr != '\0')) //Checks if the string contains an invalid character (not a digit)
                {
                    fprintf(stderr, "The provided string \"%s\" is not a valid number\n", optarg);
                    return EXIT_FAILURE;
                }
                if(errno != 0) //Overflow and underflow check
                {
                    fprintf(stderr, "The provided number \"%s\" is too large\n", optarg);
                    return EXIT_FAILURE;
                }
                if(max == 0) //Conversion is good, but the number must be greater than 0, or else the program displays nothing
                {
                    fprintf(stderr, "The number must be greater than 0\n");
                    return EXIT_FAILURE;
                }
            }
            break;
            default:
            {
                bad_aguments(argv[0]);
            }
        }
    }

    if(path == NULL) //required argument
    {
        bad_aguments(argv[0]);
    }

    //Because linux doesn't have forbidden characters in a path (or at least according to stackoverflow), we will not check for the case like we would do on windows (characters like ? | > < etc)
    //If we had the same limitations as windows, we would've done something like
    //The code is tested and works but linux accepts these characters so we are going to leave it commented
    //char* invalid_chars = "><:\"/\\|?*";
    //if(strpbrk(path, invalid_chars)){
    //fprintf(stderr, "The path is incorrect, it must not contain any of the following characters %s\n", invalid_chars);
    //return EXIT_FAILURE;
    //}

    DIR* directory_stream;

    directory_stream = opendir(path);
    if(directory_stream == NULL)
    {
        perror(""); //Print the error with which opendir fails
        return EXIT_FAILURE;
    }

    struct dirent* directory_entry;

    size_t file_index = 0;

    while((directory_entry = readdir(directory_stream)) != NULL)
    {
        struct stat file_stats; //Used for holding file stats such as size, mode etc

        //Use stat to get the file status information
        if(stat(path, &file_stats) != 0) //Check if the stat syscall errors
        //Print an error message if stat fails
        {
            perror(path);
            closedir(directory_stream); //Close the directory stream before exiting, this might not be needed since the program ends
            return EXIT_FAILURE;
        }

        //Implementation of "all", skip the file if it starts with a "." (it is hidden)
        if((all == 0) && (directory_entry->d_name[0] == '.'))
        {
            continue;
        }

        //This is here because we only count files that were actually read (i.e. hidden files don't need to be counted)
        if(++file_index > max)
        {
            return EXIT_SUCCESS;
        }

        if(detailed)
        {
            char f_type = parse_ftype(directory_entry->d_type);             //Parsing the d_type into a custom character
            printf("%lu\t%s\t%s\t%ld bytes\t%c\t%s", directory_entry->d_ino //i_node, %lu is defined __ino_t -> __INO_T_TYPE -> __SYSCALL_ULONG_TYPE -> __ULONGWORD_TYPE -> unsigned long int
                , get_user(file_stats.st_uid)                               //username from userid
                , directory_entry->d_name                                   //file name
                , file_stats.st_size                                        //file size in bytes
                , f_type                                                    //file type
                , ctime(&file_stats.st_mtim.tv_sec)                         //https://stackoverflow.com/questions/32438355/how-to-print-date-and-time-returned-by-stat-function
                );
        }
        else
        {
            //Simple printing like ls does
            printf("%s\t", directory_entry->d_name);
        }
    }

    return EXIT_SUCCESS;
}

/*> Exemple de compilare si rulare a programului

   gcc app1.c -o app1
   SAU
   ./build.sh

   ./app1 -z
   ./app1: invalid option -- 'z'
   Usage: ./app1 [options]
   Options:
   -p, --path <path>                Specify path where to list the files.
   -a, --all                        Shows all files (including hidden ones).
   -d, --detailed                   Sets the output mode to "detailed", having more information.
   -h, --help                       Display the help and exit.
   -m, --max <number>               Sets the maximum number of entries to show, defaulting to 1000.


   ./app1 -p .
   CMakeLists.txt  main.c  cmake-build-debug       foo     a.out   app1

   ./app1 -p . -a
   .       ..      CMakeLists.txt  main.c  cmake-build-debug   foo     a.out   app1

   ./app1 -p . -da
   10627931        admin   .       108 bytes       D       Wed Feb 28 19:50:19 2024
   1900483 admin   ..      108 bytes       D       Wed Feb 28 19:50:19 2024
   10627954        admin   CMakeLists.txt  108 bytes       F       Wed Feb 28 19:50:19 2024
   10627955        admin   main.c  108 bytes       F       Wed Feb 28 19:50:19 2024
   10627976        admin   cmake-build-debug       108 bytes       D       Wed Feb 28 19:50:19 2024
   10628859        admin   foo     108 bytes       D       Wed Feb 28 19:50:19 2024
   10631663        admin   a.out   108 bytes       F       Wed Feb 28 19:50:19 2024
   10631819        admin   app1    108 bytes       F       Wed Feb 28 19:50:19 2024


   // Deoarece am facut m optional pentru a putea avea default value, -m 3 nu merge, dar -m3 merge, e un tradeoff pe care l-am facut pentru default value
   ./app1 -p . -m3
   CMakeLists.txt  main.c  cmake-build-debug

 */
