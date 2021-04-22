
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <semaphore.h>
 
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

sem_t semaphore;
// Define semaphore(s)
struct Head *find_buddy(struct Head *block);

// Define your stuctures and variables.

struct Head
{
    int is_alloc;
    int size;
    struct Head *next;
};

struct SharedMemInfo
{
    int size;
    sem_t semaphore;
};

int *pointerToSharedSegment = NULL;
struct SharedMemInfo *info;

/* Checks if the given number is a power of 2 */
int is_pow2(int val)
{
    return (val != 0) && ((val & (val - 1)) == 0);
}

int sbmem_init(int segmentsize)
{
    if (!is_pow2(segmentsize))
        errExit("[-] Segment size must be a power of 2.\n");

    if (segmentsize > MAX_SEG_SIZE || segmentsize < MIN_SEG_SIZE)
        errExit("[-] Segment size must be between 32KB and 256KB.\n");

    int shm_fd = shm_open(MBMEM_NAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (shm_fd == -1)
        errExit("An error occured while creating shared memory");

    if (ftruncate(shm_fd, segmentsize + sizeof(struct SharedMemInfo)) != 0)
        errExit("An error occured while creating shared memory");

    void *info_and_head = mmap(0, sizeof(struct Head) + sizeof(struct SharedMemInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (info_and_head == MAP_FAILED)
        errExit("An error occured mmapping shared memory");

    ((struct SharedMemInfo*)info_and_head)->size = segmentsize;
    sem_init(&((struct SharedMemInfo*)info_and_head)->semaphore, 1, 1);

    info_and_head = (char *)info_and_head + sizeof(struct SharedMemInfo);
    
    ((struct Head *)info_and_head)->is_alloc = 0;
    ((struct Head *)info_and_head)->size = segmentsize;
    ((struct Head *)info_and_head)->next = NULL;

    return (0);
}

int sbmem_remove()
{
    sem_destroy(&info->semaphore);
    if (shm_unlink(MBMEM_NAME) == -1) { 
        errExit("An error occured while unlinking the shared memory!");
    }
    
    return (0);
}

int sbmem_open()
{
    // Open shared memory
    int shm_fd = shm_open(MBMEM_NAME, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    
    if (shm_fd == -1) 
        errExit("An error occured while creating shared memory");

    // Map and read the size information of shared memory
    info = mmap(0, sizeof(struct SharedMemInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
   
    if (info == MAP_FAILED)
        errExit("An error occured mmapping shared memory");

    // Map the whole shared memory
    pointerToSharedSegment = mmap(0, info->size + sizeof(struct SharedMemInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (pointerToSharedSegment == MAP_FAILED)
        errExit("An error occured mmapping shared memory");

    // Move the pointer so that it points to the begining of the block    
    pointerToSharedSegment = (char *)pointerToSharedSegment + sizeof(struct SharedMemInfo);

    return (0);
}

void *sbmem_alloc(int size)
{   
    sem_wait(&info->semaphore);

    size += sizeof(struct Head);
    printf("Trying to allocate: %d\n", size);

    struct Head *tmpPointer = ((struct Head *) pointerToSharedSegment);
    void *ptr = NULL;

    do
    {   // Find a suitable block and split blocks accordingly
        if ((tmpPointer->size/2) >= size && tmpPointer->is_alloc == 0){
            split_chunck(tmpPointer);
        } else if (tmpPointer->is_alloc == 1 || tmpPointer->size < size) {
            tmpPointer = tmpPointer->next;
        } else {
            tmpPointer->is_alloc = 1;
            ptr = tmpPointer + 1;
            break;
        }
    } while (tmpPointer != NULL);

    print_memory();

    sem_post(&info->semaphore);
    return (ptr);
}

/**
 * 
 */
void sbmem_free(void *p)
{
    sem_wait(&info->semaphore);

    // printf("Frying P = %d of size %d \n", p, ((struct Head *)p)[-1].size);
    struct Head *block = ((struct Head *)p)-1;
    block->is_alloc = 0;

    // Find buddy
    struct Head *buddy = find_buddy(block);
    if (buddy->is_alloc != 1 && buddy->size == block->size){
        // Merge buddy
        merge_buddies(buddy, block);
    }

    print_memory();
    sem_post(&info->semaphore);
}

/**
 * Finds the buddy of a given block
 * A buddy: is a block of the same size of the given block, 
 * where the two blocks were part of one larger block of 
 * size 2^k+1 if the smaller blocks are of size 2^k
 * @param block The block we want to find its buddy
 */
struct Head *find_buddy(struct Head *block){
    int size_class = log2(block->size);

    // if the address of the given block (the block of size 2^K) % 2^k+1 then its buddy is at: (current block's address) + 2^k
    // else it is at (current block's address) - 2^k
    if (((char *)block - (char *)pointerToSharedSegment) % (long)pow(2, size_class + 1) == 0){
        return (struct Head *)((char *)block + block->size);
    } else {
        return (struct Head *)(char *)block - block->size;
    }
}

/**
 * Merges back two blocks that were already 
 * one larger block (buddies).
 * @param buddy1 The first block
 * @param buddy2 The second block
 */
void merge_buddies(struct Head *buddy1, struct Head *buddy2){
    int size_class = log2(buddy1->size);
    struct Head *buddy_left;
    struct Head *buddy_right;

    if (((char *)buddy1 - (char *)pointerToSharedSegment) % (long)pow(2, size_class + 1) == 0){
        buddy_left = buddy1;
        buddy_right = buddy2;
    } else {
        buddy_left = buddy2;
        buddy_right = buddy1;
    }

    buddy_left->next = buddy_right->next;
    buddy_left->size += buddy_right->size;
}

void split_chunck(struct Head *left_chunck){
    // printf("\n\nAT SPLIT\n\n");
    // printf("Before splitting: size: %d, begin: %d, next, %d \n", left_chunck->size, left_chunck, left_chunck->next);

    struct Head *buddy_begin = left_chunck + (left_chunck->size / 2)/sizeof(struct Head);
    buddy_begin->size = left_chunck->size / 2;
    buddy_begin->is_alloc = 0;
    buddy_begin->next = left_chunck->next;

    left_chunck->size = left_chunck->size / 2;
    left_chunck->next = buddy_begin;

    // printf("After Splitting: \n");
    // printf("Left : size: %d, begin: %d, next, %d \n", left_chunck->size, left_chunck, left_chunck->next);
    // printf("Right: size: %d, begin: %d, next, %d \n\n", buddy_begin->size, buddy_begin, buddy_begin->next);
}

void print_memory(){
    struct Head *mem_pointer = (struct Head *) pointerToSharedSegment;
    
    printf("---------------------------\n");
    printf("     MEMORY CONTENT        \n");
    printf("___________________________\n");
    while (mem_pointer != NULL)
    {
        printf("| Addr:           \b %d \b\n", mem_pointer);
        printf("| size:           \b %d \b\n", mem_pointer->size);
        printf("| is Allocated:   \b %d \b\n", mem_pointer->is_alloc);
        printf("| next's address: \b %d \b\n", mem_pointer->next);
        printf("___________________________\n");    

        mem_pointer = mem_pointer->next;
    }
    printf("___________________________\n");    
}

int sbmem_close()
{
    munmap(pointerToSharedSegment - sizeof(struct SharedMemInfo), info->size + sizeof(int));
    return (0);
}
