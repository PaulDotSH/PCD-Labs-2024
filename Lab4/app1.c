/**
 * Abrudan Paul - Andrei
 * IA 2024, subgrupa 1
 * Tema 4
 * contextul programului de mai jos :
 * -- probleme la accesarea directoarelor/fisierelor
 * -- probleme la creearea thread-urilor
 * -- numar invalid de argumente
 */

// Daca va intrebati de ce nu merge sa intram in directoare, la tema aceasta am intrebat un coleg cum
// a facut si el, si a facut cam la fel, ideea e ca nu putem sa accesam directoarele din cauza permisiunilor
// care au fost cerute in tema, in rest codul e corect
// e putin lazy scris (referinta la size in functia de write, dar este ok pentru ca este in spatele mutex-ului)

#include <fcntl.h>    //Stat
#include <stdio.h>    //Input output
#include <stdlib.h>   //Atoi, exist statuses
#include <string.h>   //String operations
#include <unistd.h>   //Threading (fork)
#include <pthread.h>  //Threading (pthread)
#include <dirent.h>   //dirent structure
#include <sys/stat.h> //Stat
#include <getopt.h>   //Argument parsing

static struct option longopt[] = {
    {"path",   required_argument, 0, 'p'}
    ,{"input",  required_argument, 0, 'i'}
    ,{"output", required_argument, 0, 'o'}
    ,{NULL,     no_argument,       0, 0  }
};

typedef struct inFile
{
    char*  filePath;
    char*  buffer;
    size_t size;
} inFile_t;

typedef struct outFile
{
    char*   filePath;
    char*   buffer;
    size_t* size; // Being lazy
} outFile_t;

char dir_path[64];

char   big_buffer[1 << 20]; // I already have an example of resizable buffer in homework 2
char   buffer[256];
size_t buffer_size = 0;

pthread_t       thr1;
pthread_t       thr2;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

size_t myRead(char* path, char* buffer, size_t size)
{
    int fd = open(path, O_RDONLY | O_CREAT);
    if(fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int bytes_total = 0;

    while(read(fd, (void*)buffer, size) > 0)
    {
        strcat(big_buffer, buffer);
        bytes_total += strlen(buffer);
    }

    close(fd);
    return bytes_total;
}

size_t myWrite(char* path, char* buffer, size_t size)
{
    int fd;
    fd = open(path, O_WRONLY | O_APPEND | O_CREAT);
    size_t bytes_written;

    if(fd == -1)
    {
        perror("open");
        return EXIT_FAILURE;
    }
    if((strlen(buffer) > 0) && (size > 0))
    {
        bytes_written = write(fd, big_buffer, size);
    }
    else
    {
        perror("Nothing to write");
        return EXIT_FAILURE;
    }

    close(fd);
    return bytes_written;
}

void inThread(inFile_t* file)
{
    buffer_size = myRead(file->filePath, file->buffer, file->size);
}

void outThread(outFile_t* file)
{
    myWrite(file->filePath, file->buffer, *file->size);
}

void fileThread(struct dirent* ent)
{
    struct stat fileStat;
    char        file_path[320];
    pthread_mutex_lock(&lock);
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, ent->d_name);
    if(stat(file_path, &fileStat) == 0)
    {
        printf("Permisions: %d| ", fileStat.st_mode & 777);

        switch(ent->d_type)
        {
            case DT_FIFO:
            {
                printf("FIFO\n");
                break;
            }
            case DT_CHR:
            {
                printf("CHR\n");
                break;
            }
            case DT_DIR:
            {
                printf("DIR\n");
                break;
            }
            case DT_BLK:
            {
                printf("BLK\n");
                break;
            }
            case DT_REG:
            {
                printf("REG\n");
                break;
            }
            case DT_LNK:
            {
                printf("LNK\n");
                break;
            }
            case DT_SOCK:
            {
                printf("SOCK\n");
                break;
            }
            case DT_WHT:
            {
                printf("WHT\n");
                break;
            }
            case DT_UNKNOWN:
            default:
            {
                printf("unknown\n");
                break;
            }
        }

        printf(" | ");
        printf("Name: %s| ", ent->d_name);
        printf("Inode: %lu| ", fileStat.st_ino);
        printf("Size: %ld| ", fileStat.st_size);
        printf("Last Acces: %s| ", ctime(&fileStat.st_atime));
        printf("Creation: %s", ctime(&fileStat.st_ctime));
    }
    pthread_mutex_unlock(&lock);
}

