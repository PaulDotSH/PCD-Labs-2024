/**
 * Abrudan Paul - Andrei
 * IA3 2023, subgrupa 1
 * Tema 3.1.1
 * Programul de mai jos se comporta ca un daemon basic, acesta periodic (in fiecare secunda)
 * sterge fisierele cu o anumita extensie dintr-un folder, acesta se foloseste de un config file
 * daca user-ul ii trimite un semnal SIGUSR1, acesta se va restarta cu cofig-ul nou folosind
 * execv care va da replace complet procesului
 *
 */

#include <stdio.h> // Operatii basic de IO
#include <string.h> // Strlen
#include <stdlib.h> // malloc si altele
#include <unistd.h> // execv si sleep
#include <signal.h> // handling semnale
#include <dirent.h> // cautat prin directories

char* EXTENSION;
char* CLEANUP_PATH;
char* EXEC_PATH;

void restart_handler(int sig)
{
    if(sig == SIGUSR1)
    {
        char* argv[] = {EXEC_PATH, NULL};
        execv(argv[0], argv);
        perror("execv"); //This line won't execute if execv doesn't fail since it replaces the current process
        exit(EXIT_FAILURE);
    }
}

char has_extension(char* string, char* extension)
{
    unsigned long string_length    = strlen(string);
    unsigned long extension_length = strlen(extension);

    if(extension_length > string_length)
    {
        return 0;
    }

    //Compare character by character in reverse to be faster
    for(unsigned long i = 0; i < extension_length; i++)
    {
        if(string[string_length - i - 1] != extension[extension_length - i - 1])
        {
            return 0; //Stop at the first character
        }
    }

    return 1;
}

void cleanup()
{
    DIR* directory_stream = opendir(CLEANUP_PATH);

    if(directory_stream == NULL)
    {
        perror(""); //Print the error with which opendir fails
        exit(EXIT_FAILURE);
    }

    struct dirent* directory_entry;

    while((directory_entry = readdir(directory_stream)) != NULL)
    {
        if(has_extension(directory_entry->d_name, EXTENSION))
        {
            char buf[1024] = {0};
            sprintf(buf, "%s/%s", CLEANUP_PATH, directory_entry->d_name);
            if(remove(buf) != 0)
            {
                printf("Unable to delete the file\n");
                perror("Error");
            }
        }
    }
}

void parse_config(FILE* fp)
{
    char buf[1024] = {0}; //Sample buffer because I don't want to spend too much time on this doing it dynamically, resizing buffers or using stat
    fread(buf, 1024, 1, fp);
    unsigned long length          = strlen(buf);
    char          separator_count = 0;

    //Parsing foarte simplu, nu am vrut sa implementez un protocol custom sau sa folosesc o librarie doar pentru atat
    unsigned long idx = 0;

    for(unsigned long i = 0; i != length; i++)
    {
        if(buf[i] == ';')
        {
            separator_count++;
            idx = i;
        }
    }

    if(separator_count != 1)
    {
        printf("Malformed config file\n");
    }

    //Nu e memory leak pentru ca daemon-ul se foloseste de memorie pana isi da restart, caz in care memoria e
    //replaced either way, deci nu are sens sa dam free undeva la EXTENSION si CLEANUP_PATH
    EXTENSION = malloc(50);
    memcpy(EXTENSION, buf, idx);
    EXTENSION[idx + 1] = 0;

    CLEANUP_PATH = malloc(1024);
    memcpy(CLEANUP_PATH, buf + idx + 1, length - idx);
    CLEANUP_PATH[strlen(CLEANUP_PATH) - 1] = 0; //Remove trailing \n
}


int main(int argc, char** argv)
{
    //Setam EXEC_PATH pentru restartul procesului
    EXEC_PATH = argv[0];

    //Functie callback pentru cand primim semnalul
    signal(SIGUSR1, restart_handler);

    FILE* file = fopen("config", "r");
    if(file == NULL)
    {
        perror("Error opening config file");
        exit(EXIT_FAILURE);
    }

    //Dam parse la config pentru ca nu avem default values
    parse_config(file);

    //Main loop
    while(1)
    {
        cleanup();
        sleep(1);
    }

    //Unreachable
    return EXIT_SUCCESS;
}

/*> Exemplu de rulare a programului

   cat config
   .tmp;/tmp/foo/bar

   ╰─λ ls /tmp/foo/bar
   .rw-r--r-- 2 admin 12 Mar 12:09  test.tmp
   .rw-r--r-- 2 admin 12 Mar 12:09  test2.zzz

   ./app1.1

   // Dupa o secunda

   ╰─λ ls /tmp/foo/bar
   .rw-r--r-- 2 admin 12 Mar 12:09  test2.zzz


   Daca modificam fisierul config in asa fel incat sa contina
   .zzz;/tmp/foo/bar

   atunci

   ╰─λ ls /tmp/foo/bar

   nu returneaza nimic, procesul fiind restartat cu succes

 */
