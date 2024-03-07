#include <stdio.h>  //IO, including perror
#include <stdlib.h> //Used for exit statuses, exit function and dynamic memory allocation
#include <string.h> //Used for string search
#include <dirent.h> //Directory stream and directory entry
#include <stdint.h> //For type names that actually make sense like uint32_t

//Panics when char* is null
short is_starting_with_number(char* str)
{
    if(strchr("0123456789", str[0]) != 0)
    {
        return 1;
    }
    return 0;
}

//This could've been done with an actual state machine, using  https://en.wikipedia.org/wiki/Aho%E2%80%93Corasick_algorithm
//But this was already taking too long to write
void search_proc_info(const char* buffer)
{
    const char* keywords[]    = {"Name:", "PPid:", "Uid:", "Gid:"};
    int         keyword_count = sizeof(keywords) / sizeof(keywords[0]);

    //We move the pointer to the buffer
    while(*buffer != 0)
    {
        //Iterate through all the keywords
        for(int i = 0; i < keyword_count; i++)
        {
            unsigned long len = strlen(keywords[i]);
            //Guard clause / Early return patter since the nesting is already deep enough for my liking
            if((strncmp(buffer, keywords[i], len) != 0))
            {
                continue;
            }

            //Print every char until we arrive at a \n or at the ent of the buffer, we shouldn't need the end of buffer part, but I left it here just in case
            while(*buffer != '\n' && *buffer != 0)
            {
                putchar(*buffer++);
            }

            //Actually print the \n, since in the previous while we exit if the current char is \n, we could've used a while true and exit if one of the
            //conditions were met but this looks cleaner IMO
            if(*buffer == '\n')
            {
                putchar(*buffer);
            }
        }

        //Advance the pointer
        buffer++;
    }

    //Newline between processes
    putchar('\n');
}


#define INITIAL_BUFFER_SIZE 1024
#define READ_SIZE           512

//Global buffers
char*  FILE_BUFFER;
size_t FILE_BUFFER_SIZE = INITIAL_BUFFER_SIZE;

void print_proc_info(uint32_t pid)
{
    //strlen("/proc") + Uint32_max should have 4294967295 which is 10 digits + strlen("/status") + 1 for \0
    char process_stat_path[5 + 10 + 7 + 1];
    sprintf(process_stat_path, "/proc/%d/status", pid);

    //Open the process's file to get the NAME, PPID, UID and GID
    FILE* file = fopen(process_stat_path, "r");

    if(file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    //We already know the PID, we don't need to search for it
    printf("PID: %u\n", pid);
    //Store the read bytes from the file
    size_t bytes_read;
    //Intermediary buffer that stores the data between the file and the file buffer we have
    char buffer[READ_SIZE]; //Buffer for reading chunks of the file
    //Used for offet
    size_t buffer_offset = 0;
    //Made the buffer global to avoid reallocating a buffer for each file,e this way this should only get called once or twice

    //While there are still bytes to read
    while((bytes_read = fread(buffer, 1, READ_SIZE, file)) > 0)
    {
        //If the current file buffer needs to be reallocated since it might be too small
        if(buffer_offset + bytes_read > FILE_BUFFER_SIZE)
        {
            //We allocate more than we need, just to reduce the number of allocations
            size_t new_buffer_size = (buffer_offset + bytes_read) * 2;
            //Using realloc to not have to alloc, move, free
            char* new_buffer = (char*)realloc(FILE_BUFFER, new_buffer_size);
            if(new_buffer == NULL)
            {
                perror("Failed to reallocate buffer");
                free(FILE_BUFFER);
                fclose(file);
                exit(EXIT_FAILURE);
            }
            //Set the new buffer globally, this was done to avoid reallocations for no reason
            FILE_BUFFER      = new_buffer;
            FILE_BUFFER_SIZE = new_buffer_size;
        }

        //Copy from current read buffer to file buffer
        memcpy(FILE_BUFFER + buffer_offset, buffer, bytes_read);
        //Move cursor/pointer
        buffer_offset += bytes_read;
    }

    //Set the end to 0, even though it probably is already from realloc.
    FILE_BUFFER[FILE_BUFFER_SIZE] = 0;

    //Actual searching
    search_proc_info(FILE_BUFFER);
}
int main()
{
    DIR* directory_stream;
    FILE_BUFFER = malloc(FILE_BUFFER_SIZE);

    directory_stream = opendir("/proc"); ///proc is a virtual folder, we can just use dirent
    //fopen() fclose() etc, just like in Lab1 app1
    if(directory_stream == NULL)
    {
        perror("Opening /proc"); //Print the error with which opendir fails
        return EXIT_FAILURE;
    }

    struct dirent* directory_entry;

    while((directory_entry = readdir(directory_stream)) != NULL)
    {
        //The only "files" starting with a number in /proc are processes
        if(!is_starting_with_number(directory_entry->d_name))
        {
            continue;
        }

        //We used uint32_t since apparently there's standard limit for PID on Linux
        //We also can assume atoll will work fine since all the values here should be proper integers
        uint32_t pid = atoll(directory_entry->d_name);

        //Handle the printing of info for the processes
        print_proc_info(pid);
    }

    return EXIT_SUCCESS;
}
