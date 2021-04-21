
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
#define MBMEM_NAME "/sbmemlib"
#define MIN_SEG_SIZE 32000
#define MAX_SEG_SIZE 256000
#define MIN_REQ_SIZE 128
#define MAX_REQ_SIZE 4096

#define errExit(msg)        \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0);

// Define semaphore(s)

// Define your stuctures and variables.
void *pointerToSharedSegment = NULL;
int sizeOfSharedSegment = 0;

/* Checks if the given number is a power of 2 */
int is_pow2(int val)
{
    return (val != 0) && ((val & (val - 1)) == 0);
}

/* Returns the smallest power of 2 greater than the given number */
int next_pow2(int val)
{
    int pos = ceil(log2(val));
    return pow(2, pos);
}


int sbmem_init(int segmentsize)
{
    if (!is_pow2(segmentsize))
        errExit("[-] Segment size must be a power of 2.\n");

    if (segmentsize > MAX_SEG_SIZE || segmentsize < MIN_SEG_SIZE)
        errExit("[-] Segment size must be between 32KB and 256KB.\n");

    int shm_fd = shm_open(MBMEM_NAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (shm_fd == -1)
    {
        errExit("An error occured while creating shared memory");
    }

    int res = ftruncate(shm_fd, segmentsize);
    if (res != 0)
    {
        errExit("An error occured while creating shared memory");
    }

    int *sizeOfSegment = (int *) mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    if (sizeOfSegment == MAP_FAILED)
    {
        errExit("An error occured mmapping shared memory");
    }

    *sizeOfSegment = segmentsize;

    printf("sbmem init called"); // remove all printfs when you are submitting to us.
    return (0);
}

int sbmem_remove()
{
    if (shm_unlink(MBMEM_NAME) == -1) { 
        errExit("An error occured while unlinking the shared memory!");
    }

    // TODO delete all semaphores used!!!
    return (0);
}

int sbmem_open()
{
    // Open shared memory
    int shm_fd = shm_open(MBMEM_NAME, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    
    if (shm_fd == -1) 
        errExit("An error occured while creating shared memory");

    // Map and read the size information of shared memory
    int *sizeOfSegment = (int *) mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
   
    if (sizeOfSegment == MAP_FAILED)
        errExit("An error occured mmapping shared memory");

    // Map the whole shared memory
    sizeOfSharedSegment = *sizeOfSegment;
    pointerToSharedSegment = mmap(0, sizeOfSharedSegment, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (pointerToSharedSegment == MAP_FAILED)
        errExit("An error occured mmapping shared memory");
        
    return (0);
}

void *sbmem_alloc(int size)
{
    return (NULL);
}

void sbmem_free(void *p)
{
}

int sbmem_close()
{

    return (0);
}