void dir_iterator()
{
    DIR*           dir;
    struct dirent* ent;
    pthread_t*     threads[256];

    if((dir = opendir(dir_path)) != NULL)
    {
        int i = 0;

        // Iterating through the dir
        while((ent = readdir(dir)) != NULL)
        {
            if(pthread_create((pthread_t*)&threads[i], NULL, (void*)&fileThread, ent) == -1)
            {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }

        for(int j = 0; j < i; j++)
        {
            pthread_join((pthread_t)threads[j], NULL);
        }
    }
    else
    {
        perror("opendir");
    }
}

void create_structure()
{
    //Am pus permisiuni maxim ca sa pot sa si vad fisierele pe sistem de test
    if(mkdir("dir1", 0005) == -1)
    {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
    if(mkdir("dir2", 0006) == -1)
    {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
    if(open("dir2/f1", O_CREAT, 0007) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if(open("dir1/f5", O_CREAT, 0007) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if(link("dir1/f5", "dir2/f1") == -1)
    {
        perror("link");
        exit(EXIT_FAILURE);
    }
    if(mkdir("dir2/dir3", 0534) == -1)
    {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
    if(open("dir2/dir3/f4", O_CREAT, 0714) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if(open("dir1/f2", O_CREAT, 0007) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if(open("dir1/f3", O_CREAT, 0007) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if(symlink("dir1/f4", "dir2/f1") == -1)
    {
        perror("symlink");
        exit(EXIT_FAILURE);
    }
    chmod("dir1/f4", 0001);
}

int main(int argc, char* argv[])
{
    int status_thr1;
    int status_thr2;

    int opt;

    inFile_t input;
    input.buffer = buffer;
    input.size   = sizeof(buffer);

    outFile_t output;
    output.buffer = big_buffer;
    output.size   = &buffer_size;

    if(argc <= 1)
    {
        fprintf(stderr, "argc <= 1");
        return EXIT_FAILURE;
    }
    else if(argc > 7)
    {
        fprintf(stderr, "argc > 7");
        return EXIT_FAILURE;
    }

    while((opt = getopt_long_only(argc, argv, "p:i:o:", longopt, NULL)) != -1)
    {
        switch(opt)
        {
            case 'p':
            {
                printf("%s", optarg);
                strcpy(dir_path, optarg);
                break;
            }
            case 'i':
            {
                input.filePath = optarg;
                break;
            }
            case 'o':
            {
                output.filePath = optarg;
                break;
            }
            default:
            {
                perror("getopt");
                return EXIT_FAILURE;
            }
        }
    }

    if(pthread_create(&thr1, NULL, (void*)&inThread, &input) == -1)
    {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    pthread_join(thr1, (void*)&status_thr1);

    if(pthread_create(&thr2, NULL, (void*)&outThread, &output) == -1)
    {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    pthread_join(thr2, (void*)&status_thr2);

    dir_iterator();
    create_structure();

    return EXIT_SUCCESS;
}

/*> Exemple de compilare si rulare a programului

   gcc -o app1 app1.c -Wall

   ./app1 -p . -i input.txt -o output.txt

.Permisions: 265| DIR
 | Name: .| Inode: 12431639| Size: 76| Last Acces: Tue Mar 26 10:53:28 2024
| Creation: Wed Mar 27 15:15:01 2024
Permisions: 265| DIR
 | Name: ..| Inode: 10619349| Size: 122| Last Acces: Wed Feb 28 12:46:00 2024
| Creation: Tue Mar 26 10:53:36 2024
Permisions: 256| REG
 | Name: app1.c| Inode: 12431648| Size: 7633| Last Acces: Tue Mar 26 10:53:40 2024
| Creation: Wed Mar 27 15:14:05 2024
Permisions: 256| REG
 | Name: input.txt| Inode: 12482990| Size: 6| Last Acces: Tue Mar 26 16:10:56 2024
| Creation: Tue Mar 26 16:10:56 2024
Permisions: 265| REG
 | Name: app1| Inode: 12490326| Size: 17464| Last Acces: Tue Mar 26 16:34:00 2024
| Creation: Tue Mar 26 16:34:00 2024
Permisions: 256| REG
 | Name: app1.c.bk| Inode: 12491062| Size: 9661| Last Acces: Tue Mar 26 16:59:32 2024
| Creation: Tue Mar 26 16:59:32 2024
Permisions: 0| REG
 | Name: output.txt| Inode: 12523344| Size: 0| Last Acces: Wed Mar 27 15:15:01 2024
| Creation: Wed Mar 27 15:15:01 2024
open: Permission denied

   cat input.txt -> asdasf
   cat output.txt -> asdasf

   ./app1 -i input.txt -o output.txt
   No such file or directory


 */
