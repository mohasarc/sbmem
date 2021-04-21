
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

struct Head
{
    int is_alloc;
    void* begin;
    void* end;
    int size;
};


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

    int res = ftruncate(shm_fd, segmentsize + sizeof(int));
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

    struct Head *head = (struct Head *) mmap(0, sizeof(struct Head), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    head += sizeof(int);

    if (head == MAP_FAILED)
    {
        errExit("An error occured mmapping shared memory");
    }

    head->is_alloc = 0;
    head->begin = sizeof(int);
    head->end = head->begin + segmentsize;
    head->size = segmentsize;

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
    pointerToSharedSegment += sizeof(int);

    if (pointerToSharedSegment == MAP_FAILED)
        errExit("An error occured mmapping shared memory");

    return (0);
}

void *sbmem_alloc(int size)
{
    size += sizeof(struct Head);

    struct Head *tmpPointer = ((struct Head *) pointerToSharedSegment);
    void *ptr = NULL;

    while (tmpPointer->end < ((struct Head *)pointerToSharedSegment)->begin+sizeOfSharedSegment)
    {
        printf("Splitting? %d > %d \n", tmpPointer->size, size);

        if (tmpPointer->size > size){
            printf("Splitting because %d > %d \n", tmpPointer->size, size);
            split_chunck(tmpPointer);
        } else if (tmpPointer->is_alloc == 1) {
            printf("Found size but was already allocated!\n");
            tmpPointer = tmpPointer->end;
        } else {
            printf("Found an unallocated chunck \n");
            tmpPointer->is_alloc = 1;
            ptr = tmpPointer;
            break;
        }
    }

    printf("allocating at: %int", ptr);
    
    return (ptr);
}

void sbmem_free(void *p)
{

}

void split_chunck(struct Head *chunc_begin){
    struct Head *buddy_begin = (chunc_begin->end - chunc_begin->begin) / 2;
    buddy_begin->begin = buddy_begin;
    buddy_begin->end = chunc_begin->begin;
    buddy_begin->is_alloc = 0;

    chunc_begin->end = buddy_begin->begin;
}

void *find_buddy(void* buddy){

}

int sbmem_close()
{

    return (0);
}
